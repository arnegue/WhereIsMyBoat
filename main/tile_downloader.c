#include "tile_downloader.h"

#include <math.h>

#include "wifi.h"
#include "lvgl.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "pngle.h"
#include "smallBoat.c"

#define TILE_SIZE 256 // Tile size in pixels
#define TILE_PIXELS (TILE_SIZE * TILE_SIZE)

#define TILES_PER_COLUMN 3
#define TILES_PER_ROW 2
#define TILES_COUNT (TILES_PER_COLUMN * TILES_PER_ROW)

#define IMAGE_WIDTH (TILES_PER_COLUMN * TILE_SIZE)
#define IMAGE_HEIGHT (TILES_PER_ROW * TILE_SIZE)

#define TILE_URL_TEMPLATE "http://tile.openstreetmap.org/%d/%d/%d.png"

static const char *LOG_TAG = "TileDownloader";

static lv_obj_t *img_widgets[TILES_COUNT] = {NULL}; // Array to hold image widgets
static lv_img_dsc_t img_descs[TILES_COUNT];         // Array to hold image descriptors
static lv_color_t *image_buffers[TILES_COUNT];      // Buffers for image data
static lv_obj_t *shipMarker = NULL;                 // Ship position marked on map
static uint8_t httpData[TILE_PIXELS];               // Buffer for http
static pngle_t *pngle_handle;

static size_t pixel_index = 0;             // Index to keep track of the current pixel in a tile
static uint8_t tile_index = 0;             // Index to keep track of current tile
static lv_coord_t shipTileCoordinateX = 0; // X-Coordinate of ship in tile
static lv_coord_t shipTileCoordinateY = 0; // Y-Coordinate of ship in tile

// Converts Position to tile coordinates
void position_to_tile_coordinates(double lat, double lon, int zoom, int *x_tile, int *y_tile)
{
    int n = 1 << zoom;
    *x_tile = (int)((lon + 180.0) / 360.0 * n);
    *y_tile = (int)((1.0 - log(tan(lat * M_PI / 180.0) + 1 / cos(lat * M_PI / 180.0)) / M_PI) / 2.0 * n);
}

bool new_tiles_for_position_needed(const double oldLatitude, const double oldLongitude, const int oldZoom, const double latitude, const double longitude, const int zoom)
{
    int oldX;
    int oldY;
    position_to_tile_coordinates(oldLatitude, oldLongitude, oldZoom, &oldX, &oldY);

    int newX;
    int newY;
    position_to_tile_coordinates(latitude, longitude, zoom, &newX, &newY);

    return (oldX != newX) || (oldY != newY);
}

// Function to calculate pixel coordinates in a tile
void get_pixel_coordinates(const double latitude, const double longitude, const int zoom, lv_coord_t *x_pixel, lv_coord_t *y_pixel)
{
    // Convert latitude and longitude to radians
    double lat_rad = latitude * M_PI / 180.0;

    // Calculate the x and y tile coordinates
    double n = pow(2.0, zoom);
    double x_tile = (longitude + 180.0) / 360.0 * n;
    double y_tile = (1.0 - log(tan(lat_rad) + 1.0 / cos(lat_rad)) / M_PI) / 2.0 * n;

    // Calculate the pixel coordinates within the tile
    *x_pixel = (lv_coord_t)fmod(x_tile * TILE_SIZE, TILE_SIZE);
    *y_pixel = (lv_coord_t)fmod(y_tile * TILE_SIZE, TILE_SIZE);
}

// Adds a shipmarker to screen
void add_ship_marker()
{
    if (shipMarker == NULL)
    {
        shipMarker = lv_img_create(lv_scr_act());
        lv_img_set_src(shipMarker, &smallBoat);
    }

    lv_coord_t xCoord = (TILE_SIZE * 1) + shipTileCoordinateX - (smallBoat.header.w / 2); // Column 0
    lv_coord_t yCoord = (TILE_SIZE * 0) + shipTileCoordinateY - (smallBoat.header.h / 2); // Row 1

    lv_obj_set_pos(shipMarker, xCoord, yCoord);
}

void update_ship_marker(const double latitude, const double longitude, const int zoom)
{
    get_pixel_coordinates(latitude, longitude, zoom, &shipTileCoordinateX, &shipTileCoordinateY);
    add_ship_marker();
    lv_obj_invalidate(shipMarker);
}

// Shows all downloaded tiles on screen
void show_tiles()
{
    // Update all tiles at once
    int i = 0;
    for (int row = 0; row < TILES_PER_ROW; row++)
    {
        for (int column = 0; column < TILES_PER_COLUMN; column++)
        {

            if (img_widgets[i] == NULL) // Initial stuff to create image
            {
                // Create an image widget
                img_widgets[i] = lv_img_create(lv_scr_act());

                // Initialize the image descriptor
                img_descs[i].header.always_zero = 0;
                img_descs[i].header.w = TILE_SIZE;
                img_descs[i].header.h = TILE_SIZE;
                img_descs[i].header.cf = LV_IMG_CF_TRUE_COLOR;
                img_descs[i].data_size = TILE_PIXELS * sizeof(lv_color_t);
                img_descs[i].data = (uint8_t *)image_buffers[i];

                // Set the image source to the widget
                lv_img_set_src(img_widgets[i], &img_descs[i]);

                // Position the widget on the screen
                lv_coord_t x = column * TILE_SIZE;
                lv_coord_t y = row * TILE_SIZE;
                lv_obj_set_pos(img_widgets[i], x, y);

                // Move to background so that labels, buttons, etc are in front
                lv_obj_move_background(img_widgets[i]);
            }
            else
            {
                img_descs[i].data = (uint8_t *)image_buffers[i];
            }

            if (column == 1 && row == 0)
            {
                add_ship_marker();
            }

            // Tell screen to updates this on next loading
            lv_obj_invalidate(img_widgets[i]);
            i++;
        }
    }
}

// Gets called every time a pixel of that PNG-data got converted. Converts each pixel int o a lv_color-object and puts it into convertedImageData-buffer
void on_draw(pngle_t *, uint32_t, uint32_t, uint32_t, uint32_t, uint8_t rgba[4])
{
    uint8_t r = rgba[0]; // 0 - 255
    uint8_t g = rgba[1]; // 0 - 255
    uint8_t b = rgba[2]; // 0 - 255

    image_buffers[tile_index][pixel_index] = lv_color_make(r, g, b); // Convert the RGB values to an lv_color_t and store it in the buffer
    pixel_index = (pixel_index + 1) % TILE_PIXELS;
}

// Downloads a tile and puts it into given buffer
esp_err_t download_tile(const int x_tile, const int y_tile, const int zoom)
{
    if (wifi_get_state() != CONNECTED)
    {
        ESP_LOGE(LOG_TAG, "Not Downloading. Currently not connected");
        return ESP_FAIL;
    }

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

    int headerResult = (int)esp_http_client_fetch_headers(client);
    if (headerResult < 0)
    {
        int statusCode = esp_http_client_get_status_code(client);
        ESP_LOGE(LOG_TAG, "Problem in esp_http_client_fetch_headers (%s): %d. StatusCode: %d", url, headerResult, statusCode);
        return ESP_FAIL;
    }
    int clientReadResult = esp_http_client_read(client, (char *)httpData, headerResult);
    if (clientReadResult < 0)
    {
        ESP_LOGE(LOG_TAG, "Problem in esp_http_client_read %d", clientReadResult);
        return ESP_FAIL;
    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    ESP_LOGI(LOG_TAG, "Download okay of url %s", url);
    return ESP_OK;
}

esp_err_t setup_tile_downloader()
{
    // instantiate PNGLE and set callbacks
    pngle_handle = pngle_new();
    pngle_set_draw_callback(pngle_handle, on_draw);

    // instantiate buffers
    for (int i = 0; i < TILES_COUNT; i++)
    {
        image_buffers[i] = (lv_color_t *)heap_caps_malloc(TILE_PIXELS * sizeof(lv_color_t), MALLOC_CAP_SPIRAM); // Buffer for one tile
        if (image_buffers[i] == NULL)
        {
            ESP_LOGE(LOG_TAG, "Failed to allocate memory for tile in PSRAM");
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

esp_err_t download_and_display_image(const double latitude, const double longitude, const int zoom)
{
    esp_err_t ret = ESP_OK;
    // Get tile coordinates
    int baseX;
    int baseY;
    position_to_tile_coordinates(latitude, longitude, zoom, &baseX, &baseY);

    // Get coordinates to put ship marker on tile
    get_pixel_coordinates(latitude, longitude, zoom, &shipTileCoordinateX, &shipTileCoordinateY);

    // Download and draw tiles
    for (int row = 0; row < TILES_PER_ROW; row++)
    {
        for (int column = 0; column < TILES_PER_COLUMN; column++)
        {
            int xTile = baseX + column - 1; // -1 so that the current position is in the middle
            int yTile = baseY + row;

            if (download_tile(xTile, yTile, zoom) == ESP_OK)
            {
                int fed = pngle_feed(pngle_handle, httpData, TILE_PIXELS);
                if (fed < 0)
                {
                    ESP_LOGI(LOG_TAG, "PNGLE_Error: %s", pngle_error(pngle_handle));
                    ret = ESP_FAIL;
                }
                pngle_reset(pngle_handle);
            }
            else
            {
                ESP_LOGE(LOG_TAG, "Problem when download tile %d/%d at %d/%d", xTile, yTile, column, row);
                ret = ESP_FAIL;
            }

            tile_index = (tile_index + 1) % TILES_COUNT;
            lv_timer_handler();
        }
    }

    show_tiles();
    return ret;
}
