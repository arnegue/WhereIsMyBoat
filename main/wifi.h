#ifndef WIFI_H_
#define WIFI_H_

#include "esp_wifi.h"

// Sets up wifi
esp_err_t wifi_init();

// Connects to given network
esp_err_t wifi_connect(wifi_config_t* wifi_config);

// Connects to last saved network
esp_err_t wifi_connect_last_saved();

// Scans for available networks
esp_err_t wifi_scan_networks(uint16_t* amount_networks_found, wifi_ap_record_t** records);

#endif // WIFI_H_
