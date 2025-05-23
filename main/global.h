#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <math.h>
#include <stdbool.h>

#include "lvgl.h"

#define MMSI_LENGTH (9 + 1) // 9 chars + 1 null terminator

// The pixel number in horizontal and vertical
#define LCD_H_RES 800
#define LCD_V_RES 480

// Shows an error popup with given message and a close button
lv_obj_t *show_error_message(const char *message);

inline bool AreEqual(double a, double b)
{
    return fabs(a - b) < 0.00001;
}

#endif // GLOBAL_H_
