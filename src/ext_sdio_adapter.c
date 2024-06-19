/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>

#include "esp_dma_utils.h"
#include "sd_protocol_defs.h"
#include "esp_check.h"
#include "ext_sdio_adapter.h"
#include "sdio_host_reg.h"
#include "esp_extconn_sdmmc.h"

typedef struct {
    sdmmc_card_t *card;
    uint32_t total_tx;
    uint32_t total_rx;
} sdio_host_t;

static const char *TAG = "esp_host";
static sdio_host_t *host = NULL;

static esp_err_t esp_extconn_sdio_init_slave_link(void);

static esp_err_t extconn_sdio_read_bytes(sdmmc_card_t* card, uint32_t function,
                                         uint32_t addr, void* dst, size_t size)
{
    uint8_t *pc_dst = dst;
    uint32_t arg = SD_ARG_CMD53_READ;

    if (function == EXT_CONN_WIFI_SDIO_FUNC) {
        arg |= SD_ARG_CMD53_INCREMENT;
    }

    while (size > 0) {
        size_t size_aligned = size & (~3);
        size_t will_transfer = size_aligned > 0 ? size_aligned : size;

        esp_err_t err = sdmmc_io_rw_extended(card, function, addr, arg,
                                             pc_dst, will_transfer);
        if (unlikely(err != ESP_OK)) {
            return err;
        }
        pc_dst += will_transfer;
        size -= will_transfer;
        addr += will_transfer;
    }
    return ESP_OK;
}

static esp_err_t extconn_sdio_write_bytes(sdmmc_card_t* card, uint32_t function,
                                          uint32_t addr, const void* src, size_t size)
{
    const uint8_t *pc_src = (const uint8_t*) src;
    uint32_t arg = SD_ARG_CMD53_WRITE;

    if (function == EXT_CONN_WIFI_SDIO_FUNC) {
        arg |= SD_ARG_CMD53_INCREMENT;
    }

    while (size > 0) {
        size_t size_aligned = size & (~3);
        size_t will_transfer = size_aligned > 0 ? size_aligned : size;

        esp_err_t err = sdmmc_io_rw_extended(card, function, addr, arg,
                                             (void*) pc_src, will_transfer);
        if (unlikely(err != ESP_OK)) {
            return err;
        }
        pc_src += will_transfer;
        size -= will_transfer;
        addr += will_transfer;
    }
    return ESP_OK;
}

static esp_err_t extconn_sdio_read_blocks(sdmmc_card_t* card, uint32_t function,
                                          uint32_t addr, void* dst, size_t size)
{
    uint32_t arg = SD_ARG_CMD53_READ | SD_ARG_CMD53_BLOCK_MODE;

    if (function == EXT_CONN_WIFI_SDIO_FUNC) {
        arg |= SD_ARG_CMD53_INCREMENT;
    }

    if (unlikely(size % 4 != 0)) {
        return ESP_ERR_INVALID_SIZE;
    }
    return sdmmc_io_rw_extended(card, function, addr, arg, dst, size);
}

static esp_err_t extconn_sdio_write_blocks(sdmmc_card_t* card, uint32_t function,
                                           uint32_t addr, const void* src, size_t size)
{
    uint32_t arg = SD_ARG_CMD53_WRITE | SD_ARG_CMD53_BLOCK_MODE;

    if (function == EXT_CONN_WIFI_SDIO_FUNC) {
        arg |= SD_ARG_CMD53_INCREMENT;
    }

    if (unlikely(size % 4 != 0)) {
        return ESP_ERR_INVALID_SIZE;
    }
    return sdmmc_io_rw_extended(card, function, addr, arg, (void*) src, size);
}

esp_err_t esp_extconn_sdio_write_bytes(uint32_t function, uint32_t addr, void *src, size_t size)
{
    return extconn_sdio_write_bytes(host->card, function, addr, src, size);
}

esp_err_t esp_extconn_sdio_read_bytes(uint32_t function, uint32_t addr, void *src, size_t size)
{
    return extconn_sdio_read_bytes(host->card, function, addr, src, size);
}

static esp_err_t esp_extconn_sdio_start(void)
{
    esp_err_t err = ESP_FAIL;
    uint8_t ioe = 0, ie = 0;

    err = sdmmc_send_cmd_go_idle_state(host->card);
    while (err == ESP_ERR_INVALID_RESPONSE) {
        ESP_LOGE(TAG, "Please restart slave and test again,error code:%d", err);
        vTaskDelay(pdMS_TO_TICKS(1000));
        err = sdmmc_send_cmd_go_idle_state(host->card);
    }
    ESP_RETURN_ON_FALSE(err == ESP_OK, err, TAG, "Send CMD0 error");

#if 0  //TODO IDF-9387
    if (host_is_spi(host->card)) {
        /* IO_SEND_OP_COND(CMD5), Determine if the card is an IO card. */
        err = sdmmc_io_send_op_cond(host->card, MMC_OCR_3_3V_3_4V, NULL);
        ESP_RETURN_ON_FALSE(err == ESP_OK, err, TAG, "Send CMD5 error");

        /* Disable CRC16 checks for data transfers in SPI mode */
        err = sdmmc_send_cmd_crc_on_off(host->card, false);
        ESP_RETURN_ON_FALSE(err == ESP_OK, err, TAG, "Close CRC error");
    }
#endif

    /* Enable function 1 */
    ioe |= BIT(1) | BIT(2);
    err = sdmmc_io_write_byte(host->card, 0, SD_IO_CCCR_FN_ENABLE, ioe, &ioe);
    ESP_RETURN_ON_FALSE(err == ESP_OK, err, TAG, "Set function 1 failed");
    ESP_LOGI(TAG, "IOE: 0x%02x", ioe);

    /* Enable interrupts for function 1&2 and master enable */
    ie |= BIT(0) | BIT(1) | BIT(2);
    err = sdmmc_io_write_byte(host->card, 0, SD_IO_CCCR_INT_ENABLE, ie, &ie);
    ESP_RETURN_ON_FALSE(err == ESP_OK, err, TAG, "Set interrupts failed");
    ESP_LOGI(TAG, "IE: 0x%02x", ie);

    // Get bus width register
    uint8_t bus_width;
    err = sdmmc_io_read_byte(host->card, 0, SD_IO_CCCR_BUS_WIDTH, &bus_width);
    ESP_RETURN_ON_FALSE(err == ESP_OK, err, TAG, "Get bus width failed");
    ESP_LOGI(TAG, "BUS_WIDTH GET: 0x%02x", bus_width);

    // Set bus width register
    bus_width |= CCCR_BUS_WIDTH_ECSI;
    err = sdmmc_io_write_byte(host->card, 0, SD_IO_CCCR_BUS_WIDTH, bus_width, &bus_width);
    ESP_RETURN_ON_FALSE(err == ESP_OK, err, TAG, "Set bus width failed");
    ESP_LOGI(TAG, "BUS_WIDTH SET: 0x%02x", bus_width);

    uint16_t bs = 512;
    const uint8_t *bs_u8 = (const uint8_t *)&bs;
    uint16_t bs_read = 0;
    uint8_t *bs_read_u8 = (uint8_t *)&bs_read;

    // Set block sizes for functions 1 to given value (default value = 512).
    size_t offset = 0x100;
    sdmmc_io_read_byte(host->card, 0, 0x100 + SD_IO_CCCR_BLKSIZEL, &bs_read_u8[0]);
    sdmmc_io_read_byte(host->card, 0, offset + SD_IO_CCCR_BLKSIZEH, &bs_read_u8[1]);
    ESP_LOGI(TAG, "Function 1 read: %04x", (int)bs_read);

    sdmmc_io_write_byte(host->card, 0, offset + SD_IO_CCCR_BLKSIZEL, bs_u8[0], NULL);
    sdmmc_io_write_byte(host->card, 0, offset + SD_IO_CCCR_BLKSIZEH, bs_u8[1], NULL);
    sdmmc_io_read_byte(host->card, 0, offset + SD_IO_CCCR_BLKSIZEL, &bs_read_u8[0]);
    sdmmc_io_read_byte(host->card, 0, offset + SD_IO_CCCR_BLKSIZEH, &bs_read_u8[1]);
    ESP_LOGI(TAG, "Function 1  set: %04x", (int)bs_read);

    offset = 0x200;
    sdmmc_io_read_byte(host->card, 0, offset + SD_IO_CCCR_BLKSIZEL, &bs_read_u8[0]);
    sdmmc_io_read_byte(host->card, 0, offset + SD_IO_CCCR_BLKSIZEH, &bs_read_u8[1]);
    ESP_LOGI(TAG, "Function 2 BS: %04x", (int)bs_read);
    sdmmc_io_write_byte(host->card, 0, offset + SD_IO_CCCR_BLKSIZEL, bs_u8[0], NULL);
    sdmmc_io_write_byte(host->card, 0, offset + SD_IO_CCCR_BLKSIZEH, bs_u8[1], NULL);
    sdmmc_io_read_byte(host->card, 0, offset + SD_IO_CCCR_BLKSIZEL, &bs_read_u8[0]);
    sdmmc_io_read_byte(host->card, 0, offset + SD_IO_CCCR_BLKSIZEH, &bs_read_u8[1]);
    ESP_LOGI(TAG, "Function 2 BS: %04x", (int)bs_read);

    return ESP_OK;
}

esp_err_t esp_extconn_sdio_init(sdmmc_card_t *card)
{
    if (host) {
        return ESP_ERR_INVALID_STATE;
    }
    host = calloc(1, sizeof(sdio_host_t));
    ESP_RETURN_ON_FALSE(host != NULL, ESP_ERR_NO_MEM, TAG, "memory exhausted");
    host->card = card;

    esp_err_t ret = ESP_FAIL;
    ret = esp_extconn_sdio_start();
    ESP_RETURN_ON_FALSE(ret == ESP_OK, ret, TAG, "esp host start failed");
    ret = esp_extconn_sdio_init_slave_link();
    ESP_RETURN_ON_FALSE(ret == ESP_OK, ret, TAG, "esp host init slave link failed");
    return ret;
}

esp_err_t esp_extconn_sdio_read_reg_window(unsigned int reg_addr, uint8_t *value)
{
#define MAX_RETRY (1)
    uint8_t *p_tbuf = NULL;
    int ret = 0;
    int retry = MAX_RETRY;

    reg_addr >>= 2;
    ESP_RETURN_ON_FALSE(reg_addr <= 0x7f, ESP_ERR_INVALID_ARG, TAG, "Invalid parameters");

    size_t actual_size = 0;
    esp_dma_mem_info_t dma_mem_info = {
        .extra_heap_caps = MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL,
        .dma_alignment_bytes = 4, //legacy API behaviour is only check max dma buffer alignment
    };
    esp_dma_capable_malloc(sizeof(uint32_t), &dma_mem_info, (void*)&p_tbuf, &actual_size);
    ESP_RETURN_ON_FALSE(p_tbuf != NULL, ESP_ERR_NO_MEM, TAG, "Fatal: Sufficient memory");

    p_tbuf[0] = (reg_addr & 0x7f);
    p_tbuf[1] = 0x80;

    ret = extconn_sdio_write_bytes(host->card, 1, ESP_SDIO_WIN_CMD, p_tbuf, 4);
    if (ret == ESP_OK) {
        do {
            if (retry < MAX_RETRY) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            retry--;
            ret = extconn_sdio_read_bytes(host->card, 1, ESP_SDIO_STATE_W0, p_tbuf, 4);
        } while (retry > 0 && ret != 0);
    }
    if (ret == ESP_OK) {
        memcpy(value, p_tbuf, 4);
    }
    free(p_tbuf);

    return ret;
}

static int esp_extconn_sdio_write_reg_window(unsigned int reg_addr, uint8_t *value)
{
    uint8_t *p_tbuf = NULL;
    int ret = ESP_OK;

    reg_addr >>= 2;
    ESP_RETURN_ON_FALSE(reg_addr <= 0x7f, ESP_ERR_INVALID_ARG, TAG, "Invalid parameters");

    size_t actual_size = 0;
    esp_dma_mem_info_t dma_mem_info = {
        .extra_heap_caps = MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL,
        .dma_alignment_bytes = 4, //legacy API behaviour is only check max dma buffer alignment
    };
    esp_dma_capable_malloc(sizeof(uint32_t), &dma_mem_info, (void*)&p_tbuf, &actual_size);
    ESP_RETURN_ON_FALSE(p_tbuf != NULL, ESP_ERR_NO_MEM, TAG, "Fatal: Sufficient memory");

    memcpy(p_tbuf, value, 4);
    p_tbuf[4] = (reg_addr & 0x7f);
    p_tbuf[5] = 0xc0;

    ret = extconn_sdio_write_bytes(host->card, 1, ESP_SDIO_CONFIG_W5, p_tbuf, 8);

    free(p_tbuf);
    return ret;
}

static esp_err_t esp_extconn_sdio_init_slave_link(void)
{
    uint32_t *t_buf = NULL;
    size_t actual_size = 0;
    esp_dma_mem_info_t dma_mem_info = {
        .extra_heap_caps = MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL,
        .dma_alignment_bytes = 4, //legacy API behaviour is only check max dma buffer alignment
    };
    esp_dma_capable_malloc(sizeof(uint32_t), &dma_mem_info, (void*)&t_buf, &actual_size);
    ESP_RETURN_ON_FALSE(t_buf != NULL, ESP_ERR_NO_MEM, TAG, "Fatal: Sufficient memory");

    // set stitch en
    esp_err_t err = esp_extconn_sdio_read_reg_window(ESP_SLC_CONF1_REG, (uint8_t *)t_buf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "read ESP_SLC_CONF1_REG error 0x%X", err);
        return err;
    }
    ESP_LOGI(TAG, "read ESP_SLC_CONF1_REG is 0x%" PRIx32, t_buf[0]);

    *t_buf |= SLC_SLC0_RX_STITCH_EN | SLC_SLC0_TX_STITCH_EN;
    ESP_LOGI(TAG, "write ESP_SLC_CONF1_REG = 0x%" PRIx32, t_buf[0]);
    ESP_RETURN_ON_FALSE(esp_extconn_sdio_write_reg_window(ESP_SLC_CONF1_REG, (uint8_t *)t_buf) == ESP_OK, ESP_FAIL, TAG, "Write SCLCONF1 failed");

    ESP_RETURN_ON_FALSE(esp_extconn_sdio_read_reg_window(ESP_SLC_CONF1_REG, (uint8_t *)t_buf) == ESP_OK, ESP_FAIL, TAG, "read SCLCONF1 failed");
    ESP_LOGI(TAG, "read ESP_SLC_CONF1_REG is 0x%" PRIx32, t_buf[0]);

    // set tx packet load en
    ESP_RETURN_ON_FALSE(esp_extconn_sdio_read_reg_window(ESP_SLC_0_LEN_CONF_REG, (uint8_t *)t_buf) == ESP_OK, ESP_FAIL, TAG, "read SLC0_LEN_CONF failed");
    ESP_LOGI(TAG, "read ESP_SLC_0_LEN_CONF_REG is 0x%" PRIx32, t_buf[0]);

    *t_buf |= SLC_SLC0_TX_PACKET_LOAD_EN;
    ESP_LOGI(TAG, "write ESP_SLC_0_LEN_CONF_REG is 0x%" PRIx32, t_buf[0]);
    ESP_RETURN_ON_FALSE(esp_extconn_sdio_write_reg_window(ESP_SLC_0_LEN_CONF_REG, (uint8_t *)t_buf) == ESP_OK, ESP_FAIL, TAG, "Write SLC0_LEN_CONF failed");

    ESP_RETURN_ON_FALSE(esp_extconn_sdio_read_reg_window(ESP_SLC_0_LEN_CONF_REG, (uint8_t *)t_buf) == ESP_OK, ESP_FAIL, TAG, "read SLC0_LEN_CONF failed");
    ESP_LOGI(TAG, "read ESP_SLC_0_LEN_CONF_REG is 0x%" PRIx32, t_buf[0]);

    // Enable target interrupt
    uint32_t *val = 0;
    esp_dma_capable_malloc(sizeof(uint32_t), &dma_mem_info, (void*)&val, &actual_size);
    ESP_RETURN_ON_FALSE(val != NULL, ESP_ERR_NO_MEM, TAG, "Fatal: Sufficient memory");

    extconn_sdio_read_bytes(host->card, 1, ESP_SDIO_FUNC1_INT_ENA, (uint8_t *)val, sizeof(uint32_t));
    *val |= SLCHOST_FN1_GPIO_SDIO_INT_ENA;
    extconn_sdio_write_bytes(host->card, 1, ESP_SDIO_FUNC1_INT_ENA, (uint8_t *)val, sizeof(uint32_t));

    free(t_buf);
    free(val);
    return ESP_OK;
}

esp_err_t esp_extconn_sdio_get_buffer_size(uint32_t *buffer_size)
{
    uint32_t len = 0;
    esp_err_t ret = extconn_sdio_read_bytes(host->card, 1, ESP_SDIO_TOKEN_RDATA, &len, 4);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Read length error, ret=%d", ret);
        return ret;
    }
    ESP_LOGD(TAG, " Read len: %" PRIu32, len);
    len = (len >> ESP_SDIO_SEND_OFFSET) & TX_BUFFER_MASK;
    len = (len + TX_BUFFER_MAX - host->total_tx) % TX_BUFFER_MAX;
    *buffer_size = len;
    return ret;
}

static esp_err_t esp_extconn_sdio_get_rx_data_size(uint32_t function, uint32_t *rx_size)
{
    uint32_t len = 0;
    uint32_t addr = 0;
    if (function == EXT_CONN_BT_SDIO_FUNC) {
        addr = ESP_SDIO_SLC1_HOST_PF;
    } else {
        addr = ESP_SDIO_PKT_LEN;
    }
    esp_err_t err = extconn_sdio_read_bytes(host->card, 1, addr, (uint8_t *)&len, 4);
    ESP_RETURN_ON_FALSE(err == ESP_OK, err, TAG, "read failed");

    if (function == EXT_CONN_BT_SDIO_FUNC) {
        uint8_t *pf = (uint8_t *)&len;
        *rx_size = (((pf[1] << 16) | (pf[2] << 8) | (pf[3] << 0)) & 0xffffff);
    } else {
        len &= RX_BYTE_MASK;
        len = (len + RX_BYTE_MAX - host->total_rx) % RX_BYTE_MAX;
        *rx_size = len;
    }
    return ESP_OK;
}

esp_err_t esp_extconn_sdio_get_packet(uint32_t function, void *out_buf, size_t size, size_t *out_length, uint32_t wait_ms)
{
    esp_err_t err = ESP_OK;
    uint32_t len = 0;
    uint32_t wait_time = 0;

    ESP_RETURN_ON_FALSE(size > 0, ESP_ERR_INVALID_ARG, TAG, "Invalid size");

    for (;;) {
        err = esp_extconn_sdio_get_rx_data_size(function, &len);
        if (err == ESP_OK && len > 0) {
            break;
        }
        if (err != ESP_OK) {
            return err;
        }

        if (++wait_time >= wait_ms) {
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    ESP_LOGV(TAG, "get_packet: slave len=%" PRIu32", max read size=%d", len, size);

    if (len > size) {
        len = size;
        err = ESP_ERR_NOT_FINISHED;
    }

    uint32_t len_remain = len;
    uint8_t *start_ptr = (uint8_t *)out_buf;

    do {
        const int block_size = 512; // currently our driver don't support block size other than 512
        int len_to_send;
        int block_n = len_remain / block_size;
        uint32_t addr = (function == EXT_CONN_WIFI_SDIO_FUNC) ? (ESP_SLAVE_CMD53_END_ADDR - len_remain) : 0;

        if (block_n != 0) {
            len_to_send = block_n * block_size;
            err = extconn_sdio_read_blocks(host->card, function, addr, start_ptr, len_to_send);
        } else {
            len_to_send = len_remain;
            /*
             * though the driver supports to split packet of unaligned size into length
             * of 4x and 1~3, we still get aligned size of data to get higher
             * effeciency. The length is determined by the SDIO address, and the
             * remainning will be ignored by the slave hardware.
             */
            err = extconn_sdio_read_bytes(host->card, function, addr, start_ptr, (len_to_send + 3) & (~3));
        }

        if (err != ESP_OK) {
            return err;
        }

        start_ptr += len_to_send;
        len_remain -= len_to_send;
    } while (len_remain != 0);

    *out_length = len;
    if (function != EXT_CONN_BT_SDIO_FUNC) {
        host->total_rx += len;
    }
    return ESP_OK;
}

esp_err_t esp_extconn_sdio_send_packet(uint32_t function, void *start, size_t length)
{
    esp_err_t err = ESP_FAIL;
    uint8_t *start_ptr = (uint8_t *)start;
    uint32_t len_remain = length;
    uint32_t block_size = 512;
    int buffer_used = (length + block_size - 1) / block_size;
    uint32_t addr = (function == EXT_CONN_WIFI_SDIO_FUNC) ? (ESP_SLAVE_CMD53_END_ADDR - len_remain) : 0;

    do {
        /* Though the driver supports to split packet of unaligned size into
         * length of 4x and 1~3, we still send aligned size of data to get
         * higher effeciency. The length is determined by the SDIO address, and
         * the remainning will be discard by the slave hardware.
         */
        int block_n = len_remain / block_size;
        int len_to_send;

        if (block_n) {
            len_to_send = block_n * block_size;
            err = extconn_sdio_write_blocks(host->card, function, addr, start_ptr, len_to_send);
        } else {
            len_to_send = len_remain;
            err = extconn_sdio_write_bytes(host->card, function, addr, start_ptr, (len_to_send + 3) & (~3));
        }
        ESP_RETURN_ON_FALSE(err == ESP_OK, err, TAG, "write bytes failed");

        start_ptr += len_to_send;
        len_remain -= len_to_send;
    } while (len_remain);

    if (function != EXT_CONN_BT_SDIO_FUNC) {
        host->total_tx += buffer_used;
        if (host->total_tx >= TX_BUFFER_MAX) {
            host->total_tx -= TX_BUFFER_MAX;
        }
    }
    return ESP_OK;
}

esp_err_t esp_extconn_sdio_clear_intr(uint32_t intr_0, uint32_t intr_1)
{
    esp_err_t r = ESP_FAIL;

    if (intr_0 == 0 && intr_1 == 0) {
        return ESP_OK;
    }

    if (intr_0 != 0) {
        r = extconn_sdio_write_bytes(host->card, 1, ESP_SDIO_SLC0_INT_CLR, (uint8_t *)&intr_0, 4);
        ESP_RETURN_ON_FALSE(r == ESP_OK, r, TAG, "clear intr_0 failed");
    }

    if (intr_1 != 0) {
        r = extconn_sdio_write_bytes(host->card, 1, ESP_SDIO_SLC1_INT_CLR, (uint8_t *)&intr_1, 4);
        ESP_RETURN_ON_FALSE(r == ESP_OK, r, TAG, "clear intr_1 failed");
    }

    return ESP_OK;
}

esp_err_t esp_extconn_sdio_get_intr(uint32_t *intr_raw, uint32_t *intr_st)
{
    esp_err_t r = ESP_FAIL;
    ESP_RETURN_ON_FALSE((intr_raw != NULL || intr_st != NULL), ESP_ERR_INVALID_ARG, TAG, "NULL Ptr");

    if (intr_raw != NULL) {
        r = extconn_sdio_read_bytes(host->card, 1, ESP_SDIO_INT_RAW, (uint8_t *)intr_raw, 4);
        ESP_RETURN_ON_FALSE(r == ESP_OK, r, TAG, "intr_raw read failed");
    }

    if (intr_st != NULL) {
        r = extconn_sdio_read_bytes(host->card, 1, ESP_SDIO_INT_RAW1, (uint8_t *)intr_st, 4);
        ESP_RETURN_ON_FALSE(r == ESP_OK, r, TAG, "intr_sr read failed");
    }

    return ESP_OK;
}

esp_err_t esp_extconn_sdio_wait_int(uint32_t wait)
{
    return sdmmc_io_wait_int(host->card, wait);
}
