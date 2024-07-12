# ESP External Connection

This component provides external wireless connectivity(Wi-Fi & Bluetooth) for ESP chips that do not have built-in wireless capabilities. The APIs used by this component are compatible with the Wi-Fi and Bluetooth component APIs in the ESP-IDF.

## Supported Features
<table>
  <tr>
    <th>Features</th>
    <th>Supported</th>
    <th>Sill not Supported </th>
  </tr>
  <tr>
    <td>Virtual Wi-Fi interfaces</td>
    <td>Station, SoftAP</td>
    <td>Sniffer, Wi-Fi Aware (NAN)</td>
  </tr>
  <tr>
    <td>Wi-Fi Modes</td>
    <td>Station, SoftAP, Station/AP-coexistence mode</td>
    <td></td>
  </tr>
  <tr>
    <td>Wi-Fi Protocols</td>
    <td>802.11 b/g/n</td>
    <td></td>
  </tr>
  <tr>
    <td>Wi-Fi Security Modes</td>
    <td>Open / WPA / WPA2 / WPA3 / WPA2-Enterprise / WPA3-Enterprise / WPS</td>
    <td>WAPI / DPP</td>
  </tr>
  <tr>
    <td>Power Save</td>
    <td></td>
    <td>Modem-sleep</td>
  </tr>
  <tr>
    <td>Wi-Fi Data Transmission</td>
    <td>AMSDU, AMPDU, HT40, QoS</td>
    <td></td>
  </tr>
  <tr>
    <td>Vendor Features</td>
    <td></td>
    <td>ESP-NOW, Long Range mode, SmartConfig, Channel State Information</td>
  </tr>
  <tr>
    <td>Scan</td>
    <td>Fast scan, All-channel scan</td>
    <td></td>
  </tr>
  <tr>
    <td>Antennas</td>
    <td></td>
    <td>Multiple</td>
  </tr>
  <tr>
    <td>Bluetooth features</td>
    <td>Classic Bluetooth, BLE 4.2</td>
    <td></td>
  </tr>
</table>

## Supported ESP Chips
*  Supported Host : **ESP32P4**
*  Supported Target : **ESP8689**

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
<table>
  <tr>
    <th>Classification</th>
    <th>Supported APIs</th>
  </tr>
  <tr>
    <td rowspan="2">Initialization</td>
    <td>esp_wifi_<b>init</b> / esp_wifi_<b>deinit</b></td>
  </tr>
  <tr>
    <td>esp_wifi_<b>start</b> / esp_wifi_<b>stop</b></td>
  </tr>
  <tr>
    <td rowspan="13">Configuration</td>
    <td>esp_wifi_<b>set_mode</b> / esp_wifi_<b>get_mode</b></td>
  </tr>
  <tr>
    <td>esp_wifi_<b>set_config</b> / esp_wifi_<b>get_config</b></td>
  </tr>
  <tr>
    <td>esp_wifi_<b>set_storage</b></td>
  </tr>
  <tr>
    <td>esp_wifi_<b>set_protocol</b> / esp_wifi_<b>get_protocol</b></td>
  </tr>
  <tr>
    <td>esp_wifi_<b>set_country</b> / esp_wifi_<b>get_country</b></td>
  </tr>
  <tr>
    <td> esp_wifi_<b>set_country_code</b> / esp_wifi_<b>get_country_code</b></td>
  </tr>
  <tr>
    <td> esp_wifi_<b>set_bandwidth</b> / esp_wifi_<b>get_bandwidth</b></td>
  </tr>
  <tr>
    <td>  esp_wifi_<b>set_mac</b> /  esp_wifi_<b>get_mac</b></td>
  </tr>
  <tr>
    <td>  esp_wifi_<b>restore</b></td>
  </tr>
  <tr>
    <td>  esp_wifi_<b>set_vendor_ie</b> / esp_wifi_<b>set_vendor_ie_cb</b></td>
  </tr>
  <tr>
    <td>  esp_wifi_<b>set_channel</b> / esp_wifi_<b>get_channel</b></td>
  </tr>
  <tr>
    <td>  esp_wifi_<b>set_inactive_time</b> / esp_wifi_<b>get_inactive_time</b></td>
  </tr>
  <tr>
    <td>  esp_wifi_<b>set_rssi_threshold</b>
  </tr>
  <tr>
    <td rowspan="7">Connection</td>
    <td>esp_wifi_<b>connect</b> / esp_wifi_<b>disconnect</b></td>
  </tr>
  <tr>
    <td>esp_wifi_<b>sta_get_aid</b> / esp_wifi_<b>ap_get_sta_aid</b></td>
  </tr>
  <tr>
    <td>esp_wifi_<b>sta_get_ap_info</b></td>
  </tr>
  <tr>
    <td>esp_wifi_ap_<b>get_sta_list_with_ip</b> / esp_wifi_ap_<b>get_sta_list</b></td>
  </tr>
  <tr>
    <td>esp_wifi_<b>deauth_sta</b></td>
  </tr>
  <tr>
  <td>esp_wifi_<b>sta_get_rssi</b></td>
  </tr>
  <tr>
    <td>esp_wifi_<b>sta_get_negotiated_phymode</b></td>
  </tr>
  <tr>
    <td rowspan="5">Scan</td>
    <td>esp_wifi_<b>scan_start</b> / esp_wifi_<b>scan_stop</b></td>
  </tr>
  <tr>
    <td>esp_wifi_<b>set_scan_parameters</b> / esp_wifi_<b>get_scan_parameters</b></td>
  </tr>
  <tr>
    <td>esp_wifi_<b>scan_get_ap_num</b></td>
  </tr>
  <tr>
    <td>esp_wifi_scan_get_ap_<b>records</b> / esp_wifi_scan_get_ap_<b>record</b></td>
  </tr>
  <tr>
    <td>esp_wifi_<b>clear_ap_list</b></td>
  </tr>
  <tr>
    <td rowspan="2">Others</td>
    <td>esp_wifi_<b>80211_tx</b></td>
  </tr>
  <tr>
    <td>esp_wifi_<b>statis_dump</b></td>
  </tr>
</table>

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