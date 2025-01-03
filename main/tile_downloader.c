#include "tile_downloader.h"

#include <math.h>
#include "lvgl.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "pngle.h"
#include "smallBoat.c"

#define TILE_SIZE 256 // Tile size in pixels
#define TILE_PIXELS (TILE_SIZE * TILE_SIZE)

#define TILES_PER_COLUMN 4
#define TILES_PER_ROW 2
#define TILES_COUNT (TILES_PER_COLUMN * TILES_PER_ROW)

#define IMAGE_WIDTH (TILES_PER_COLUMN * TILE_SIZE)
#define IMAGE_HEIGHT (TILES_PER_ROW * TILE_SIZE)

#define TILE_URL_TEMPLATE "http://tile.openstreetmap.org/%d/%d/%d.png"

lv_obj_t *img_widgets[TILES_COUNT] = {NULL}; // Array to hold image widgets
lv_img_dsc_t img_descs[TILES_COUNT];         // Array to hold image descriptors
lv_color_t *image_buffers[TILES_COUNT];      // Buffers for image data
lv_obj_t *shipMarker = NULL;                 // Ship position marked on map
uint8_t httpData[TILE_PIXELS];               // Buffer for http
pngle_t *pngle_handle;

size_t pixel_index = 0;    // Index to keep track of the current pixel
int currentTileColumn = 0; // Currently handled row of tiles
int currentTileRow = 0;    // Currently handled column of tiles
int shipCoordinateX = 0;   // X-Coordinate of ship in tile
int shipCoordinateY = 0;   // Y-Coordinate of ship in tile

// Converts Position to tile coordinates
void latlon_to_tile(double lat, double lon, int zoom, int *x_tile, int *y_tile)
{
    int n = 1 << zoom;
    *x_tile = (int)((lon + 180.0) / 360.0 * n);
    *y_tile = (int)((1.0 - log(tan(lat * M_PI / 180.0) + 1 / cos(lat * M_PI / 180.0)) / M_PI) / 2.0 * n);
}

bool new_tiles_for_position_needed(double oldLatitude, double oldLongitude, int oldZoom, double latitude, double longitude, int zoom)
{
    int oldX;
    int oldY;
    latlon_to_tile(oldLatitude, oldLongitude, oldZoom, &oldX, &oldY);

    int newX;
    int newY;
    latlon_to_tile(latitude, longitude, zoom, &newX, &newY);

    return (oldX != newX) || (oldY != newY);
}

// Function to calculate pixel coordinates
void get_pixel_coordinates(double latitude, double longitude, int zoom, int *x_pixel, int *y_pixel)
{
    // Convert latitude and longitude to radians
    double lat_rad = latitude * M_PI / 180.0;

    // Calculate the x and y tile coordinates
    double n = pow(2.0, zoom);
    double x_tile = (longitude + 180.0) / 360.0 * n;
    double y_tile = (1.0 - log(tan(lat_rad) + 1.0 / cos(lat_rad)) / M_PI) / 2.0 * n;

    // Calculate the pixel coordinates within the tile
    *x_pixel = (int)fmod(x_tile * TILE_SIZE, TILE_SIZE);
    *y_pixel = (int)fmod(y_tile * TILE_SIZE, TILE_SIZE);
}

// Adds a shipmarker to given parent
void add_ship_marker(lv_obj_t *parent)
{
    if (shipMarker == NULL)
    {
        shipMarker = lv_img_create(parent);
        lv_img_set_src(shipMarker, &smallBoat);
    }

    lv_obj_set_pos(shipMarker, shipCoordinateX - (smallBoat.header.w / 2), shipCoordinateY - (smallBoat.header.h / 2));
}

// Gets called when every pixel of a tile was converted. Creates an image object and render it on screen
void on_finished(pngle_t *pngle)
{
    int i = currentTileRow * TILES_PER_COLUMN + currentTileColumn;
    pngle_ihdr_t pngle_header = *pngle_get_ihdr(pngle);

    if (img_widgets[i] == NULL) // Initial stuff to create image
    {
        // Create an image widget
        img_widgets[i] = lv_img_create(lv_scr_act());

        // Initialize the image descriptor
        img_descs[i].header.always_zero = 0;
        img_descs[i].header.w = pngle_header.width;
        img_descs[i].header.h = pngle_header.height;
        img_descs[i].header.cf = LV_IMG_CF_TRUE_COLOR;
        img_descs[i].data_size = pngle_header.width * pngle_header.height * sizeof(lv_color_t);
        img_descs[i].data = (uint8_t *)image_buffers[i];

        // Set the image source to the widget
        lv_img_set_src(img_widgets[i], &img_descs[i]);

        // Position the widget on the screen
        lv_coord_t x = currentTileColumn * pngle_header.width;
        lv_coord_t y = currentTileRow * pngle_header.height;
        lv_obj_set_pos(img_widgets[i], x, y);

        ESP_LOGI("TileDownloader", "Image finished %d/%d. Displaying it at %d/%d", currentTileColumn, currentTileRow, x, y);
    }
    else
    {
        img_descs[i].data = (uint8_t *)image_buffers[i];
        ESP_LOGI("TileDownloader", "Image finished %d/%d. Updating it", currentTileColumn, currentTileRow);
    }

    if (currentTileColumn == 1 && currentTileRow == 0)
    {
        add_ship_marker(img_widgets[i]);
    }

    // Move to background so that labels, buttons, etc are in front
    lv_obj_move_background(img_widgets[i]);
    // Update screen
    lv_obj_invalidate(img_widgets[i]);
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

    int i = currentTileRow * TILES_PER_COLUMN + currentTileColumn;
    image_buffers[i][pixel_index] = color;
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

void setup_tile_downloader()
{
    // instantiate PNGLE and set callbacks
    pngle_handle = pngle_new();
    pngle_set_done_callback(pngle_handle, on_finished);
    pngle_set_draw_callback(pngle_handle, on_draw);

    // instantiate buffers
    for (int i = 0; i < TILES_COUNT; i++)
    {
        image_buffers[i] = (lv_color_t *)heap_caps_malloc(TILE_PIXELS * sizeof(lv_color_t), MALLOC_CAP_SPIRAM); // Buffer for one tile
        if (image_buffers[i] == NULL)
        {
            ESP_LOGE("TileDownloader", "Failed to allocate memory for tile in PSRAM");
            while (1)
            {
            }
        }
    }
}

void download_and_display_image(double latitude, double longitude, int zoom)
{
    // Get tile coordinates
    int baseX;
    int baseY;
    latlon_to_tile(latitude, longitude, zoom, &baseX, &baseY);

    // Get coordinates to put ship marker on tile
    get_pixel_coordinates(latitude, longitude, zoom, &shipCoordinateX, &shipCoordinateY);

    // Download and draw tiles
    for (currentTileRow = 0; currentTileRow < TILES_PER_ROW; currentTileRow++)
    {
        for (currentTileColumn = 0; currentTileColumn < TILES_PER_COLUMN; currentTileColumn++)
        {
            int xTile = baseX + currentTileColumn - 1; // -1 so that the current position is in the middle
            int yTile = baseY + currentTileRow;

            if (download_tile(xTile, yTile, zoom, httpData) == ESP_OK)
            {
                int fed = pngle_feed(pngle_handle, httpData, TILE_PIXELS);
                if (fed < 0)
                {
                    ESP_LOGI("TileDownloader", "PNGLE_Error: %s", pngle_error(pngle_handle));
                }
                pngle_reset(pngle_handle);
            }
            else
            {
                ESP_LOGE("TileDownloader", "Problem when download tile %d/%d", xTile, yTile);
            }
        }
    }
}
