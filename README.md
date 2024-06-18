# ESP External Connection

This component provides external wireless connectivity(Wi-Fi & Bluetooth) for ESP chips that do not have built-in wireless capabilities. The APIs used by this component are compatible with the Wi-Fi and Bluetooth component APIs in the ESP-IDF.

## Supported Features

- Wi-Fi Features:
  - 802.11b/g/n
  - Operating Mode: STA, SoftAP
  - Security Mode: Open, WPA, WPA2, WPA3

- BT/BLE
  - Classic Bluetooth
  - BLE 4.2

## Supported ESP Chips

| Supported Hosts | ESP32P4 | 
| ----------------- | ----- |

| Supported Targets | ESP8689 | 
| ----------------- | ------- |

## Supported Boards

ESP32-P4_Function_EV_Board with ESP32_Module_SDIO_Converter_Board

## Supported Transports

* SDIO Only
    * Wi-Fi and Bluetooth, traffic for both runs over SDIO

## API

### Step to enable this component in an example code:

1. Add this component to your project using ```idf.py add-dependency esp-extconn``` command.

2. In the main file of the example, add the following line:
    ```c
    #ifdef CONFIG_ESP_EXT_CONN_ENABLE
    #include "esp_extconn.h"
    #endif
    ```

3. In your app_main() function, add the following line as the first line:
    ```c
    #ifdef CONFIG_ESP_EXT_CONN_ENABLE
    esp_extconn_config_t config = ESP_EXTCONN_CONFIG_DEFAULT();
    esp_extconn_init(&config);
    #endif
    ```

4. Using esp_wifi components like built-in wireless chips :)
