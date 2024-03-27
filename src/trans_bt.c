/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_bluedroid_hci.h"
#include "ext_default.h"
#include "ext_sdio_adapter.h"
#include "esp_extconn.h"
#include "esp_dma_utils.h"

typedef struct {
    uint32_t len     : 24,
             type    : 2,
             subtype : 6;
    uint32_t seq;
    uint8_t  data[0];
} sbp_hdr_t;

typedef struct {
    uint8_t *data;
    uint32_t len;
} tx_msg_t;

typedef struct {
    SemaphoreHandle_t tx_sem;
    QueueHandle_t     tx_que;
    TaskHandle_t      task_handle;
} bt_tx_ctx_t;

static const char *TAG = "trans_bt";
static bt_tx_ctx_t *bt_tx = NULL;
static esp_bluedroid_hci_driver_callbacks_t s_callback = { 0 };

static void esp_extconn_trans_bt_send_lock()
{
    xSemaphoreTake(bt_tx->tx_sem, portMAX_DELAY);
}

void esp_extconn_trans_bt_send_unlock()
{
    xSemaphoreGive(bt_tx->tx_sem);
}

static void bt_tx_task(void *arg)
{
    int ret = 0;
    uint32_t tx_seq = 0;
    sbp_hdr_t *hdr = NULL;
    tx_msg_t buf;

    ESP_LOGI(TAG, "BT Send START");

    while (1) {
        esp_extconn_trans_bt_send_lock();

        if (xQueueReceive(bt_tx->tx_que, &buf, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        if (buf.data && buf.len) {
            size_t actual_size = 0;
            ESP_ERROR_CHECK(esp_dma_calloc(1, sizeof(sbp_hdr_t) + buf.len, 0, (void*)&hdr, &actual_size));

            hdr->len = sizeof(sbp_hdr_t) + buf.len;
            hdr->type = 0;
            hdr->subtype = 0;
            hdr->seq = tx_seq++;
            memcpy(hdr->data, buf.data, buf.len);
            esp_extconn_sdio_lock();
            ret = esp_extconn_sdio_send_packet(EXT_CONN_BT_SDIO_FUNC, (void *)hdr, (size_t)(hdr->len));
            esp_extconn_sdio_unlock();
            if (ret) {
                ESP_LOGE(TAG, "tx packet err! ret=%d", ret);
            }

            free(buf.data);
            free(hdr);
        }
    }

    vTaskDelete(NULL);
    return;
}

static void bt_send_data(uint8_t *data, uint16_t len)
{
    tx_msg_t msg = { 0 };
    msg.data = calloc(1, len);
    msg.len = len;
    memcpy(msg.data, data, len);

    xQueueSend(bt_tx->tx_que, &msg, pdMS_TO_TICKS(5000));
}

static bool esp_extconn_bt_check_receive_available(void)
{
    return true;
}

static void bt_start_up(esp_extconn_config_t *config)
{
    bt_tx->tx_sem = xSemaphoreCreateCounting(1, 1);
    bt_tx->tx_que = xQueueCreate(25, sizeof(tx_msg_t));

    xTaskCreatePinnedToCore(bt_tx_task, "bt_tx",
                            config->bt_task_stack,
                            NULL,
                            config->bt_task_prio,
                            &bt_tx->task_handle,
                            config->bt_task_core);
}

static void esp_extconn_bt_shut_down(void)
{
    vSemaphoreDelete(bt_tx->tx_sem);
    vQueueDelete(bt_tx->tx_que);
    if (bt_tx->task_handle) {
        vTaskDelete(bt_tx->task_handle);
    }
}

void esp_extconn_trans_bt_recv(uint8_t *buff, size_t len)
{
    if (s_callback.notify_host_recv) {
        ESP_LOGV(TAG, "bt recv %d, %d", sizeof(sbp_hdr_t), len);
        s_callback.notify_host_recv(buff + sizeof(sbp_hdr_t), len - sizeof(sbp_hdr_t));
    }
}

esp_err_t esp_extconn_bt_register_host_callback(const esp_bluedroid_hci_driver_callbacks_t *callback)
{
    s_callback.notify_host_send_available = callback->notify_host_send_available;
    s_callback.notify_host_recv = callback->notify_host_recv;

    return ESP_OK;
}

esp_err_t esp_extconn_trans_bt_init(esp_extconn_config_t *config)
{
    bt_tx = malloc(sizeof(bt_tx_ctx_t));

    esp_bluedroid_hci_driver_operations_t operations = {
        .send = bt_send_data,
        .check_send_available = esp_extconn_bt_check_receive_available,
        .register_host_callback = esp_extconn_bt_register_host_callback,
    };
    esp_bluedroid_attach_hci_driver(&operations);

    bt_start_up(config);
    return ESP_OK;
}
