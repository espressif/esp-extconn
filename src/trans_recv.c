/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/portmacro.h"
#include "freertos/projdefs.h"
#include "freertos/semphr.h"

#include "esp_check.h"
#include "esp_bit_defs.h"
#include "esp_err.h"
#include "ext_sdio_adapter.h"
#include "esp_sip.h"
#include "esp_extconn.h"
#include "sdio_host_reg.h"
#include "sip2_common.h"
#include "ext_default.h"
#include "esp_dma_utils.h"

// TODO: 32K!!!
#define RECV_BUF_LEN (32 * 1024)
#define RECV_WAIT_MS (50)

static char *TAG = "trans_recv";
static uint8_t *recv_buf = NULL;
static  SemaphoreHandle_t sdio_mutex = NULL;

extern void sip_rx_process(uint8_t *buf, uint32_t len);

/* Function 1 & 2 can not operate at the same time. The global lock is still needed */
void esp_extconn_sdio_lock(void)
{
    xSemaphoreTake(sdio_mutex, portMAX_DELAY);
}

void esp_extconn_sdio_unlock(void)
{
    xSemaphoreGive(sdio_mutex);
}

static esp_err_t handle_intr0(uint32_t intr, uint32_t wait_ms)
{
    size_t rlen = 0;
    esp_extconn_sdio_lock();
    esp_extconn_sdio_get_packet(EXT_CONN_WIFI_SDIO_FUNC, recv_buf, RECV_BUF_LEN, &rlen, wait_ms);
    esp_extconn_sdio_unlock();
    ESP_RETURN_ON_FALSE(rlen >= sizeof(struct sip_hdr), ESP_FAIL, TAG, "recv error!");

    uint8_t *buf = recv_buf;
    while (rlen) {
        struct sip_hdr *hdr = (struct sip_hdr *)buf;
        ESP_LOGV(TAG, "total len %d FC0 %d len %d recycled_credits %" PRIu32" seq %" PRIu32,
                 rlen, hdr->fc[0], hdr->len, hdr->u.recycled_credits, hdr->seq);
        if (hdr->len <= 0 || (hdr->len & 3) != 0) {
            ESP_LOGE(TAG, "hdrlen %d err!!!", hdr->len);
            return ESP_FAIL;
        }
        uint32_t rxseq = esp_sip_increase_rxseq();
        if (hdr->seq != rxseq) {
            ESP_LOGE(TAG, "seq err!!! %" PRIu32 "%" PRIu32, hdr->seq, rxseq);
            return ESP_FAIL;
        }
        if (SIP_HDR_IS_CTRL(hdr)) {
            ESP_LOGV(TAG, "rx ctrl len %d, seq %" PRIu32, hdr->len, hdr->seq);
            esp_sip_parse_events(buf);
        }
#ifdef CONFIG_ESP_EXT_CONN_WIFI_ENABLE
        else if (SIP_HDR_IS_DATA(hdr)) {
            ESP_LOGV(TAG, "rx data len %d, seq %" PRIu32, hdr->len, hdr->seq);
            sip_rx_process(buf, hdr->len);
        } else if (SIP_HDR_IS_AMPDU(hdr)) {
            ESP_LOGV(TAG, "rx ampdu len %d, seq %" PRIu32, hdr->len, hdr->seq);
            sip_rx_process(buf, hdr->len);
        }
#endif
        else {
            ESP_LOGE(TAG, "%s ERROR!!!", __func__);
        }

        if (hdr->len < rlen) {
            rlen -= hdr->len;
        } else {
            ESP_LOGV(TAG, "rxseq %ld, hdr->sep %ld", rxseq, hdr->seq);
            break;
        }
        buf += hdr->len;
    }
    return ESP_OK;
}

#ifdef CONFIG_ESP_EXT_CONN_BT_ENABLE
static esp_err_t handle_intr1(uint32_t intr, uint32_t wait_ms)
{
    esp_err_t ret = ESP_FAIL;

    if (intr & SLCHOST_SLC1_BT_RX_NEW_PACKET_INT_RAW) {
        const size_t wanted = 1034;
        size_t rlen = 0;

        esp_extconn_sdio_lock();
        ret = esp_extconn_sdio_get_packet(EXT_CONN_BT_SDIO_FUNC, recv_buf, wanted, &rlen, wait_ms);
        esp_extconn_sdio_unlock();
        if (ret == ESP_OK) {
            esp_extconn_trans_bt_recv(recv_buf, rlen);
        }
    }

    if (intr & SLCHOST_SLC1_TOHOST_BIT0_INT_RAW) {
        uint32_t config_w1 = 0;

        esp_extconn_sdio_lock();
        ret = esp_extconn_sdio_clear_intr(0, SLCHOST_SLC1_TOHOST_BIT0_INT_CLR);
        if (ret != ESP_OK) {
            esp_extconn_sdio_unlock();
            ESP_LOGE(TAG, "clear intr failed");
        }

        ret = esp_extconn_sdio_read_bytes(1, ESP_SDIO_CONFIG_W1, (uint8_t *)&config_w1, 4);
        if (ret != ESP_OK) {
            esp_extconn_sdio_unlock();
            ESP_LOGE(TAG, "read bytes failed");
        }
        esp_extconn_sdio_unlock();

        if (config_w1 & 0x1) {
            esp_extconn_trans_bt_send_unlock();
        }
    }

    return ret;
}
#endif

static void trans_recv_task(void *args)
{
    ESP_LOGI(TAG, "TRANS RECV START");
    while (true) {
        uint32_t intr_0 = 0;
        uint32_t intr_1 = 0;

        esp_err_t ret = esp_extconn_sdio_wait_int(portMAX_DELAY);
        if (ret != ESP_OK) {
            continue;
        }

        esp_extconn_sdio_lock();
        ret = esp_extconn_sdio_get_intr(&intr_0, &intr_1);
        ESP_RETURN_ON_FALSE(ret == ESP_OK, esp_extconn_sdio_unlock(), TAG, "interrupt read failed");
        if (intr_0 == 0 && intr_1 == 0) {
            esp_extconn_sdio_unlock();
            continue;
        }
        ret = esp_extconn_sdio_clear_intr(intr_0, 0);
        esp_extconn_sdio_unlock();
        if (intr_0 & SLCHOST_SLC0_RX_NEW_PACKET_INT_RAW) {
            handle_intr0(intr_0, RECV_WAIT_MS);
        }

#if CONFIG_ESP_EXT_CONN_BT_ENABLE
        if ((intr_1 & SLCHOST_SLC1_BT_RX_NEW_PACKET_INT_RAW) || (intr_1 & SLCHOST_SLC1_TOHOST_BIT0_INT_RAW)) {
            handle_intr1(intr_1, RECV_WAIT_MS);
        }
#endif
    }

    vTaskDelete(NULL);
}

esp_err_t esp_extconn_trans_recv_init(esp_extconn_config_t *config)
{
    sdio_mutex = xSemaphoreCreateMutex();
    size_t actual_size = 0;
    ESP_ERROR_CHECK(esp_dma_calloc(1, RECV_BUF_LEN, 0, (void*)&recv_buf, &actual_size));

    ESP_RETURN_ON_FALSE(recv_buf, ESP_ERR_NO_MEM, TAG, "buffer malloc failed");

    esp_err_t ret = xTaskCreatePinnedToCore(trans_recv_task, "trans_recv",
                                            config->recv_task_stack,
                                            NULL,
                                            config->recv_task_prio,
                                            NULL,
                                            config->recv_task_core)
                    == pdTRUE
                    ? ESP_OK
                    : ESP_FAIL;
    return ret;
}
