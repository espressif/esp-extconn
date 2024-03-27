/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _SIP2_COMMON_H
#define _SIP2_COMMON_H

#ifndef __packed
#define __packed __attribute__((__packed__))
#endif

/* max 16 types */
typedef enum {
    SIP_CTRL = 0,
    SIP_DATA,
    SIP_DATA_AMPDU,
    SIP_HRBRID_DATA
} SIP_TYPE;

typedef enum {
    SIP_TX_CTRL_BUF = 0, /* from host */
    SIP_RX_CTRL_BUF,     /* to host   */
    SIP_TX_DATA_BUF,     /* from host */
    SIP_RX_DATA_BUF      /* to host   */
} SIP_BUF_TYPE;

enum sip_cmd_id {
    SIP_CMD_GET_VER = 0,
    SIP_CMD_WRITE_MEMORY,    // 1 ROM code
    SIP_CMD_READ_MEMORY,     // 2
    SIP_CMD_WRITE_REG,       // 3 ROM code
    SIP_CMD_READ_REG,        // 4
    SIP_CMD_BOOTUP,          // 5 ROM code
    SIP_CMD_COPYBACK,        // 6
    SIP_CMD_INIT,            // 7
    SIP_CMD_SCAN,            // 8
    SIP_CMD_SETKEY,          // 9
    SIP_CMD_CONFIG,          // 10
    SIP_CMD_BSS_INFO_UPDATE, // 11
    SIP_CMD_LOOPBACK,        // 12  ROM code
    // do not add cmd before this line
    SIP_CMD_SET_WMM_PARAM,   // 13
    SIP_CMD_AMPDU_ACTION,    // 14
    SIP_CMD_HB_REQ,          // 15
    SIP_CMD_RESET_MAC,       // 16
    SIP_CMD_PRE_DOWN,        // 17
    SIP_CMD_SLEEP,           // 18 /* for sleep testing */
    SIP_CMD_WAKEUP,          // 19 /* for sleep testing */
    SIP_CMD_DEBUG,           // 20 /* for general testing */
    SIP_CMD_GET_FW_VER,      // 21 /* get fw rev. */
    SIP_CMD_SETVIF,          // 22
    SIP_CMD_SETSTA,          // 23
    SIP_CMD_PS,              // 24
    SIP_CMD_ATE,             // 25
    SIP_CMD_SUSPEND,         // 26
    SIP_CMD_RECALC_CREDIT,   // 27
    SIP_CMD_BT_STATE,        // 28
    SIP_CMD_TEST,            // 29
    SIP_CMD_MAX,
};

enum {
    SIP_EVT_TARGET_ON = 0,    //
    SIP_EVT_BOOTUP,           // 1 in ROM code
    SIP_EVT_COPYBACK,         // 2
    SIP_EVT_SCAN_RESULT,      // 3
    SIP_EVT_TX_STATUS,        // 4
    SIP_EVT_CREDIT_RPT,       // 5, in ROM code
    SIP_EVT_ERROR,            // 6
    SIP_EVT_LOOPBACK,         // 7, in ROM code
    SIP_EVT_SNPRINTF_TO_HOST, // 8  in ROM code
    // do not add evt before this line
    SIP_EVT_HB_ACK,           // 9
    SIP_EVT_RESET_MAC_ACK,    // 10
    SIP_EVT_WAKEUP,           // 11 /* for sleep testing */
    SIP_EVT_DEBUG,            // 12 /* for general testing */
    SIP_EVT_PRINT_TO_HOST,    // 13
    SIP_EVT_TRC_AMPDU,        // 14
    SIP_EVT_ROC,              // 15
    SIP_EVT_RESETTING,        // 16
    SIP_EVT_ATE,              // 17
    SIP_EVT_EP,               // 18
    SIP_EVT_INIT_EP,          // 19
    SIP_EVT_SLEEP,            // 20
    SIP_EVT_TXIDLE,           // 21
    SIP_EVT_NOISEFLOOR,       // 22
    SIP_EVT_NULLFUNC_REPORT,  // 23
    SIP_EVT_COEX_STATE,       // 24
    SIP_EVT_MAX
};

#define SIP_IFIDX_MASK 0xf0
#define SIP_IFIDX_S    4
#define SIP_TYPE_MASK  0x0f
#define SIP_TYPE_S     0

#define SIP_HDR_GET_IFIDX(fc0)        (((fc0)&SIP_IFIDX_MASK) >> SIP_IFIDX_S)
#define SIP_HDR_SET_IFIDX(fc0, ifidx) ((fc0) = ((fc0) & ~SIP_IFIDX_MASK) | ((ifidx) << SIP_IFIDX_S & SIP_IFIDX_MASK))
#define SIP_HDR_GET_TYPE(fc0)         ((fc0)&SIP_TYPE_MASK)
/* assume type field is cleared */
#define SIP_HDR_SET_TYPE(fc0, type)   ((fc0) = ((fc0) & ~SIP_TYPE_MASK) | ((type)&SIP_TYPE_MASK))

/* sip 2.0, not hybrid header so far */
#define SIP_HDR_IS_CTRL(hdr)   (SIP_HDR_GET_TYPE((hdr)->fc[0]) == SIP_CTRL)
#define SIP_HDR_IS_DATA(hdr)   (SIP_HDR_GET_TYPE((hdr)->fc[0]) == SIP_DATA)
#define SIP_HDR_IS_AMPDU(hdr)  (SIP_HDR_GET_TYPE((hdr)->fc[0]) == SIP_DATA_AMPDU)
#define SIP_HDR_IS_HRBRID(hdr) (SIP_HDR_GET_TYPE((hdr)->fc[0]) == SIP_HRBRID_DATA)

/* fc[1] flags, only for data pkt. Ctrl pkts use fc[1] as eventID */
#define SIP_HDR_SET_FLAGS(hdr, flags) ((hdr)->fc[1] |= (flags))
#define SIP_HDR_F_MORE_PKT            0x1
#define SIP_HDR_F_NEED_CRDT_RPT       0x2
#define SIP_HDR_F_SYNC                0x4
#define SIP_HDR_F_SYNC_RESET          0x8
#define SIP_HDR_F_PM_TURNING_ON       0x10
#define SIP_HDR_F_PM_TURNING_OFF      0x20

#define SIP_HDR_NEED_CREDIT_UPDATE(hdr) ((hdr)->fc[1] & SIP_HDR_F_NEED_CRDT_RPT)
#define SIP_HDR_IS_MORE_PKT(hdr)        ((hdr)->fc[1] & SIP_HDR_F_MORE_PKT)
#define SIP_HDR_IS_CRDT_RPT(hdr)        ((hdr)->fc[1] & SIP_HDR_F_CRDT_RPT)
#define SIP_HDR_IS_SYNC(hdr)            ((hdr)->fc[1] & SIP_HDR_F_SYNC)
#define SIP_HDR_IS_SYNC_RESET(hdr)      ((hdr)->fc[1] & SIP_HDR_F_SYNC_RESET)
#define SIP_HDR_IS_SYNC_PKT(hdr)        (SIP_HDR_IS_SYNC(hdr) | SIP_HDR_IS_SYNC_RESET(hdr))
#define SIP_HDR_SET_SYNC(hdr)           SIP_HDR_SET_FLAGS((hdr), SIP_HDR_F_SYNC)
#define SIP_HDR_SET_SYNC_RESET(hdr)     SIP_HDR_SET_FLAGS((hdr), SIP_HDR_F_SYNC_RESET)
#define SIP_HDR_SET_MORE_PKT(hdr)       SIP_HDR_SET_FLAGS((hdr), SIP_HDR_F_MORE_PKT)
#define SIP_HDR_SET_PM_TURNING_ON(hdr)  SIP_HDR_SET_FLAGS((hdr), SIP_HDR_F_PM_TURNING_ON)
#define SIP_HDR_IS_PM_TURNING_ON(hdr)   ((hdr)->fc[1] & SIP_HDR_F_PM_TURNING_ON)
#define SIP_HDR_SET_PM_TURNING_OFF(hdr) SIP_HDR_SET_FLAGS((hdr), SIP_HDR_F_PM_TURNING_OFF)
#define SIP_HDR_IS_PM_TURNING_OFF(hdr)  ((hdr)->fc[1] & SIP_HDR_F_PM_TURNING_OFF)

/*
 * fc[0]: first 4bit: ifidx; last 4bit: type
 * fc[1]: flags
 *
 *   Don't touch the header definitons
 */
struct sip_hdr_min {
    uint8_t fc[2];
    uint16_t len;
} __packed;

/* not more than 4byte long */
struct sip_tx_data_info {
    uint8_t tid;
    uint8_t ac;
    uint8_t p2p : 1,
            enc_flag : 7;
    uint8_t hw_kid;
} __packed;

/* NB: this structure should be not more than 4byte !! */
struct sip_tx_info {
    union {
        uint32_t cmdid;
        struct sip_tx_data_info dinfo;
    } u;
} __packed;

struct sip_hdr {
    uint8_t fc[2]; // fc[0]: type and ifidx ; fc[1] is eventID if the first ctrl pkt in the chain. data pkt still can use fc[1] to set flag
    uint16_t len;
    union {
        volatile uint32_t recycled_credits; /* last 12bits is credits, first 20 bits is actual length of the first pkt in the chain */
        struct sip_tx_info tx_info;
    } u;
    uint32_t seq;
} __packed;

#define h_credits  u.recycled_credits
#define c_evtid    fc[1]
#define c_cmdid    u.tx_info.u.cmdid
#define d_ac       u.tx_info.u.dinfo.ac
#define d_tid      u.tx_info.u.dinfo.tid
#define d_p2p      u.tx_info.u.dinfo.p2p
#define d_enc_flag u.tx_info.u.dinfo.enc_flag
#define d_hw_kid   u.tx_info.u.dinfo.hw_kid

#define SIP_CREDITS_MASK 0xfff /* last 12 bits */

#ifdef HOST_RC

#define RC_CNT_MASK 0xf

struct sip_rc_status {
    uint32_t rc_map;
    union {
        uint32_t rc_cnt1 : 4,
                 rc_cnt2 : 4,
                 rc_cnt3 : 4,
                 rc_cnt4 : 4,
                 rc_cnt5 : 4;

        uint32_t rc_cnt_store;
    };
};

/* copy from mac80211.h */
struct sip_tx_rc {
    struct ieee80211_tx_rate rates[IEEE80211_TX_MAX_RATES];
    s8 rts_cts_rate_idx;
};
#endif /* HOST_RC */

#define SIP_HDR_MIN_LEN   4
#define SIP_HDR_LEN       sizeof(struct sip_hdr)
#define SIP_CTRL_HDR_LEN  SIP_HDR_LEN /* same as sip_hdr in sip2 design */
#define SIP_BOOT_BUF_SIZE 256
#define SIP_CTRL_BUF_SZ   256 /* too much?? */
#define SIP_CTRL_BUF_N    6
#define SIP_CTRL_TXBUF_N  2
#define SIP_CTRL_RXBUF_N  4

/* WAR for mblk */
#define SIP_RX_ADDR_PREFIX_MASK 0xfc000000
#define SIP_RX_ADDR_SHIFT       6 /* [31:5],  shift 6 bits*/

struct sip_cmd_write_memory {
    uint32_t addr;
    uint32_t len;
} __packed;

struct sip_cmd_read_memory {
    uint32_t addr;
    uint32_t len;
} __packed;

struct sip_cmd_write_i2c {
    uint8_t addr;
    uint8_t block;
    uint8_t hostid;
    uint32_t pdata;
} __packed;

struct sip_cmd_read_i2c {
    uint8_t addr;
    uint8_t block;
    uint8_t hostid;
    uint32_t val;
} __packed;

struct sip_cmd_write_reg {
    uint32_t addr;
    uint32_t val;
} __packed;

struct sip_cmd_init {
    uint8_t wifi_bt_open;
    uint8_t phy_init_data[128];
} __packed;

struct sip_cmd_bootup {
    uint32_t boot_addr;
    uint32_t discard_link;
} __packed;

struct sip_cmd_loopback {
    uint32_t txlen;   // host to target packet len, 0 means no txpacket
    uint32_t rxlen;   // target to host packet len, 0 means no rxpacket
    uint32_t pack_id; // sequence of packet
} __packed;

struct sip_evt_loopback {
    uint32_t txlen;   // host to target packet len, 0 means no txpacket
    uint32_t rxlen;   // target to host packet len, 0 means no rxpacket
    uint32_t pack_id; // sequence of packet
} __packed;

struct sip_cmd_copyback {
    uint32_t addr;
    uint32_t len;
} __packed;

struct sip_cmd_scan {
    //        uint8_t  ssid[32];
    uint8_t ssid_len;
    //        uint8_t hw_channel[14];
    uint8_t n_channels;
    uint8_t ie_len;
    uint8_t aborted;
} __packed; // ie[] append at the end

#define EXT_ETH_ALEN     (6)

struct sip_cmd_setkey {
    uint8_t bssid_no;
    uint8_t addr[EXT_ETH_ALEN];
    uint8_t alg;
    uint8_t keyidx;
    uint8_t hw_key_idx;
    uint8_t flags;
    uint8_t keylen;
    uint8_t key[32];
    uint8_t en_pmf: 1,
            en_amsdu: 1,
            recv: 6;

} __packed;

#define IEEE80211_CHAN_HT40U 0x00020000
#define IEEE80211_CHAN_HT40D 0x00040000
struct sip_cmd_config {
    uint16_t center_freq;
    uint16_t duration;
    uint32_t channel_type;
    // bool scan_flag;
} __packed;

struct sip_cmd_bss_info_update {
    uint8_t  bssid[EXT_ETH_ALEN];
    uint16_t isassoc;
    uint32_t beacon_int;
    uint8_t  bssid_no;
} __packed;

struct sip_evt_bootup {
    uint16_t tx_blksz;
    uint8_t mac_addr[EXT_ETH_ALEN];
    /* anything else ? */
} __packed;

struct sip_cmd_setvif {
    uint8_t index;
    uint8_t mac[EXT_ETH_ALEN];
    uint8_t set;
    uint8_t op_mode;
    uint8_t is_p2p;
} __packed;

enum esp_ieee80211_phytype {
    ESP_IEEE80211_T_CCK = 0,
    ESP_IEEE80211_T_OFDM = 1,
    ESP_IEEE80211_T_HT20_L = 2,
    ESP_IEEE80211_T_HT20_S = 3,
    ESP_IEEE80211_T_HT40_L = 4,
    ESP_IEEE80211_T_HT40_S = 5,
};

struct sip_cmd_setsta {
    uint8_t ifidx;
    uint8_t index;
    uint8_t set;
    uint8_t phymode;
    uint8_t mac[EXT_ETH_ALEN];
    uint16_t aid;
    uint8_t ampdu_factor;
    uint8_t ampdu_density;
    uint16_t rssi;
    uint8_t  max_rate;
    uint8_t  is_sig_test;
} __packed;

struct sip_cmd_ps {
    uint8_t  pm_enable;
    uint8_t  ifidx;
    uint16_t aid;
    uint8_t  addr[EXT_ETH_ALEN];
    uint16_t bcn_interval;
    uint32_t rx_bcn_time;
    uint64_t tsfstamp;
} __packed;

struct sip_cmd_suspend {
    uint8_t suspend;
    uint8_t resv[3];
} __packed;

#define SIP_DUMP_RPBM_ERR  BIT(0)
#define SIP_RXABORT_FIXED  BIT(1)
#define SIP_SUPPORT_BGSCAN BIT(2)
struct sip_evt_bootup2 {
    uint16_t tx_blksz;
    uint8_t  mac_addr[EXT_ETH_ALEN];
    uint16_t rx_blksz;
    uint8_t  credit_to_reserve;
    uint8_t  options;
    short    noise_floor;
    uint8_t  mac_type;
    uint8_t  resv[1];
    /* anything else ? */
} __packed;

typedef enum {
    TRC_TX_AMPDU_STOPPED = 1,
    TRC_TX_AMPDU_OPERATIONAL,
    TRC_TX_AMPDU_WAIT_STOP,
    TRC_TX_AMPDU_WAIT_OPERATIONAL,
    TRC_TX_AMPDU_START,
} trc_ampdu_state_t;

struct sip_evt_trc_ampdu {
    uint8_t state;
    uint8_t tid;
    uint8_t addr[EXT_ETH_ALEN];
} __packed;

struct sip_cmd_set_wmm_params {
    uint8_t  aci;
    uint8_t  aifs;
    uint8_t  ecw_min;
    uint8_t  ecw_max;
    uint16_t txop_us;
} __packed;

#define SIP_AMPDU_RX_START       0
#define SIP_AMPDU_RX_STOP        1
#define SIP_AMPDU_TX_OPERATIONAL 2
#define SIP_AMPDU_TX_STOP        3
struct sip_cmd_ampdu_action {
    uint8_t  action;
    uint8_t  index;
    uint8_t  tid;
    uint8_t  win_size;
    uint16_t ssn;
    uint8_t  addr[EXT_ETH_ALEN];
} __packed;

#define SIP_TX_ST_OK     0
#define SIP_TX_ST_NOEB   1
#define SIP_TX_ST_ACKTO  2
#define SIP_TX_ST_ENCERR 3

// NB: sip_tx_status must be 4 bytes aligned
struct sip_tx_status {
    uint32_t sip_seq;
#ifdef HOST_RC
    struct sip_rc_status rcstatus;
#endif        /* HOST_RC */
    uint8_t errno; /* success or failure code */
    uint8_t rate_index;
    char    ack_signal;
    uint8_t pad;
} __packed;

struct sip_evt_tx_report {
    uint32_t pkts;
    struct   sip_tx_status status[0];
} __packed;

struct sip_evt_tx_mblk {
    uint32_t mblk_map;
} __packed;

struct sip_evt_scan_report {
    uint16_t scan_id;
    uint16_t aborted;
} __packed;

struct sip_evt_roc {
    uint16_t state; // start:1, end :0
    uint16_t is_ok;
} __packed;

struct sip_evt_txidle {
    uint32_t last_seq;
} __packed;

struct sip_evt_noisefloor {
    short noise_floor;
    uint16_t pad;
} __packed;

struct sip_evt_nullfunc_report {
    uint8_t ifidx;
    uint8_t index;
    uint8_t status;
    uint8_t pad;
} __packed;

struct sip_evt_coex_state {
    uint16_t wifi_st;
    uint16_t ble_st;
    uint16_t bt_st;
    uint16_t recv;
} __packed;

/*
 *  for mblk direct memory access, no need for sip_hdr. tx: first 2k for contrl msg,
 *  rest of 14k for data.  rx, same.
 */
#ifdef TEST_MODE

struct sip_cmd_sleep {
    uint32_t sleep_mode;
    uint32_t sleep_tm_ms;
    uint32_t wakeup_tm_ms; // zero: after receive bcn, then sleep, nozero: delay nozero ms to sleep
    uint32_t sleep_times;  // zero: always sleep, nozero: after nozero number sleep/wakeup, then end up sleep
} __packed;

struct sip_cmd_wakeup {
    uint32_t check_data; // 0:copy to event
} __packed;

struct sip_cmd_bt_state {
    uint8_t state;
} __packed;

struct sip_evt_wakeup {
    uint32_t check_data;
} __packed;

struct sip_cmd_test {
    uint8_t  cmd;
    uint32_t addr;
    uint32_t val;
} __packed;
// general debug command
struct sip_cmd_debug {
    uint32_t cmd_type;
    uint32_t para_num;
    uint32_t para[10];
} __packed;

struct sip_evt_debug {
    uint16_t len;
    uint32_t results[12];
    uint16_t pad;
} __packed;

struct sip_cmd_ate {
    // uint8_t  len;
    uint8_t cmdstr[0];
} __packed;

#endif // ifdef TEST_MODE

#endif /* _SIP_COMMON_H_ */
