/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/portmacro.h"
#include "freertos/semphr.h"

#include "esp_check.h"
#include "esp_bit_defs.h"
#include "esp_err.h"
#include "esp_log_buffer.h"
#include "esp_sip.h"
#include "ext_default.h"
#include "sip2_common.h"
#include "esp_heap_caps.h"
#include "esp_extconn.h"

#include "ext_sdio_adapter.h"

#define WIFI_NEED_SEND           (BIT0)
#define WIFI_SEND_BUFFER_LEN     (1600)
#define PP_TXCB_SCAN_PROBEREQ_ID (1)

typedef struct {
    esf_buf_t *tx_head;
    esf_buf_t *tx_tail;
    SemaphoreHandle_t list_lock;
    EventGroupHandle_t list_event;
    uint32_t queued_count;
    uint32_t ctrl_alloc_count;
    uint32_t ctrl_alloc_fail_count;
    uint32_t data_enqueue_count;
    uint32_t dequeue_count;
    uint32_t recycle_count;
    uint32_t tx_callback_count;
} wifi_tx_ctx_t;

extern esf_buf *esf_buf_alloc(void *buffer, esf_buf_type_t type, uint32_t len);
extern void esf_buf_recycle(esf_buf *eb);
extern void net80211_en_txdq(esf_buf *eb);
extern bool esp_wifi_is_tx_callback(esf_buf *eb);
extern void esp_sip_txd_post(void);

static const char *TAG = "trans_wifi";
static wifi_tx_ctx_t *wifi_tx;

static void wifi_tx_stats_log(const char *where)
{
    if (wifi_tx == NULL) {
        return;
    }

    ESP_LOGW(TAG,
             "tx stats[%s]: queued=%u ctrl_alloc=%u ctrl_fail=%u "
             "data_enq=%u dequeue=%u recycle=%u tx_cb=%u",
             where,
             (unsigned)wifi_tx->queued_count,
             (unsigned)wifi_tx->ctrl_alloc_count,
             (unsigned)wifi_tx->ctrl_alloc_fail_count,
             (unsigned)wifi_tx->data_enqueue_count,
             (unsigned)wifi_tx->dequeue_count,
             (unsigned)wifi_tx->recycle_count,
             (unsigned)wifi_tx->tx_callback_count);
}

static void wifi_dump_eb_abi(const esf_buf_t *eb)
{
    static bool layout_logged;

    if (!layout_logged) {
        ESP_LOGI(TAG,
                 "esf ABI local: sizeof(eb)=%u sizeof(tx_desc)=%u "
                 "off(data_len)=%u off(type)=%u off(desc)=%u",
                 (unsigned)sizeof(esf_buf_t), (unsigned)sizeof(esf_tx_desc_t),
                 (unsigned)offsetof(esf_buf_t, data_len),
                 (unsigned)offsetof(esf_buf_t, type),
                 (unsigned)offsetof(esf_buf_t, desc));
        layout_logged = true;
    }

    if (eb == NULL) {
        return;
    }

    ESP_LOGW(TAG,
             "esf ABI AC: data_len=%u type=%u desc=%p",
             (unsigned)eb->data_len, (unsigned)eb->type, (void *)TO_TX_DESC(eb));
}

static const char *wifi_sip_cmd_name(uint32_t cmdid)
{
    switch (cmdid) {
    case SIP_CMD_GET_VER: return "GET_VER";
    case SIP_CMD_WRITE_MEMORY: return "WRITE_MEMORY";
    case SIP_CMD_READ_MEMORY: return "READ_MEMORY";
    case SIP_CMD_WRITE_REG: return "WRITE_REG";
    case SIP_CMD_READ_REG: return "READ_REG";
    case SIP_CMD_BOOTUP: return "BOOTUP";
    case SIP_CMD_COPYBACK: return "COPYBACK";
    case SIP_CMD_INIT: return "INIT";
    case SIP_CMD_SCAN: return "SCAN";
    case SIP_CMD_SETKEY: return "SETKEY";
    case SIP_CMD_CONFIG: return "CONFIG";
    case SIP_CMD_BSS_INFO_UPDATE: return "BSS_INFO_UPDATE";
    case SIP_CMD_LOOPBACK: return "LOOPBACK";
    case SIP_CMD_SET_WMM_PARAM: return "SET_WMM_PARAM";
    case SIP_CMD_AMPDU_ACTION: return "AMPDU_ACTION";
    case SIP_CMD_HB_REQ: return "HB_REQ";
    case SIP_CMD_RESET_MAC: return "RESET_MAC";
    case SIP_CMD_PRE_DOWN: return "PRE_DOWN";
    case SIP_CMD_SLEEP: return "SLEEP";
    case SIP_CMD_WAKEUP: return "WAKEUP";
    case SIP_CMD_DEBUG: return "DEBUG";
    case SIP_CMD_GET_FW_VER: return "GET_FW_VER";
    case SIP_CMD_SETVIF: return "SETVIF";
    case SIP_CMD_SETSTA: return "SETSTA";
    case SIP_CMD_PS: return "PS";
    case SIP_CMD_ATE: return "ATE";
    case SIP_CMD_SUSPEND: return "SUSPEND";
    case SIP_CMD_RECALC_CREDIT: return "RECALC_CREDIT";
    case SIP_CMD_BT_STATE: return "BT_STATE";
    case SIP_CMD_TEST: return "TEST";
    case SIP_CMD_SNIFFER: return "SNIFFER";
    case SIP_CMD_SET_RX_POLICY: return "SET_RX_POLICY";
    default: return "UNKNOWN";
    }
}

static void wifi_dump_sip_cmd(const esf_buf_t *eb)
{
    struct sip_hdr hdr;

    if (eb == NULL) {
        return;
    }

    if (eb->type != ESF_BUF_TX_SIP && eb->type != ESF_BUF_TX_SIP_TEST) {
        return;
    }

    if (eb->buf_begin == NULL || eb->data_len < sizeof(hdr) || eb->data_len > WIFI_SEND_BUFFER_LEN) {
        ESP_LOGW(TAG,
                 "AC SIP candidate: eb=%p type=%u data_len=%u buf_begin=%p "
                 "(header unavailable)",
                 eb, (unsigned)eb->type, (unsigned)eb->data_len, eb->buf_begin);
        return;
    }

    memcpy(&hdr, eb->buf_begin, sizeof(hdr));
    ESP_LOGW(TAG,
             "AC SIP command: eb=%p type=%u data_len=%u buf=%p "
             "fc0=0x%02x fc1=0x%02x len=%u cmdid=%u(%s) seq=%u",
             eb, (unsigned)eb->type, (unsigned)eb->data_len, eb->buf_begin,
             hdr.fc[0], hdr.fc[1], (unsigned)hdr.len,
             (unsigned)hdr.c_cmdid, wifi_sip_cmd_name(hdr.c_cmdid),
             (unsigned)hdr.seq);
    ESP_LOG_BUFFER_HEXDUMP(TAG, eb->buf_begin, eb->data_len, ESP_LOG_WARN);
}

static void wifi_dump_eb_state(const char *where, const esf_buf_t *eb)
{
    if (eb == NULL) {
        ESP_LOGE(TAG, "%s eb=NULL", where);
        return;
    }

    wifi_dump_eb_abi(eb);

    ESP_LOGW(TAG,
             "%s eb=%p ds_head=%p ds_tail=%p tx_desc=%p "
             "type=%u data_len=%u buf_begin=%p",
             where, eb, eb->ds_head, eb->ds_tail, TO_TX_DESC(eb),
             eb->type, eb->data_len, eb->buf_begin);
    ESP_LOG_BUFFER_HEXDUMP(TAG, eb, 64, ESP_LOG_WARN);
}

static void list_wait_item()
{
    xEventGroupWaitBits(wifi_tx->list_event, WIFI_NEED_SEND, false, true, portMAX_DELAY);
}

static esf_buf *list_remove()
{
    esf_buf *eb = NULL;
    xSemaphoreTake(wifi_tx->list_lock, portMAX_DELAY);
    if (wifi_tx->tx_head != NULL) {
        eb = wifi_tx->tx_head;
        wifi_tx->tx_head = STAILQ_NEXT(wifi_tx->tx_head, bqentry);
        STAILQ_NEXT(eb, bqentry) = NULL;
        wifi_tx->queued_count--;
        wifi_tx->dequeue_count++;
    } else {
        wifi_tx->tx_tail = NULL;
    }
    if (eb == NULL) {
        xEventGroupClearBits(wifi_tx->list_event, WIFI_NEED_SEND);
    }
    xSemaphoreGive(wifi_tx->list_lock);
    return eb;
}

static esp_err_t list_insert(esf_buf *eb)
{
    ESP_RETURN_ON_FALSE(eb != NULL, ESP_ERR_INVALID_ARG, TAG, "eb NULL");

    xSemaphoreTake(wifi_tx->list_lock, portMAX_DELAY);
    if (wifi_tx->tx_head == NULL) {
        wifi_tx->tx_head = eb;
        STAILQ_NEXT(wifi_tx->tx_head, bqentry) = NULL;
        wifi_tx->tx_tail = wifi_tx->tx_head;
    } else {
        STAILQ_NEXT(wifi_tx->tx_tail, bqentry) = eb;
        STAILQ_NEXT(eb, bqentry) = NULL;
        wifi_tx->tx_tail = STAILQ_NEXT(wifi_tx->tx_tail, bqentry);
    }
    wifi_tx->queued_count++;
    xEventGroupSetBits(wifi_tx->list_event, WIFI_NEED_SEND);
    xSemaphoreGive(wifi_tx->list_lock);
    return ESP_OK;
}

static esp_err_t list_head_insert(esf_buf *eb)
{
    ESP_RETURN_ON_FALSE(eb != NULL, ESP_ERR_INVALID_ARG, TAG, "eb NULL");

    xSemaphoreTake(wifi_tx->list_lock, portMAX_DELAY);
    if (wifi_tx->tx_head == NULL) {
        wifi_tx->tx_head = eb;
        STAILQ_NEXT(wifi_tx->tx_head, bqentry) = NULL;
        wifi_tx->tx_tail = wifi_tx->tx_head;
    } else {
        STAILQ_NEXT(eb, bqentry) = wifi_tx->tx_head;
        wifi_tx->tx_head = eb;
    }
    wifi_tx->queued_count++;
    xEventGroupSetBits(wifi_tx->list_event, WIFI_NEED_SEND);
    xSemaphoreGive(wifi_tx->list_lock);
    return ESP_OK;
}

static void list_clear(void)
{
    esf_buf *eb = NULL;
    esf_buf *eb_head = NULL;
    esf_buf *eb_end = NULL;

    xSemaphoreTake(wifi_tx->list_lock, portMAX_DELAY);

    while ((eb = wifi_tx->tx_head) != NULL) {
        wifi_tx->tx_head = STAILQ_NEXT(wifi_tx->tx_head, bqentry);
        STAILQ_NEXT(eb, bqentry) = NULL;
        wifi_tx->queued_count--;

        if (eb->type == ESF_BUF_TX_PB || ((eb->type == ESF_BUF_TX_SIP) && (((struct sip_hdr *)eb->buf_begin)->c_cmdid == SIP_CMD_CONFIG)) || ((eb->type == ESF_BUF_MGMT_SBUF || eb->type == ESF_BUF_MGMT_LBUF) && (TO_TX_DESC(eb)->comp_cb_map == (1 << PP_TXCB_SCAN_PROBEREQ_ID)))) {
            esf_buf_recycle(eb);
            wifi_tx->recycle_count++;
        } else {
            if (eb_end == NULL) {
                eb_head = eb;
                STAILQ_NEXT(eb_head, bqentry) = NULL;
                eb_end = eb;
            } else {
                STAILQ_NEXT(eb_end, bqentry) = eb;
                STAILQ_NEXT(eb, bqentry) = NULL;
                eb_end = eb;
            }
        }
        if (wifi_tx->tx_head == NULL) {
            wifi_tx->tx_tail = NULL;
            break;
        }
    }

    wifi_tx->tx_head = eb_head;
    wifi_tx->tx_tail = eb_end;
    xSemaphoreGive(wifi_tx->list_lock);
}

static void wifi_send_task(void *args)
{
    esp_err_t err = ESP_OK;
    uint8_t *send_buf = heap_caps_calloc(1, WIFI_SEND_BUFFER_LEN, MALLOC_CAP_DMA | MALLOC_CAP_CACHE_ALIGNED);

    ESP_ERROR_CHECK(send_buf ? ESP_OK : ESP_ERR_NO_MEM);

    ESP_LOGI(TAG, "WiFi Send START");

    while (true) {
        list_wait_item();

        esf_buf *eb = list_remove();
        if (eb == NULL) {
            continue;
        }

        wifi_dump_eb_state("wifi_send_task dequeued", eb);
        wifi_dump_sip_cmd(eb);

        ESP_LOGE(TAG,
                 "wifi_send_task before data/descriptor access: eb=%p ds_head=%p "
                 "tx_desc=%p",
                 eb, eb->ds_head, TO_TX_DESC(eb));

        uint32_t send_len = 0;
        const uint8_t *wire_payload = NULL;
        uint32_t wire_payload_len = 0;
        struct sip_hdr *shdr = (struct sip_hdr *)send_buf;
        memset(shdr, 0x0, SIP_CTRL_HDR_LEN);

        if (eb->type == ESF_BUF_TX_SIP || eb->type == ESF_BUF_TX_SIP_TEST) {
            send_len = roundup(eb->data_len, esp_sip_get_tx_blks());
            memcpy(send_buf, (uint8_t *)(eb->buf_begin), eb->data_len);

            SIP_HDR_SET_TYPE(shdr->fc[0], SIP_CTRL);
            SIP_HDR_SET_SYNC(shdr);
        } else {
            send_len = roundup((eb->ds_head->length + SIP_CTRL_HDR_LEN), esp_sip_get_tx_blks());

            if (PP_IS_AMPDU(eb)) {
                SIP_HDR_SET_TYPE(shdr->fc[0], SIP_DATA_AMPDU);
            } else {
                SIP_HDR_SET_TYPE(shdr->fc[0], SIP_DATA);
            }
            SIP_HDR_SET_SYNC(shdr);

            shdr->len = eb->ds_head->length + SIP_CTRL_HDR_LEN;
            shdr->d_tid = TO_TX_DESC(eb)->tid;
            shdr->d_ac = TO_TX_DESC(eb)->ac;
            shdr->d_p2p = 0;
            shdr->d_enc_flag = TO_TX_DESC(eb)->crypto_type;
            shdr->d_hw_kid = TO_TX_DESC(eb)->kid;

            wire_payload = (const uint8_t *)eb->u_data_start;
            wire_payload_len = eb->ds_head->length;
            memcpy((send_buf + SIP_CTRL_HDR_LEN), wire_payload, wire_payload_len);
        }

        uint32_t sdio_wait_loops = 0;
        while (1) {
            uint32_t num = 0;
            uint32_t cnt = 0;

            esp_extconn_sdio_lock();
            esp_extconn_sdio_get_buffer_size(&num);
            esp_extconn_sdio_unlock();
            cnt++;
            if (num * 512 < send_len) {
                sdio_wait_loops++;
                if (sdio_wait_loops == 1 || (sdio_wait_loops % 500) == 0) {
                    ESP_LOGW(TAG,
                             "SDIO credit wait: eb=%p type=%u send_len=%u "
                             "avail=%u loops=%u queued=%u",
                             eb, (unsigned)eb->type, (unsigned)send_len,
                             (unsigned)(num * 512), (unsigned)sdio_wait_loops,
                             (unsigned)wifi_tx->queued_count);
                }
                if (cnt % 1000 == 0) {
                    ESP_LOGI(TAG, "now num %" PRIu32, num);
                }
                vTaskDelay(pdMS_TO_TICKS(2));
            } else {
                if (sdio_wait_loops != 0) {
                    ESP_LOGW(TAG,
                             "SDIO credit available: eb=%p type=%u "
                             "send_len=%u avail=%u wait_loops=%u",
                             eb, (unsigned)eb->type, (unsigned)send_len,
                             (unsigned)(num * 512), (unsigned)sdio_wait_loops);
                }
                break;
            }
        }

        if (shdr->c_cmdid != SIP_CMD_WRITE_MEMORY || shdr->c_cmdid != SIP_CMD_BOOTUP || shdr->c_cmdid != SIP_CMD_WRITE_REG || shdr->c_cmdid != SIP_CMD_LOOPBACK) {
            shdr->seq = esp_sip_increase_txseq();
        }

        ESP_LOGW(TAG,
                 "SDIO TX PRE eb=%p type=%u data=%p data_len=%u "
                 "desc_len=%u send_len=%u fc0=0x%02x fc1=0x%02x "
                 "sip_len=%u seq=%" PRIu32,
                 eb, (unsigned)eb->type, wire_payload,
                 (unsigned)wire_payload_len,
                 eb->ds_head ? (unsigned)eb->ds_head->length : 0,
                 (unsigned)send_len, shdr->fc[0], shdr->fc[1],
                 (unsigned)shdr->len, shdr->seq);
        if (wire_payload != NULL) {
            ESP_LOG_BUFFER_HEXDUMP(TAG, wire_payload,
                                   wire_payload_len < 64 ? wire_payload_len : 64,
                                   ESP_LOG_WARN);
        }
        ESP_LOG_BUFFER_HEXDUMP(TAG, send_buf,
                               shdr->len < 96 ? shdr->len : 96,
                               ESP_LOG_WARN);

        esp_extconn_sdio_lock();
        err = esp_extconn_sdio_send_packet(EXT_CONN_WIFI_SDIO_FUNC, send_buf, send_len);
        esp_extconn_sdio_unlock();
        ESP_LOGW(TAG, "SDIO TX POST eb=%p ret=%d send_len=%u",
                 eb, err, (unsigned)send_len);
        ESP_RETURN_ON_FALSE(err == ESP_OK,, TAG, "WiFi send error");

        if (esp_wifi_is_tx_callback(eb)) {
            net80211_en_txdq(eb);
            esp_sip_txd_post();
            wifi_tx->tx_callback_count++;
        } else {
            esf_buf_recycle(eb);
            wifi_tx->recycle_count++;
        }
    }
    vTaskDelete(NULL);
}

static esp_err_t wifi_send_cmd(enum sip_cmd_id cid, uint32_t cmdlen, void *cmd)
{
    esp_err_t err = 0;
    struct sip_hdr *shdr = NULL;
    esf_buf *eb = NULL;
    uint16_t len = cmdlen + SIP_CTRL_HDR_LEN;
    ESP_LOGV(TAG, "wifi cid %d", cid);

    if (cid == SIP_CMD_TEST) {
        eb = esf_buf_alloc(NULL, ESF_BUF_TX_SIP_TEST, len);
    } else {
        eb = esf_buf_alloc(NULL, ESF_BUF_TX_SIP, len);
    }
    wifi_tx->ctrl_alloc_count++;

    if (!eb) {
        wifi_tx->ctrl_alloc_fail_count++;
        wifi_tx_stats_log("ctrl_alloc_fail");
        ESP_LOGE(TAG, "m eb fail!!!");
        return ESP_FAIL;
    }

    shdr = (struct sip_hdr *)(eb->buf_begin);
    memset(shdr, 0x0, SIP_CTRL_HDR_LEN);
    SIP_HDR_SET_TYPE(shdr->fc[0], SIP_CTRL);
    shdr->len = len;
    shdr->c_cmdid = cid;
    if (cmd) {
        memcpy(eb->buf_begin + SIP_CTRL_HDR_LEN, (uint8_t *)cmd, cmdlen);
    }

    if (cid == SIP_CMD_SETSTA) {
        struct sip_cmd_setsta *set_sta = (struct sip_cmd_setsta *)cmd;
        // clear tx list when disconnect
        if (set_sta->set == 0) {
            list_clear();
        }
    }

    if (cid == SIP_CMD_TEST) {
        err = list_head_insert(eb);
    } else {
        err = list_insert(eb);
    }

    return err;
}

static esp_err_t wifi_send_data(esf_buf_t *eb)
{
    if (eb == NULL) {
        return ESP_FAIL;
    }

    wifi_dump_eb_state("wifi_send_data enqueue", eb);

    ESP_LOGW(TAG,
             "wifi_send_data eb=%p type=%u data_len=%u buf_begin=%p "
             "ds_head=%p ds_tail=%p ds_len=%u tx_desc=%p",
             eb, eb->type, eb->data_len, eb->buf_begin,
             eb->ds_head, eb->ds_tail, eb->ds_len, TO_TX_DESC(eb));
    ESP_LOG_BUFFER_HEXDUMP(TAG, eb, sizeof(*eb), ESP_LOG_WARN);

    if (eb->ds_head != NULL) {
        ESP_LOGW(TAG,
                 "eb ds_head=%p size=%u length=%u offset=%u sosf=%u eof=%u "
                 "owner=%u buf=%p",
                 eb->ds_head, eb->ds_head->size, eb->ds_head->length,
                 eb->ds_head->offset, eb->ds_head->sosf, eb->ds_head->eof,
                 eb->ds_head->owner, eb->ds_head->buf);
        ESP_LOG_BUFFER_HEXDUMP(TAG, eb->ds_head, sizeof(*eb->ds_head), ESP_LOG_WARN);
    }

    if (TO_TX_DESC(eb) != NULL) {
        const esf_tx_desc_t *tx_desc = TO_TX_DESC(eb);
        ESP_LOGW(TAG,
                 "eb tx_desc=%p flags=0x%08x tid=%u ac=%u rrc=%u "
                 "src=%u lrc=%u qid=%u ifidx=%u status=%u",
                 tx_desc, (unsigned int)tx_desc->flags, tx_desc->tid, tx_desc->ac,
                 tx_desc->rrc, tx_desc->src, tx_desc->lrc, tx_desc->qid,
                 tx_desc->ifidx, tx_desc->status);
        ESP_LOG_BUFFER_HEXDUMP(TAG, tx_desc, sizeof(*tx_desc), ESP_LOG_WARN);
    }
    uint8_t *payload = eb->buf_begin + sizeof(esf_tx_desc_t);

    ESP_LOGW(TAG, "payload=%p payload_len=%u",
             payload, eb->data_len);
    ESP_LOG_BUFFER_HEXDUMP(TAG, payload, eb->data_len, ESP_LOG_WARN);
    wifi_tx->data_enqueue_count++;
    list_insert(eb);
    if ((wifi_tx->data_enqueue_count & 0x3U) == 0) {
        wifi_tx_stats_log("data_enqueue");
    }
    ESP_LOGV(TAG, "wifi send data");
    return ESP_OK;
}

int get_coex_status_cb(void)
{
    return EXT_CONN_FUNCTIONS_ENABLE;
}

esp_err_t esp_extconn_trans_wifi_init(esp_extconn_config_t *config)
{
    wifi_tx = calloc(1, sizeof(wifi_tx_ctx_t));
    if (!wifi_tx) {
        return ESP_ERR_NO_MEM;
    }
    wifi_tx->list_lock = xSemaphoreCreateMutex();
    if (!wifi_tx->list_lock) {
        return ESP_ERR_NO_MEM;
    }
    wifi_tx->list_event = xEventGroupCreate();
    if (!wifi_tx->list_event) {
        return ESP_ERR_NO_MEM;
    }

    sip_register_tx_cmd_cb(wifi_send_cmd);
    //sip_register_tx_data_cb(wifi_send_data);
    sip_register_get_coex_status_cb(get_coex_status_cb);

    esp_err_t ret = xTaskCreatePinnedToCore(wifi_send_task,
                                            "wifi_send",
                                            config->wifi_task_stack,
                                            NULL,
                                            config->wifi_task_prio,
                                            NULL,
                                            config->wifi_task_core)
                    == pdTRUE ? ESP_OK : ESP_ERR_NO_MEM;

    return ret;
}
