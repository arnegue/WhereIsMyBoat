#ifndef WIFI_H_
#define WIFI_H_

#include "esp_wifi.h"

// Sets up wifi
esp_err_t wifi_init();

// Connects to given network
esp_err_t wifi_connect(char* ssid, char* password, wifi_auth_mode_t auth_mode);

// Connects to last saved network
esp_err_t wifi_connect_last_saved();

// Scans for available networks
wifi_ap_record_t* wifi_scan_networks(uint16_t* amount_networks_found); // TODO put return value as param and return esP_err_t instead

#endif // WIFI_H_
