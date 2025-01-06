#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "nvs_position.h"

static const char *LOG_TAG = "NVS";

esp_err_t init_nvs()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    return err;
}

// Returns last store position
esp_err_t get_last_stored_position(double *latitude, double *longitude)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open NVS handle
    err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(LOG_TAG, "Error opening NVS handle!");
        return err;
    }

    // Read latitude
    size_t size = sizeof(double);
    err = nvs_get_blob(my_handle, "latitude", latitude, &size);
    if (err != ESP_OK)
    {
        ESP_LOGE(LOG_TAG, "Error reading latitude!");
        nvs_close(my_handle);
        return err;
    }

    // Read longitude
    err = nvs_get_blob(my_handle, "longitude", longitude, &size);
    if (err != ESP_OK)
    {
        ESP_LOGE(LOG_TAG, "Error reading longitude!");
        nvs_close(my_handle);
        return err;
    }

    // Close NVS handle
    nvs_close(my_handle);
    return ESP_OK;
}

// Stores both positions in nsv
esp_err_t store_position(double latitude, double longitude)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open NVS handle
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(LOG_TAG, "Error opening NVS handle!");
        return err;
    }

    // Write latitude
    err = nvs_set_blob(my_handle, "latitude", &latitude, sizeof(latitude));
    if (err != ESP_OK)
    {
        ESP_LOGE(LOG_TAG, "Error writing latitude!");
        return err;
    }

    // Write longitude
    err = nvs_set_blob(my_handle, "longitude", &longitude, sizeof(longitude));
    if (err != ESP_OK)
    {
        ESP_LOGE(LOG_TAG, "Error writing longitude!");
        return err;
    }

    // Commit changes
    err = nvs_commit(my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(LOG_TAG, "Error committing changes!");
        return err;
    }

    // Close NVS handle
    nvs_close(my_handle);
    return ESP_OK;
}
