/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __EXT_SDIO_ADAPTER_H__
#define __EXT_SDIO_ADAPTER_H__

#include "esp_err.h"
#include "esp_types.h"

#include "sdmmc_cmd.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EXT_CONN_WIFI_SDIO_FUNC (1)
#define EXT_CONN_BT_SDIO_FUNC   (2)

void esp_extconn_sdio_lock(void);

void esp_extconn_sdio_unlock(void);

esp_err_t esp_extconn_sdio_init(sdmmc_card_t *card);

esp_err_t esp_extconn_sdio_write_bytes(uint32_t function, uint32_t addr, void *src, size_t size);

esp_err_t esp_extconn_sdio_read_bytes(uint32_t function, uint32_t addr, void *src, size_t size);

esp_err_t esp_extconn_sdio_send_packet(uint32_t function, void *start, size_t length);

esp_err_t esp_extconn_sdio_get_packet(uint32_t function, void *out_buf, size_t size, size_t *out_length, uint32_t wait_ms);

esp_err_t esp_extconn_sdio_get_intr(uint32_t *intr_0, uint32_t *intr_1);

esp_err_t esp_extconn_sdio_clear_intr(uint32_t intr_0, uint32_t intr_1);

esp_err_t esp_extconn_sdio_wait_int(uint32_t wait);

esp_err_t esp_extconn_sdio_get_buffer_size(uint32_t *buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* __EXT_SDIO_ADAPTER_H__ */
