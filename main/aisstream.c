#include "aisstream.h"

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_websocket_client.h"
#include "esp_transport_ws.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cJSON.h"

#include "config.h"

#define WEBSOCKET_URI "wss://stream.aisstream.io/v0/stream"

char ship_mmsi[MMSI_LENGTH];
struct AIS_DATA lastAisData = {};
bool sendSinceLastConnection = false;
static const char *LOG_TAG = "aisstream";

void parseData(const esp_websocket_event_data_t *data)
{
    ESP_LOGI(LOG_TAG, "Data Length: %d, OP-Code: %d, Data: %.*s", data->data_len, data->op_code, data->data_len, (char *)data->data_ptr);
    if (data->op_code != WS_TRANSPORT_OPCODES_TEXT && data->op_code != WS_TRANSPORT_OPCODES_BINARY)
    {
        ESP_LOGW(LOG_TAG, "No data to parse were received");
        if (lastAisData.validity == NO_CONNECTION) // First connection but no data
        {
            lastAisData.validity = CONNECTION_BUT_NO_DATA;
        }
        return;
    }

    char *json_data = strndup(data->data_ptr, data->data_len); // Create a null-terminated string
    if (json_data == NULL)
    {
        ESP_LOGE(LOG_TAG, "Failed to allocate memory for JSON data");
        return;
    }

    cJSON *root = cJSON_Parse(json_data);
    if (root == NULL)
    {
        ESP_LOGE(LOG_TAG, "Failed to parse JSON");
        if (lastAisData.validity == NO_CONNECTION) // First connection but no data
        {
            lastAisData.validity = CONNECTION_BUT_NO_DATA;
        }
        cJSON_Delete(root);
        free(json_data);
        return;
    }

    const cJSON *meta_data = cJSON_GetObjectItem(root, "MetaData");
    if (meta_data == NULL)
    {
        ESP_LOGE("JSON", "MetaData object not found");
        lastAisData.validity = CONNECTION_BUT_CORRUPT_DATA;
        cJSON_Delete(root);
        free(json_data);
        return;
    }

    const cJSON *mmsi = cJSON_GetObjectItem(meta_data, "MMSI");
    if (cJSON_IsNumber(mmsi))
    {
        // MMSI
        ESP_LOGI(LOG_TAG, "MMSI: %d", mmsi->valueint);
        lastAisData.validity = VALID;
        lastAisData.mmsi = mmsi->valueint;

        // Longitude
        const cJSON *longitude = cJSON_GetObjectItem(meta_data, "Longitude");
        if (cJSON_IsNumber(longitude))
        {
            ESP_LOGI(LOG_TAG, "Longitude: %f", longitude->valuedouble);
            lastAisData.longitude = longitude->valuedouble;
        }
        else
        {
            ESP_LOGW(LOG_TAG, "Unable to get Longitude");
            lastAisData.validity = CONNECTION_BUT_CORRUPT_DATA;
        }

        // Latitude
        const cJSON *latitude = cJSON_GetObjectItem(meta_data, "Latitude");
        if (cJSON_IsNumber(latitude))
        {
            ESP_LOGI(LOG_TAG, "Latitude: %f", latitude->valuedouble);
            lastAisData.latitude = latitude->valuedouble;
        }
        else
        {
            ESP_LOGW(LOG_TAG, "Unable to get Latitude");
            lastAisData.validity = CONNECTION_BUT_CORRUPT_DATA;
        }

        // ShipName
        const cJSON *shipName = cJSON_GetObjectItem(meta_data, "ShipName");
        if (cJSON_IsString(shipName))
        {
            if (shipName->valuestring != NULL && strlen(shipName->valuestring))
            {
                ESP_LOGI(LOG_TAG, "ShipName: %s", shipName->valuestring);
                lastAisData.shipName = strdup(shipName->valuestring);
            }
            else
            {
                ESP_LOGW(LOG_TAG, "Empty name. Ignoring it.");
            }
        }
        else
        {
            ESP_LOGW(LOG_TAG, "Unable to get ShipName");
            lastAisData.validity = CONNECTION_BUT_CORRUPT_DATA;
        }

        // time_utc
        const cJSON *timeUTC = cJSON_GetObjectItem(meta_data, "time_utc");
        if (cJSON_IsString(timeUTC))
        {
            ESP_LOGI(LOG_TAG, "timeUTC: %s", timeUTC->valuestring);
            lastAisData.time_utc = strdup(timeUTC->valuestring);
        }
        else
        {
            ESP_LOGW(LOG_TAG, "Unable to get timeUTC");
            lastAisData.validity = CONNECTION_BUT_CORRUPT_DATA;
        }
    }
    else
    {
        ESP_LOGW(LOG_TAG, "MMSI not found or not a number");
        lastAisData.validity = CONNECTION_BUT_CORRUPT_DATA;
    }

    // Clean up
    cJSON_Delete(root);
    free(json_data);
}

// WebSocket Event Handler
static void websocket_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    const esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id)
    {
    case WEBSOCKET_EVENT_BEFORE_CONNECT:
    case WEBSOCKET_EVENT_BEGIN:
        ESP_LOGI(LOG_TAG, "Establishing connection");
        break;
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(LOG_TAG, "WebSocket Connected");
        break;
    case WEBSOCKET_EVENT_CLOSED:
    case WEBSOCKET_EVENT_FINISH:
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(LOG_TAG, "WebSocket Closed/Finish/Disconnected");
        sendSinceLastConnection = false;
        lastAisData.validity = NO_CONNECTION;
        break;
    case WEBSOCKET_EVENT_DATA:
        ESP_LOGI(LOG_TAG, "Received WebSocket Data");
        parseData(data);
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGE(LOG_TAG, "WebSocket Error");
        lastAisData.validity = NO_CONNECTION;
        break;
    default:
        ESP_LOGE(LOG_TAG, "Unknown Error: %" PRId32, event_id);
        lastAisData.validity = NO_CONNECTION;
        break;
    }
}

void websocket_task(void *)
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
            const uint8_t MAX_SIZE = 255;
            char msg[MAX_SIZE];
            snprintf(msg, MAX_SIZE, "{\"APIKey\":\"" AISSTREAM_API_KEY "\",\"BoundingBoxes\":[[[-90,-180],[90,180]]],\"FiltersShipMMSI\":[\"%s\"]}", ship_mmsi);
            ESP_LOGI(LOG_TAG, "Sending: %s", msg);
            esp_websocket_client_send_text(client, msg, strlen(msg), portMAX_DELAY);
            sendSinceLastConnection = true;
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    esp_websocket_client_stop(client);
    esp_websocket_client_destroy(client);
}

void set_mmsi(const char mmsi[MMSI_LENGTH])
{
    strcpy(ship_mmsi, mmsi);
    sendSinceLastConnection = false; // bad way to notify thread
}

void setup_aisstream(const char mmsi[MMSI_LENGTH])
{
    set_mmsi(mmsi);
    // Start WebSocket task
    xTaskCreate(&websocket_task, "websocket_task", 8192, NULL, 5, NULL);
}

struct AIS_DATA *get_last_ais_data()
{
    return &lastAisData;
}
