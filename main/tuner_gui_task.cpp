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
#include "standby_ui_blank.h"
#include "tuner_standby_ui_interface.h"
#include "tuner_ui_interface.h"
#include "user_settings.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_timer.h"

#include <esp_lcd_panel_io.h>

#include "waveshare.h"

#include <cmath> // for log2()

//
// These are the tuner UIs available.
//
#include "tuner_ui_attitude.h"
#include "tuner_ui_needle.h"
#include "tuner_ui_note_quiz.h"
#include "tuner_ui_record_time.h"
#include "tuner_ui_strobe.h"

//
// LVGL Support
//
#include "lvgl.h"
#include "esp_lvgl_port.h"

static const char *TAG = "GUI";

extern TunerController *tunerController;
extern UserSettings *userSettings;
extern "C" const lv_font_t fontawesome_48;
extern QueueHandle_t frequencyQueue;

// Local Function Declarations
void update_ui(TunerState old_state, TunerState new_state);

void create_standby_ui();
void create_tuning_ui();
void create_settings_ui();

void settings_button_cb(lv_event_t *e);
void create_settings_menu_button(lv_obj_t * parent);
static esp_err_t app_lvgl_main();

static esp_lcd_panel_io_handle_t lcd_io;
static esp_lcd_panel_handle_t lcd_panel;

lv_coord_t screen_width = 0;
lv_coord_t screen_height = 0;
bool is_landscape = false;

extern lv_display_t *lvgl_display;
lv_obj_t *main_screen = NULL;

bool is_gui_loaded = false;

FrequencyInfo freqInfo;

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

TunerGUIInterface record_time_ui = {
    .get_id = record_time_gui_get_id,
    .get_name = record_time_gui_get_name,
    .init = record_time_gui_init,
    .display_frequency = record_time_gui_display_frequency,
    .cleanup = record_time_gui_cleanup
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
    record_time_ui,
    note_quiz_gui,
};

size_t num_of_available_guis = 5;

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

/// @brief The main GUI task.
///
/// This is the main GUI FreeRTOS task and is declared as an extern in main.cpp.
///
/// @param pvParameter User data (unused).
void tuner_gui_task(void *pvParameter) {

    ESP_ERROR_CHECK(waveshare_lcd_init());
    ESP_ERROR_CHECK(waveshare_lvgl_init());
    // ESP_ERROR_CHECK(waveshare_touch_init()); // Don't use Touch for now

    // Make sure the user's preferred rotation is set up before we draw the screen.
    if (lvgl_port_lock(0)) {
        ESP_ERROR_CHECK(lcd_display_rotate(lvgl_display, userSettings->getDisplayOrientation()));
        lvgl_port_unlock();
    }

    ESP_ERROR_CHECK(app_lvgl_main());
    
    is_gui_loaded = true; // Prevents some other threads that rely on LVGL from running until the UI is loaded

    ESP_LOGI(TAG, "Mem: %d", heap_caps_get_free_size(MALLOC_CAP_DMA));
    
    // Use old_tuner_ui_state to keep track of the old state locally (in this
    // function).
    TunerState old_tuner_ui_state = tunerController->getState();
    TunerState initial_state = userSettings->initialState;
    tunerController->setState(initial_state);

    while(1) {
        TunerState newState = tunerController->getState();

        if (old_tuner_ui_state != newState) {
            update_ui(old_tuner_ui_state, newState);
            old_tuner_ui_state = newState;
        }

        bool monitoring_mode = userSettings->monitoringMode && newState == tunerStateStandby;
        if ((monitoring_mode || newState == tunerStateTuning) && lvgl_port_lock(0)) {
            bool show_mute_indicator = newState == tunerStateTuning && userSettings->monitoringMode;

            if (!xQueuePeek(frequencyQueue, &freqInfo, 0)) {
                freqInfo.frequency = -1;
            }
            if (freqInfo.frequency > 0) {
                get_active_gui().display_frequency(freqInfo.frequency, freqInfo.frequency, freqInfo.targetNote, freqInfo.targetOctave, freqInfo.cents, show_mute_indicator);
            } else {
                get_active_gui().display_frequency(0, 0, NOTE_NONE, 0, 0, show_mute_indicator);
            }

            lvgl_port_unlock();
        }
        
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(33));
    }
    vTaskDelay(portMAX_DELAY);
}

void update_ui(TunerState old_state, TunerState new_state) {
    ESP_LOGI(TAG, "Old State: %d, New State: %d", old_state, new_state);
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

void user_settings_updated() {
    if (!is_gui_loaded || !lvgl_port_lock(0)) {
        return;
    }

    screen_width = lv_obj_get_width(main_screen);
    screen_height = lv_obj_get_height(main_screen);
    is_landscape = screen_width > screen_height;

    lvgl_port_unlock();
}

void create_standby_ui() {
    get_active_standby_gui().init(main_screen);
}

void create_tuning_ui() {
    // First build the Tuner UI
    get_active_gui().init(main_screen);

    // // Place the settings button on the UI (bottom left)
    // create_settings_menu_button(main_screen);
}

void create_settings_ui() {
    userSettings->showSettings();
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
    is_landscape = screen_width > screen_height;
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