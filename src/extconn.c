/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"

#include "esp_extconn.h"
#include "ext_default.h"

static char *TAG = "extconn";

esp_err_t esp_extconn_init(esp_extconn_config_t *config)
{
    esp_err_t ret = esp_extconn_boot();
    if (ret == ESP_OK) {
        ret = esp_extconn_fw_init(config);
    }
#ifdef CONFIG_ESP_EXT_CONN_WIFI_ENABLE
    if (ret == ESP_OK) {
        ret = esp_extconn_trans_wifi_init(config);
    }
#endif
#ifdef CONFIG_ESP_EXT_CONN_BT_ENABLE
    /* BT will send packet immediately afetr initiation, so delay its initiation to wait chip on */
    vTaskDelay(100);
    if (ret == ESP_OK) {
        ret = esp_extconn_trans_bt_init(config);
    }
#endif

    ESP_LOGI(TAG, "extconn init ret 0x%X", ret);
    return ret;
}
