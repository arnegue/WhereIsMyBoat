#include <math.h>
#include "esp_log.h"
#include "esp_lcd_types.h"
#include "esp_lcd_panel_ops.h"
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

void convert_to_degrees_minutes(float decimal_degrees, char *result)
{
    // Get degrees
    int degrees = (int)decimal_degrees;
    // Get minutes by multiplying fractional part by 60
    float fractional_part = fabs(decimal_degrees - degrees);
    float minutes = fractional_part * 60.0;
    // Format result as degrees and minutes
    snprintf(result, 50, "%d° %.3f'", degrees, minutes);
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

    char labelBuffer[100];
    char latitudeBuffer[20];
    char longitudeBuffer[20];
    while (1)
    {
        struct AIS_DATA *aisData = get_last_ais_data();

        // If data is valid and a new map has to be downloaded
        if (aisData->isValid)
        {
            if (new_tiles_for_position_needed(prevLatitude, prevLongitude, prevZoom, aisData->latitude, aisData->longitude, currentZoom))
            {
                ESP_LOGI(TAG, "New position, updating map...");
                download_and_display_image(aisData->latitude, aisData->longitude, currentZoom);
                prevLatitude = aisData->latitude;
                prevLongitude = aisData->longitude;
                prevZoom = currentZoom;
            }

            convert_to_degrees_minutes(aisData->latitude, latitudeBuffer);
            convert_to_degrees_minutes(aisData->longitude, longitudeBuffer);
            snprintf(labelBuffer, sizeof(labelBuffer), "%s\n%s\n%s\n%s", aisData->shipName, latitudeBuffer, longitudeBuffer, aisData->time_utc);
            lv_label_set_text(label, labelBuffer);
        }
        else if (prevZoom != currentZoom) // Invalid data, but zoom changed
        {
            download_and_display_image(prevLatitude, prevLongitude, currentZoom);
            prevZoom = currentZoom;
        }
        update_display();
    }
}
