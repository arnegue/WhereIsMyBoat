#ifndef NVS_POSITION_H_
#define NVS_POSITION_H_

#include "esp_err.h"

// Initializes NVS
esp_err_t init_nvs();

// Returns last store position
esp_err_t get_last_stored_position(double *latitude, double *longitude);

// Stores both positions in nsv
esp_err_t store_position(double latitude, double longitude);

#endif // NVS_POSITION_H_
