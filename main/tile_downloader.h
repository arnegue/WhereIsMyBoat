#ifndef TILE_DOWNLOADER_H_
#define TILE_DOWNLOADER_H_

#include "esp_lcd_types.h"
#include "esp_err.h"

// Sets up downloader and png-converter
esp_err_t setup_tile_downloader();

// Checks if a new tile download has to be started
bool new_tiles_for_position_needed(const double oldLatitude, const double oldLongitude, const int oldZoom, const double latitude, const double longitude, const int zoom);

// Downloads tiles and displays them
esp_err_t download_and_display_image(const double latitude, const double longitude, const int zoom);

// Manually call this to update the ship marker of the middle tile (usually needed if position changed but no new tiles are needed [e.g. new_tiles_for_position_needed returns false])
void update_ship_marker(const double latitude, const double longitude, const int zoom);

#endif
