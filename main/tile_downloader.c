#include "tile_downloader.h"

#include "math.h"
#include "lvgl.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "pngle.h"

#define TILE_SIZE 256 // Tile size in pixels
#define TILE_PIXELS (TILE_SIZE * TILE_SIZE)

#define TILES_PER_COLUMN 3
#define TILES_PER_ROW 2

#define IMAGE_WIDTH (TILES_PER_COLUMN * TILE_SIZE)
#define IMAGE_HEIGHT (TILES_PER_ROW * TILE_SIZE)

// #define TILE_URL_TEMPLATE "http://tiles.openseamap.org/seamark/%d/%d/%d.png"
#define TILE_URL_TEMPLATE "http://tile.openstreetmap.org/%d/%d/%d.png"

uint8_t httpData[TILE_PIXELS];         // = heap_caps_malloc(TILE_SIZE * TILE_SIZE, MALLOC_CAP_SPIRAM);
lv_color_t *convertedImageData = NULL; // Buffer to hold the image data
esp_lcd_panel_handle_t esp_lcd_panel_handle;
pngle_t *pngle_handle;

size_t pixel_index = 0; // Index to keep track of the current pixel
int currentTileColumn = 0;
int currentTileRow = 0;

// Converts Position to tile coordinates
void latlon_to_tile(double lat, double lon, int zoom, int *x_tile, int *y_tile)
{
    int n = 1 << zoom;
    *x_tile = (int)((lon + 180.0) / 360.0 * n);
    *y_tile = (int)((1.0 - log(tan(lat * M_PI / 180.0) + 1 / cos(lat * M_PI / 180.0)) / M_PI) / 2.0 * n);
}

// Gets called when every pixel of a tile was converted. Creates an image object and render it on screen
void on_finished(pngle_t *pngle)
{
    pngle_ihdr_t pngle_header = *pngle_get_ihdr(pngle);

    static lv_img_dsc_t img_dsc;
    img_dsc.header.always_zero = 0;
    img_dsc.header.w = pngle_header.width;
    img_dsc.header.h = pngle_header.height;
    img_dsc.data_size = pngle_header.width * pngle_header.height * sizeof(lv_color_t);
    img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    img_dsc.data = (const uint8_t *)convertedImageData;

    lv_coord_t x = currentTileColumn * pngle_header.width;
    lv_coord_t y = currentTileRow * pngle_header.height;

    lv_obj_t *img_obj = lv_img_create(lv_scr_act());

    // Set the image descriptor to the image object
    lv_img_set_src(img_obj, &img_dsc);

    lv_obj_set_style_border_width(img_obj, 0, 0); // No border
    lv_obj_set_style_pad_all(img_obj, -1, 0);     // Remove padding

    // ESP_LOGI("TileDownloader", "Image finished %d/%d. Displaying it at %d/%d", currentTileColumn, currentTileRow, x, y);
    lv_obj_set_pos(img_obj, x, y);

    lv_obj_invalidate(img_obj);

    // Update screen
    lv_timer_handler();
}

// Gets called every time a pixel of that PNG-data got converted. Converts each pixel int o a lv_color-object and puts it into convertedImageData-buffer
void on_draw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4])
{
    uint8_t r = rgba[0]; // 0 - 255
    uint8_t g = rgba[1]; // 0 - 255
    uint8_t b = rgba[2]; // 0 - 255

    // Convert the RGB values to an lv_color_t and store it in the buffer
    lv_color_t color;
    color.full = lv_color_make(r, g, b).full; // Use LVGL's color conversion

    convertedImageData[pixel_index] = color; // Store the color in the buffer
    pixel_index = (pixel_index + 1) % TILE_PIXELS;
}

// Downloads a tile and puts it into given buffer
esp_err_t download_tile(int x_tile, int y_tile, int zoom, uint8_t *httpData)
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
        ESP_LOGI("TileDownloader", "Problem in esp_http_client_fetch_headers (%s): %d. StatusCode: %d", url, headerResult, statusCode);
        return ESP_FAIL;
    }
    int clientReadResult = esp_http_client_read(client, (char *)httpData, headerResult);
    if (clientReadResult < 0)
    {
        ESP_LOGI("TileDownloader", "Problem in esp_http_client_read %d", clientReadResult);
        return ESP_FAIL;
    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    ESP_LOGI("TileDownloader", "Download okay of url %s", url);
    return ESP_OK;
}

void setup_tile_downloader(esp_lcd_panel_handle_t display_handle)
{
    esp_lcd_panel_handle = display_handle;
    pngle_handle = pngle_new();
    convertedImageData = heap_caps_malloc(TILE_PIXELS * sizeof(lv_color_t), MALLOC_CAP_SPIRAM); // Buffer for one tile
    if (convertedImageData == NULL)
    {
        ESP_LOGE("TileDownloader", "Failed to allocate memory for tile in PSRAM");
        while (1)
        {
        }
    }

    pngle_set_done_callback(pngle_handle, on_finished);
    pngle_set_draw_callback(pngle_handle, on_draw);
}

void download_image(double latitude, double longitude, int zoom)
{
    if (pngle_handle == NULL)
    {
        ESP_LOGE("TileDownloader", "PNGLE_Error, handle empty");
    }
    int baseX;
    int baseY;
    latlon_to_tile(latitude, longitude, zoom, &baseX, &baseY);

    for (currentTileRow = TILES_PER_ROW; currentTileRow >= 0; currentTileRow--)
    {
        for (currentTileColumn = TILES_PER_COLUMN; currentTileColumn >= 0; currentTileColumn--)
        {
            int xTile = baseX + currentTileColumn;
            int yTile = baseY + currentTileRow;

            if (download_tile(xTile, yTile, zoom, httpData) == ESP_OK)
            {
                int fed = pngle_feed(pngle_handle, httpData, TILE_SIZE * TILE_SIZE);
                if (fed < 0)
                {
                    ESP_LOGI("TileDownloader", "PNGLE_Error: %s", pngle_error(pngle_handle));
                }
                // TODO ensure that the finished callback was called (I mean there is no threading at all anyway)
                pngle_reset(pngle_handle);
            }
            else
            {
                ESP_LOGE("TileDownloader", "Problem when download tile %d/%d", xTile, yTile);
            }
        }
    }
}
