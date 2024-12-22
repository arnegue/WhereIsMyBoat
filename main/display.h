#ifndef DISPLAY_H_
#define DISPLAY_H_
#include "esp_lcd_types.h"

esp_lcd_panel_handle_t init_display();

void display_image();

void update_display();

#endif // DISPLAY_H_
