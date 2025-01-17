#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "wifi.h"

#define MAX_WIFI_LIST_SIZE 20

static const char *TAG = "WiFi";
static esp_netif_t *sta_netif = NULL;

enum WIFI_STATE currentWiFiState = DISCONNECTED;

enum WIFI_STATE wifi_get_state()
{
    return currentWiFiState;
}

// Event handler for WiFi and IP events
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        currentWiFiState = STARTING;
        ESP_LOGI(TAG, "WiFi started, connecting...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        currentWiFiState = DISCONNECTED;
        ESP_LOGI(TAG, "Disconnected from WiFi, attempting to reconnect...");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        currentWiFiState = CONNECTED;
        const ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

esp_err_t wifi_init()
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize the TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Create the default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default WiFi station
    sta_netif = esp_netif_create_default_wifi_sta();

    // Initialize WiFi with default configurations
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));

    // Set WiFi mode to STA
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    return ESP_OK;
}

esp_err_t wifi_connect(wifi_config_t *wifi_config, bool wait_for_connection)
{
    esp_wifi_disconnect();
    esp_err_t ret = esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config);
    if (ret != ESP_OK)
    {
        ESP_LOGW("WiFi", "Failed to set config: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_wifi_connect();

    if (wait_for_connection)
    {
        // Allow time for connection to establish and IP to be acquired
        for (int i = 0; i < 5; i++)
        {
            // TODO maybe wait for event bits instead?
            if (currentWiFiState == CONNECTED)
            {
                ESP_LOGI(TAG, "Successfully connected to %s", wifi_config->sta.ssid);
                return ESP_OK;
            }
            else
            {
                ESP_LOGI(TAG, "Waiting for connection...");
                vTaskDelay(2000 / portTICK_PERIOD_MS); // Adjust delay as needed
            }
        }
        ESP_LOGW(TAG, "Could not connect to %s", wifi_config->sta.ssid);
        esp_wifi_disconnect();
        return ESP_FAIL;
    }
    else
    {
        return ESP_OK;
    }
}

esp_err_t wifi_connect_last_saved(bool wait_for_connection)
{
    wifi_config_t wifi_configOrig;

    if (esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_configOrig) == ESP_OK)
    {
        ESP_LOGI(TAG, "Loaded WiFi settings from NVS with SSID: %s", wifi_configOrig.sta.ssid);
        return wifi_connect(&wifi_configOrig, wait_for_connection);
    }
    else
    {
        ESP_LOGI(TAG, "Could not load WiFi settings from NVS");
        return ESP_FAIL;
    }
}

wifi_ap_record_t wifi_list[MAX_WIFI_LIST_SIZE];

esp_err_t wifi_scan_networks(uint16_t *amount_networks_found, wifi_ap_record_t **records)
{
    // TODO this wont work if it's currently trying to connect to a network
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true};

    ESP_LOGI(TAG, "Starting WiFi scan...");

    // TODO this will fail if its currently trying to connect
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true)); // true = block until scan is complete

    esp_wifi_scan_get_ap_num(amount_networks_found);
    ESP_LOGI(TAG, "Number of access points found: %d", *amount_networks_found);

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(amount_networks_found, wifi_list));

    *records = wifi_list;
    return ESP_OK;
}
