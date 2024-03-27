/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_dma_utils.h"

#include "esp_sip.h"
#include "ext_sdio_adapter.h"
#include "esp_extconn.h"
#include "if_ebuf.h"
#include "sdio_host_reg.h"
#include "sip2_common.h"

#include "eagle_init_data.h"

static const char *TAG = "sip";
static struct esp_sip *sip = NULL;
static SemaphoreHandle_t boot_sem = NULL;

extern void coex_schm_status_set(uint16_t wifi_st, uint16_t ble_st, uint16_t bt_st);

esp_err_t esp_sip_send_cmd(int cid, uint32_t cmdlen, void *cmd)
{
    esp_err_t err = 0;
    struct sip_hdr *shdr = NULL;
    uint16_t len = cmdlen + SIP_CTRL_HDR_LEN;

    ESP_LOGI(TAG, "sip cmd %d", cid);

    uint8_t *buffer = calloc(1, len);
    ESP_RETURN_ON_FALSE(buffer != NULL, ESP_ERR_NO_MEM, TAG, "buffer calloc failed");

    shdr = (struct sip_hdr *)(buffer);
    memset(shdr, 0x0, SIP_CTRL_HDR_LEN);
    SIP_HDR_SET_TYPE(shdr->fc[0], SIP_CTRL);
    SIP_HDR_SET_SYNC(shdr);
    shdr->len = len;
    shdr->c_cmdid = cid;

    if (cid == SIP_CMD_BOOTUP) {
        ESP_LOGI(TAG, "bootup rxseq=%" PRIu32, sip->rxseq);
        sip->rxseq = 0;
        sip->txseq = 0;
    } else if (cid == SIP_CMD_INIT) {
        sip->txseq++;
    }

    shdr->seq = 0;

    memcpy(buffer + SIP_CTRL_HDR_LEN, (uint8_t *)cmd, cmdlen);
    esp_extconn_sdio_lock();
    err = esp_extconn_sdio_send_packet(1, buffer, len);
    esp_extconn_sdio_unlock();
    free(buffer);
    return err;
}

esp_err_t esp_sip_write_mem(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    struct sip_cmd_write_memory *cmd;
    struct sip_hdr *chdr;
    uint32_t remains;
    uint16_t hdrs, bufsize;
    uint32_t loadaddr;
    const uint8_t *src;
    esp_err_t err = 0;

    if (sip->rawbuf == NULL) {
        size_t actual_size = 0;
        err = esp_dma_calloc(1, SIP_BOOT_BUF_SIZE, 0, (void*)&sip->rawbuf, &actual_size);
    }

    chdr = (struct sip_hdr *)sip->rawbuf;
    SIP_HDR_SET_TYPE(chdr->fc[0], SIP_CTRL);
    chdr->c_cmdid = SIP_CMD_WRITE_MEMORY;

    remains = len;
    hdrs = sizeof(struct sip_hdr) + sizeof(struct sip_cmd_write_memory);

    while (remains) {
        src = &buf[len - remains];
        loadaddr = addr + (len - remains);

        if (remains < (SIP_BOOT_BUF_SIZE - hdrs)) {
            /* aligned with 4 bytes */
            bufsize = roundup(remains, 4);
            memset(sip->rawbuf + hdrs, 0x0, bufsize);
            remains = 0;
        } else {
            bufsize = SIP_BOOT_BUF_SIZE - hdrs;
            remains -= bufsize;
        }

        chdr->len = bufsize + hdrs;
        chdr->seq = sip->txseq++;
        cmd = (struct sip_cmd_write_memory *)(sip->rawbuf + SIP_CTRL_HDR_LEN);
        cmd->len = bufsize;
        cmd->addr = loadaddr;

        vTaskDelay(pdMS_TO_TICKS(1));
        memcpy(sip->rawbuf + hdrs, src, bufsize);
        err = esp_extconn_sdio_write_bytes(1, ESP_SLAVE_CMD53_END_ADDR - chdr->len, sip->rawbuf, (chdr->len + 3) & (~3));
        ESP_RETURN_ON_FALSE(err == ESP_OK, ESP_FAIL, TAG, "Send buffer failed");
    }
    return ESP_OK;
}

static int esp_sip_post_init(struct sip_evt_bootup2 *bevt)
{
    sip->tx_blksz = bevt->tx_blksz;
    sip->rx_blksz = bevt->rx_blksz;
    sip->credit_to_reserve = bevt->credit_to_reserve;

    sip->noise_floor = bevt->noise_floor;

    ESP_LOGI(TAG, "%02X:%02X:%02X:%02X:%02X:%02X", bevt->mac_addr[0], bevt->mac_addr[1], bevt->mac_addr[2], bevt->mac_addr[3], bevt->mac_addr[4], bevt->mac_addr[5]);
    memcpy(sip->wifi_addr, bevt->mac_addr, MAC_ADDR_LEN);
    return 0;
}

static esp_err_t sip_send_chip_init(struct esp_sip *sip)
{
    struct sip_cmd_init init_data;

    init_data.wifi_bt_open = EXT_CONN_FUNCTIONS_ENABLE;

    ESP_LOGI(TAG, "wifi_bt_open: %d\n", init_data.wifi_bt_open);

    memcpy(init_data.phy_init_data, (esp_init_data + 8), 128);

    return esp_sip_send_cmd(SIP_CMD_INIT, sizeof(esp_init_data), (void *)&init_data);
}

uint8_t *esp_extconn_get_mac(void)
{
    if (sip != NULL) {
        ESP_LOGI(TAG, "%s %x:%x:%x:%x:%x:%x",
                 __func__,
                 sip->wifi_addr[0],
                 sip->wifi_addr[1],
                 sip->wifi_addr[2],
                 sip->wifi_addr[3],
                 sip->wifi_addr[4],
                 sip->wifi_addr[5]);
        return sip->wifi_addr;
    }
    return NULL;
}

esp_err_t esp_sip_parse_events(uint8_t *buf)
{
    struct sip_hdr *hdr = (struct sip_hdr *)buf;
    esp_err_t ret = ESP_OK;

    switch (hdr->c_evtid) {
    case SIP_EVT_TARGET_ON: {
        /* use rx work queue to send... */
        if (sip->state == SIP_PREPARE_BOOT || sip->state == SIP_BOOT) {
            ESP_LOGI(TAG, "target on");
            sip->state = SIP_SEND_INIT;
            ret = sip_send_chip_init(sip);
            if (ret == ESP_OK) {
                sip->state = SIP_WAIT_BOOTUP;
            }
        } else {
            ESP_LOGE(TAG, "%s boot during wrong state %ld", __func__, sip->state);
        }
        break;
    }
    case SIP_EVT_BOOTUP: {
        struct sip_evt_bootup2 *bootup_evt = (struct sip_evt_bootup2 *)(buf + SIP_CTRL_HDR_LEN);
        if (sip->rawbuf) {
            free(sip->rawbuf);
            sip->rawbuf = NULL;
        }
        ESP_LOGI(TAG, "SIP_EVT_BOOTUP\n");
        esp_sip_post_init(bootup_evt);
        sip->state = SIP_RUN;
        xSemaphoreGive(boot_sem);
        break;
    }
#ifdef CONFIG_ESP_EXT_CONN_WIFI_ENABLE
    case SIP_EVT_COEX_STATE: {
        struct sip_evt_coex_state *coex_evt = (struct sip_evt_coex_state *)(buf + SIP_CTRL_HDR_LEN);
        uint16_t wifi_st =  coex_evt->wifi_st;
        uint16_t ble_st  =  coex_evt->ble_st;
        uint16_t bt_st   =  coex_evt->bt_st;
        coex_schm_status_set(wifi_st, ble_st, bt_st);
        break;
    }
#endif
    case SIP_EVT_CREDIT_RPT:

        break;
    default:
        ESP_LOGW(TAG, "%s default: %u", __func__, hdr->c_evtid);
        ret = ESP_OK;
        break;
    }

    return ret;
}

esp_err_t esp_sip_bootup(uint32_t entry_addr)
{
    esp_err_t ret = ESP_FAIL;
    struct sip_cmd_bootup bootcmd = {
        .boot_addr = entry_addr,
        .discard_link = 1,
    };

    if (esp_sip_send_cmd(SIP_CMD_BOOTUP, sizeof(struct sip_cmd_bootup), &bootcmd) == ESP_OK) {
        sip->state = SIP_PREPARE_BOOT;
        boot_sem = xSemaphoreCreateCounting(1, 0);
#ifdef CONFIG_ESP_EXT_CONN_WIFI_ENABLE
        if (xSemaphoreTake(boot_sem, pdMS_TO_TICKS(10000)) == pdTRUE) {
            ret = ESP_OK;
        } else {
            ret = ESP_ERR_TIMEOUT;
        }
#else
        ret = ESP_OK;
#endif
    } else {
        ESP_LOGE(TAG, "bootup cmd send failed");
    }
    vSemaphoreDelete(boot_sem);
    ESP_LOGI(TAG, "boot ret 0x%X", ret);
    return ret;
}

esp_err_t esp_sip_init(void)
{
    sip = calloc(1, sizeof(struct esp_sip));
    ESP_RETURN_ON_FALSE(sip != NULL, ESP_ERR_NO_MEM, TAG, "No MEM");
    sip->state = SIP_INIT;

    return ESP_OK;
}

uint32_t esp_sip_increase_rxseq(void)
{
    return sip->rxseq++;
}

uint32_t esp_sip_increase_txseq(void)
{
    return sip->txseq++;
}

uint32_t esp_sip_get_tx_blks(void)
{
    return sip->tx_blksz;
}
