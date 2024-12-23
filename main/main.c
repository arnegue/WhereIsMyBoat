#include "esp_log.h"
#include "esp_lcd_types.h"
#include "esp_lcd_panel_ops.h"
#include "lvgl.h"

#include "display.h"
#include "wifi.h"
#include "aisstream.h"
#include "tile_downloader.h"

esp_lcd_panel_handle_t display_handle;

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting up");
    wifi_init_sta();
    display_handle = init_display();
    setup_tile_downloader(display_handle);
    setup_aisstream();

    ESP_LOGI(TAG, "Setup okay. Start");

    lv_scr_load(lv_scr_act());
    lv_obj_t *label = lv_label_create(lv_scr_act());

    double prevLatitude = 53.57227333;
    double prevLongitude = 9.6468483;
    int prevZoom = 11;

    int currentZoom = 11; // TODO

    // Initially display it once, so that something is shown until a valid position was received
    download_and_display_image(prevLatitude, prevLongitude, prevZoom);

    while (1)
    {
        struct AIS_DATA aisData = get_last_ais_data();

        // If data is valid and something changed
        if (aisData.isValid && (aisData.latitude != prevLatitude || aisData.longitude != prevLongitude || currentZoom != prevZoom))
        {
            ESP_LOGI(TAG, "New position, updating map...");
            download_and_display_image(aisData.latitude, aisData.longitude, currentZoom);
            prevLatitude = aisData.latitude;
            prevLongitude = aisData.longitude;
            prevZoom = currentZoom;

            lv_label_set_text(label, "Hello, ESP32!"); // TODO put some infos of aistream into it
        }
        // TODO auto position = MarineTraffic::APIGetBoatPosition("Aeolus", "MMSI")
        // if position != oldposition
        //      TODO auto image = OpenSeaMap.GetMapImage(position)
        //      (TODO drawMarkerInMiddleOfBitmap(image)
        //      TODO displayImage(image)

        // TODO check if error (wifi, download, etc)
        update_display();
    }
}
