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

* ESP32-P4_Function_EV_Board with ESP32_Module_SDIO_Converter_Board

* Hardware setup
  - Pin Connections
    | ESP32P4 | ESP8689 |   Function  |
    |:--------:|:--------:|:--------:|
    | 53   | EN   |Reset|
    | GND   | IO0  | Forcing ESP8689 in download mode|
    -  The ESP32P4 controls the reset of the ESP8689 through pin 53.

## Supported Transports

* SDIO Only
    * Wi-Fi and Bluetooth, traffic for both runs over SDIO

## Supported APIs
1. **Initialization** 
   *  esp_wifi_init
   *  esp_wifi_deinit
   *  esp_wifi_start
   *  esp_wifi_stop
2. **Configuration**
    * esp_wifi_set_mode
    * esp_wifi_get_mode
    * esp_wifi_set_config
    * esp_wifi_get_config
    * esp_wifi_set_storage
    * esp_wifi_set_protocol
    * esp_wifi_get_protocol
    * esp_wifi_set_country
    * esp_wifi_get_country
    * esp_wifi_set_country_code
    * esp_wifi_get_country_code
    * esp_wifi_set_bandwidth
    * esp_wifi_get_bandwidth
    * esp_wifi_set_channel
    * esp_wifi_get_channel
    * esp_wifi_get_mac
3. **Connection**
    * esp_wifi_connect
    * esp_wifi_disconnect
4. **Scan**
    * esp_wifi_scan_start
    * esp_wifi_scan_get_ap_num
    * esp_wifi_scan_get_ap_records
    * esp_wifi_sta_get_ap_info

## Step to enable this component in an example code:

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

## Throughput Performance
#### 1. TCP TX
```sh
[ ID] Interval       Transfer     Bandwidth
[  4]  0.0- 1.0 sec  5.47 MBytes  45.9 Mbits/sec
[  4]  1.0- 2.0 sec  5.51 MBytes  46.2 Mbits/sec
[  4]  2.0- 3.0 sec  5.56 MBytes  46.7 Mbits/sec
[  4]  3.0- 4.0 sec  5.56 MBytes  46.6 Mbits/sec
[  4]  4.0- 5.0 sec  5.51 MBytes  46.3 Mbits/sec
[  4]  5.0- 6.0 sec  5.58 MBytes  46.8 Mbits/sec
[  4]  6.0- 7.0 sec  5.53 MBytes  46.4 Mbits/sec
[  4]  7.0- 8.0 sec  5.55 MBytes  46.5 Mbits/sec
[  4]  8.0- 9.0 sec  5.59 MBytes  46.9 Mbits/sec
[  4]  9.0-10.0 sec  5.56 MBytes  46.7 Mbits/sec
[  4]  0.0-10.0 sec  55.4 MBytes  46.5 Mbits/sec
```

#### 2. TCP RX
```sh
Interval       Bandwidth
 0.0- 1.0 sec  44.47 Mbits/sec
 1.0- 2.0 sec  45.66 Mbits/sec
 2.0- 3.0 sec  45.50 Mbits/sec
 3.0- 4.0 sec  45.55 Mbits/sec
 4.0- 5.0 sec  45.93 Mbits/sec
 5.0- 6.0 sec  45.52 Mbits/sec
 6.0- 7.0 sec  45.36 Mbits/sec
 7.0- 8.0 sec  45.12 Mbits/sec
 8.0- 9.0 sec  45.92 Mbits/sec
 9.0-10.0 sec  45.50 Mbits/sec
 0.0-10.0 sec  45.45 Mbits/sec
 ```

 #### 3. UDP TX
 ```sh
[ ID] Interval       Transfer     Bandwidth        Jitter   Lost/Total Datagrams
[  3]  0.0- 1.0 sec  6.12 MBytes  51.3 Mbits/sec   0.242 ms    0/ 4366 (0%)
[  3]  1.0- 2.0 sec  6.09 MBytes  51.1 Mbits/sec   0.237 ms    0/ 4343 (0%)
[  3]  2.0- 3.0 sec  6.06 MBytes  50.9 Mbits/sec   0.245 ms    0/ 4324 (0%)
[  3]  3.0- 4.0 sec  6.06 MBytes  50.9 Mbits/sec   0.437 ms    0/ 4326 (0%)
[  3]  4.0- 5.0 sec  6.05 MBytes  50.7 Mbits/sec   0.231 ms    0/ 4315 (0%)
[  3]  5.0- 6.0 sec  6.06 MBytes  50.8 Mbits/sec   0.249 ms    0/ 4321 (0%)
[  3]  6.0- 7.0 sec  6.06 MBytes  50.8 Mbits/sec   0.253 ms    0/ 4322 (0%)
[  3]  7.0- 8.0 sec  6.06 MBytes  50.8 Mbits/sec   0.242 ms    0/ 4323 (0%)
[  3]  8.0- 9.0 sec  6.05 MBytes  50.8 Mbits/sec   0.257 ms    0/ 4317 (0%)
[  3]  9.0-10.0 sec  6.05 MBytes  50.7 Mbits/sec   0.251 ms    0/ 4315 (0%)
[  3]  0.0-10.0 sec  60.7 MBytes  50.9 Mbits/sec   0.251 ms    0/43272 (0%)
```

#### 4. UDP RX
 ```sh
Interval       Bandwidth
 0.0- 1.0 sec  66.11 Mbits/sec
 1.0- 2.0 sec  65.82 Mbits/sec
 2.0- 3.0 sec  66.54 Mbits/sec
 3.0- 4.0 sec  65.83 Mbits/sec
 4.0- 5.0 sec  65.78 Mbits/sec
 5.0- 6.0 sec  66.44 Mbits/sec
 6.0- 7.0 sec  66.25 Mbits/sec
 7.0- 8.0 sec  66.09 Mbits/sec
 8.0- 9.0 sec  65.72 Mbits/sec
 9.0-10.0 sec  65.77 Mbits/sec
 0.0-10.0 sec  66.04 Mbits/sec
```