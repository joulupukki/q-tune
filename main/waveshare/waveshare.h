#pragma once

#include "esp_err.h"
#include "ST7789.h"
#include "LVGL_Driver.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t waveshare_lcd_init();
esp_err_t waveshare_lvgl_init();

esp_err_t lcd_display_rotate(lv_display_t * lvgl_disp, lv_display_rotation_t dir);
esp_err_t lcd_display_brightness_set(uint8_t brightness);


#ifdef __cplusplus
}
#endif