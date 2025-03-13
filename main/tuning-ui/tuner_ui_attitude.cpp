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
#include "tuner_ui_attitude.h"

#include <stdlib.h>

#include "globals.h"
#include "user_settings.h"

#include "esp_log.h"
#include "esp_lvgl_port.h"

#define ATTITUDE_INDICATOR_LINE_HEIGHT 14
#define ATTITUDE_INDICATOR_BORDER_WIDTH 5
#define ATTITUDE_INDICATOR_LINE_WIDTH 100
// #define ATTITUDE_ARROWS_SIZE 100
// #define ATTITUDE_ARROWS_PIVOT_LOCATION 50
#define ATTITUDE_ARROWS_SIZE 100
#define ATTITUDE_ARROWS_PIVOT_LOCATION 50
#define ATTITUDE_RULER_LINE_WIDTH 4
#define ATTITUDE_PORTRAIT_VERTICAL_OFFSET -30
#define ATTITUDE_LANDSCAPE_VERTICAL_OFFSET -10
// #define ATTITUDE_GROUND_COLOR 0x8B4513
#define ATTITUDE_GROUND_COLOR 0x000000
#define ATTITUDE_SKY_COLOR 0x87CEEB
#define ATTITUDE_RESOLUTION_FACTOR 2.5 // Move 2.5 pixels for every cent of change

static const char *TAG = "ATTITUDE_UI";

extern UserSettings *userSettings;
extern lv_coord_t screen_width;
extern lv_coord_t screen_height;

LV_IMG_DECLARE(tuner_font_image_a)
LV_IMG_DECLARE(tuner_font_image_b)
LV_IMG_DECLARE(tuner_font_image_c)
LV_IMG_DECLARE(tuner_font_image_d)
LV_IMG_DECLARE(tuner_font_image_e)
LV_IMG_DECLARE(tuner_font_image_f)
LV_IMG_DECLARE(tuner_font_image_g)
LV_IMG_DECLARE(tuner_font_image_none)
LV_IMG_DECLARE(tuner_font_image_sharp)

//
// Function Definitions
//
void attitude_create_slider(lv_obj_t * parent);
void attitude_create_slider_ruler(lv_obj_t *parent);
void attitude_create_labels(lv_obj_t * parent);
void attitude_create_arrows(lv_obj_t * parent);
void attitude_update_note_name(TunerNoteName new_value);
void attitude_start_note_fade_animation();
void attitude_stop_note_fade_animation();
void attitude_last_note_anim_cb(lv_obj_t *obj, int32_t value);
void attitude_last_note_anim_completed_cb(lv_anim_t *);

//
// Local Variables
//
lv_obj_t *attitude_parent_screen = NULL;

// Keep track of the last note that was displayed so telling the UI to update
// can be avoided if it is the same.
TunerNoteName attitude_last_displayed_note = NOTE_NONE;

lv_obj_t *attitude_slider;

lv_obj_t *attitude_note_img_container;
lv_obj_t *attitude_note_img;
lv_obj_t *attitude_sharp_img;

lv_obj_t *attitude_left_tick;
lv_obj_t *attitude_right_tick;

lv_obj_t *attitude_mute_label;
lv_obj_t *attitude_frequency_label;
lv_style_t attitude_frequency_label_style;
lv_obj_t *attitude_cents_label;
lv_style_t attitude_cents_label_style;
bool attitude_is_landscape = false;

float attitude_rotation_current_pos = 0;

lv_anim_t *attitude_last_note_anim = NULL;

uint8_t attitude_gui_get_id() {
    return 2;
}

const char * attitude_gui_get_name() {
    return "Attitude";
}

void attitude_gui_init(lv_obj_t *screen) {
    switch (userSettings->displayOrientation) {
        case orientationLeft:
        case orientationRight:
            attitude_is_landscape = true;
            break;
        default:
            attitude_is_landscape = false;
            break;
    }
    attitude_parent_screen = screen;
    attitude_create_slider(screen);
    attitude_create_arrows(screen);
    attitude_create_labels(screen);
}

void attitude_gui_display_frequency(float frequency, TunerNoteName note_name, float cents, bool show_mute_indicator) {
    if (note_name < 0) { return; } // Strangely I'm sometimes seeing negative values. No idea how.
    if (note_name != NOTE_NONE) {
        lv_label_set_text_fmt(attitude_frequency_label, "%.2f", frequency);
        lv_obj_clear_flag(attitude_frequency_label, LV_OBJ_FLAG_HIDDEN);

        if (attitude_last_displayed_note != note_name) {
            attitude_update_note_name(note_name);
            attitude_last_displayed_note = note_name; // prevent setting this so often to help prevent an LVGL crash
        }

        lv_label_set_text_fmt(attitude_cents_label, "%.1f", cents);
        lv_obj_clear_flag(attitude_cents_label, LV_OBJ_FLAG_HIDDEN);

        if (abs(cents) <= userSettings->inTuneCentsWidth) {
            // When the tuning is within the threshold, show the left and right
            // triangles in orange.
            lv_obj_set_style_bg_color(attitude_left_tick, lv_palette_main(LV_PALETTE_ORANGE), 0);
            lv_obj_set_style_bg_color(attitude_right_tick, lv_palette_main(LV_PALETTE_ORANGE), 0);
        } else {
            lv_obj_set_style_bg_color(attitude_left_tick, lv_color_white(), 0);
            lv_obj_set_style_bg_color(attitude_right_tick, lv_color_white(), 0);
        }

        // Set the vertical position of the slider to indicate the horizon moving
        lv_obj_align(
            attitude_slider,
            LV_ALIGN_CENTER,
            0,
            (attitude_is_landscape ? ATTITUDE_LANDSCAPE_VERTICAL_OFFSET : ATTITUDE_PORTRAIT_VERTICAL_OFFSET) + cents * ATTITUDE_RESOLUTION_FACTOR * -1);
    } else {
        // Hide the pitch and indicators since it's not detected
        if (attitude_last_displayed_note != NOTE_NONE) {
            attitude_update_note_name(NOTE_NONE);
            attitude_last_displayed_note = NOTE_NONE;
        }

        // Hide the frequency, and cents labels
        lv_obj_add_flag(attitude_cents_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(attitude_frequency_label, LV_OBJ_FLAG_HIDDEN);
    }

    if (show_mute_indicator) {
        lv_obj_clear_flag(attitude_mute_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(attitude_mute_label, LV_OBJ_FLAG_HIDDEN);
    }
}

void attitude_gui_cleanup() {
    // TODO: Do any cleanup needed here. 
}

void attitude_create_slider(lv_obj_t * parent) {
    // Put some sky and ground behind the slider so black isn't exposed when
    // the slider is moved.
    lv_obj_t *background_sky = lv_obj_create(parent);
    lv_obj_set_size(background_sky, lv_pct(100), lv_pct(50));
    lv_obj_set_style_bg_color(background_sky, lv_color_hex(ATTITUDE_SKY_COLOR), 0);
    lv_obj_set_style_radius(background_sky, 0, 0);
    lv_obj_align(background_sky, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *background_ground = lv_obj_create(parent);
    lv_obj_set_size(background_ground, lv_pct(100), lv_pct(50));
    lv_obj_set_style_bg_color(background_ground, lv_color_hex(ATTITUDE_GROUND_COLOR), 0);
    lv_obj_set_style_radius(background_ground, 0, 0);
    lv_obj_set_style_border_opa(background_ground, 0, 0);
    lv_obj_align(background_ground, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    // Create the container for the slider
    attitude_slider = lv_obj_create(parent);
    lv_obj_set_size(attitude_slider, lv_pct(100), lv_pct(100));
    lv_obj_set_style_pad_all(attitude_slider, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(attitude_slider, 0, 0);
    lv_obj_set_scrollbar_mode(attitude_slider, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(attitude_slider, lv_color_white(), 0);
    lv_obj_align(attitude_slider, LV_ALIGN_CENTER, 0, attitude_is_landscape ? ATTITUDE_LANDSCAPE_VERTICAL_OFFSET : ATTITUDE_PORTRAIT_VERTICAL_OFFSET);

    // Create the top box that represents the sky
    lv_obj_t *sky = lv_obj_create(attitude_slider);
    lv_obj_set_style_border_width(sky, 0, 0);
    lv_obj_set_size(sky, lv_pct(100), lv_pct(50));
    lv_obj_set_style_bg_color(sky, lv_color_hex(ATTITUDE_SKY_COLOR), 0);
    lv_obj_set_style_radius(background_sky, 0, 0);
    lv_obj_align(sky, LV_ALIGN_TOP_LEFT, 0, -2); // Offset by 2 pixels to get the slider white background to show through and look like the horizon line

    // Create the bottom box that represents the ground
    lv_obj_t *ground = lv_obj_create(attitude_slider);
    lv_obj_set_style_border_width(ground, 0, 0);
    lv_obj_set_size(ground, lv_pct(100), lv_pct(50));
    lv_obj_set_style_bg_color(ground, lv_color_hex(ATTITUDE_GROUND_COLOR), 0); // brownish
    lv_obj_set_style_radius(ground, 0, 0);
    // lv_obj_set_style_bg_color(ground, lv_color_black(), 0);
    lv_obj_align(ground, LV_ALIGN_BOTTOM_LEFT, 0, 2); // Offset by 2 pixels to get the slider white background to show through

    attitude_create_slider_ruler(attitude_slider);
}

void attitude_create_slider_ruler(lv_obj_t *parent) {
    for (int i = 10; i < 50; i += 10) {
        bool is_label_line = i % 20 == 0;
        int line_width = (is_label_line ? 20 + i : 20);

        // Top line
        lv_obj_t *line = lv_obj_create(parent);
        lv_obj_set_size(line, line_width, ATTITUDE_RULER_LINE_WIDTH);
        lv_obj_set_style_bg_color(line, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(line, LV_OPA_50, 0);
        lv_obj_set_style_border_width(line, 0, 0);
        lv_obj_align(line, LV_ALIGN_CENTER, 0, i * -1 * ATTITUDE_RESOLUTION_FACTOR);

        lv_obj_t *label; 
        if (is_label_line) {
            label = lv_label_create(parent);
            lv_label_set_text_fmt(label, "%d", i);
            lv_obj_set_style_opa(label, LV_OPA_50, 0);
            lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
            lv_obj_align_to(label, line, LV_ALIGN_OUT_LEFT_MID, -10, 0);

            label = lv_label_create(parent);
            lv_label_set_text_fmt(label, "%d", i);
            lv_obj_set_style_opa(label, LV_OPA_50, 0);
            lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
            lv_obj_align_to(label, line, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
        }

        // Bottom line
        line = lv_obj_create(parent);
        lv_obj_set_size(line, line_width, ATTITUDE_RULER_LINE_WIDTH);
        lv_obj_set_style_bg_color(line, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(line, LV_OPA_50, 0);
        lv_obj_set_style_border_width(line, 0, 0);
        lv_obj_align(line, LV_ALIGN_CENTER, 0, i * ATTITUDE_RESOLUTION_FACTOR);

        if (is_label_line) {
            label = lv_label_create(parent);
            lv_label_set_text_fmt(label, "%d", i);
            lv_obj_set_style_opa(label, LV_OPA_50, 0);
            lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
            lv_obj_align_to(label, line, LV_ALIGN_OUT_LEFT_MID, -10, 0);

            label = lv_label_create(parent);
            lv_label_set_text_fmt(label, "%d", i);
            lv_obj_set_style_opa(label, LV_OPA_50, 0);
            lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
            lv_obj_align_to(label, line, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
        }
    }
}

void attitude_create_labels(lv_obj_t * parent) {
    // Place the note name and # symbole in the same container so the opacity
    // can be animated.
    attitude_note_img_container = lv_obj_create(parent);
    lv_obj_set_size(attitude_note_img_container, lv_pct(100), lv_pct(50));
    lv_obj_set_style_bg_opa(attitude_note_img_container, LV_OPA_0, 0);
    lv_obj_set_style_border_width(attitude_note_img_container, 0, 0);
    lv_obj_set_scrollbar_mode(attitude_note_img_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_align(attitude_note_img_container, LV_ALIGN_BOTTOM_MID, 0, attitude_is_landscape ? ATTITUDE_LANDSCAPE_VERTICAL_OFFSET : ATTITUDE_PORTRAIT_VERTICAL_OFFSET);

    // Note Name Image (the big name in the middle of the screen)
    attitude_note_img = lv_image_create(attitude_note_img_container);
    lv_image_set_src(attitude_note_img, &tuner_font_image_none);
    lv_obj_center(attitude_note_img);

    attitude_sharp_img = lv_image_create(attitude_note_img_container);
    lv_image_set_src(attitude_sharp_img, &tuner_font_image_sharp);
    lv_obj_align_to(attitude_sharp_img, attitude_note_img, LV_ALIGN_TOP_RIGHT, 40, -30);
    lv_obj_add_flag(attitude_sharp_img, LV_OBJ_FLAG_HIDDEN);
    
    // Enable recoloring on the images
    lv_obj_set_style_img_recolor_opa(attitude_note_img, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(attitude_sharp_img, LV_OPA_COVER, LV_PART_MAIN);
    lv_palette_t palette = userSettings->noteNamePalette;
    // if (palette == LV_PALETTE_NONE) {
        lv_obj_set_style_img_recolor(attitude_note_img, lv_color_white(), 0);
        lv_obj_set_style_img_recolor(attitude_sharp_img, lv_color_white(), 0);
    // } else {
    //     lv_obj_set_style_img_recolor(attitude_note_img, lv_palette_main(palette), 0);
    //     lv_obj_set_style_img_recolor(attitude_sharp_img, lv_palette_main(palette), 0);
    // }

    // MUTE label (for monitoring mode)
    attitude_mute_label = lv_label_create(parent);
    lv_label_set_text_static(attitude_mute_label, "MUTE");
    lv_obj_set_style_text_font(attitude_mute_label, &lv_font_montserrat_18, 0);
    lv_obj_align(attitude_mute_label, LV_ALIGN_TOP_LEFT, 2, 0);
    lv_obj_add_flag(attitude_mute_label, LV_OBJ_FLAG_HIDDEN);

    // Frequency Label (very bottom)
    attitude_frequency_label = lv_label_create(parent);
    lv_label_set_long_mode(attitude_frequency_label, LV_LABEL_LONG_CLIP);

    lv_label_set_text_static(attitude_frequency_label, "-");
    lv_obj_set_width(attitude_frequency_label, screen_width);
    lv_obj_set_style_text_align(attitude_frequency_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(attitude_frequency_label, LV_ALIGN_BOTTOM_RIGHT, -2, 0);

    lv_style_init(&attitude_frequency_label_style);
    lv_style_set_text_font(&attitude_frequency_label_style, &lv_font_montserrat_18);
    lv_obj_add_style(attitude_frequency_label, &attitude_frequency_label_style, 0);
    lv_obj_add_flag(attitude_frequency_label, LV_OBJ_FLAG_HIDDEN);

    // Cents display
    attitude_cents_label = lv_label_create(parent);
    
    lv_style_init(&attitude_cents_label_style);
    lv_style_set_text_font(&attitude_cents_label_style, &lv_font_montserrat_18);
    lv_obj_add_style(attitude_cents_label, &attitude_cents_label_style, 0);

    lv_obj_set_width(attitude_cents_label, screen_width / 2);
    lv_obj_set_style_text_align(attitude_cents_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(attitude_cents_label, LV_ALIGN_BOTTOM_LEFT, 2, 0);
    lv_obj_add_flag(attitude_cents_label, LV_OBJ_FLAG_HIDDEN);
}

void attitude_create_arrows(lv_obj_t * parent) {
    // lv_obj_add_flag(parent, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    // Show tick marks on the left and right sides of the frame
    attitude_left_tick = lv_obj_create(parent);
    lv_obj_set_size(attitude_left_tick, ATTITUDE_ARROWS_SIZE, ATTITUDE_ARROWS_SIZE);
    lv_obj_set_style_border_width(attitude_left_tick, 0, 0);
    lv_obj_set_style_bg_color(attitude_left_tick, lv_color_white(), 0);
    lv_obj_set_style_radius(attitude_left_tick, 0, 0);
    lv_obj_set_style_transform_pivot_x(attitude_left_tick, ATTITUDE_ARROWS_PIVOT_LOCATION, 0);
    lv_obj_set_style_transform_pivot_y(attitude_left_tick, ATTITUDE_ARROWS_PIVOT_LOCATION, 0);
    lv_obj_set_style_transform_angle(attitude_left_tick, 450, 0);    
    lv_obj_align(attitude_left_tick, LV_ALIGN_LEFT_MID, -ATTITUDE_ARROWS_PIVOT_LOCATION, attitude_is_landscape ? ATTITUDE_LANDSCAPE_VERTICAL_OFFSET : ATTITUDE_PORTRAIT_VERTICAL_OFFSET);

    attitude_right_tick = lv_obj_create(parent);
    lv_obj_set_size(attitude_right_tick, ATTITUDE_ARROWS_SIZE, ATTITUDE_ARROWS_SIZE);
    lv_obj_set_style_border_width(attitude_right_tick, 0, 0);
    lv_obj_set_style_bg_color(attitude_right_tick, lv_color_white(), 0);
    lv_obj_set_style_radius(attitude_right_tick, 0, 0);
    lv_obj_set_style_transform_pivot_x(attitude_right_tick, ATTITUDE_ARROWS_PIVOT_LOCATION, 0);
    lv_obj_set_style_transform_pivot_y(attitude_right_tick, ATTITUDE_ARROWS_PIVOT_LOCATION, 0);
    lv_obj_set_style_transform_angle(attitude_right_tick, 450, 0);
    lv_obj_align(attitude_right_tick, LV_ALIGN_RIGHT_MID, ATTITUDE_ARROWS_PIVOT_LOCATION, attitude_is_landscape ? ATTITUDE_LANDSCAPE_VERTICAL_OFFSET : ATTITUDE_PORTRAIT_VERTICAL_OFFSET);

    // Create the center lines
    lv_obj_t *center_line_left = lv_obj_create(parent);
    lv_obj_set_size(center_line_left, ATTITUDE_INDICATOR_LINE_WIDTH, ATTITUDE_INDICATOR_LINE_HEIGHT);
    lv_obj_set_style_border_color(center_line_left, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_set_style_border_width(center_line_left, ATTITUDE_INDICATOR_BORDER_WIDTH, 0);
    // lv_obj_set_style_bg_color(center_line_left, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_set_style_bg_opa(center_line_left, LV_OPA_0, 0);
    lv_obj_set_scrollbar_mode(center_line_left, LV_SCROLLBAR_MODE_OFF);
    lv_obj_align(center_line_left, LV_ALIGN_CENTER, -ATTITUDE_INDICATOR_LINE_WIDTH, attitude_is_landscape ? ATTITUDE_LANDSCAPE_VERTICAL_OFFSET : ATTITUDE_PORTRAIT_VERTICAL_OFFSET);

    lv_obj_t *center_line_right = lv_obj_create(parent);
    lv_obj_set_size(center_line_right, ATTITUDE_INDICATOR_LINE_WIDTH, ATTITUDE_INDICATOR_LINE_HEIGHT);
    lv_obj_set_style_border_width(center_line_right, 0, 0);
    lv_obj_set_style_bg_color(center_line_right, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_set_scrollbar_mode(center_line_right, LV_SCROLLBAR_MODE_OFF);
    lv_obj_align(center_line_right, LV_ALIGN_CENTER, ATTITUDE_INDICATOR_LINE_WIDTH, attitude_is_landscape ? ATTITUDE_LANDSCAPE_VERTICAL_OFFSET : ATTITUDE_PORTRAIT_VERTICAL_OFFSET);

    // Create the center dot
    lv_obj_t *center_dot = lv_obj_create(parent);
    lv_obj_set_size(center_dot, ATTITUDE_INDICATOR_LINE_HEIGHT, ATTITUDE_INDICATOR_LINE_HEIGHT);
    lv_obj_set_style_border_width(center_dot, 0, 0);
    lv_obj_set_style_bg_color(center_dot, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_set_style_radius(center_dot, ATTITUDE_INDICATOR_LINE_HEIGHT/ 2, 0);
    lv_obj_set_scrollbar_mode(center_dot, LV_SCROLLBAR_MODE_OFF);
    lv_obj_align(center_dot, LV_ALIGN_CENTER, 0, attitude_is_landscape ? ATTITUDE_LANDSCAPE_VERTICAL_OFFSET : ATTITUDE_PORTRAIT_VERTICAL_OFFSET);
}

void attitude_update_note_name(TunerNoteName new_value) {
    // Set the note name with a timer so it doesn't get
    // set too often for LVGL. ADC makes it run SUPER
    // fast and can crash the software.
    const lv_image_dsc_t *img_desc;
    bool show_sharp_symbol = false;
    bool show_note_fade_anim = false;
    switch (new_value) {
    case NOTE_A_SHARP:
        show_sharp_symbol = true;
    case NOTE_A:
        img_desc = &tuner_font_image_a;
        break;
    case NOTE_B:
        img_desc = &tuner_font_image_b;
        break;
    case NOTE_C_SHARP:
        show_sharp_symbol = true;
    case NOTE_C:
        img_desc = &tuner_font_image_c;
        break;
    case NOTE_D_SHARP:
        show_sharp_symbol = true;
    case NOTE_D:
        img_desc = &tuner_font_image_d;
        break;
    case NOTE_E:
        img_desc = &tuner_font_image_e;
        break;
    case NOTE_F_SHARP:
        show_sharp_symbol = true;
    case NOTE_F:
        img_desc = &tuner_font_image_f;
        break;
    case NOTE_G_SHARP:
        show_sharp_symbol = true;
    case NOTE_G:
        img_desc = &tuner_font_image_g;
        break;
    case NOTE_NONE:
        show_note_fade_anim = true;
        break;
    default:
        return;
    }

    if (show_note_fade_anim) {
        attitude_start_note_fade_animation();
    } else {
        attitude_stop_note_fade_animation();

        if (show_sharp_symbol) {
            lv_obj_clear_flag(attitude_sharp_img, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(attitude_sharp_img, LV_OBJ_FLAG_HIDDEN);
        }

        lv_image_set_src(attitude_note_img, img_desc);
    }
}

void attitude_start_note_fade_animation() {
    attitude_last_note_anim = (lv_anim_t *)malloc(sizeof(lv_anim_t));
    lv_anim_init(attitude_last_note_anim);
    lv_anim_set_exec_cb(attitude_last_note_anim, (lv_anim_exec_xcb_t)attitude_last_note_anim_cb);
    lv_anim_set_completed_cb(attitude_last_note_anim, attitude_last_note_anim_completed_cb);
    lv_anim_set_var(attitude_last_note_anim, attitude_note_img_container);
    lv_anim_set_duration(attitude_last_note_anim, LAST_NOTE_FADE_INTERVAL_MS);
    lv_anim_set_values(attitude_last_note_anim, 100, 0); // Fade from 100% to 0% opacity
    lv_anim_start(attitude_last_note_anim);
}

void attitude_stop_note_fade_animation() {
    lv_obj_set_style_opa(attitude_note_img_container, LV_OPA_100, 0);
    if (attitude_last_note_anim == NULL) {
        return;
    }
    lv_anim_del_all();
    delete(attitude_last_note_anim);
    attitude_last_note_anim = NULL;
}

void attitude_last_note_anim_cb(lv_obj_t *obj, int32_t value) {
    if (!lvgl_port_lock(0)) {
        return;
    }

    lv_obj_set_style_opa(obj, value, LV_PART_MAIN);

    lvgl_port_unlock();
}

void attitude_last_note_anim_completed_cb(lv_anim_t *) {
    if (!lvgl_port_lock(0)) {
        return;
    }
    // The animation has completed so hide the note name and set
    // the opacity back to 100%.
    lv_obj_add_flag(attitude_sharp_img, LV_OBJ_FLAG_HIDDEN);
    lv_image_set_src(attitude_note_img, &tuner_font_image_none);
    attitude_last_displayed_note = NOTE_NONE;

    attitude_stop_note_fade_animation();

    lvgl_port_unlock();
}