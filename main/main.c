#include <math.h>
#include <stdio.h>
#include "esp_log.h"
#include "lvgl.h"

#include "display.h"
#include "wifi.h"
#include "aisstream.h"
#include "tile_downloader.h"

static const char *TAG = "main";

int currentZoom = 10;

// Get's called if zoom in button event occurred. Increases zoom
void zoom_in_button_callback(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED && currentZoom <= 20)
    {
        currentZoom++;
    }
    ESP_LOGI(TAG, "zoom_in_button_callback! Type: %d, Zoom: %d", code, currentZoom);
}

// Get's called if zoom out button event occurred. Decreases zoom
void zoom_out_button_callback(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED && currentZoom >= 0)
    {
        currentZoom--;
    }
    ESP_LOGI(TAG, "zoom_out_button_callback! Type: %d, Zoom: %d", code, currentZoom);
}

// Puts decimal position to a degree position string with N/S or E/W char infront
void decimal_to_dms(double decimal, char *result, bool isLat)
{
    char direction;
    int degrees, minutes;
    double seconds;

    if (isLat)
    {
        direction = (decimal >= 0) ? 'N' : 'S';
    }
    else
    {
        direction = (decimal >= 0) ? 'E' : 'W';
    }

    decimal = fabs(decimal);
    degrees = (int)decimal;
    double fractional_part = decimal - degrees;
    minutes = (int)(fractional_part * 60);
    seconds = (fractional_part * 60 - minutes) * 60;
    sprintf(result, "%c %d°%02d'%02.0f", direction, degrees, minutes, seconds);
}

lv_obj_t *create_button(const char *sign, int x_pos, int y_pos, lv_event_cb_t event_cb)
{
    // Create a button on the screen
    lv_obj_t *btn = lv_btn_create(lv_scr_act());

    // Set button position and size
    lv_obj_set_pos(btn, x_pos, y_pos); // x = 50, y = 50
    lv_obj_set_size(btn, 50, 50);      // width = 50, height = 50

    // Add an event callback to the button
    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);

    // Add a label to the button
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, sign);
    lv_obj_center(label); // Center the label within the button
    return btn;
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting up");
    wifi_init_sta();
    init_display();
    setup_tile_downloader();
    setup_aisstream();
    create_button("+", 50, 50, zoom_in_button_callback);
    create_button("-", 50, 100, zoom_out_button_callback);

    ESP_LOGI(TAG, "Setup okay. Start");
    lv_scr_load(lv_scr_act());

    double prevLatitude = 53.5743;
    double prevLongitude = 9.6826;
    int prevZoom = currentZoom;

    // Initially display it once, so that something is shown until a valid position was received
    download_and_display_image(prevLatitude, prevLongitude, prevZoom);
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Waiting for data...");
    lv_obj_set_pos(label, 0, 410);

    // Create state marker (colored dot)
    lv_obj_t *stateMarker = lv_obj_create(lv_scr_act());
    lv_obj_set_size(stateMarker, 15, 15);                      // Dot size
    lv_obj_set_style_radius(stateMarker, LV_RADIUS_CIRCLE, 0); // Make it circular
    lv_obj_set_style_bg_opa(stateMarker, LV_OPA_COVER, 0);     // Fully opaque
    lv_obj_set_pos(stateMarker, 770, 30);

    char labelBuffer[100];
    char latitudeBuffer[20];
    char longitudeBuffer[20];
    while (1)
    {
        struct AIS_DATA *aisData = get_last_ais_data();

        // If data is valid and a new map has to be downloaded
        if (aisData->validity == VALID)
        {
            if (new_tiles_for_position_needed(prevLatitude, prevLongitude, prevZoom, aisData->latitude, aisData->longitude, currentZoom))
            {
                ESP_LOGI(TAG, "New position, updating map...");
                download_and_display_image(aisData->latitude, aisData->longitude, currentZoom);
                prevLatitude = aisData->latitude;
                prevLongitude = aisData->longitude;
                prevZoom = currentZoom;
            }

            decimal_to_dms(aisData->latitude, latitudeBuffer, true);
            decimal_to_dms(aisData->longitude, longitudeBuffer, false);

            char timeBuffer[9]; // HH:MM:SS is 8 characters + 1 for null terminator
            strncpy(timeBuffer, aisData->time_utc + 11, 8);
            timeBuffer[8] = '\0';

            snprintf(labelBuffer, sizeof(labelBuffer), "%s\n%s\n%s\n%s", aisData->shipName, latitudeBuffer, longitudeBuffer, timeBuffer);
            lv_label_set_text(label, labelBuffer);
        }
        else if (prevZoom != currentZoom) // Invalid data, but zoom changed
        {
            download_and_display_image(prevLatitude, prevLongitude, currentZoom);
            prevZoom = currentZoom;
        }

        switch (aisData->validity)
        {
        // TODO Wifi state -> black
        case NO_CONNECTION:
            lv_obj_set_style_bg_color(stateMarker, lv_color_hex(0xFF0000), 0); // Red color
            break;
        case CONNECTION_BUT_NO_DATA:
            lv_obj_set_style_bg_color(stateMarker, lv_color_hex(0xFFA500), 0); // Orange color
            break;
        case CONNECTION_BUT_CORRUPT_DATA:
            lv_obj_set_style_bg_color(stateMarker, lv_color_hex(0xFFFF00), 0); // Yellow color
            break;
        case VALID:
            lv_obj_set_style_bg_color(stateMarker, lv_color_hex(0x00FF00), 0); // Green color
        default:
            break;
        }
        lv_obj_set_pos(stateMarker, 770, 30);
        update_display();
    }
}
