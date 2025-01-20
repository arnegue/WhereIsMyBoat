#ifndef WIFI_H_
#define WIFI_H_

#include "esp_wifi.h"

// Sets up wifi
esp_err_t wifi_init();

// Connects to given network
esp_err_t wifi_connect(wifi_config_t *wifi_config, bool wait_for_connection);

enum WIFI_STATE
{
    DE_INIT,      // Default
    STARTING,     // Starting up / connecting
    CONNECTED,    // Connected to WiFi
    DISCONNECTED, // Was connected, now disconnected
    SCANNING      // Trying to scan for WiFi networks
};

enum WIFI_STATE wifi_get_state();

// Connects to last saved network
esp_err_t wifi_connect_last_saved(bool wait_for_connection);

// Scans for available networks
esp_err_t wifi_scan_networks(uint16_t *amount_networks_found, wifi_ap_record_t **records);

#endif // WIFI_H_
