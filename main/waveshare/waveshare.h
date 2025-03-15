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
#pragma once

#include "esp_err.h"
#include "ST7789.h"
#include "LVGL_Driver.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t waveshare_lcd_init();
esp_err_t waveshare_lvgl_init();
esp_err_t waveshare_touch_init();

esp_err_t lcd_display_rotate(lv_display_t * lvgl_disp, lv_display_rotation_t dir);
esp_err_t lcd_display_brightness_set(uint8_t brightness);


#ifdef __cplusplus
}
#endif