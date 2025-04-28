if(CONFIG_ESP_EXT_CONN_ENABLE)
    set(srcs
        "src/extconn.c"
        "src/ext_boot.c"
        "src/esp_sip.c"
        "src/trans_recv.c"
        "src/ext_sdio_adapter.c")

    if(CONFIG_ESP_EXT_CONN_WIFI_ENABLE)
        list(APPEND srcs "src/trans_wifi.c")
    endif()

    if(CONFIG_ESP_EXT_CONN_BT_ENABLE)
        list(APPEND srcs "src/trans_bt.c")
    endif()

    set(priv_includes "priv_include")

    if(CONFIG_ESP_EXT_CONN_TARGET_ESP32C6)
        set(target_include_dirs priv_include/target/esp32c6)
    elseif(CONFIG_ESP_EXT_CONN_TARGET_ESP32C5)
        set(target_include_dirs priv_include/target/esp32c5)
    elseif(CONFIG_ESP_EXT_CONN_TARGET_ESP32T2)
        set(target_include_dirs priv_include/target/esp32t2)
    else()
        set(target_include_dirs priv_include/target/esp32)
    endif()
endif()

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "include" "${target_include_dirs}"
                       PRIV_INCLUDE_DIRS "${priv_includes}"
                       PRIV_REQUIRES esp_driver_sdmmc esp_driver_sdspi bt)
