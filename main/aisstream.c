#include "aisstream.h"

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_websocket_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "config.h"

#define WEBSOCKET_URI "wss://stream.aisstream.io/v0/stream"

bool receivedValidData = false;

static const char *TAG = "AISSTREAM";

void parseData(esp_websocket_event_data_t *data)
{
    ESP_LOGI(TAG, "Data Length: %d, Data: %.*s", data->data_len, data->data_len, (char *)data->data_ptr);
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
        break;
    case WEBSOCKET_EVENT_DATA:
        ESP_LOGI(TAG, "Received WebSocket Data");
        parseData(data);
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGE(TAG, "WebSocket Error");
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
        if (esp_websocket_client_is_connected(client) && !receivedValidData)
        {
            const char *msg = "{\"APIKey\": \"" AISSTREAM_API_KEY "\",\"BoundingBoxes\":[[[-90, -180], [90, 180]]],\"FilterMessageTypes\":[\"PositionReport\"],\"FiltersShipMMSI\": [\"371836000\"]}";
            ESP_LOGI(TAG, "Sending: %s", msg);
            esp_websocket_client_send_text(client, msg, strlen(msg), portMAX_DELAY);
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

void get_position(double *latitude, double *longitude)
{
    // TODO use some API
    *latitude = 53.57227333;
    *longitude = 9.6468483;
}
