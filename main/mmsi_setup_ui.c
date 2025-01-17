#include "mmsi_setup_ui.h"

#include "esp_log.h"
#include "wifi.h"
#include "nvs_wrapper.h"
#include "aisstream.h"
#include "global.h"

static const char *LOG_TAG = "MMSI_SETUP_UI";
static lv_obj_t *keyboard = NULL;
static lv_obj_t *container = NULL;

static void close_ui()
{
    if (keyboard != NULL)
    {
        lv_obj_del(keyboard);
    }
    if (container != NULL)
    {
        lv_obj_del(container);
    }
}

// Event handler for keyboard
static void keyboard_event_handler(lv_event_t *e)
{
    const lv_obj_t *textarea = lv_keyboard_get_textarea(keyboard);

    if (lv_event_get_code(e) == LV_EVENT_READY)
    {
        // The "OK" key was pressed
        const char *mmsi = lv_textarea_get_text(textarea);
        ESP_LOGI(LOG_TAG, "User wants to set MMSI to '%s'", mmsi);

        size_t mmsi_len = strlen(mmsi);
        const size_t expected_len = MMSI_LENGTH - 1; // minus null terminator
        if (mmsi_len != expected_len)
        {
            ESP_LOGE(LOG_TAG, "MMSI too short/long. Expected: %d, Actual: %d", expected_len, mmsi_len);
            show_error_message("Wrong length of MMSI");
            return;
        }

        // Set nvs
        esp_err_t store_error = store_mmsi(mmsi);
        if (store_error != ESP_OK)
        {
            ESP_LOGE(LOG_TAG, "Failed to store MMSI to NVS: '%s': %s", mmsi, esp_err_to_name(store_error));
            show_error_message("Failed to store MMSI to NVS");
            return;
        }

        // notify aisstream
        set_mmsi(mmsi);

        // Close container
        close_ui();
    }
}

// Gets called if user wants to abort mmsi-setup
void mmsi_setup_abort_button_callback(lv_event_t *e)
{
    close_ui();
}

void mmsi_setup_button_callback(lv_event_t *e)
{
    char currentMMSI[MMSI_LENGTH];
    get_last_stored_mmsi(currentMMSI);
    lv_obj_t *scr = lv_scr_act(); // Get the current screen

    // Create a container
    container = lv_obj_create(scr);
    lv_obj_set_size(container, 300, 200);             // Set the size of the container
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 20); // Align the container at the top center with a margin of 20px

    // Add a label to the container
    lv_obj_t *label = lv_label_create(container);
    lv_label_set_text(label, "Enter a number:");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10); // Align the label at the top of the container

    // Add a text area to the container
    lv_obj_t *text_area = lv_textarea_create(container);
    lv_textarea_set_cursor_click_pos(text_area, true);
    lv_textarea_set_text(text_area, currentMMSI);
    lv_obj_add_state(text_area, LV_STATE_FOCUSED);
    lv_obj_set_width(text_area, 250);               // Set the width of the text area
    lv_obj_align(text_area, LV_ALIGN_CENTER, 0, 0); // Align the text area in the center of the container
    lv_textarea_set_cursor_click_pos(text_area, true);

    // Create a keyboard and set it to only accept digits
    keyboard = lv_keyboard_create(scr);
    lv_keyboard_set_textarea(keyboard, text_area);                       // Link the keyboard to the text area
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_NUMBER);             // Set keyboard to number mode
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);                   // Position the keyboard at the bottom of the screen
    lv_obj_add_event_cb(keyboard, keyboard_event_handler, LV_EVENT_ALL, NULL); // Attach the event callback to the keyboard

    // Add a close button to the container
    lv_obj_t *abort_btn = lv_btn_create(container);
    lv_obj_set_size(abort_btn, 70, 30);
    lv_obj_align(abort_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_t *close_label = lv_label_create(abort_btn);
    lv_label_set_text(close_label, "Abort");
    lv_obj_center(close_label);
    lv_obj_add_event_cb(abort_btn, mmsi_setup_abort_button_callback, LV_EVENT_CLICKED, NULL);
}
