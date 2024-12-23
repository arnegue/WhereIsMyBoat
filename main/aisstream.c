#include "aisstream.h"

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_websocket_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cJSON.h"

#include "config.h"

#define WEBSOCKET_URI "wss://stream.aisstream.io/v0/stream"

struct AIS_DATA lastAisData = {};
bool sendSinceLastConnection = false;
static const char *TAG = "AISSTREAM";

void parseData(esp_websocket_event_data_t *data)
{
    ESP_LOGI(TAG, "Data Length: %d, Data: %.*s", data->data_len, data->data_len, (char *)data->data_ptr);

    char *json_data = strndup(data->data_ptr, data->data_len); // Create a null-terminated string
    if (json_data == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for JSON data");
        return;
    }

    cJSON *root = cJSON_Parse(json_data);
    if (root == NULL)
    {
        ESP_LOGE(TAG, "Failed to parse JSON");
        lastAisData.isValid = false;
        cJSON_Delete(root);
        free(json_data);
        return;
    }

    cJSON *meta_data = cJSON_GetObjectItem(root, "MetaData");
    if (meta_data == NULL)
    {
        ESP_LOGE("JSON", "MetaData object not found");
        lastAisData.isValid = false;
        cJSON_Delete(root);
        free(json_data);
        return;
    }

    cJSON *mmsi = cJSON_GetObjectItem(meta_data, "MMSI");
    if (cJSON_IsNumber(mmsi))
    {
        // MMSI
        ESP_LOGI(TAG, "MMSI: %d", mmsi->valueint);
        lastAisData.isValid = true;
        lastAisData.mmsi = mmsi->valueint;

        // Longitude
        cJSON *longitude = cJSON_GetObjectItem(meta_data, "Longitude");
        if (cJSON_IsNumber(longitude))
        {
            ESP_LOGI(TAG, "Longitude: %f", longitude->valuedouble);
            lastAisData.longitude = longitude->valuedouble;
        }
        else
        {
            ESP_LOGW(TAG, "Unable to get Longitude");
            lastAisData.isValid = false;
        }

        // Latitude
        cJSON *latitude = cJSON_GetObjectItem(meta_data, "Latitude");
        if (cJSON_IsNumber(latitude))
        {
            ESP_LOGI(TAG, "Latitude: %f", latitude->valuedouble);
            lastAisData.latitude = latitude->valuedouble;
        }
        else
        {
            ESP_LOGW(TAG, "Unable to get Latitude");
        }

        // ShipName
        cJSON *shipName = cJSON_GetObjectItem(meta_data, "ShipName");
        if (cJSON_IsString(shipName))
        {
            ESP_LOGI(TAG, "ShipName: %s", shipName->valuestring);
            lastAisData.shipName = strdup(shipName->valuestring);
        }
        else
        {
            ESP_LOGW(TAG, "Unable to get ShipName");
            lastAisData.isValid = false;
        }

        // time_utc
        cJSON *timeUTC = cJSON_GetObjectItem(meta_data, "time_utc");
        if (cJSON_IsString(timeUTC))
        {
            ESP_LOGI(TAG, "timeUTC: %s", timeUTC->valuestring);
            lastAisData.time_utc = strdup(timeUTC->valuestring);
        }
        else
        {
            ESP_LOGW(TAG, "Unable to get timeUTC");
            lastAisData.isValid = false;
        }
    }
    else
    {
        ESP_LOGW(TAG, "MMSI not found or not a number");
        lastAisData.isValid = false;
    }

    // Clean up
    cJSON_Delete(root);
    free(json_data);
}

// WebSocket Event Handler
static void websocket_event_handler(void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id)
    {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WebSocket Connected");
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WebSocket Disconnected");
        sendSinceLastConnection = false;
        lastAisData.isValid = false;
        break;
    case WEBSOCKET_EVENT_DATA:
        ESP_LOGI(TAG, "Received WebSocket Data");
        parseData(data);
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGE(TAG, "WebSocket Error");
        lastAisData.isValid = false;
    }
}

void websocket_task(void *pvParameters)
{
    esp_websocket_client_config_t websocket_cfg = {
        .uri = WEBSOCKET_URI,
        .skip_cert_common_name_check = true};

    esp_websocket_client_handle_t client = esp_websocket_client_init(&websocket_cfg);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, NULL);

    // Start the WebSocket client
    esp_websocket_client_start(client);

    while (true)
    {
        if (esp_websocket_client_is_connected(client) && !sendSinceLastConnection)
        {
            const char *msg = "{\"APIKey\": \"" AISSTREAM_API_KEY "\",\"BoundingBoxes\":[[[-90, -180], [90, 180]]],\"FilterMessageTypes\":[\"PositionReport\"],\"FiltersShipMMSI\": [\"" SHIP_MMSI "\"]}";
            ESP_LOGI(TAG, "Sending: %s", msg);
            esp_websocket_client_send_text(client, msg, strlen(msg), portMAX_DELAY);
            sendSinceLastConnection = true;
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    esp_websocket_client_stop(client);
    esp_websocket_client_destroy(client);
}

void setup_aisstream()
{
    // Start WebSocket task
    xTaskCreate(&websocket_task, "websocket_task", 8192, NULL, 5, NULL);
}

struct AIS_DATA* get_last_ais_data()
{
    return &lastAisData;
}
