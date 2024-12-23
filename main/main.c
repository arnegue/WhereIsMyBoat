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

    double prevLatitude = 0;
    double prevLongitude = 0;
    int prevZoom = 0;

    double curLatitude;
    double curLongitude;
    int currentZoom = 11; // TODO
    while (1)
    {
        get_position(&curLatitude, &curLongitude);
        if (curLatitude != prevLatitude || curLongitude != prevLongitude || currentZoom != prevZoom)
        {
            ESP_LOGI(TAG, "New position, updating map...");
            download_and_display_image(curLatitude, curLongitude, currentZoom);
            prevLatitude = curLatitude;
            prevLongitude = curLongitude;
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
