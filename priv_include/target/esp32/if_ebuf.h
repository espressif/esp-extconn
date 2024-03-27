/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_assert.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STAILQ_ENTRY(type)    \
struct {                      \
    struct type *stqe_next;   \
}

typedef struct lldesc_s {
    volatile uint32_t size: 12,
             length: 12,
             offset: 5,
             sosf: 1,
             eof: 1,
             owner: 1;
    volatile uint8_t *buf;
    union {
        volatile uint32_t empty;
        STAILQ_ENTRY(lldesc_s) qe;
    };
} lldesc_t;
ESP_STATIC_ASSERT(sizeof(lldesc_t) == 0xc, "Invalid size of lldesc_t structure");

typedef struct esf_tx_desc_t {
    uint32_t flags;
    uint32_t tid: 4,
             ac: 4,
             rrc: 8,
             src: 8,
             lrc: 8;

    uint32_t rsv: 16,
             acktime: 16;

    uint32_t rate: 8,
             ackrssi: 8,
             ctstime: 16;

    uint32_t kid: 8,
             crypto_type: 4,
             antenna: 4,
             phy_lora_only: 1,
             probreq_lora: 1,
             reserved: 1,
             ifidx: 1,
             qid: 4,
             status: 8;

    unsigned comp_cb_map: 32;
    unsigned timestamp: 32;

    void *rcSched;
} esf_tx_desc_t;
ESP_STATIC_ASSERT(sizeof(esf_tx_desc_t) == 0x20, "Invalid size of esf_tx_desc_t structure");

typedef struct esf_rx_desc_t {
    unsigned flags: 12;
    unsigned antenna: 4;
    unsigned service: 8;
    unsigned rsv: 8;
    unsigned timestamp: 32;

    uint8_t  ch_num;
    uint8_t  snd_ch;
    uint8_t  resv[2];
} esf_rx_desc_t;
ESP_STATIC_ASSERT(sizeof(esf_rx_desc_t) == 0xc, "Invalid size of esf_rx_desc_t structure");

#define LLDESC 1
#define ESP_MESH_SUPPORT 1
typedef struct esf_buf_s {
#ifndef ESP_SIP
    void     *pbuf;
#endif
#ifdef LLDESC
    lldesc_t *ds_head;
    lldesc_t *ds_tail;
    uint16_t  ds_len;
#define u_data_start ds_head->buf
#else
    uint8_t *data_start;
#define u_data_start data_start
#endif
    uint8_t  *buf_begin;

#ifndef ESP_SIP
    uint16_t hdr_len;
#endif

    uint16_t data_len;

    int16_t  chl_freq_offset;

    uint8_t  type;
    uint8_t  winsize;

#ifdef ESP_MESH_SUPPORT
    uint32_t mesh_seq: 16;
    uint32_t is_mesh_pkt: 1;
    uint32_t mesh_reserved: 15;
    uint8_t  is_header_len;
#endif

    void *trc;
    STAILQ_ENTRY(esf_buf_s) bqentry;

    union {
        esf_tx_desc_t *tx_desc;
        esf_rx_desc_t *rx_desc;
    } desc;
} esf_buf_t;
ESP_STATIC_ASSERT(sizeof(esf_buf_t) == 0x30, "Invalid size of esf_buf_t structure");

#define esf_buf esf_buf_t

#define STAILQ_NEXT(elm, field) ((elm)->field.stqe_next)

#define TO_TX_DESC(_eb) ((_eb)->desc.tx_desc)
#define TO_RX_DESC(_eb) ((_eb)->desc.rx_desc)

#define PP_F_AMPDU 0x00040000

#define PP_IS_AMPDU(_eb) (TO_TX_DESC(_eb)->flags & PP_F_AMPDU)

typedef enum {
    ESF_BUF_TX_PB = 1,
    ESF_BUF_MGMT_LBUF,
    ESF_BUF_MGMT_SBUF,
    ESF_BUF_MGMT_LLBUF,
    ESF_BUF_TX_SIP,    //ESF_BUF_BAR

    ESF_BUF_RX_BLOCK,
    ESF_BUF_RX_BLOCK_RAW,

    ESF_BUF_RX_MGMT,
    ESF_BUF_TX_CACHE,

    ESF_BUF_TX_SIP_TEST,

    ESF_BUF_MAX
} esf_buf_type_t;
ESP_STATIC_ASSERT(ESF_BUF_MAX == 0xb, "Invalid size of esf_buf_type_t structure");

#ifdef __cplusplus
}
#endif
