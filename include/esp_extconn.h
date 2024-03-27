/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ESP_EXTCONN_H__
#define __ESP_EXTCONN_H__

#include "sdkconfig.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_ESP_EXT_CONN_WIFI_ENABLE) && defined(CONFIG_ESP_EXT_CONN_BT_ENABLE)
#define EXT_CONN_FUNCTIONS_ENABLE (3)
#elif defined(CONFIG_ESP_EXT_CONN_BT_ENABLE)
#define EXT_CONN_FUNCTIONS_ENABLE (2)
#else
#define EXT_CONN_FUNCTIONS_ENABLE (1)
#endif

#ifdef CONFIG_ESP_EXT_CONN_RECV_TASK_STACK
#define ESP_EXT_CONN_RECV_TASK_STACK CONFIG_ESP_EXT_CONN_RECV_TASK_STACK
#else
#define ESP_EXT_CONN_RECV_TASK_STACK 3072
#endif

#ifdef CONFIG_ESP_EXT_CONN_RECV_TASK_PRIO
#define ESP_EXT_CONN_RECV_TASK_PRIO CONFIG_ESP_EXT_CONN_RECV_TASK_PRIO
#else
#define ESP_EXT_CONN_RECV_TASK_PRIO 23
#endif

#ifdef COFNIG_ESP_EXT_CONN_RECV_TASK_CORE
#define ESP_EXT_CONN_RECV_TASK_CORE COFNIG_ESP_EXT_CONN_RECV_TASK_CORE
#else
#define ESP_EXT_CONN_RECV_TASK_CORE 1
#endif

#ifdef COFNIG_ESP_EXT_CONN_WIFI_TASK_STACK
#define ESP_EXT_CONN_WIFI_TASK_STACK COFNIG_ESP_EXT_CONN_WIFI_TASK_STACK
#else
#define ESP_EXT_CONN_WIFI_TASK_STACK 3072
#endif

#ifdef COFNIG_ESP_EXT_CONN_WIFI_TASK_PRIO
#define ESP_EXT_CONN_WIFI_TASK_PRIO COFNIG_ESP_EXT_CONN_WIFI_TASK_PRIO
#else
#define ESP_EXT_CONN_WIFI_TASK_PRIO 21
#endif

#ifdef COFNIG_ESP_EXT_CONN_WIFI_TASK_CORE
#define ESP_EXT_CONN_WIFI_TASK_CORE COFNIG_ESP_EXT_CONN_WIFI_TASK_CORE
#else
#define ESP_EXT_CONN_WIFI_TASK_CORE 1
#endif

#ifdef COFNIG_ESP_EXT_CONN_BT_TASK_STACK
#define ESP_EXT_CONN_BT_TASK_STACK COFNIG_ESP_EXT_CONN_BT_TASK_STACK
#else
#define ESP_EXT_CONN_BT_TASK_STACK 3072
#endif

#ifdef COFNIG_ESP_EXT_CONN_BT_TASK_PRIO
#define ESP_EXT_CONN_BT_TASK_PRIO COFNIG_ESP_EXT_CONN_BT_TASK_PRIO
#else
#define ESP_EXT_CONN_BT_TASK_PRIO 23
#endif

#ifdef COFNIG_ESP_EXT_CONN_BT_TASK_CORE
#define ESP_EXT_CONN_BT_TASK_CORE COFNIG_ESP_EXT_CONN_BT_TASK_CORE
#else
#define ESP_EXT_CONN_BT_TASK_CORE 1
#endif

/*
 * @brief External connectivity configuration parameters passed to esp_extconn_init call.
 */
typedef struct {
    uint32_t recv_task_stack; /* Trans recv task stack */
    uint32_t recv_task_prio;  /* Trans recv task priority */
    uint32_t recv_task_core;  /* Trans recv task core ID */
    uint32_t wifi_task_stack; /* Trans WiFi task stack */
    uint32_t wifi_task_prio;  /* Trans WiFi task priority */
    uint32_t wifi_task_core;  /* Trans WiFi task core ID */
    uint32_t bt_task_stack;   /* Trans BT task stack */
    uint32_t bt_task_prio;    /* Trans BT task priority */
    uint32_t bt_task_core;    /* Trans BT task core ID */
} esp_extconn_config_t;

#define ESP_EXTCONN_CONFIG_DEFAULT() { \
    .recv_task_stack = ESP_EXT_CONN_RECV_TASK_STACK, \
    .recv_task_prio  = ESP_EXT_CONN_RECV_TASK_PRIO,  \
    .recv_task_core  = ESP_EXT_CONN_RECV_TASK_CORE,  \
    .wifi_task_stack = ESP_EXT_CONN_WIFI_TASK_STACK, \
    .wifi_task_prio  = ESP_EXT_CONN_WIFI_TASK_PRIO,  \
    .wifi_task_core  = ESP_EXT_CONN_WIFI_TASK_CORE,  \
    .bt_task_stack   = ESP_EXT_CONN_BT_TASK_STACK,   \
    .bt_task_prio    = ESP_EXT_CONN_BT_TASK_PRIO,    \
    .bt_task_core    = ESP_EXT_CONN_BT_TASK_CORE     \
}

/**
 * @brief Initialize the driver for external Wi-Fi/BT
 *
 * @param  config provide esp_extcon init configuration
 *
 * @return
 *    - ESP_OK: succeed
 *    - ESP_ERR_NO_MEM: out of memory
 *    - others: refer to error code esp_err.h
 */
esp_err_t esp_extconn_init(esp_extconn_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* __ESP_EXTCONN_H__ */
