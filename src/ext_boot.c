/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "esp_log.h"

#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"

#include "sd_pwr_ctrl_by_on_chip_ldo.h"

#include "esp_sip.h"
#include "esp_extconn.h"
#include "ext_sdio_adapter.h"
#include "ext_default.h"

#include "eagle_fw.h"

#define FIRMWARE_MAGIC_HEADER (0xE9)

static const char *TAG = "fw_dl";

static void pins_init(void)
{
    int reset_pin = CONFIG_ESP_EXT_CONN_SLAVE_ENABLE_PIN;
    int boot_pin = CONFIG_ESP_EXT_CONN_SLAVE_BOOT_PIN;

    if (reset_pin < 0) {
        ESP_LOGE(TAG, "Should always set a reset pin");
        return;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(reset_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = false,
    };
    if (boot_pin != GPIO_NUM_NC) {
        io_conf.pin_bit_mask |= BIT64(boot_pin);
    }
    gpio_config(&io_conf);
    if (boot_pin != GPIO_NUM_NC) {
        gpio_set_level(boot_pin, 0);
    }
    gpio_set_level(reset_pin, !CONFIG_ESP_EXT_CONN_SLAVE_ENABLE_LVL);
}

static void reset_2_dl_mode(void)
{
    gpio_set_level(CONFIG_ESP_EXT_CONN_SLAVE_ENABLE_PIN, !CONFIG_ESP_EXT_CONN_SLAVE_ENABLE_LVL);
    vTaskDelay(pdMS_TO_TICKS(30));
    gpio_set_level(CONFIG_ESP_EXT_CONN_SLAVE_ENABLE_PIN, CONFIG_ESP_EXT_CONN_SLAVE_ENABLE_LVL);
    vTaskDelay(pdMS_TO_TICKS(30));
}

static sdmmc_card_t *sdmmc_init(void)
{
    sdmmc_host_t config = SDMMC_HOST_DEFAULT();

#if CONFIG_ESP_EXT_CONN_VIA_SDIO
    config.flags        = SDMMC_HOST_FLAG_4BIT | SDMMC_HOST_FLAG_ALLOC_ALIGNED_BUF;
    config.max_freq_khz = SDMMC_FREQ_DEFAULT;

    sd_pwr_ctrl_ldo_config_t ldo_config = {
        .ldo_chan_id = 4, // `LDO_VO4` is used as the SDMMC IO power
    };
    sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;

    esp_err_t err = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to new an on-chip ldo power control driver");
        return NULL;
    }
    config.pwr_ctrl_handle = pwr_ctrl_handle;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.clk = CONFIG_ESP_EXT_CONN_SDIO_CLK_PIN;
    slot_config.cmd = CONFIG_ESP_EXT_CONN_SDIO_CMD_PIN;
    slot_config.d0  = CONFIG_ESP_EXT_CONN_SDIO_D0_PIN;
    slot_config.d1  = CONFIG_ESP_EXT_CONN_SDIO_D1_PIN;
    slot_config.d2  = CONFIG_ESP_EXT_CONN_SDIO_D2_PIN;
    slot_config.d3  = CONFIG_ESP_EXT_CONN_SDIO_D3_PIN;
    slot_config.width = 4;

    err = sdmmc_host_init();
    ESP_ERROR_CHECK(err);

    err = sdmmc_host_init_slot(CONFIG_ESP_EXT_CONN_SDIO_SLOT, &slot_config);
    ESP_ERROR_CHECK(err);

#else
    // TOD0
    ESP_LOGE(TAG, "ERROR Configuration");
    return NULL;
#endif

    sdmmc_card_t *card = (sdmmc_card_t *)calloc(1, sizeof(sdmmc_card_t));
    if (card == NULL) {
        return NULL;
    }
    esp_err_t ret = ESP_FAIL;
    for (int i = 0; i < 10; i++) {
        ret = sdmmc_card_init(&config, card);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "sdmmc init success 0x%X", ret);
            break;
        }
        ESP_LOGW(TAG, "slave init failed 0x%X, retry...", ret);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    sdmmc_card_print_info(stdout, card);

    return card;
}

static esp_err_t esp_fw_download(const uint8_t *fw)
{
    esp_err_t ret = ESP_OK;
    struct esp_fw_hdr *fhdr = (struct esp_fw_hdr *)fw;
    struct esp_fw_blk_hdr *bhdr = NULL;

    if (fhdr->magic != FIRMWARE_MAGIC_HEADER) {
        ESP_LOGE(TAG, "Wrong magic num!");
        return ESP_FAIL;
    }

    uint8_t blocks = fhdr->blocks;
    uint32_t offset = sizeof(struct esp_fw_hdr) + 16;

    ESP_LOGI(TAG, "blocks is %u", blocks);

    while (blocks) {
        bhdr = (struct esp_fw_blk_hdr *)(&fw[offset]);
        offset += sizeof(struct esp_fw_blk_hdr);

        ESP_LOGI(TAG, "blocks:%u ->%" PRIx32 " %" PRIu32, blocks, bhdr->load_addr, bhdr->data_len);

        ret = esp_sip_write_mem(bhdr->load_addr, &fw[offset], bhdr->data_len);
        if (ret != ESP_OK) {
            ESP_LOGI(TAG, "%s|%d, function write failed\n", __func__, __LINE__);
            return ret;
        }

        blocks--;
        offset += bhdr->data_len;
    }

    return ret;
}

esp_err_t esp_extconn_fw_init(esp_extconn_config_t *config)
{
    esp_err_t ret = ESP_OK;
    uint8_t *fw = (uint8_t *)&eagle_fw1[0];

    ret = esp_sip_init();
    if (ret == ESP_OK) {
        ret = esp_fw_download(fw);
    }
    if (ret == ESP_OK) {
        ret = esp_extconn_trans_recv_init(config);
    }
    if (ret == ESP_OK) {
        struct esp_fw_hdr *fhdr = (struct esp_fw_hdr *)fw;

        ret = esp_sip_bootup(fhdr->entry_addr);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "bootup failed");
        }
    }

    return ret;
}

esp_err_t esp_extconn_boot(void)
{
    esp_err_t ret = ESP_FAIL;
    pins_init();
    reset_2_dl_mode();

    sdmmc_card_t *card = sdmmc_init();
    if (card != NULL) {
        ret = esp_extconn_sdio_init(card);
    }
    return ret;
}
