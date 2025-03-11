/*
 * Copyright (c) 2024 Boyd Timothy. All rights reserved.
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
#include "tuner_gui_task.h"

#include "defines.h"
#include "globals.h"
#include "standby_ui_blank.h"
#include "tuner_standby_ui_interface.h"
#include "tuner_ui_interface.h"
#include "user_settings.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_adc/adc_continuous.h"
#include "esp_timer.h"

#include <cmath> // for log2()

//
// These are the tuner UIs available.
//
#include "tuner_ui_attitude.h"
#include "tuner_ui_needle.h"
#include "tuner_ui_strobe.h"
#include "tuner_ui_note_quiz.h"

//
// LVGL Support
//
#include "lvgl.h"
#include "esp_lvgl_port.h"

extern "C" { // because these files are C and not C++
    // #include "lcd.h"
    // #include "touch.h"
    #include "ST7701S.h"
}

static const char *TAG = "GUI";

extern TunerController *tunerController;
extern UserSettings *userSettings;
extern "C" const lv_font_t fontawesome_48;

// Local Function Declarations
void update_ui(TunerState old_state, TunerState new_state);

void create_standby_ui();
void create_tuning_ui();
void create_settings_ui();

float midi_note_from_frequency(float freq);
TunerNoteName get_pitch_name_and_cents_from_frequency(float freq, float *cents);
void settings_button_cb(lv_event_t *e);
void create_settings_menu_button(lv_obj_t * parent);
static esp_err_t app_lvgl_main();

static void *buf1 = NULL;
static void *buf2 = NULL;

lv_coord_t screen_width = 0;
lv_coord_t screen_height = 0;

lv_display_t *lvgl_display = NULL;
lv_obj_t *main_screen = NULL;

bool is_gui_loaded = false;

/// This variable is used to keep track of what state the UI is in. Initially
/// the code would try to rebuild the UI inside of the button press handling of
/// gpio_task but that was causing problems probably because not much memory is
/// allocated to that task. This now allows the UI task to change after-the-fact
/// and do the GUI changes inside tuner_gui_task.
TunerState current_ui_tuner_state = tunerStateBooting;
portMUX_TYPE current_ui_tuner_state_mutex = portMUX_INITIALIZER_UNLOCKED;

//
// GPIO Footswitch and Relay Pin Variables
//

// Keep track of what the relay state is so the app doesn't have to keep making
// calls to set it over and over (which chews up CPU).
// uint32_t current_relay_gpio_level = 0; // Off at launch

// int last_footswitch_state = 1; // Assume initial state is open (active-high logic and conveniently matches the physical position of the footswitch)
// int footswitch_press_count = 0;
// int64_t footswitch_press_start_time = 0;
// int64_t last_footswitch_press_time = 0;

///
/// Add Standby GUIs here.
///
TunerStandbyGUIInterface blank_standby_gui = {
    .get_id = blank_standby_gui_get_id,
    .get_name = blank_standby_gui_get_name,
    .init = blank_standby_gui_init,
    .cleanup = blank_standby_gui_cleanup
};

TunerStandbyGUIInterface available_standby_guis[] = {
    blank_standby_gui, // ID = 0
};

size_t num_of_available_standby_guis = 1;

///
/// Add Tuning GUIs here.
///
TunerGUIInterface needle_gui = {
    .get_id = needle_gui_get_id,
    .get_name = needle_gui_get_name,
    .init = needle_gui_init,
    .display_frequency = needle_gui_display_frequency,
    .cleanup = needle_gui_cleanup
};

TunerGUIInterface strobe_gui = {
    .get_id = strobe_gui_get_id,
    .get_name = strobe_gui_get_name,
    .init = strobe_gui_init,
    .display_frequency = strobe_gui_display_frequency,
    .cleanup = strobe_gui_cleanup
};

TunerGUIInterface attitude_gui = {
    .get_id = attitude_gui_get_id,
    .get_name = attitude_gui_get_name,
    .init = attitude_gui_init,
    .display_frequency = attitude_gui_display_frequency,
    .cleanup = attitude_gui_cleanup
};

TunerGUIInterface note_quiz_gui = {
    .get_id = quiz_gui_get_id,
    .get_name = quiz_gui_get_name,
    .init = quiz_gui_init,
    .display_frequency = quiz_gui_display_frequency,
    .cleanup = quiz_gui_cleanup
};

TunerGUIInterface available_guis[] = {

    // IMPORTANT: Make sure you update `num_of_available_guis` below so any new
    // Tuner GUI you add here will show up in the user settings as an option.
    
    needle_gui, // ID = 0
    strobe_gui,
    attitude_gui,
    note_quiz_gui,
};

size_t num_of_available_guis = 4;

TunerStandbyGUIInterface *active_standby_gui = NULL;
TunerGUIInterface *active_gui = NULL;

TunerStandbyGUIInterface get_active_standby_gui() {
    uint8_t standby_gui_index = userSettings->standbyGUIIndex;
    TunerStandbyGUIInterface active_standby_gui = available_standby_guis[standby_gui_index];
    return active_standby_gui;
}

TunerGUIInterface get_active_gui() {
    uint8_t tuner_gui_index = userSettings->tunerGUIIndex;
    TunerGUIInterface active_gui = available_guis[tuner_gui_index];
    return active_gui;
}

// esp_lcd_panel_io_handle_t lcd_io;
esp_lcd_panel_handle_t lcd_panel;

/// @brief Flush callback for LVGL to draw the display.
/// 
/// This function is called by LVGL to flush a specific area of the display with pixel data.
/// 
/// @param display Pointer to the LVGL display.
/// @param area Pointer to the area to be flushed.
/// @param px_map Pointer to the pixel data to be drawn.
void lvgl_port_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map) {
    esp_lcd_panel_draw_bitmap(lcd_panel, area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map);

#if CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM
    xSemaphoreGive(sem_gui_ready);
    xSemaphoreTake(sem_vsync_end, portMAX_DELAY);
#endif
    lv_display_flush_ready(display);
}

esp_err_t lvgl_init() {
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 4096,
        .task_affinity = -1,
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5
    };

    esp_err_t e = lvgl_port_init(&lvgl_cfg); // This also calls lv_init()
    ESP_LOGI(TAG, "lvgl_port_init() returned %s", esp_err_to_name(e));
    
    lvgl_display = lv_display_create(LCD_H_RES, LCD_V_RES);
    lv_display_set_flush_cb(lvgl_display, lvgl_port_flush_cb);

    // Allocate and set the display buffers
    buf1 = heap_caps_malloc(LCD_DRAWBUF_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    assert(buf1);
    buf2 = heap_caps_malloc(LCD_DRAWBUF_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    assert(buf2);

    lv_display_set_buffers(lvgl_display, buf1, buf2, LCD_DRAWBUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);

    if (lvgl_port_lock(0)) {
        ESP_ERROR_CHECK(lcd_display_brightness_set(userSettings->displayBrightness * 10 + 10)); // Adjust for 0 - 10%, 1 - 20%, etc.
        ESP_ERROR_CHECK(lcd_display_rotate(lvgl_display, userSettings->getDisplayOrientation()));
        // ESP_ERROR_CHECK(lcd_display_rotate(lvgl_display, LV_DISPLAY_ROTATION_0)); // Upside Down
        lvgl_port_unlock();
    }

    return ESP_OK;
}

/// @brief The main GUI task.
///
/// This is the main GUI FreeRTOS task and is declared as an extern in main.cpp.
///
/// @param pvParameter User data (unused).
void tuner_gui_task(void *pvParameter) {

    ESP_ERROR_CHECK(LCD_Init(&lcd_panel));
    ESP_ERROR_CHECK(lvgl_init());
    ESP_ERROR_CHECK(app_lvgl_main());

    is_gui_loaded = true; // Prevents some other threads that rely on LVGL from running until the UI is loaded

    float cents;

    // Use old_tuner_ui_state to keep track of the old state locally (in this
    // function). When gpio_task signals that the state should change, the
    // callback quickly writes to the current_ui_tuner_state (using a mutex) and
    // then tuner_gui_task does the actual update inside the while loop.
    TunerState old_tuner_ui_state = tunerController->getState();
    TunerState initial_state = userSettings->initialState;
    tunerController->setState(initial_state);

    while(1) {
        TunerState newState = tunerStateBooting;
        portENTER_CRITICAL(&current_ui_tuner_state_mutex);
        newState = current_ui_tuner_state;
        portEXIT_CRITICAL(&current_ui_tuner_state_mutex);

        if (old_tuner_ui_state != current_ui_tuner_state) {
            update_ui(old_tuner_ui_state, current_ui_tuner_state);
            old_tuner_ui_state = current_ui_tuner_state;
        }

        bool monitoring_mode = userSettings->monitoringMode && current_ui_tuner_state == tunerStateStandby;
        if ((monitoring_mode || current_ui_tuner_state == tunerStateTuning) && lvgl_port_lock(0)) {
            bool show_mute_indicator = current_ui_tuner_state == tunerStateTuning && userSettings->monitoringMode;
            float frequency = get_current_frequency();
            if (frequency > 0) {
                TunerNoteName note_name = get_pitch_name_and_cents_from_frequency(frequency, &cents);
                // ESP_LOGI(TAG, "%s - %d", noteName, cents);
                get_active_gui().display_frequency(frequency, note_name, cents, show_mute_indicator);
            } else {
                get_active_gui().display_frequency(0, NOTE_NONE, 0, show_mute_indicator);
            }

            lvgl_port_unlock();
        }
        
        lv_timer_handler();
        // vTaskDelay(pdMS_TO_TICKS(33));
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelay(portMAX_DELAY);
}

void update_ui(TunerState old_state, TunerState new_state) {
    if (!lvgl_port_lock(0)) {
        return;
    }

    // Close the old UI
    switch (old_state) {
    case tunerStateSettings:
        userSettings->exitSettings();
        break;
    case tunerStateStandby:
        // TODO: Maybe look at setting the brightness to 0? Maybe set this up
        // as something that can be specified by the active standby interface.
        if (!userSettings->monitoringMode) {
            get_active_standby_gui().cleanup();
        }
        break;
    case tunerStateTuning:
        if (!userSettings->monitoringMode) {
            get_active_gui().cleanup();
        }
        break;
    default:
        break;
    }

    // Clean up any left-over LVGL objects on the main screen before loading the
    // next UI.
    if ((userSettings->monitoringMode && old_state == tunerStateSettings) || !userSettings->monitoringMode) {
        lv_obj_clean(main_screen);
    }

    // Load the new UI
    switch (new_state) {
    case tunerStateSettings:
        create_settings_ui();
        break;
    case tunerStateStandby:
        if (!userSettings->monitoringMode) {
            lcd_display_brightness_set(0); // Turn off the display
            create_standby_ui();
        }
        break;
    case tunerStateTuning:
        // If monitoring is enabled, only create the tuning UI if the previous
        // state was not standby.
        if (userSettings->monitoringMode && old_state == tunerStateStandby) {
            break; // Nothing to do because the tuning UI is already showing
        }
        lcd_display_brightness_set(userSettings->displayBrightness * 10 + 10); // Turn the display back on
        create_tuning_ui();
        break;
    default:
        break;
    }

    lvgl_port_unlock();
}

void tuner_gui_task_tuner_state_changed(TunerState old_state, TunerState new_state) {
    // Change the value of current_ui_tuner_state so the main UI thread will be
    // able to change after the tuner state changes.
    portENTER_CRITICAL(&current_ui_tuner_state_mutex);
    current_ui_tuner_state = new_state;
    portEXIT_CRITICAL(&current_ui_tuner_state_mutex);
}

void user_settings_updated() {
    if (!is_gui_loaded || !lvgl_port_lock(0)) {
        return;
    }

    screen_width = lv_obj_get_width(main_screen);
    screen_height = lv_obj_get_height(main_screen);

    lvgl_port_unlock();
}

void create_standby_ui() {
    get_active_standby_gui().init(main_screen);
}

void create_tuning_ui() {
    // First build the Tuner UI
    get_active_gui().init(main_screen);

    // Place the settings button on the UI (bottom left)
    // create_settings_menu_button(main_screen);
}

void create_settings_ui() {
    userSettings->showSettings();
}

// Function to calculate the MIDI note number from frequency
float midi_note_from_frequency(float freq) {
    return 69 + 12 * log2(freq / A4_FREQ);
}

// Function to get pitch name and cents from MIDI note number
TunerNoteName get_pitch_name_and_cents_from_frequency(float freq, float *cents) {
    float midi_note = midi_note_from_frequency(freq);
    int note_index = (int)fmod(midi_note, 12);
    float fractional_note = midi_note - (int)midi_note;

    *cents = (fractional_note * CENTS_PER_SEMITONE);
    if (*cents >= CENTS_PER_SEMITONE / 2) {
        // Adjust the note index so that it still shows the proper note
        note_index++;
        if (note_index >= 12) {
            note_index = 0; // Overflow to the beginning of the note names
        }
        *cents -= CENTS_PER_SEMITONE; // Adjust to valid range
    }

    if (note_index > 12) { // safeguard to prevent crash of snprintf()
        note_index = 0;
    }
    return (TunerNoteName)note_index;
}

void settings_button_cb(lv_event_t *e) {
    ESP_LOGI(TAG, "Settings button clicked");
    tunerController->setState(tunerStateSettings);
}

void create_settings_menu_button(lv_obj_t * parent) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_style_opa(btn, LV_OPA_40, 0);
    lv_obj_remove_style_all(btn);
    lv_obj_set_ext_click_area(btn, 100);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, GEAR_SYMBOL);
    lv_obj_add_event_cb(btn, settings_button_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
}

static esp_err_t app_lvgl_main() {
    lvgl_port_lock(0);

    ESP_LOGI(TAG, "Starting LVGL Main");

    lv_obj_t *scr = lv_scr_act();
    screen_width = lv_obj_get_width(scr);
    screen_height = lv_obj_get_height(scr);
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    main_screen = scr;

    // Prevent scrolling on the main screen so the UI doesn't move around if
    // objects are placed outside of the bounds of the screen.
    lv_obj_set_scroll_dir(main_screen, LV_DIR_NONE);

    // ESP_LOGI("LOCK", "unlocking in app_lvgl_main");
    lvgl_port_unlock();

    userSettings->setDisplayAndScreen(lvgl_display, main_screen);

    return ESP_OK;
}