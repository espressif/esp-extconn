/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ESP_SIP__
#define __ESP_SIP__

#include <stdio.h>

#include "esp_err.h"
#include "if_ebuf.h"
#include "sip2_common.h"

#define roundup(x, y) (((x) + ((y)-1)) & (~((y)-1)))

#define MAC_ADDR_LEN  (6)

struct esp_fw_hdr {
    uint8_t magic;
    uint8_t blocks;
    uint8_t pad[2];
    uint32_t entry_addr;
};

struct esp_fw_blk_hdr {
    uint32_t load_addr;
    uint32_t data_len;
};

typedef enum SIP_STATE {
    SIP_INIT = 0,
    SIP_PREPARE_BOOT,
    SIP_BOOT,
    SIP_SEND_INIT,
    SIP_WAIT_BOOTUP,
    SIP_RUN,
    SIP_SUSPEND,
    SIP_STOP
} SIP_STATE;

struct esp_sip {
    uint32_t rxseq;
    uint32_t txseq;
    volatile int32_t state;
    uint8_t  *rawbuf;
    uint16_t tx_blksz;
    uint16_t rx_blksz;
    uint32_t credit_to_reserve;
    uint32_t noise_floor;
    uint32_t slc_window_end_addr;
    uint8_t  wifi_addr[MAC_ADDR_LEN];
    uint8_t  bt_addr[MAC_ADDR_LEN];
};

esp_err_t esp_sip_init(void);
esp_err_t esp_sip_send_cmd(int cid, uint32_t cmdlen, void *cmd);
esp_err_t esp_sip_write_mem(uint32_t addr, const uint8_t *buf, uint32_t len);
esp_err_t esp_sip_bootup(uint32_t entry_addr);
esp_err_t esp_sip_parse_events(uint8_t *buf);

uint32_t esp_sip_increase_rxseq(void);
uint32_t esp_sip_increase_txseq(void);
uint32_t esp_sip_get_tx_blks(void);

uint8_t *esp_sip_get_mac(void);

typedef esp_err_t (* sip_tx_data_t)(esf_buf *eb);
typedef esp_err_t (* sip_tx_cmd_t)(enum sip_cmd_id cmd_id, uint32_t cmd_len, void *cmd);
typedef int (* sip_get_coex_status)(void);

void sip_register_tx_data_cb(sip_tx_data_t fn);
void sip_register_tx_cmd_cb(sip_tx_cmd_t fn);
void sip_register_get_coex_status_cb(sip_get_coex_status fn);

#endif /* __ESP_SIP__ */
