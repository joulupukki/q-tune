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
#include "tuner_ui_note_quiz.h"

#include <stdlib.h>

#include "globals.h"
#include "user_settings.h"

#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_timer.h"
#include "esp_random.h"

#define NOTE_QUIZ_TARGET_NOTE_DURATION_MS 100
#define NOTE_QUIZ_IN_TUNE_SLIDER_OFFSET 18
#define NOTE_QUIZ_SLIDER_HEIGHT 25

static const char *TAG = "NOTE_QUIZ";

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

LV_IMG_DECLARE(tuner_font_image_a2x)
LV_IMG_DECLARE(tuner_font_image_b2x)
LV_IMG_DECLARE(tuner_font_image_c2x)
LV_IMG_DECLARE(tuner_font_image_d2x)
LV_IMG_DECLARE(tuner_font_image_e2x)
LV_IMG_DECLARE(tuner_font_image_f2x)
LV_IMG_DECLARE(tuner_font_image_g2x)
LV_IMG_DECLARE(tuner_font_image_none2x)
LV_IMG_DECLARE(tuner_font_image_sharp2x)

//
// Function Definitions
//
TunerNoteName get_random_note();
void quiz_create_labels(lv_obj_t * parent);;
void quiz_update_note_name(lv_obj_t *img, lv_obj_t *sharp_img, TunerNoteName new_value, bool use_2x);
void quiz_set_new_target_note();

// void quiz_start_note_fade_animation();
// void quiz_stop_note_fade_animation();
// void quiz_last_note_anim_cb(lv_obj_t *obj, int32_t value);
// void quiz_last_note_anim_completed_cb(lv_anim_t *);

//
// Local Variables
//
lv_obj_t *quiz_parent_screen = NULL;

// The note shown that should be played.
TunerNoteName quiz_current_target_note = NOTE_NONE;

// Keep track of the last note that was displayed so telling the UI to update
// can be avoided if it is the same.
TunerNoteName quiz_last_displayed_note = NOTE_NONE;

// The upcoming note.
TunerNoteName quiz_upcoming_note = NOTE_NONE;

lv_obj_t *quiz_note_img_container;
lv_obj_t *quiz_note_img;
lv_obj_t *quiz_sharp_img;

lv_obj_t *quiz_next_note_img_container;
lv_obj_t *quiz_next_note_img;
lv_obj_t *quiz_next_sharp_img;

lv_obj_t *quiz_mute_label;
lv_obj_t *quiz_note_label;
lv_obj_t *quiz_frequency_label;
lv_style_t quiz_frequency_label_style;
lv_obj_t *quiz_cents_label;
lv_style_t quiz_cents_label_style;

lv_obj_t *quiz_slider_container_left;
lv_obj_t *quiz_slider_container_right;
lv_obj_t *quiz_slider_left;
lv_obj_t *quiz_slider_right;

lv_palette_t quiz_user_note_palette;
lv_color_t quiz_user_note_color;
lv_color_t quiz_slider_out_color;

// When a new note is detected, this should be set to the current time. This
// allows us to determine the elapsed seconds for how long a note should be
// plucked to be determined a "Hit" to advance to the next note in the quiz.
int64_t quiz_last_note_change_time_ms = -1;

uint8_t quiz_gui_get_id() {
    return 3;
}

const char * quiz_gui_get_name() {
    return "Note Quiz";
}

void quiz_gui_init(lv_obj_t *screen) {
    ESP_LOGI(TAG, "Initializing the note quiz tuner UI");
    quiz_parent_screen = screen;
    quiz_create_labels(screen);

    quiz_current_target_note = NOTE_NONE;
    quiz_last_displayed_note = NOTE_NONE;
    quiz_upcoming_note = NOTE_NONE;
    quiz_last_note_change_time_ms = -1;

    quiz_user_note_palette = userSettings->noteNamePalette;
    if (quiz_user_note_palette == LV_PALETTE_NONE) {
        quiz_user_note_color = lv_color_white();
    } else {
        quiz_user_note_color = lv_palette_main(quiz_user_note_palette);
    }
    quiz_slider_out_color = lv_palette_main(LV_PALETTE_GREY);

    lv_obj_set_style_img_recolor(quiz_note_img, quiz_user_note_color, 0);
    lv_obj_set_style_img_recolor(quiz_sharp_img, quiz_user_note_color, 0);
}

void quiz_gui_display_frequency(float frequency, float target_frequency, TunerNoteName note_name, int octave, float cents, bool show_mute_indicator) {
    if (note_name < 0) { return; } // Strangely I'm sometimes seeing negative values. No idea how.

    if (quiz_current_target_note == NOTE_NONE) {
        // This will be called at the beginning of the quiz
        quiz_set_new_target_note();
    }

    if (note_name != NOTE_NONE) {
        lv_label_set_text_fmt(quiz_frequency_label, "%.2f", frequency);
        lv_obj_clear_flag(quiz_frequency_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(quiz_slider_container_left, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(quiz_slider_container_right, LV_OBJ_FLAG_HIDDEN);

        if (quiz_last_displayed_note != note_name) {
            lv_label_set_text_static(quiz_note_label, name_for_note(note_name));
            quiz_last_displayed_note = note_name; // prevent setting this so often to help prevent an LVGL crash
            quiz_last_note_change_time_ms = esp_timer_get_time() / 1000; // milliseconds
        }

        lv_label_set_text_fmt(quiz_cents_label, "%.1f", cents);
        lv_obj_clear_flag(quiz_cents_label, LV_OBJ_FLAG_HIDDEN);

        int64_t elapsed_time_ms = (esp_timer_get_time() / 1000) - quiz_last_note_change_time_ms;

        if (note_name == quiz_current_target_note && elapsed_time_ms >= NOTE_QUIZ_TARGET_NOTE_DURATION_MS) {
            // If the note has been displayed for the target duration, advance to the next note.
            quiz_set_new_target_note();

            // TODO: Might need to also call reset on the tuner to clear out
            // any frequency averaging that might be happening. Not sure yet how
            // the best way to do that is.
        }

        bool is_in_tune = abs(cents) <= userSettings->inTuneCentsWidth;
        lv_color_t left_slider_color = (is_in_tune || cents > 0) ? quiz_user_note_color : quiz_slider_out_color;
        lv_color_t right_slider_color = (is_in_tune || cents < 0) ? quiz_user_note_color : quiz_slider_out_color;

        lv_obj_set_style_bg_color(quiz_slider_left, left_slider_color, 0);
        lv_obj_set_style_bg_color(quiz_slider_right, right_slider_color, 0);

        lv_obj_align(quiz_slider_container_left, LV_ALIGN_LEFT_MID, (cents * (cents >= 0 ? 0 : 1)) - NOTE_QUIZ_IN_TUNE_SLIDER_OFFSET, 0);
        lv_obj_align(quiz_slider_container_right, LV_ALIGN_RIGHT_MID, (cents * (cents <= 0 ? 0 : 1)) + NOTE_QUIZ_IN_TUNE_SLIDER_OFFSET, 0);
    } else {
        // Hide the pitch and indicators since it's not detected
        if (quiz_last_displayed_note != NOTE_NONE) {
            lv_label_set_text_static(quiz_note_label, name_for_note(note_name));
            quiz_last_displayed_note = NOTE_NONE;
        }

        // Hide the frequency, and cents labels
        lv_obj_add_flag(quiz_cents_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(quiz_frequency_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(quiz_slider_container_left, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(quiz_slider_container_right, LV_OBJ_FLAG_HIDDEN);

        quiz_last_note_change_time_ms = -1; // Reset the timer
    }

    if (show_mute_indicator) {
        lv_obj_clear_flag(quiz_mute_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(quiz_mute_label, LV_OBJ_FLAG_HIDDEN);
    }
}

void quiz_gui_cleanup() {
}

TunerNoteName get_random_note() {
    TunerNoteName note = (TunerNoteName)(esp_random() % NOTE_NONE); // Random number from 0 to 11
    if (note == quiz_current_target_note) {
        // If the random note is the same as the current target note, try again.
        return get_random_note();
    }
    return note;
}

void quiz_create_labels(lv_obj_t * parent) {

    //
    // Target Note
    // 
    // Place the note name and # symbole in the same container so the opacity
    // can be animated.
    quiz_note_img_container = lv_obj_create(parent);
    lv_obj_set_size(quiz_note_img_container, lv_pct(100), lv_pct(50));
    lv_obj_set_style_bg_opa(quiz_note_img_container, LV_OPA_0, 0);
    lv_obj_set_style_border_width(quiz_note_img_container, 0, 0);
    lv_obj_set_scrollbar_mode(quiz_note_img_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_align(quiz_note_img_container, LV_ALIGN_TOP_MID, 0, 0);

    // Target Note Name Image (the big name at the top of the screen)
    quiz_note_img = lv_image_create(quiz_note_img_container);
    lv_image_set_src(quiz_note_img, &tuner_font_image_none);
    lv_obj_center(quiz_note_img);

    quiz_sharp_img = lv_image_create(quiz_note_img_container);
    lv_image_set_src(quiz_sharp_img, &tuner_font_image_sharp);
    lv_obj_align_to(quiz_sharp_img, quiz_note_img, LV_ALIGN_TOP_RIGHT, 40, -30);
    lv_obj_add_flag(quiz_sharp_img, LV_OBJ_FLAG_HIDDEN);
    
    // Enable recoloring on the images
    lv_obj_set_style_img_recolor_opa(quiz_note_img, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(quiz_sharp_img, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_img_recolor(quiz_note_img, quiz_user_note_color, 0);
    lv_obj_set_style_img_recolor(quiz_sharp_img, quiz_user_note_color, 0);

    //
    // Next Note
    //
    quiz_next_note_img_container = lv_obj_create(parent);
    lv_obj_set_size(quiz_next_note_img_container, lv_pct(100), lv_pct(50));
    lv_obj_set_style_bg_opa(quiz_next_note_img_container, LV_OPA_0, 0);
    lv_obj_set_style_border_width(quiz_next_note_img_container, 0, 0);
    lv_obj_set_scrollbar_mode(quiz_next_note_img_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_align(quiz_next_note_img_container, LV_ALIGN_BOTTOM_MID, 0, 0);

    // Target Note Name Image (the big name at the top of the screen)
    quiz_next_note_img = lv_image_create(quiz_next_note_img_container);
    lv_image_set_src(quiz_next_note_img, &tuner_font_image_none);
    lv_obj_center(quiz_next_note_img);

    quiz_next_sharp_img = lv_image_create(quiz_next_note_img_container);
    lv_image_set_src(quiz_next_sharp_img, &tuner_font_image_sharp);
    lv_obj_align_to(quiz_next_sharp_img, quiz_next_note_img, LV_ALIGN_TOP_RIGHT, 20, -15);
    lv_obj_add_flag(quiz_next_sharp_img, LV_OBJ_FLAG_HIDDEN);
    
    // Enable recoloring on the images
    lv_obj_set_style_img_recolor_opa(quiz_next_note_img, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(quiz_next_sharp_img, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_img_recolor(quiz_next_note_img, lv_color_hex(0x777777), 0);
    lv_obj_set_style_img_recolor(quiz_next_sharp_img, lv_color_hex(0x777777), 0);

    // MUTE label (for monitoring mode)
    quiz_mute_label = lv_label_create(parent);
    lv_label_set_text_static(quiz_mute_label, "MUTE");
    lv_obj_set_style_text_font(quiz_mute_label, &lv_font_montserrat_18, 0);
    lv_obj_align(quiz_mute_label, LV_ALIGN_TOP_LEFT, 2, 0);
    lv_obj_add_flag(quiz_mute_label, LV_OBJ_FLAG_HIDDEN);

    // Frequency Label (very bottom)
    quiz_frequency_label = lv_label_create(parent);
    lv_label_set_long_mode(quiz_frequency_label, LV_LABEL_LONG_CLIP);

    lv_label_set_text_static(quiz_frequency_label, "-");
    lv_obj_set_width(quiz_frequency_label, screen_width);
    lv_obj_set_style_text_align(quiz_frequency_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(quiz_frequency_label, LV_ALIGN_RIGHT_MID, -2, 0);

    lv_style_init(&quiz_frequency_label_style);
    lv_style_set_text_font(&quiz_frequency_label_style, &lv_font_montserrat_18);
    lv_obj_add_style(quiz_frequency_label, &quiz_frequency_label_style, 0);
    lv_obj_add_flag(quiz_frequency_label, LV_OBJ_FLAG_HIDDEN);

    // Cents display
    quiz_cents_label = lv_label_create(parent);
    
    lv_style_init(&quiz_cents_label_style);
    lv_style_set_text_font(&quiz_cents_label_style, &lv_font_montserrat_18);
    lv_obj_add_style(quiz_cents_label, &quiz_cents_label_style, 0);

    lv_obj_set_width(quiz_cents_label, screen_width / 2);
    lv_obj_set_style_text_align(quiz_cents_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(quiz_cents_label, LV_ALIGN_LEFT_MID, 2, 0);
    lv_obj_add_flag(quiz_cents_label, LV_OBJ_FLAG_HIDDEN);

    // Actual note display
    quiz_note_label = lv_label_create(parent);
    lv_label_set_text_static(quiz_note_label, "-");
    lv_obj_set_width(quiz_note_label, screen_width);
    lv_obj_set_style_text_align(quiz_note_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(quiz_note_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(quiz_note_label, &quiz_frequency_label_style, 0);

    //
    // Cents Sliders
    //

    // left side
    quiz_slider_container_left = lv_obj_create(parent);
    lv_obj_set_size(quiz_slider_container_left, lv_pct(50), NOTE_QUIZ_SLIDER_HEIGHT);
    lv_obj_set_style_bg_opa(quiz_slider_container_left, LV_OPA_0, 0);
    lv_obj_set_scrollbar_mode(quiz_slider_container_left, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(quiz_slider_container_left, 0, 0);
    lv_obj_set_style_pad_all(quiz_slider_container_left, 0, 0);
    lv_obj_align(quiz_slider_container_left, LV_ALIGN_LEFT_MID, -NOTE_QUIZ_IN_TUNE_SLIDER_OFFSET, 0);

    quiz_slider_left = lv_obj_create(quiz_slider_container_left);
    lv_obj_set_size(quiz_slider_left, lv_pct(100), lv_pct(100));
    lv_obj_set_style_border_width(quiz_slider_left, 0, 0);
    lv_obj_set_style_radius(quiz_slider_left, 0, 0);
    lv_obj_set_scrollbar_mode(quiz_slider_left, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(quiz_slider_left, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(quiz_slider_left, LV_ALIGN_RIGHT_MID, 0, 0);

    // right side
    quiz_slider_container_right = lv_obj_create(parent);
    lv_obj_set_size(quiz_slider_container_right, lv_pct(50), NOTE_QUIZ_SLIDER_HEIGHT);
    lv_obj_set_style_bg_opa(quiz_slider_container_right, LV_OPA_0, 0);
    lv_obj_set_scrollbar_mode(quiz_slider_container_right, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(quiz_slider_container_right, 0, 0);
    lv_obj_set_style_pad_all(quiz_slider_container_right, 0, 0);
    lv_obj_align(quiz_slider_container_right, LV_ALIGN_RIGHT_MID, NOTE_QUIZ_IN_TUNE_SLIDER_OFFSET, 0);

    quiz_slider_right = lv_obj_create(quiz_slider_container_right);
    lv_obj_set_size(quiz_slider_right, lv_pct(100), lv_pct(100));
    lv_obj_set_style_border_width(quiz_slider_right, 0, 0);
    lv_obj_set_style_radius(quiz_slider_right, 0, 0);
    lv_obj_set_scrollbar_mode(quiz_slider_right, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(quiz_slider_right, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(quiz_slider_right, LV_ALIGN_LEFT_MID, 0, 0);
}

void quiz_update_note_name(lv_obj_t *img, lv_obj_t *sharp_img, TunerNoteName new_value, bool use_2x) {
    // Set the note name with a timer so it doesn't get
    // set too often for LVGL. ADC makes it run SUPER
    // fast and can crash the software.
    const lv_image_dsc_t *img_desc;
    bool show_sharp_symbol = false;
    // bool show_note_fade_anim = false;
    switch (new_value) {
    case NOTE_A_SHARP:
        show_sharp_symbol = true;
    case NOTE_A:
        img_desc = use_2x ? &tuner_font_image_a2x : &tuner_font_image_a;
        break;
    case NOTE_B:
        img_desc = use_2x ? &tuner_font_image_b2x : &tuner_font_image_b;
        break;
    case NOTE_C_SHARP:
        show_sharp_symbol = true;
    case NOTE_C:
        img_desc = use_2x ? &tuner_font_image_c2x : &tuner_font_image_c;
        break;
    case NOTE_D_SHARP:
        show_sharp_symbol = true;
    case NOTE_D:
        img_desc = use_2x ? &tuner_font_image_d2x : &tuner_font_image_d;
        break;
    case NOTE_E:
        img_desc = use_2x ? &tuner_font_image_e2x : &tuner_font_image_e;
        break;
    case NOTE_F_SHARP:
        show_sharp_symbol = true;
    case NOTE_F:
        img_desc = use_2x ? &tuner_font_image_f2x : &tuner_font_image_f;
        break;
    case NOTE_G_SHARP:
        show_sharp_symbol = true;
    case NOTE_G:
        img_desc = use_2x ? &tuner_font_image_g2x : &tuner_font_image_g;
        break;
    case NOTE_NONE:
        // show_note_fade_anim = true;
        img_desc = use_2x ? &tuner_font_image_none2x : &tuner_font_image_none;
        break;
    default:
        return;
    }

    // if (show_note_fade_anim) {
    //     quiz_start_note_fade_animation();
    // } else {
    //     quiz_stop_note_fade_animation();

        if (show_sharp_symbol) {
            lv_obj_clear_flag(sharp_img, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(sharp_img, LV_OBJ_FLAG_HIDDEN);
        }

        lv_image_set_src(img, img_desc);
    // }
}

void quiz_set_new_target_note() {
    if (quiz_upcoming_note == NOTE_NONE) {
        quiz_current_target_note = get_random_note();
    } else{
        quiz_current_target_note = quiz_upcoming_note;
    }
    quiz_upcoming_note = get_random_note();

    quiz_update_note_name(quiz_note_img, quiz_sharp_img, quiz_current_target_note, false);
    quiz_update_note_name(quiz_next_note_img, quiz_next_sharp_img, quiz_upcoming_note, false);
}

// void quiz_start_note_fade_animation() {
//     quiz_last_note_anim = (lv_anim_t *)malloc(sizeof(lv_anim_t));
//     lv_anim_init(quiz_last_note_anim);
//     lv_anim_set_exec_cb(quiz_last_note_anim, (lv_anim_exec_xcb_t)quiz_last_note_anim_cb);
//     lv_anim_set_completed_cb(quiz_last_note_anim, quiz_last_note_anim_completed_cb);
//     lv_anim_set_var(quiz_last_note_anim, quiz_note_img_container);
//     lv_anim_set_duration(quiz_last_note_anim, LAST_NOTE_FADE_INTERVAL_MS);
//     lv_anim_set_values(quiz_last_note_anim, 100, 0); // Fade from 100% to 0% opacity
//     lv_anim_start(quiz_last_note_anim);
// }

// void quiz_stop_note_fade_animation() {
//     lv_obj_set_style_opa(quiz_note_img_container, LV_OPA_100, 0);
//     if (quiz_last_note_anim == NULL) {
//         return;
//     }
//     lv_anim_del_all();
//     delete(quiz_last_note_anim);
//     quiz_last_note_anim = NULL;
// }

// void quiz_last_note_anim_cb(lv_obj_t *obj, int32_t value) {
//     if (!lvgl_port_lock(0)) {
//         return;
//     }

//     lv_obj_set_style_opa(obj, value, LV_PART_MAIN);

//     lvgl_port_unlock();
// }

// void quiz_last_note_anim_completed_cb(lv_anim_t *) {
//     if (!lvgl_port_lock(0)) {
//         return;
//     }
//     // The animation has completed so hide the note name and set
//     // the opacity back to 100%.
//     lv_obj_add_flag(quiz_sharp_img, LV_OBJ_FLAG_HIDDEN);
//     lv_image_set_src(quiz_note_img, &tuner_font_image_none);
//     quiz_last_displayed_note = NOTE_NONE;

//     quiz_stop_note_fade_animation();

//     lvgl_port_unlock();
// }