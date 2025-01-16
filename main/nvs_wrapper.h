#ifndef NVS_WRAPPER_H_
#define NVS_WRAPPER_H_

#include "esp_err.h"

#include "config.h"

// Initializes NVS
esp_err_t init_nvs();

// Returns last store position
esp_err_t get_last_stored_position(double *latitude, double *longitude);

// Stores both positions in NVS
esp_err_t store_position(const double latitude, const double longitude);

// Returns last store MMSI
esp_err_t get_last_stored_mmsi(char mmsi[MMSI_LENGTH]);

// Stores MMSI in NVS
esp_err_t store_mmsi(const char mmsi[MMSI_LENGTH]);

#endif // NVS_WRAPPER_H_
