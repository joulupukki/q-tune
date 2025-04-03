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
#include "tuner_ui_record_time.h"

#include <stdlib.h>

#include "user_settings.h"

#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_random.h"

static const char *TAG = "RECORD_TIME";

#define RECORD_TIME_RECORD_COLOR            0x3D3D3D
#define RECORD_TIME_ARC_COLOR               0x666666
#define RECORD_TIME_SLIDER_CONTAINER_COLOR  0x3D3D3D

#define RECORD_TIME_ARC_BOUNDS_OUTER        135
#define RECORD_TIME_ARC_BOUNDS_INNER        120
#define RECORD_TIME_ARC_WIDTH               2
#define RECORD_TIME_RECORD_OFFSET           -60
#define RECORD_TIME_NOTE_OFFSET             90
#define RECORD_TIME_ARM_MAX_ANGLE           30
#define RECORD_TIME_ARM_MIN_ANGLE           14
#define RECORD_TIME_ARM_ANGLE_RANGE         (RECORD_TIME_ARM_MAX_ANGLE - RECORD_TIME_ARM_MIN_ANGLE)
#define RECORD_TIME_ARM_ZERO_ANGLE          (RECORD_TIME_ARM_ANGLE_RANGE / 2) + RECORD_TIME_ARM_MIN_ANGLE

#define RECORD_TIME_SLIDER_CONTAINER_WIDTH  32
#define RECORD_TIME_SLIDER_BUTTON_HEIGHT    19
#define RECORD_TIME_SLIDER_LINE_HEIGHT      7
#define RECORD_TIME_RULER_LINE_HEIGHT       3

extern UserSettings *userSettings;
extern lv_coord_t screen_width;
extern lv_coord_t screen_height;
extern bool is_landscape;

LV_IMG_DECLARE(tuner_font_image_a)
LV_IMG_DECLARE(tuner_font_image_b)
LV_IMG_DECLARE(tuner_font_image_c)
LV_IMG_DECLARE(tuner_font_image_d)
LV_IMG_DECLARE(tuner_font_image_e)
LV_IMG_DECLARE(tuner_font_image_f)
LV_IMG_DECLARE(tuner_font_image_g)
LV_IMG_DECLARE(tuner_font_image_none)
LV_IMG_DECLARE(tuner_font_image_sharp)
LV_IMG_DECLARE(record_time_title_1)
LV_IMG_DECLARE(record_time_title_2)
LV_IMG_DECLARE(record_time_title_3)
LV_IMG_DECLARE(record_time_title_4)
LV_IMG_DECLARE(record_time_title_5)
LV_IMG_DECLARE(record_time_arm)

lv_image_dsc_t record_time_titles[] = {
    record_time_title_1,
    record_time_title_2,
    record_time_title_3,
    record_time_title_4,
    record_time_title_5,
};
#define RECORD_TIME_NUM_OF_TITLES 5

//
// Function Definitions
//
void record_time_create_labels(lv_obj_t * parent);
void record_time_create_record(lv_obj_t * parent);
void record_time_create_arcs(lv_obj_t * parent);
void record_time_create_arm(lv_obj_t * parent);
void record_time_create_slider(lv_obj_t * parent);
void record_time_update_note_name(TunerNoteName new_value);
void record_time_switch_to_none_note(lv_timer_t *timer);

//
// Local Variables
//
lv_obj_t *record_time_parent_screen = NULL;

// Keep track of the last note that was displayed so telling the UI to update
// can be avoided if it is the same.
TunerNoteName record_time_last_displayed_note = NOTE_NONE;
lv_image_dsc_t record_time_title_src_img;

lv_obj_t *record_time_note_img_container;
lv_obj_t *record_time_note_img;
lv_obj_t *record_time_sharp_img;

lv_obj_t *record_time_mute_label;
// lv_obj_t *record_time_frequency_label;
// lv_style_t record_time_frequency_label_style;
// lv_obj_t *record_time_cents_label;
// lv_style_t record_time_cents_label_style;

lv_obj_t *record_time_record;

lv_obj_t *record_time_arc_container;
lv_obj_t *record_time_arc1;
lv_obj_t *record_time_arc2;
lv_obj_t *record_time_arc3;
lv_obj_t *record_time_arc4;
lv_timer_t *record_time_fade_timer = NULL;

lv_obj_t *record_time_arm_img;

lv_obj_t *record_time_slider_container;
lv_obj_t *record_time_slider_track;
lv_obj_t *record_time_slider;
lv_obj_t *record_time_slider_line;
lv_obj_t *record_time_slider_center_ruler;

lv_obj_t *record_time_title_img;

float record_time_rotation_current_pos = 0;
float record_time_amount_to_rotate = 0;

lv_anim_t *record_time_last_note_anim = NULL;

uint8_t record_time_gui_get_id() {
    return 3;
}

const char * record_time_gui_get_name() {
    return "Record Time";
}

void record_time_gui_init(lv_obj_t *screen) {
    record_time_parent_screen = screen;

    // Randomize which record title to show
    int random_title_index = esp_random() % RECORD_TIME_NUM_OF_TITLES;
    record_time_title_src_img = record_time_titles[random_title_index];

    record_time_create_record(screen);
    record_time_create_arcs(screen);
    record_time_create_arm(screen);
    record_time_create_slider(screen);
    record_time_create_labels(screen);
}

float record_time_last_frequency = 0.0;
float record_time_last_cents = 0.0;

void record_time_gui_display_frequency(float frequency, float target_frequency, TunerNoteName note_name, int octave, float cents, bool show_mute_indicator) {
    if (note_name < 0) { return; } // Strangely I'm sometimes seeing negative values. No idea how.
    if (note_name != NOTE_NONE) {
        // if (record_time_last_frequency != frequency) {
        //     lv_label_set_text_fmt(record_time_frequency_label, "%.2f", frequency);
        //     record_time_last_frequency = frequency;
        // }
        // if (lv_obj_has_flag(record_time_frequency_label, LV_OBJ_FLAG_HIDDEN)) {
        //     lv_obj_clear_flag(record_time_frequency_label, LV_OBJ_FLAG_HIDDEN);
        // }

        if (record_time_last_displayed_note != note_name) {
            record_time_last_displayed_note = note_name; // prevent setting this so often to help prevent an LVGL crash
            record_time_update_note_name(note_name);
        }

        // if (record_time_last_cents != cents) {
        //     lv_label_set_text_fmt(record_time_cents_label, "%.1f", cents);
        //     record_time_last_cents = cents;
        // }
        // if (lv_obj_has_flag(record_time_cents_label, LV_OBJ_FLAG_HIDDEN)) {
        //     lv_obj_clear_flag(record_time_cents_label, LV_OBJ_FLAG_HIDDEN);
        // }

        record_time_amount_to_rotate = 25; // Keep spinning the record clockwise
    } else {
        record_time_amount_to_rotate = 0.0;
        // Hide the pitch and indicators since it's not detected
        if (record_time_last_displayed_note != NOTE_NONE) {
            record_time_last_displayed_note = NOTE_NONE;
            record_time_update_note_name(NOTE_NONE);
        }

        // Hide the frequency, and cents labels
        // lv_obj_add_flag(record_time_cents_label, LV_OBJ_FLAG_HIDDEN);
        // lv_obj_add_flag(record_time_frequency_label, LV_OBJ_FLAG_HIDDEN);
    }

    if (record_time_amount_to_rotate != 0) {
        record_time_rotation_current_pos += record_time_amount_to_rotate;

        // As the record "spins", simulate the record being warped a little by
        // shifting the arcs.
        bool shiftArcs = ((int)record_time_rotation_current_pos % 360) < 180;
        if (shiftArcs) {
            lv_obj_align(record_time_arc_container,
                LV_ALIGN_CENTER,
                -1,
                is_landscape ? 0 : RECORD_TIME_RECORD_OFFSET
            );
        } else {
            lv_obj_align(record_time_arc_container,
                LV_ALIGN_CENTER,
                0,
                is_landscape ? 0 : RECORD_TIME_RECORD_OFFSET
            );
        }

        // Move the arm according to how many cents off the note is. Normalize
        // the value in the range of what's available in the number of angles
        // for the arm.

        float cents_percentage = cents / 50.0; // 50 cents is the max range
        float angles_per_side = RECORD_TIME_ARM_ANGLE_RANGE / 2;
        float angles_for_cents = angles_per_side * cents_percentage;
        float arm_rotation = RECORD_TIME_ARM_ZERO_ANGLE + angles_for_cents;
        lv_obj_set_style_transform_angle(record_time_arm_img, arm_rotation * 10, 0);
        
        // Rotate the record title image
        lv_obj_set_style_transform_angle(record_time_title_img, record_time_rotation_current_pos * 10, 0);

        // Move the slider to represent the frequency
        lv_obj_align_to(record_time_slider, record_time_slider_container, LV_ALIGN_CENTER, 0, cents * -1);
        lv_obj_align_to(record_time_slider_line, record_time_slider_container, LV_ALIGN_CENTER, 0, cents * -1);

        if (abs(cents) <= userSettings->inTuneCentsWidth) {
            // Show some "In Tune" borders
            lv_obj_set_style_border_opa(record_time_slider_container, LV_OPA_100, 0);
            // lv_obj_set_style_border_opa(record_time_record, LV_OPA_100, 0);
        } else {
            lv_obj_set_style_border_opa(record_time_slider_container, LV_OPA_0, 0);
            // lv_obj_set_style_border_opa(record_time_record, LV_OPA_0, 0);
        }
    }

    if (show_mute_indicator) {
        lv_obj_clear_flag(record_time_mute_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(record_time_mute_label, LV_OBJ_FLAG_HIDDEN);
    }
}

void record_time_gui_cleanup() {
    // TODO: Do any cleanup needed here. 
}

void record_time_create_labels(lv_obj_t * parent) {
    // Place the note name and # symbole in the same container so the opacity
    // can be animated.
    record_time_note_img_container = lv_obj_create(parent);
    lv_obj_set_size(record_time_note_img_container, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(record_time_note_img_container, LV_OPA_0, 0);
    lv_obj_set_style_border_width(record_time_note_img_container, 0, 0);
    lv_obj_set_scrollbar_mode(record_time_note_img_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(record_time_note_img_container, LV_DIR_NONE);
    lv_obj_align(record_time_note_img_container,
        is_landscape ? LV_ALIGN_LEFT_MID : LV_ALIGN_CENTER,
        is_landscape ? -16 : 0,
        is_landscape ? 0 : RECORD_TIME_NOTE_OFFSET);

    // Note Name Image (the big name in the middle of the screen)
    record_time_note_img = lv_image_create(record_time_note_img_container);
    lv_image_set_src(record_time_note_img, &tuner_font_image_none);
    if (is_landscape) {
        lv_obj_align(record_time_note_img, LV_ALIGN_LEFT_MID, 0, 0);
    } else {
        lv_obj_center(record_time_note_img);
    }

    record_time_sharp_img = lv_image_create(record_time_note_img_container);
    lv_image_set_src(record_time_sharp_img, &tuner_font_image_sharp);
    lv_obj_align_to(record_time_sharp_img, record_time_note_img, LV_ALIGN_TOP_RIGHT, 20, -15);
    lv_obj_add_flag(record_time_sharp_img, LV_OBJ_FLAG_HIDDEN);
    
    // Enable recoloring on the images
    lv_obj_set_style_img_recolor_opa(record_time_note_img, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(record_time_sharp_img, LV_OPA_COVER, LV_PART_MAIN);
    lv_palette_t palette = userSettings->noteNamePalette;
    if (palette == LV_PALETTE_NONE) {
        lv_obj_set_style_img_recolor(record_time_note_img, lv_color_white(), 0);
        lv_obj_set_style_img_recolor(record_time_sharp_img, lv_color_white(), 0);
    } else {
        lv_obj_set_style_img_recolor(record_time_note_img, lv_palette_main(palette), 0);
        lv_obj_set_style_img_recolor(record_time_sharp_img, lv_palette_main(palette), 0);
    }

    // MUTE label (for monitoring mode)
    record_time_mute_label = lv_label_create(parent);
    lv_label_set_text_static(record_time_mute_label, "MUTE");
    lv_obj_set_style_text_font(record_time_mute_label, &lv_font_montserrat_18, 0);
    lv_obj_align(record_time_mute_label, LV_ALIGN_TOP_LEFT, 2, 0);
    lv_obj_add_flag(record_time_mute_label, LV_OBJ_FLAG_HIDDEN);

    // Frequency Label (very bottom)
    // record_time_frequency_label = lv_label_create(parent);
    // lv_label_set_long_mode(record_time_frequency_label, LV_LABEL_LONG_CLIP);

    // lv_label_set_text_static(record_time_frequency_label, "-");
    // lv_obj_set_width(record_time_frequency_label, screen_width);
    // lv_obj_set_style_text_align(record_time_frequency_label, LV_TEXT_ALIGN_RIGHT, 0);
    // lv_obj_align(record_time_frequency_label, LV_ALIGN_BOTTOM_RIGHT, -2, -2);

    // lv_style_init(&record_time_frequency_label_style);
    // lv_style_set_text_font(&record_time_frequency_label_style, &lv_font_montserrat_18);
    // lv_obj_add_style(record_time_frequency_label, &record_time_frequency_label_style, 0);
    // lv_obj_add_flag(record_time_frequency_label, LV_OBJ_FLAG_HIDDEN);

    // Cents display
    // record_time_cents_label = lv_label_create(parent);
    
    // lv_style_init(&record_time_cents_label_style);
    // lv_style_set_text_font(&record_time_cents_label_style, &lv_font_montserrat_18);
    // lv_obj_add_style(record_time_cents_label, &record_time_cents_label_style, 0);

    // lv_obj_set_width(record_time_cents_label, screen_width / 2);
    // lv_obj_set_style_text_align(record_time_cents_label, LV_TEXT_ALIGN_CENTER, 0);
    // lv_obj_align(record_time_cents_label, LV_ALIGN_BOTTOM_LEFT, 2, -2);
    // lv_obj_add_flag(record_time_cents_label, LV_OBJ_FLAG_HIDDEN);
}

void record_time_create_record(lv_obj_t *parent) {
    float record_size = (is_landscape ? screen_height : screen_width) * 0.7;

    lv_palette_t palette = userSettings->noteNamePalette;
    // Create the outer record here so it is under the arcs but don't put it in
    // the container so that it doesn't get moved/rotated.
    record_time_record = lv_obj_create(parent);
    lv_obj_set_size(record_time_record, record_size, record_size);
    lv_obj_set_style_bg_color(record_time_record, lv_color_hex(RECORD_TIME_RECORD_COLOR), 0);
    // lv_obj_set_style_border_width(record_time_record, 2, 0);
    // if (palette == LV_PALETTE_NONE) {
    //     lv_obj_set_style_border_color(record_time_record, lv_palette_main(LV_PALETTE_RED), 0);
    // } else {
    //     lv_obj_set_style_border_color(record_time_record, lv_palette_main(palette), 0);
    // }
    lv_obj_set_style_border_opa(record_time_record, LV_OPA_0, 0);
    lv_obj_set_scrollbar_mode(record_time_record, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(record_time_record, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(record_time_record, LV_ALIGN_CENTER,
        0,
        is_landscape ? 0 : RECORD_TIME_RECORD_OFFSET
    );

    lv_obj_t *center_circle = lv_obj_create(record_time_record);
    lv_obj_set_size(center_circle, 70, 70);
    lv_obj_set_style_border_width(center_circle, 0, 0);
    if (palette == LV_PALETTE_NONE) {
        lv_obj_set_style_bg_color(center_circle, lv_palette_main(LV_PALETTE_AMBER), 0);
    } else {
        lv_obj_set_style_bg_color(center_circle, lv_palette_main(palette), 0);
    }
    lv_obj_set_style_radius(center_circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(center_circle, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *center_dot = lv_obj_create(record_time_record);
    lv_obj_set_size(center_dot, 8, 8);
    lv_obj_set_style_border_width(center_dot, 0, 0);
    lv_obj_set_style_bg_color(center_dot, lv_color_black(), 0);
    lv_obj_set_style_radius(center_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(center_dot, LV_ALIGN_CENTER, 0, 0);


    record_time_title_img = lv_image_create(record_time_record);
    // lv_image_set_src(record_time_title_img, &record_time_title_1);
    lv_image_set_src(record_time_title_img, &record_time_title_src_img);
    lv_obj_set_style_transform_pivot_x(record_time_title_img, 35, 0);
    lv_obj_set_style_transform_pivot_y(record_time_title_img, 35, 0);
    lv_obj_align(record_time_title_img, LV_ALIGN_CENTER, 0, 0);
}

void record_time_create_arcs(lv_obj_t * parent) {
    record_time_arc_container = lv_obj_create(parent);
    lv_obj_set_size(record_time_arc_container, lv_obj_get_width(parent), lv_obj_get_height(parent));
    lv_obj_set_style_bg_opa(record_time_arc_container, LV_OPA_0, 0);
    lv_obj_set_style_border_width(record_time_arc_container, 0, 0);
    lv_obj_set_scrollbar_mode(record_time_arc_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_align(record_time_arc_container, LV_ALIGN_CENTER,
        0,
        is_landscape ? 0 : RECORD_TIME_RECORD_OFFSET
    );

    record_time_arc1 = lv_arc_create(record_time_arc_container);
    record_time_arc2 = lv_arc_create(record_time_arc_container);
    record_time_arc3 = lv_arc_create(record_time_arc_container);
    record_time_arc4 = lv_arc_create(record_time_arc_container);

    lv_obj_remove_style(record_time_arc1, NULL, LV_PART_KNOB); // Don't show a knob
    lv_obj_remove_style(record_time_arc2, NULL, LV_PART_KNOB); // Don't show a knob
    lv_obj_remove_style(record_time_arc3, NULL, LV_PART_KNOB); // Don't show a knob
    lv_obj_remove_style(record_time_arc4, NULL, LV_PART_KNOB); // Don't show a knob

    lv_obj_remove_flag(record_time_arc1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(record_time_arc2, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(record_time_arc3, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(record_time_arc4, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_set_size(record_time_arc4, RECORD_TIME_ARC_BOUNDS_OUTER, RECORD_TIME_ARC_BOUNDS_OUTER);
    lv_obj_set_size(record_time_arc2, RECORD_TIME_ARC_BOUNDS_INNER, RECORD_TIME_ARC_BOUNDS_INNER);
    lv_obj_set_size(record_time_arc3, RECORD_TIME_ARC_BOUNDS_OUTER, RECORD_TIME_ARC_BOUNDS_OUTER);
    lv_obj_set_size(record_time_arc4, RECORD_TIME_ARC_BOUNDS_INNER, RECORD_TIME_ARC_BOUNDS_INNER);

    lv_obj_set_style_arc_width(record_time_arc1, RECORD_TIME_ARC_WIDTH, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(record_time_arc2, RECORD_TIME_ARC_WIDTH, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(record_time_arc3, RECORD_TIME_ARC_WIDTH, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(record_time_arc4, RECORD_TIME_ARC_WIDTH, LV_PART_INDICATOR);

    lv_obj_set_style_arc_color(record_time_arc1, lv_color_hex(RECORD_TIME_ARC_COLOR), LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(record_time_arc2, lv_color_hex(RECORD_TIME_ARC_COLOR), LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(record_time_arc3, lv_color_hex(RECORD_TIME_ARC_COLOR), LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(record_time_arc4, lv_color_hex(RECORD_TIME_ARC_COLOR), LV_PART_INDICATOR);

    lv_obj_center(record_time_arc1);
    lv_obj_center(record_time_arc2);
    lv_obj_center(record_time_arc3);
    lv_obj_center(record_time_arc4);

    // Bottom left arcs
    lv_arc_set_angles(record_time_arc1, 100, 170);
    lv_arc_set_angles(record_time_arc2, 100, 170);

    // Top right arcs
    lv_arc_set_angles(record_time_arc3, 280, 350);
    lv_arc_set_angles(record_time_arc4, 280, 350);

    // Hide the background tracks
    lv_obj_set_style_arc_opa(record_time_arc1, LV_OPA_0, 0);
    lv_obj_set_style_arc_opa(record_time_arc2, LV_OPA_0, 0);
    lv_obj_set_style_arc_opa(record_time_arc3, LV_OPA_0, 0);
    lv_obj_set_style_arc_opa(record_time_arc4, LV_OPA_0, 0);
}

void record_time_create_arm(lv_obj_t * parent) {

    record_time_arm_img = lv_img_create(parent);
    lv_img_set_src(record_time_arm_img, &record_time_arm);
    lv_obj_set_style_transform_pivot_x(record_time_arm_img, 24, 0);
    lv_obj_set_style_transform_pivot_y(record_time_arm_img, 31, 0);
    lv_obj_align_to(record_time_arm_img, record_time_record, LV_ALIGN_TOP_RIGHT, 35, -15);
    lv_obj_set_style_transform_angle(record_time_arm_img, RECORD_TIME_ARM_ZERO_ANGLE * 10, 0);
}

void record_time_create_slider(lv_obj_t * parent) {
    int32_t slider_container_height = is_landscape ? lv_obj_get_height(record_time_record) : screen_height * 0.4;
    int32_t slider_container_width = RECORD_TIME_SLIDER_CONTAINER_WIDTH;

    record_time_slider_container = lv_obj_create(parent);
    lv_obj_set_size(record_time_slider_container, slider_container_width, slider_container_height);
    lv_obj_set_style_bg_color(record_time_slider_container, lv_color_hex(RECORD_TIME_SLIDER_CONTAINER_COLOR), 0);
    lv_obj_set_style_border_width(record_time_slider_container, 2, 0);
    lv_palette_t palette = userSettings->noteNamePalette;
    if (palette == LV_PALETTE_NONE) {
        palette = LV_PALETTE_RED;
    }
    lv_obj_set_style_border_color(record_time_slider_container, lv_palette_main(palette), 0);
    lv_obj_set_style_border_opa(record_time_slider_container, LV_OPA_0, 0);
    lv_obj_set_scrollbar_mode(record_time_slider_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(record_time_slider_container, 2, 0);
    lv_obj_align(record_time_slider_container,
        is_landscape ? LV_ALIGN_RIGHT_MID : LV_ALIGN_BOTTOM_RIGHT,
        -4,
        is_landscape ? 0 : -4
    );

    record_time_slider_track = lv_obj_create(parent);
    lv_obj_set_size(record_time_slider_track, 4, slider_container_height - 8);
    lv_obj_set_style_bg_color(record_time_slider_track, lv_color_black(), 0);
    lv_obj_set_style_radius(record_time_slider_track, 2, 0);
    lv_obj_set_scrollbar_mode(record_time_slider_track, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_opa(record_time_slider_track, LV_OPA_0, 0);
    lv_obj_align_to(record_time_slider_track, record_time_slider_container, LV_ALIGN_CENTER, 0, 0);

    // Draw the slider rulers that should be off to the left of the container
    record_time_slider_center_ruler = lv_obj_create(parent);
    lv_obj_set_size(record_time_slider_center_ruler, slider_container_width + 8, RECORD_TIME_RULER_LINE_HEIGHT);
    lv_obj_set_style_bg_color(record_time_slider_center_ruler, lv_color_white(), 0);
    lv_obj_set_scrollbar_mode(record_time_slider_center_ruler, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_opa(record_time_slider_center_ruler, LV_OPA_0, 0);
    lv_obj_align_to(record_time_slider_center_ruler, record_time_slider_container, LV_ALIGN_CENTER, 0, 0);

    // Draw the movable slider
    int32_t slider_width = slider_container_width - 4;
    record_time_slider = lv_obj_create(parent);
    lv_obj_set_size(record_time_slider, slider_width, RECORD_TIME_SLIDER_BUTTON_HEIGHT);
    lv_obj_set_style_bg_color(record_time_slider, lv_color_black(), 0);
    lv_obj_set_style_border_width(record_time_slider, 0, 0);
    // lv_obj_set_style_border_color(record_time_slider, lv_color_white(), 0);
    lv_obj_set_scrollbar_mode(record_time_slider, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(record_time_slider, 2, 0);
    lv_obj_align_to(record_time_slider, record_time_slider_container, LV_ALIGN_CENTER, 0, 0);

    record_time_slider_line = lv_obj_create(parent);
    lv_obj_set_size(record_time_slider_line, slider_width - 4, RECORD_TIME_SLIDER_LINE_HEIGHT);
    lv_obj_set_style_bg_color(record_time_slider_line, lv_color_white(), 0);
    lv_obj_set_scrollbar_mode(record_time_slider_line, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_opa(record_time_slider_line, LV_OPA_0, 0);
    lv_obj_set_style_radius(record_time_slider_line, 0, 0);
    lv_obj_align_to(record_time_slider_line, record_time_slider_container, LV_ALIGN_CENTER, 0, 0);
}

void record_time_update_note_name(TunerNoteName new_value) {
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
    if (record_time_fade_timer != NULL) {
        lv_timer_del(record_time_fade_timer);
        record_time_fade_timer = NULL;
    }

    if (show_note_fade_anim) {
        record_time_fade_timer = lv_timer_create(record_time_switch_to_none_note, 2000, NULL);
        lv_obj_set_style_border_opa(record_time_slider_container, LV_OPA_0, 0);
        // lv_obj_set_style_border_opa(record_time_record, LV_OPA_0, 0);
    } else {
        if (show_sharp_symbol) {
            lv_obj_clear_flag(record_time_sharp_img, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(record_time_sharp_img, LV_OBJ_FLAG_HIDDEN);
        }

        lv_image_set_src(record_time_note_img, img_desc);
    }
}

void record_time_switch_to_none_note(lv_timer_t *timer) {
    lv_timer_del(record_time_fade_timer);
    record_time_fade_timer = NULL;

    if (!lvgl_port_lock(0)) {
        return;
    }

    lv_image_set_src(record_time_note_img, &tuner_font_image_none);
    lv_obj_add_flag(record_time_sharp_img, LV_OBJ_FLAG_HIDDEN);

    // Restore the "in tune threshold" colors to defaults
    // lv_obj_set_style_bg_color(record_time_slider_track, lv_color_black(), 0);
    // lv_obj_set_style_border_opa(record_time_slider_container, LV_OPA_0, 0);

    lvgl_port_unlock();
    
    // Keep the system happy and prevent a watchdog event
    lv_timer_handler();
 }