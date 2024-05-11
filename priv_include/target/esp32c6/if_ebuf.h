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
    volatile uint32_t size: 14,
             length: 14,
             offset: 1,
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
             bw_ht40: 1,
             ifidx: 1,
             qid: 4,
             status: 8;

    unsigned comp_cb_map: 32;
    unsigned timestamp: 32;

    void *rcSched;
    uint8_t pti;
    uint16_t mt_time;
    uint8_t aid;
    uint16_t cbw20_symbols;
    uint16_t cbw20_min_subfrm_len;
    uint8_t cbw20_ampdu_count;
    uint16_t cbw40_min_subfrm_len;
    uint8_t cbw40_ampdu_count;
    uint32_t gi_ltf_size: 2,
             ersu_enabled: 1,
             proto: 4,
             dcm_max_constellation: 2,
             hesiga_sr_value15_allowed: 1,
             txop_duration_rts_threshold: 10,
             need_protection: 1,
             single: 1,
             dcm: 1,
             omc: 1,
             is_qos_null: 1,
             is_force_smpdu: 1,
             rsvd: 6;
    int rts_threshold;
    bool twt_setup;
    uint8_t itwt_fid;
    uint64_t itwt_fid_sp_us;
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
//#define HAL_MAC_ESP32AC 
#define HAL_MAC_ESP32AX 1

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

#ifdef HAL_MAC_ESP32AC
    uint32_t data_len;
#else
    uint16_t data_len;
#endif
    int16_t  chl_freq_offset;

    uint8_t  type;
    uint8_t  winsize;

#ifdef ESP_MESH_SUPPORT
    uint32_t mesh_seq: 16;
    uint32_t is_mesh_pkt: 1;
    uint32_t mesh_reserved: 15;
    uint8_t  is_header_len;
#endif
    
#ifdef HAL_MAC_ESP32AX
    uint8_t  eapol_msg_id;
    uint16_t msdu_len;
    uint16_t seqno:12;
    uint16_t append_qos_null:1;
    uint16_t entry:1;
    uint16_t resort:1;
    uint16_t rsvd:1;
    uint8_t  ampdu_count;
    bool     itwt_setup_frame;
    bool     itwt_teardown_frame;
#ifdef HAL_MAC_ESP32AX_ENABLE_TWT_DBG
    uint32_t start;
    uint32_t busy;
    uint32_t enable;
#endif
#endif /* HAL_MAC_ESP32AX */

    void *trc;
    STAILQ_ENTRY(esf_buf_s) bqentry;

    union {
        esf_tx_desc_t *tx_desc;
        esf_rx_desc_t *rx_desc;
    } desc;

#ifdef HAL_MAC_ESP32AX
    bool itwt_suspend_frame;
    bool btwt_setup_frame;
    bool btwt_teardown_frame;
#endif

} esf_buf_t;

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
