#ifndef TILE_DOWNLOADER_H_
#define TILE_DOWNLOADER_H_

#include "esp_lcd_types.h"

// Sets up downloader and png-converter
void setup_tile_downloader(esp_lcd_panel_handle_t display_handle);

// Checks if a new tile download has to be started
bool new_tiles_for_position_needed(double oldLatitude, double oldLongitude, int oldZoom, double latitude, double longitude, int zoom);

// Downloads tiles and displays them
void download_and_display_image(double latitude, double longitude, int zoom);

#endif