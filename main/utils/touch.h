#pragma once

#include <esp_err.h>
#include <esp_lcd_touch.h>

#include "lvgl.h"

esp_err_t touch_init(esp_lcd_touch_handle_t *tp, lv_display_t *disp);
void calibrate_touch_screen(esp_lcd_touch_handle_t tp);