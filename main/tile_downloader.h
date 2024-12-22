#ifndef TILE_DOWNLOADER_H_
#define TILE_DOWNLOADER_H_

#include "esp_lcd_types.h"

void setup_tile_downloader(esp_lcd_panel_handle_t display_handle);
void download_image(double latitude, double longitude, int zoom);

#endif