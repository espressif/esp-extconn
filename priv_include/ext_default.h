/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ESP_TRANS_H__
#define __ESP_TRANS_H__

#include "esp_extconn.h"

esp_err_t esp_extconn_boot(void);

esp_err_t esp_extconn_fw_init(esp_extconn_config_t *config);

esp_err_t esp_extconn_trans_recv_init(esp_extconn_config_t *config);

esp_err_t esp_extconn_trans_wifi_init(esp_extconn_config_t *config);

esp_err_t esp_extconn_trans_bt_init(esp_extconn_config_t *config);

void esp_extconn_trans_bt_send_unlock(void);

void esp_extconn_trans_bt_recv(uint8_t *buff, size_t len);

#endif /* __ESP_TRANS_H__ */
