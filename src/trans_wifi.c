/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/portmacro.h"
#include "freertos/semphr.h"

#include "esp_check.h"
#include "esp_bit_defs.h"
#include "esp_err.h"
#include "esp_sip.h"
#include "ext_default.h"
#include "sip2_common.h"
#include "esp_dma_utils.h"
#include "esp_extconn.h"

#include "ext_sdio_adapter.h"

#define WIFI_NEED_SEND           (BIT0)
#define WIFI_SEND_BUFFER_LEN     (2048)
#define PP_TXCB_SCAN_PROBEREQ_ID (1)

typedef struct {
    esf_buf_t *tx_head;
    esf_buf_t *tx_tail;
    SemaphoreHandle_t list_lock;
    EventGroupHandle_t list_event;
} wifi_tx_ctx_t;

extern esf_buf *esf_buf_alloc(void *buffer, esf_buf_type_t type, uint32_t len);
extern void esf_buf_recycle(esf_buf *eb);
extern void net80211_en_txdq(esf_buf *eb);
extern bool esp_wifi_is_tx_callback(esf_buf *eb);
extern void esp_sip_txd_post(void);
extern void esp_sip_recycle(esf_buf *eb);

static const char *TAG = "trans_wifi";
static wifi_tx_ctx_t *wifi_tx;

static void list_wait_item(void)
{
    xEventGroupWaitBits(wifi_tx->list_event, WIFI_NEED_SEND, false, true, portMAX_DELAY);
}

static esf_buf *list_remove(void)
{
    esf_buf *eb = NULL;
    xSemaphoreTake(wifi_tx->list_lock, portMAX_DELAY);
    if (wifi_tx->tx_head != NULL) {
        eb = wifi_tx->tx_head;
        wifi_tx->tx_head = STAILQ_NEXT(wifi_tx->tx_head, bqentry);
        STAILQ_NEXT(eb, bqentry) = NULL;
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

        if (eb->type == ESF_BUF_TX_PB || ((eb->type == ESF_BUF_TX_SIP) && (((struct sip_hdr *)eb->buf_begin)->c_cmdid == SIP_CMD_CONFIG)) || ((eb->type == ESF_BUF_MGMT_SBUF || eb->type == ESF_BUF_MGMT_LBUF) && (TO_TX_DESC(eb)->comp_cb_map == (1 << PP_TXCB_SCAN_PROBEREQ_ID)))) {
            esf_buf_recycle(eb);
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
    uint8_t *send_buf = NULL; //calloc(1, WIFI_SEND_BUFFER_LEN);

    size_t actual_size = 0;
    esp_dma_mem_info_t dma_mem_info = {
        .extra_heap_caps = MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL,
        .dma_alignment_bytes = 4, //legacy API behaviour is only check max dma buffer alignment
    };
    ESP_ERROR_CHECK(esp_dma_capable_malloc(WIFI_SEND_BUFFER_LEN, &dma_mem_info, (void*)&send_buf, &actual_size));

    ESP_LOGI(TAG, "WiFi Send START");

    while (true) {
        list_wait_item();

        esf_buf *eb = list_remove();
        if (eb == NULL) {
            continue;
        }

        uint32_t send_len = 0;
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

            memcpy((send_buf + SIP_CTRL_HDR_LEN), (uint8_t *)(eb->u_data_start), eb->ds_head->length);
        }

        if (send_len > actual_size) {
            ESP_LOGE(TAG, "wifi buffer overflow %" PRIu16 "-> %" PRIu32, actual_size, send_len);
            abort();
        }

        while (1) {
            uint32_t num = 0;
            uint32_t cnt = 0;

            esp_extconn_sdio_lock();
            esp_extconn_sdio_get_buffer_size(&num);
            esp_extconn_sdio_unlock();
            cnt++;
            if (num * 512 < send_len) {
                if (cnt % 1000 == 0) {
                    ESP_LOGI(TAG, "now num %" PRIu32, num);
                }
                vTaskDelay(pdMS_TO_TICKS(2));
            } else {
                break;
            }
        }

        if (shdr->c_cmdid != SIP_CMD_WRITE_MEMORY || shdr->c_cmdid != SIP_CMD_BOOTUP || shdr->c_cmdid != SIP_CMD_WRITE_REG || shdr->c_cmdid != SIP_CMD_LOOPBACK) {
            shdr->seq = esp_sip_increase_txseq();
        }

        esp_extconn_sdio_lock();
        err = esp_extconn_sdio_send_packet(EXT_CONN_WIFI_SDIO_FUNC, send_buf, send_len);
        esp_extconn_sdio_unlock();
        ESP_RETURN_ON_FALSE(err == ESP_OK,, TAG, "WiFi send error");

        if (esp_wifi_is_tx_callback(eb)) {
            net80211_en_txdq(eb);
            esp_sip_txd_post();
        } else {
            esp_sip_recycle(eb);
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

    if (!eb) {
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
    list_insert(eb);
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
    sip_register_tx_data_cb(wifi_send_data);
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
