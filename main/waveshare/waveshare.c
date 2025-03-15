/*
 * Copyright (c) 2025 Boyd Timothy. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "waveshare.h"

// #include "esp_lcd_touch/esp_lcd_touch.h"
#include <esp_lcd_touch.h>
#include "esp_lvgl_port.h"
#include "ST7789.h"

static const char *TAG = "Waveshare";

lv_display_t *lvgl_display = NULL;

esp_lcd_panel_io_handle_t lcd_io = NULL;
esp_lcd_panel_handle_t lcd_panel = NULL;
esp_lcd_touch_handle_t tp = NULL;
lv_indev_t *touch_device = NULL;

void lvgl_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map);

esp_err_t waveshare_lcd_init() {
    LCD_Init();

    return ESP_OK;
}

esp_err_t waveshare_lvgl_init() {
    LVGL_Init();

    return ESP_OK;
}

esp_err_t waveshare_touch_init() {
    TOUCH_Init();

    lvgl_port_touch_cfg_t touch_cfg = {
        .disp = lvgl_display,
        .handle = tp
    };
    touch_device = lvgl_port_add_touch(&touch_cfg);
    return touch_device == NULL ? ESP_FAIL : ESP_OK;
}

esp_err_t lcd_display_brightness_set(uint8_t brightness) {
    if (brightness > Backlight_MAX || brightness < 0) {
      ESP_LOGI(TAG, "Set Backlight parameters in the range of 0 to 100 ");
      return ESP_FAIL;
    }
    Set_Backlight(brightness);
  
    return ESP_OK;
  }
  
esp_err_t lcd_display_rotate(lv_display_t * lvgl_disp, lv_display_rotation_t dir) {
    if (lvgl_disp) {
        lv_display_set_rotation(lvgl_disp, dir);
        return ESP_OK;
    }

    return ESP_FAIL;
}