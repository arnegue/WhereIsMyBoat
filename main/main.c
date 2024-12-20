#include "esp_log.h"
#include "esp_http_client.h"
#include "lvgl.h"
#include "math.h"

#include "display.h"
#include "wifi.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480

#define AMOUNT_X_TILES (SCREEN_WIDTH / TILE_SIZE + 1)
#define AMOUNT_Y_TILES (SCREEN_HEIGHT / TILE_SIZE + 1)
#define TILE_SIZE 256                                // Tile size in pixels
#define TILE_BUFFER_SIZE (TILE_SIZE * TILE_SIZE * 2) // RGB565 or 16-bit color

// #define TILE_URL_TEMPLATE "http://tiles.openseamap.org/seamark/%d/%d/%d.png"
#define TILE_URL_TEMPLATE "http://tile.openstreetmap.org/%d/%d/%d.png"

void latlon_to_tile(double lat, double lon, int zoom, int *x_tile, int *y_tile)
{
    int n = 1 << zoom;
    *x_tile = (int)((lon + 180.0) / 360.0 * n);
    *y_tile = (int)((1.0 - log(tan(lat * M_PI / 180.0) + 1 / cos(lat * M_PI / 180.0)) / M_PI) / 2.0 * n);
}

esp_err_t download_tile(int x_tile, int y_tile, int zoom, uint8_t *tile_data)
{
    char url[128];
    snprintf(url, sizeof(url), TILE_URL_TEMPLATE, zoom, x_tile, y_tile);

    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 5000};
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "User-Agent", "ESP32-OSM-TileDownloader/1.0");

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK)
    {
        esp_http_client_cleanup(client);
        return err;
    }

    int headerResult = esp_http_client_fetch_headers(client);
    if (headerResult < 0)
    {
        int statusCode = esp_http_client_get_status_code(client);
        ESP_LOGI("main", "Problem in esp_http_client_fetch_headers: %d. StatusCode: %d", headerResult, statusCode);
        return ESP_FAIL;
    }
    int clientReadResult = esp_http_client_read(client, (char *)tile_data, headerResult);
    if (clientReadResult < 0)
    {
        ESP_LOGI("main", "Problem in esp_http_client_read %d", clientReadResult);
        return ESP_FAIL;
    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    ESP_LOGI("main", "Download okay of url %s", url);
    return ESP_OK;
}

void render_tile(uint8_t *tile_data, int x, int y)
{
    lv_img_dsc_t img_dsc = {
        .header.always_zero = 0,
        .header.w = TILE_SIZE,
        .header.h = TILE_SIZE,
        .header.cf = LV_IMG_CF_TRUE_COLOR,
        .data_size = TILE_BUFFER_SIZE,
        .data = tile_data,
    };

    lv_obj_t *img = lv_img_create(lv_scr_act());
    lv_img_set_src(img, &img_dsc);
    lv_obj_set_pos(img, x, y);
}

void app_main(void)
{
    ESP_LOGI("main", "Starting up");
    wifi_init_sta();
    init_display();

    double lat = 55.48720; // Example latitude
    double lon = 12.59740; // Example longitude
    int zoom = 11;

    uint8_t *tile_data = heap_caps_malloc(TILE_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
    if (tile_data == NULL)
    {
        ESP_LOGE("main", "Failed to allocate memory for tile in PSRAM");
        while (1)
        {
        }
    }

    int x_tile;
    int y_tile;
    latlon_to_tile(lat, lon, zoom, &x_tile, &y_tile);

    ESP_LOGI("main", "Setup okay. Start");
    for (int ty = 0; ty < AMOUNT_Y_TILES; ++ty)
    {
        for (int tx = 0; tx < AMOUNT_X_TILES; ++tx)
        {
            if (download_tile(x_tile + tx, y_tile + ty, zoom, tile_data) == ESP_OK)
            {
                int x_pos = tx * TILE_SIZE;
                int y_pos = ty * TILE_SIZE;
                render_tile(tile_data, x_pos, y_pos);
            }
            else
            {
                ESP_LOGE("main", "Problem when download tile %d/%d", x_tile + tx, y_tile + ty);
            }
        }
    }
    lv_scr_load(lv_scr_act());
    
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello, ESP32!");


    while (1)
    {
        // TODO auto position = MarineTraffic::APIGetBoatPosition("Aeolus", "MMSI")
        // if position != oldposition
        //      TODO auto image = OpenSeaMap.GetMapImage(position)
        //      (TODO drawMarkerInMiddleOfBitmap(image)
        //      TODO displayImage(image)

        // TODO check if error (wifi, download, etc)
        update_display();
    }
}
