#include "global.h"

#include "lvgl.h"

// Close button for error message
void close_button_callback(lv_event_t *e)
{
    lv_obj_t *container = (lv_obj_t *)lv_event_get_user_data(e);
    lv_obj_del(container);
}

void show_error_message(const char *message)
{
    // Create a container for the popup
    lv_obj_t *popup = lv_obj_create(lv_scr_act());
    lv_obj_set_size(popup, 300, 200); // Adjust size as needed
    lv_obj_center(popup);
    lv_obj_set_style_pad_all(popup, 10, 0);
    lv_obj_set_style_bg_color(popup, lv_color_hex(0xFFCCCC), 0); // Light red background for error
    lv_obj_set_style_border_width(popup, 2, 0);
    lv_obj_set_style_border_color(popup, lv_color_hex(0xFF0000), 0); // Red border

    // Add a warning label at the top-right corner
    lv_obj_t *warning_label = lv_label_create(popup);
    lv_label_set_text(warning_label, LV_SYMBOL_WARNING " Error");
    lv_obj_align(warning_label, LV_ALIGN_TOP_RIGHT, -10, 10);

    // Add the message label in the center with automatic wrapping
    lv_obj_t *message_label = lv_label_create(popup);
    lv_label_set_text(message_label, message);
    lv_label_set_long_mode(message_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(message_label, lv_pct(90)); // Set width to 90% of the popup for wrapping
    lv_obj_center(message_label);

    // Add a close button at the bottom center
    lv_obj_t *close_btn = lv_btn_create(popup);
    lv_obj_set_size(close_btn, 100, 40); // Adjust size as needed
    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, -10);

    // Add a label to the button
    lv_obj_t *btn_label = lv_label_create(close_btn);
    lv_label_set_text(btn_label, "Close");
    lv_obj_center(btn_label);

    lv_obj_add_event_cb(close_btn, close_button_callback, LV_EVENT_CLICKED, (void *)popup);
}
