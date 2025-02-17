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

* ESP32-P4 Core Board with HWD-124 ESP32-P4 Core ESP32_Sub

* Hardware setup
  - Pin Connections
    | ESP32P4 | ESP8689 |             Function             |
    | :-----: | :-----: | :------------------------------: |
    |   42    |   EN    |              Reset               |
    |   43    |   BOOT  | Forcing ESP8689 in download mode |
    |   45    |   D1    |              Data 1              |
    |   46    |   D0    |              Data 0              |
    |   47    |   CLK   |              Clock               |
    |   48    |   CMD   |              Command             |
    |   49    |   D3    |              Data 3              |
    |   50    |   D2    |              Data 2              |
    -  The ESP32P4 controls the reset of the ESP8689 through pin 42.

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
    <td rowspan="2"style="text-align:center; vertical-align:middle;">Initialization</td>
    <td>esp_wifi_<b>init</b> / esp_wifi_<b>deinit</b></td>
  </tr>
  <tr>
    <td>esp_wifi_<b>start</b> / esp_wifi_<b>stop</b></td>
  </tr>
  <tr>
    <td rowspan="13"style="text-align:center; vertical-align:middle;">Configuration</td>
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
    <td rowspan="7"style="text-align:center; vertical-align:middle;">Connection</td>
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
    <td rowspan="5"style="text-align:center; vertical-align:middle;">Scan</td>
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
    <td rowspan="2"style="text-align:center; vertical-align:middle;">Others</td>
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
### 1. Parameters

<table border="1" cellspacing="0" cellpadding="5">
    <tr>
        <td colspan="2", style="text-align: center; font-size: 20px; font-weight: bold;">Parameters</td>
        <td style="font-size: 20px; font-weight: bold;">Value</td>
    </tr>
    <tr>
        <td rowspan="3"> System Parameters </td>
        <td>CACHE_L2_CACHE</td>
        <td>128 KB</td>
    </tr>
    <tr>
        <td>CACHE_L2_CACHE_LINE</td>
        <td>64 B</td>
    </tr>
    <tr>
        <td>FREERTOS_HZ</td>
        <td>100 Hz</td>
    </tr>
    <tr>
        <td rowspan="6"> Wi-Fi Parameters </td>
        <td>WIFI_STATIC_RX_BUFFER_NUM</td>
        <td>24</td>
    </tr>
    <tr>
        <td>WIFI_DYNAMIC_RX_BUFFER_NUM</td>
        <td>128</td>
    </tr>
    <tr>
        <td>WIFI_DYNAMIC_TX_BUFFER_NUM</td>
        <td>128</td>
    </tr>
    <tr>
        <td>WIFI_RX_BA_WIN</td>
        <td>32</td>
    </tr>
    <tr>
        <td>WIFI_IRAM_OPT</td>
        <td>y</td>
    </tr>
    <tr>
        <td>WIFI_RX_IRAM_OPT </td>
        <td>y</td>
    </tr>
    <tr>
        <td rowspan="7"> Lwip Parameters </td>
        <td>LWIP_IRAM_OPTIMIZATION</td>
        <td>y</td>
    </tr>
    <tr>
        <td>LWIP_TCPIP_RECVMBOX_SIZE</td>
        <td>64</td>
    </tr>
    <tr>
        <td>LWIP_TCP_WND_DEFAULT</td>
        <td>65535</td>
    </tr>
    <tr>
        <td>LWIP_TCP_SND_BUF_DEFAULT</td>
        <td>65535</td>
    </tr>
    <tr>
        <td>LWIP_TCP_RECVMBOX_SIZE</td>
        <td>64</td>
    </tr>
    <tr>
        <td>LWIP_TCP_ACCEPTMBOX_SIZE</td>
        <td>64</td>
    </tr>
    <tr>
        <td>LWIP_UDP_RECVMBOX_SIZE</td>
        <td>64</td>
    </tr>
    <tr>
        <td rowspan="10"> Tasks Parameters </td>
        <td>Iperf traffic Core ID</td>
        <td>0</td>
    </tr>
    <tr>
        <td>Iperf traffic Priority</td>
        <td>23</td>
    </tr>
    <tr>
        <td>Lwip Core ID</td>
        <td>0</td>
    </tr>
    <tr>
        <td>Lwip Priority</td>
        <td>23</td>
    </tr>
    <tr>
        <td>Wi-Fi Core ID</td>
        <td>0</td>
    </tr>
    <tr>
        <td>Wi-Fi Priority</td>
        <td>23</td>
    </tr>
    <tr>
        <td>SDIO TX Core ID</td>
        <td>1</td>
    </tr>
    <tr>
        <td>SDIO TX Priority</td>
        <td>24</td>
    </tr>
    <tr>
        <td>SDIO RX Core ID</td>
        <td>1</td>
    </tr>
    <tr>
        <td>SDIO RX Priority</td>
        <td>23</td>
    </tr>
    <tr>
        <td rowspan="4"> Router parameters </td>
        <td>Type</td>
        <td>ASUS RT-AX88U Pro</td>
    </tr>
    <tr>
        <td>Bandwidth</td>
        <td>40MHZ</td>
    </tr>
    <tr>
        <td>Beacon interval (ms)</td>
        <td>1000</td>
    </tr>
    <tr>
        <td>Mode</td>
        <td>N only</td>
    </tr>

</table>

### 2. Performance
#### 1. TCP TX
```sh
[ ID] Interval            Transfer      Bandwidth
[  1] 0.0000-1.0000 sec   5.69 MBytes   47.7 Mbits/sec
[  1] 1.0000-2.0000 sec   5.77 MBytes   48.4 Mbits/sec
[  1] 2.0000-3.0000 sec   5.79 MBytes   48.6 Mbits/sec
[  1] 3.0000-4.0000 sec   5.81 MBytes   48.8 Mbits/sec
[  1] 4.0000-5.0000 sec   5.90 MBytes   49.5 Mbits/sec
[  1] 5.0000-6.0000 sec   5.87 MBytes   49.2 Mbits/sec
[  1] 6.0000-7.0000 sec   5.88 MBytes   49.3 Mbits/sec
[  1] 7.0000-8.0000 sec   5.80 MBytes   48.6 Mbits/sec
[  1] 8.0000-9.0000 sec   5.80 MBytes   48.6 Mbits/sec
[  1] 9.0000-10.0000 sec  5.79 MBytes   48.6 Mbits/sec
[  1] 0.0000-10.0053 sec  58.1 MBytes   48.7 Mbits/sec
```

#### 2. TCP RX
```sh
 Interval      Bandwidth
 0.0- 1.0 sec  51.24 Mbits/sec
 1.0- 2.0 sec  51.04 Mbits/sec
 2.0- 3.0 sec  51.59 Mbits/sec
 3.0- 4.0 sec  51.44 Mbits/sec
 4.0- 5.0 sec  51.58 Mbits/sec
 5.0- 6.0 sec  51.45 Mbits/sec
 6.0- 7.0 sec  51.25 Mbits/sec
 7.0- 8.0 sec  51.91 Mbits/sec
 8.0- 9.0 sec  51.66 Mbits/sec
 9.0-10.0 sec  51.21 Mbits/sec
 0.0-10.0 sec  51.44 Mbits/sec
 ```

 #### 3. UDP TX
 ```sh
[ ID] Interval            Transfer     Bandwidth        Jitter    Lost/Total Datagrams
[  1] 0.0000-1.0000 sec   7.69 MBytes  64.5 Mbits/sec   0.178 ms  0/5558 (0%)
[  1] 1.0000-2.0000 sec   7.72 MBytes  64.7 Mbits/sec   0.172 ms  0/5580 (0%)
[  1] 2.0000-3.0000 sec   7.73 MBytes  64.8 Mbits/sec   0.196 ms  0/5589 (0%)
[  1] 3.0000-4.0000 sec   7.71 MBytes  64.7 Mbits/sec   0.178 ms  0/5576 (0%)
[  1] 4.0000-5.0000 sec   7.72 MBytes  64.8 Mbits/sec   0.167 ms  0/5583 (0%)
[  1] 5.0000-6.0000 sec   7.73 MBytes  64.8 Mbits/sec   0.163 ms  0/5588 (0%)
[  1] 6.0000-7.0000 sec   7.73 MBytes  64.9 Mbits/sec   0.168 ms  22/5615 (0.39%)
[  1] 7.0000-8.0000 sec   7.73 MBytes  64.8 Mbits/sec   0.177 ms  24/5614 (0.43%)
[  1] 8.0000-9.0000 sec   7.72 MBytes  64.8 Mbits/sec   0.171 ms  20/5606 (0.36%)
[  1] 9.0000-10.0000 sec  7.72 MBytes  64.7 Mbits/sec   0.176 ms  27/5607 (0.48%)
[  1] 0.0000-10.0010 sec  77.2 MBytes  64.8 Mbits/sec   0.195 ms  93/55922 (0.17%)
```

#### 4. UDP RX
 ```sh
Interval       Bandwidth
 0.0- 1.0 sec  84.16 Mbits/sec
 1.0- 2.0 sec  84.34 Mbits/sec
 2.0- 3.0 sec  84.43 Mbits/sec
 3.0- 4.0 sec  84.14 Mbits/sec
 4.0- 5.0 sec  83.56 Mbits/sec
 5.0- 6.0 sec  84.45 Mbits/sec
 6.0- 7.0 sec  84.57 Mbits/sec
 7.0- 8.0 sec  84.20 Mbits/sec
 8.0- 9.0 sec  83.71 Mbits/sec
 9.0-10.0 sec  83.86 Mbits/sec
 0.0-10.0 sec  84.14 Mbits/sec
```

> **Note : Due to the limited clock frequency (40M) of SDIO, there may be slight packet loss in the UDP TX direction.**
