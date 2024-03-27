/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ESP_EXTCONN_SDMMC_H__
#define __ESP_EXTCONN_SDMMC_H__

#include "sd_protocol_types.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t sdmmc_send_cmd_go_idle_state(sdmmc_card_t* card);

esp_err_t sdmmc_io_send_op_cond(sdmmc_card_t* card, uint32_t ocr, uint32_t *ocrp);

esp_err_t sdmmc_send_cmd_crc_on_off(sdmmc_card_t* card, bool crc_enable);

esp_err_t sdmmc_io_rw_extended(sdmmc_card_t* card, int function, uint32_t reg, int arg, void *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* __ESP_EXTCONN_SDMMC_H__ */
