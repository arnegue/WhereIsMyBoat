#include "wifi_ui.h"

#include "lvgl.h"
#include "esp_log.h"
#include "wifi.h"

static const char *LOG_TAG = "WIFI_UI";

static lv_obj_t *lv_wifi_list = NULL;
static lv_obj_t *lv_wifi_container = NULL;
static lv_obj_t *lv_wifi_abort_btn = NULL;

// Clears wifi setup menu
void clear_wifi_setup()
{
    if (lv_wifi_list)
    {
        lv_obj_del(lv_wifi_list);
        lv_wifi_list = NULL;
    }
    if (lv_wifi_container)
    {
        lv_obj_del(lv_wifi_container);
        lv_wifi_container = NULL;
    }
    if (lv_wifi_abort_btn)
    {
        lv_obj_del(lv_wifi_abort_btn);
        lv_wifi_abort_btn = NULL;
    }
}

// Gets called if user aborts wifi connect
void wifi_abort_callback(lv_event_t *)
{
    clear_wifi_setup();
    ESP_LOGI(LOG_TAG, "wifi_abort_callback!");
}

// Event handler for keyboard
static void keyboard_event_handler(lv_event_t *e)
{
    const lv_obj_t *keyboard = lv_event_get_target(e);
    const lv_obj_t *textarea = lv_keyboard_get_textarea(keyboard);

    const char *selected_ssid = (char *)lv_event_get_user_data(e);
    if (lv_event_get_code(e) == LV_EVENT_READY)
    {
        // The "OK" key was pressed
        const char *password = lv_textarea_get_text(textarea);
        ESP_LOGI(LOG_TAG, "User wants to connect to '%s' using password: '%s'", selected_ssid, password);

        wifi_config_t wifi_config = {};
        strncpy((char *)wifi_config.sta.ssid, selected_ssid, sizeof(wifi_config.sta.ssid));
        strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

        esp_err_t error = wifi_connect(&wifi_config);
        if (error == ESP_OK)
        {
            clear_wifi_setup();
        }
        else
        { // TODO set some error message?
        }
    }
}

// Event handler to show/hide wifi password text
static void toggle_password_visibility(lv_event_t *e)
{
    const lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *textarea = lv_event_get_user_data(e);

    // Check current password mode and toggle it
    bool current_mode = lv_textarea_get_password_mode(textarea);
    lv_textarea_set_password_mode(textarea, !current_mode);

    // Update button label
    lv_label_set_text(lv_obj_get_child(btn, 0), current_mode ? "Hide password" : "Show password");
}

void create_password_input(const char *selected_ssid)
{
    // Create a container for the password input and keyboard
    lv_wifi_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(lv_wifi_container, lv_pct(100), lv_pct(100));
    lv_obj_set_flex_flow(lv_wifi_container, LV_FLEX_FLOW_COLUMN);

    // Create a password input field (and activate cursor)
    lv_obj_t *password_input = lv_textarea_create(lv_wifi_container);
    lv_textarea_set_cursor_click_pos(password_input, true);
    lv_obj_add_state(password_input, LV_STATE_FOCUSED);
    lv_obj_set_width(password_input, lv_pct(80));
    lv_textarea_set_password_mode(password_input, true);
    lv_textarea_set_placeholder_text(password_input, "Enter password");

    // Create a "Show/Hide Password" toggle button
    lv_obj_t *show_button = lv_btn_create(lv_wifi_container);
    lv_obj_t *show_label = lv_label_create(show_button);
    lv_label_set_text(show_label, "Show password");
    lv_obj_add_event_cb(show_button, toggle_password_visibility, LV_EVENT_CLICKED, password_input);

    // Add abort button
    lv_obj_t *abort_btn = lv_btn_create(lv_wifi_container);
    lv_obj_t *abort_label = lv_label_create(abort_btn);
    lv_label_set_text(abort_label, LV_SYMBOL_CLOSE);
    lv_obj_add_event_cb(abort_btn, wifi_abort_callback, LV_EVENT_CLICKED, NULL);

    // Create a keyboard
    lv_obj_t *keyboard = lv_keyboard_create(lv_wifi_container);
    lv_keyboard_set_textarea(keyboard, password_input);
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);

    // Attach the event callback to the keyboard
    lv_obj_add_event_cb(keyboard, keyboard_event_handler, LV_EVENT_ALL, (void *)selected_ssid);

    // Adjust the layout
    lv_obj_add_flag(lv_wifi_container, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_flex_grow(password_input, 1);
    lv_obj_align(lv_wifi_container, LV_ALIGN_CENTER, 0, 0);
}

void wifi_select_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    const char *selected_ssid = lv_list_get_btn_text(lv_wifi_list, btn);

    ESP_LOGI(LOG_TAG, "Selected WiFi: %s", selected_ssid);

    // Show password input field and proceed
    create_password_input(selected_ssid);
}

// Checks if given SSID was already added to list
bool ssid_in_list(const char *ssid)
{
    for (int idx = 0; idx < lv_obj_get_child_cnt(lv_wifi_list); idx++)
    {
        lv_obj_t *child = lv_obj_get_child(lv_wifi_list, idx); // Get the first child

        // Check if the child is a button (list item)
        if (lv_obj_check_type(child, &lv_list_btn_class))
        {
            const char *btn_text = lv_list_get_btn_text(lv_wifi_list, child);
            ESP_LOGI(LOG_TAG, "List item text: %s", btn_text);
            if (strcmp(ssid, btn_text) == 0)
            {
                ESP_LOGI(LOG_TAG, "SSID %s already in list", btn_text);
                return true;
            }
        }
    }
    return false;
}

void wifi_setup_button_callback(lv_event_t *e)
{
    ESP_LOGI(LOG_TAG, "wifi_setup_button_callback");
    uint16_t amountNetworks = 0;
    wifi_ap_record_t *wifi_list = NULL;
    wifi_scan_networks(&amountNetworks, &wifi_list);

    lv_wifi_list = lv_list_create(lv_scr_act());

    lv_obj_set_size(lv_wifi_list, lv_pct(50), lv_pct(100));

    // Create an abort-button
    lv_wifi_abort_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_pos(lv_wifi_abort_btn, 348, 2);
    lv_obj_set_size(lv_wifi_abort_btn, 50, 50);
    lv_obj_add_event_cb(lv_wifi_abort_btn, wifi_abort_callback, LV_EVENT_CLICKED, NULL);
    lv_obj_t *label = lv_label_create(lv_wifi_abort_btn);
    lv_label_set_text(label, LV_SYMBOL_CLOSE);
    lv_obj_center(label);

    for (int i = 0; i < amountNetworks; i++)
    {
        const char *newSSID = (const char *)wifi_list[i].ssid;
        if (!ssid_in_list(newSSID))
        {
            ESP_LOGI(LOG_TAG, "Adding SSID %s to list", newSSID);
            lv_obj_t *btn = lv_list_add_btn(lv_wifi_list, LV_SYMBOL_WIFI, newSSID);
            lv_obj_add_event_cb(btn, wifi_select_event_cb, LV_EVENT_CLICKED, NULL);
        }
    }
}
