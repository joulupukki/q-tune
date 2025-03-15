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
#include "user_settings.h"

#include "tuner_controller.h"
#include "tuner_ui_interface.h"
#include "waveshare.h"

static const char *TAG = "Settings";

extern TunerController *tunerController;
extern TunerGUIInterface available_guis[1]; // defined in tuner_gui_task.cpp
extern size_t num_of_available_guis;

#ifndef PROJECT_VERSION
#define PROJECT_VERSION "0.0.1"
#endif

#define MENU_BTN_TUNER              "Tuner"
    #define MENU_BTN_TUNER_MODE         "Mode"
    #define MENU_BTN_IN_TUNE_THRESHOLD  "In-Tune Threshold"
    #define MENU_BTN_MONITORING_MODE    "Monitoring Mode"
        #define MENU_BTN_MONITORING_OFF      "Off"
        #define MENU_BTN_MONITORING_ON       "On"

#define MENU_BTN_DISPLAY            "Display"
    #define MENU_BTN_BRIGHTNESS         "Brightness"
    #define MENU_BTN_NOTE_COLOR         "Note Color"
    #define MENU_BTN_INITIAL_SCREEN     "Initial Screen"
        #define MENU_BTN_STANDBY            "Standby"
        #define MENU_BTN_TUNING             "Tuning"
    #define MENU_BTN_ROTATION           "Rotation"
        #define MENU_BTN_ROTATION_NORMAL    "Normal"
        #define MENU_BTN_ROTATION_LEFT      "Left"
        #define MENU_BTN_ROTATION_RIGHT     "Right"
        #define MENU_BTN_ROTATION_UPSIDE_DN "Upside Down"

#define MENU_BTN_DEBUG              "Advanced"
#define MENU_BTN_EXP_SMOOTHING      "Exp Smoothing"
#define MENU_BTN_1EU_BETA           "1 EU Beta"
#define MENU_BTN_1EU_FLTR_1ST       "1 EU 1st?"
#define MENU_BTN_MOVING_AVG         "Moving Average"
#define MENU_BTN_NAME_DEBOUNCING    "Name Debouncing"

#define MENU_BTN_ABOUT              "About"
    #define MENU_BTN_FACTORY_RESET      "Factory Reset"

#define MENU_BTN_BACK               "Back"
#define MENU_BTN_EXIT               "Exit"

// Setting keys in NVS can only be up to 15 chars max
#define SETTINGS_INITIAL_SCREEN             "initial_screen"
#define SETTING_STANDBY_GUI_INDEX           "standby_gui_idx"
#define SETTING_TUNER_GUI_INDEX             "tuner_gui_index"
#define SETTING_KEY_IN_TUNE_WIDTH           "in_tune_width"
#define SETTING_KEY_MONITORING_MODE         "monitoring_mode"
#define SETTING_KEY_NOTE_NAME_PALETTE       "note_nm_palette"
#define SETTING_KEY_DISPLAY_ORIENTATION     "display_orient"
#define SETTING_KEY_EXP_SMOOTHING           "exp_smoothing"
#define SETTING_KEY_ONE_EU_BETA             "one_eu_beta"
#define SETTING_KEY_NOTE_DEBOUNCE_INTERVAL  "note_debounce"
#define SETTING_KEY_USE_1EU_FILTER_FIRST    "oneEUFilter1st"
// #define SETTING_KEY_MOVING_AVG_WINDOW_SIZE  "movingAvgWindow"
#define SETTING_KEY_DISPLAY_BRIGHTNESS      "dsp_brightness"

/*

SETTINGS
    Tuning
        [X] In Tune Width
        [x] Back - returns to the main menu

    Display Settings
        [x] Brightness
        [x] Note Color
        [x] Rotation
        [x] Back - returns to the main menu

    Debug
        [x] Exp Smoothing
        [x] 1EU Beta
        [x] Note Debouncing
        [x] Moving Average Window Size
        [x] Back - returns to the main menu

    About
        [x] Show version information
        [ ] Acknowledgements
        [x] Restore Factory Defaults
            [x] Confirmation Yes/No
        [x] Back
        
    Exit
*/

//
// Function Declarations
//
static void handleTunerButtonClicked(lv_event_t *e);
static void handleTunerModeButtonClicked(lv_event_t *e);
static void handleTunerModeSelected(lv_event_t *e);
static void handleInTuneThresholdButtonClicked(lv_event_t *e);
static void handleMonitoringModeButtonClicked(lv_event_t *e);
static void handleInTuneThresholdRadio(lv_event_t *e);
static void handleMonitoringModeRadio(lv_event_t *e);

static void handleDisplayButtonClicked(lv_event_t *e);
static void handleBrightnessButtonClicked(lv_event_t *e);
static void handleBrightnessSelected(lv_event_t *e);

static void handleNoteColorButtonClicked(lv_event_t *e);
static lv_palette_t paletteForSettingIndex(uint8_t settingIndex);
static uint8_t settingIndexForPalette(lv_palette_t palette);
static void handleNoteColorSelected(lv_event_t *e);

static void handleInitialScreenButtonClicked(lv_event_t *e);
static void handleInitialStateSelected(lv_event_t *e);

static void handleRotationButtonClicked(lv_event_t *e);
static void handleRotationSelected(lv_event_t *e);

static void handleDebugButtonClicked(lv_event_t *e);
static void handleExpSmoothingButtonClicked(lv_event_t *e);
static void handle1EUBetaButtonClicked(lv_event_t *e);
static void handle1EUFilterFirstButtonClicked(lv_event_t *e);
// static void handleMovingAvgButtonClicked(lv_event_t *e);
static void handleNameDebouncingButtonClicked(lv_event_t *e);

static void handleAboutButtonClicked(lv_event_t *e);
static void handleFactoryResetButtonClicked(lv_event_t *e);

static void handleBackButtonClicked(lv_event_t *e);
static void handleExitButtonClicked(lv_event_t *e);

//
// PRIVATE Methods
//

void UserSettings::loadSettings() {
    ESP_LOGI(TAG, "load settings");
    nvs_flash_init();
    nvs_open("settings", NVS_READWRITE, &nvsHandle);

    uint8_t value;
    uint32_t value32;

    if (nvs_get_u8(nvsHandle, SETTINGS_INITIAL_SCREEN, &value) == ESP_OK) {
        initialState = (TunerState)value;
    } else {
        initialState = DEFAULT_INITIAL_STATE;
    }
    if (initialState == tunerStateBooting) {
        initialState = DEFAULT_INITIAL_STATE;
    }
    ESP_LOGI(TAG, "Initial State: %d", initialState);

    if (nvs_get_u8(nvsHandle, SETTING_STANDBY_GUI_INDEX, &value) == ESP_OK) {
        standbyGUIIndex = value;
    } else {
        standbyGUIIndex = DEFAULT_STANDBY_GUI_INDEX;
    }
    ESP_LOGI(TAG, "Standby GUI Index: %d", standbyGUIIndex);

    if (nvs_get_u8(nvsHandle, SETTING_TUNER_GUI_INDEX, &value) == ESP_OK) {
        tunerGUIIndex = value;
    } else {
        tunerGUIIndex = DEFAULT_TUNER_GUI_INDEX;
    }
    ESP_LOGI(TAG, "Tuner GUI Index: %d", tunerGUIIndex);

    if (nvs_get_u8(nvsHandle, SETTING_KEY_IN_TUNE_WIDTH, &value) == ESP_OK) {
        inTuneCentsWidth = value;
    } else {
        inTuneCentsWidth = DEFAULT_IN_TUNE_CENTS_WIDTH;
    }
    ESP_LOGI(TAG, "In Tune Width: %d", inTuneCentsWidth);

    if (nvs_get_u8(nvsHandle, SETTING_KEY_MONITORING_MODE, &value) == ESP_OK) {
        monitoringMode = value;
    } else {
        monitoringMode = DEFAULT_MONITORING_MODE;
    }
    ESP_LOGI(TAG, "Monitoring Mode: %d", monitoringMode);

    if (nvs_get_u8(nvsHandle, SETTING_KEY_NOTE_NAME_PALETTE, &value) == ESP_OK) {
        noteNamePalette = (lv_palette_t)value;
    } else {
        noteNamePalette = DEFAULT_NOTE_NAME_PALETTE;
    }
    ESP_LOGI(TAG, "Note Name Palette: %d", noteNamePalette);

    if (nvs_get_u8(nvsHandle, SETTING_KEY_DISPLAY_ORIENTATION, &value) == ESP_OK) {
        displayOrientation = (TunerOrientation)value;
    } else {
        displayOrientation = DEFAULT_DISPLAY_ORIENTATION;
    }
    ESP_LOGI(TAG, "Display Orientation: %d", displayOrientation);

    if (nvs_get_u8(nvsHandle, SETTING_KEY_EXP_SMOOTHING, &value) == ESP_OK) {
        expSmoothing = ((float)value) * 0.01;
    } else {
        expSmoothing = DEFAULT_EXP_SMOOTHING;
    }

    if (nvs_get_u32(nvsHandle, SETTING_KEY_ONE_EU_BETA, &value32) == ESP_OK) {
        oneEUBeta = ((float)value32) * 0.001;
    } else {
        oneEUBeta = DEFAULT_ONE_EU_BETA;
    }

    if (nvs_get_u8(nvsHandle, SETTING_KEY_NOTE_DEBOUNCE_INTERVAL, &value) == ESP_OK) {
        noteDebounceInterval = (float)value;
    } else {
        noteDebounceInterval = DEFAULT_NOTE_DEBOUNCE_INTERVAL;
    }

    if (nvs_get_u8(nvsHandle, SETTING_KEY_USE_1EU_FILTER_FIRST, &value) == ESP_OK) {
        use1EUFilterFirst = (bool)value;
    } else {
        use1EUFilterFirst = DEFAULT_USE_1EU_FILTER_FIRST;
    }

    // if (nvs_get_u32(nvsHandle, SETTING_KEY_MOVING_AVG_WINDOW_SIZE, &value32) == ESP_OK) {
    //     movingAvgWindow = (float)value32;
    // } else {
    //     movingAvgWindow = DEFAULT_MOVING_AVG_WINDOW;
    // }

    if (nvs_get_u8(nvsHandle, SETTING_KEY_DISPLAY_BRIGHTNESS, &value) == ESP_OK) {
        displayBrightness = value;
    } else {
        displayBrightness = DEFAULT_DISPLAY_BRIGHTNESS;
    }
    ESP_LOGI(TAG, "Display Brightness: %d", displayBrightness);
}

void UserSettings::advanceToNextButton() {
    lv_group_t *group = lv_group_get_default();
    if (group == NULL) {
        ESP_LOGI(TAG, "advanceToNextButton - group is NULL");
        return;
    }
    lv_group_focus_next(group);
}

void UserSettings::pressFocusedButton() {
    lv_group_t *group = lv_group_get_default();
    if (group == NULL) {
        ESP_LOGI(TAG, "pressFocusedButton - group is NULL");
        return;
    }
    lv_obj_t *btn = lv_group_get_focused(group);
    if (btn == NULL) {
        ESP_LOGI(TAG, "pressFocusedButton - btn is NULL");
        return;
    }
    // const lv_obj_class_t *btnClass = lv_obj_get_class(btn);
    // if (btnClass == &lv_button_class || btnClass == &lv_checkbox_class) {
    //     lv_obj_send_event(btn, LV_EVENT_CLICKED, NULL);
    // } else {
        lv_obj_send_event(btn, LV_EVENT_CLICKED, NULL);
    // }
}

//
// PUBLIC Methods
//

UserSettings::UserSettings(settings_will_show_cb_t showCallback, settings_changed_cb_t changedCallback, settings_will_exit_cb_t exitCallback) {
    settingsWillShowCallback = showCallback;
    settingsChangedCallback = changedCallback;
    settingsWillExitCallback = exitCallback;
    currentSettingIndex = 0;
    loadSettings();
}

void UserSettings::saveSettings() {
    ESP_LOGI(TAG, "save settings");
    uint8_t value;
    uint32_t value32;

    value = initialState;
    ESP_LOGI(TAG, "Initial State: %d", value);
    nvs_set_u8(nvsHandle, SETTINGS_INITIAL_SCREEN, value);

    value = standbyGUIIndex;
    ESP_LOGI(TAG, "Standby GUI Index: %d", value);
    nvs_set_u8(nvsHandle, SETTING_STANDBY_GUI_INDEX, value);

    value = tunerGUIIndex;
    ESP_LOGI(TAG, "Tuner GUI Index: %d", value);
    nvs_set_u8(nvsHandle, SETTING_TUNER_GUI_INDEX, value);

    value = inTuneCentsWidth;
    ESP_LOGI(TAG, "In Tune Width: %d", value);
    nvs_set_u8(nvsHandle, SETTING_KEY_IN_TUNE_WIDTH, value);

    value = monitoringMode;
    ESP_LOGI(TAG, "Monitoring Mode: %d", value);
    nvs_set_u8(nvsHandle, SETTING_KEY_MONITORING_MODE, value);

    value = (uint8_t)noteNamePalette;
    ESP_LOGI(TAG, "Note Name Palette: %d", value);
    nvs_set_u8(nvsHandle, SETTING_KEY_NOTE_NAME_PALETTE, value);

    value = (uint8_t)displayOrientation;
    ESP_LOGI(TAG, "Display Orientation: %d", value);
    nvs_set_u8(nvsHandle, SETTING_KEY_DISPLAY_ORIENTATION, value);

    value = (uint8_t)(expSmoothing * 100);
    nvs_set_u8(nvsHandle, SETTING_KEY_EXP_SMOOTHING, value);

    value32 = (uint32_t)(oneEUBeta * 1000);
    nvs_set_u32(nvsHandle, SETTING_KEY_ONE_EU_BETA, value32);

    value = (uint8_t)noteDebounceInterval;
    nvs_set_u8(nvsHandle, SETTING_KEY_NOTE_DEBOUNCE_INTERVAL, value);

    value = (uint8_t)use1EUFilterFirst;
    nvs_set_u8(nvsHandle, SETTING_KEY_USE_1EU_FILTER_FIRST, value);

    // value32 = (uint32_t)movingAvgWindow;
    // nvs_set_u32(nvsHandle, SETTING_KEY_MOVING_AVG_WINDOW_SIZE, value32);

    value = displayBrightness;
    ESP_LOGI(TAG, "Display Brightness: %d", value);
    nvs_set_u8(nvsHandle, SETTING_KEY_DISPLAY_BRIGHTNESS, value);

    nvs_commit(nvsHandle);

    ESP_LOGI(TAG, "Settings saved");

    settingsChangedCallback();
}

void UserSettings::restoreDefaultSettings() {
    standbyGUIIndex = DEFAULT_STANDBY_GUI_INDEX;
    tunerGUIIndex = DEFAULT_TUNER_GUI_INDEX;
    inTuneCentsWidth = DEFAULT_IN_TUNE_CENTS_WIDTH;
    noteNamePalette = DEFAULT_NOTE_NAME_PALETTE;
    displayOrientation = DEFAULT_DISPLAY_ORIENTATION;
    expSmoothing = DEFAULT_EXP_SMOOTHING;
    oneEUBeta = DEFAULT_ONE_EU_BETA;
    noteDebounceInterval = DEFAULT_NOTE_DEBOUNCE_INTERVAL;
    use1EUFilterFirst = DEFAULT_USE_1EU_FILTER_FIRST;
    // movingAvgWindow = DEFAULT_MOVING_AVG_WINDOW;
    displayBrightness = DEFAULT_DISPLAY_BRIGHTNESS;

    saveSettings();

    // Reboot!
    esp_restart();
}

lv_display_rotation_t UserSettings::getDisplayOrientation() {
    switch (this->displayOrientation) {
        case orientationNormal:
            return LV_DISPLAY_ROTATION_180;
        case orientationLeft:
            return LV_DISPLAY_ROTATION_90;
        case orientationRight:
            return LV_DISPLAY_ROTATION_270;
        default:
            return LV_DISPLAY_ROTATION_0;
    }
}

void UserSettings::setDisplayAndScreen(lv_display_t *display, lv_obj_t *screen) {
    lvglDisplay = display;
    screenStack.push_back(screen);
}

void UserSettings::showSettings() {
    settingsWillShowCallback();
    const char *symbolNames[] = {
        LV_SYMBOL_HOME,
        LV_SYMBOL_IMAGE,
        LV_SYMBOL_SETTINGS,
        LV_SYMBOL_EYE_OPEN,
    };
    const char *buttonNames[] = {
        MENU_BTN_TUNER,
        MENU_BTN_DISPLAY,
        MENU_BTN_DEBUG,
        MENU_BTN_ABOUT,
        // MENU_BTN_EXIT,
    };
    lv_event_cb_t callbackFunctions[] = {
        handleTunerButtonClicked,
        handleDisplayButtonClicked,
        handleDebugButtonClicked,
        handleAboutButtonClicked,
    };

    lv_style_init(&focusedButtonStyle);
    lv_style_set_border_color(&focusedButtonStyle, lv_color_white());
    lv_style_set_border_width(&focusedButtonStyle, 2);

    lv_style_init(&radioStyle);
    lv_style_set_radius(&radioStyle, LV_RADIUS_CIRCLE);

    lv_style_init(&radioCheckStyle);
    lv_style_set_bg_img_src(&radioCheckStyle, NULL);

    createMenu(buttonNames, symbolNames, NULL, callbackFunctions, 4);
}

static lv_group_t * find_group_in_parent(lv_obj_t *parent) {
    lv_group_t *group = lv_obj_get_group(parent);
    if (group != NULL) {
        return group;
    }

    // Recursively look through children of the parent
    int numOfChildren = lv_obj_get_child_cnt(parent);
    if (numOfChildren == 0) {
        return NULL;
    }
    for (int i = 0; i < numOfChildren; i++) {
        lv_obj_t *child = lv_obj_get_child(parent, i);
        group = find_group_in_parent(child);
        if (group != NULL) {
            return group;
        }
    }
    return NULL;
}

void UserSettings::createMenu(const char *buttonNames[], const char *buttonSymbols[], lv_palette_t *buttonColors, lv_event_cb_t eventCallbacks[], int numOfButtons) {
    // Create a new screen, add all the buttons to it,
    // add the screen to the stack, and activate the new screen
    if (!lvgl_port_lock(0)) {
        return;
    }

    lv_obj_t *scr = lv_obj_create(NULL);

    // Create a scrollable container
    lv_obj_t *scrollable = lv_obj_create(scr);
    lv_obj_set_size(scrollable, lv_pct(100), lv_pct(100)); // Full size of the parent
    lv_obj_set_flex_flow(scrollable, LV_FLEX_FLOW_COLUMN); // Arrange children in a vertical list
    lv_obj_set_scroll_dir(scrollable, LV_DIR_VER);         // Enable vertical scrolling
    lv_obj_set_scrollbar_mode(scrollable, LV_SCROLLBAR_MODE_AUTO); // Show scrollbar when scrolling
    lv_obj_set_style_pad_all(scrollable, 10, 0);           // Add padding for aesthetics
    // lv_obj_set_style_bg_color(scrollable, lv_palette_darken(LV_PALETTE_BLUE_GREY, 4), 0); // Optional background color
    lv_obj_set_style_bg_color(scrollable, lv_color_black(), 0); // Optional background color

    lv_obj_t *btn;
    lv_obj_t *label;
    lv_group_t *group = lv_group_create();
    lv_group_set_wrap(group, true);
    lv_group_set_default(group);

    int32_t buttonWidthPercentage = 100;

    for (int i = 0; i < numOfButtons; i++) {
        ESP_LOGI(TAG, "Creating menu item: %d of %d", i, numOfButtons);
        const char *buttonName = buttonNames[i];
        lv_event_cb_t eventCallback = eventCallbacks[i];
        btn = lv_btn_create(scrollable);
        if (i == 0) {
            lv_obj_add_state(btn, LV_STATE_FOCUSED);
        }
        lv_obj_add_style(btn, &focusedButtonStyle, LV_STATE_FOCUSED);
        lv_obj_set_width(btn, lv_pct(buttonWidthPercentage));
        lv_obj_set_user_data(btn, this);
        lv_obj_add_event_cb(btn, eventCallback, LV_EVENT_CLICKED, btn);
        label = lv_label_create(btn);
        lv_label_set_text_static(label, buttonName);
        if (buttonSymbols != NULL) {
            const char *symbol = buttonSymbols[i];
            lv_obj_t *img = lv_image_create(btn);
            lv_image_set_src(img, symbol);
            lv_obj_align(img, LV_ALIGN_LEFT_MID, 0, 0);
            lv_obj_align_to(label, img, LV_ALIGN_OUT_RIGHT_MID, 6, 0);
        }

        if (buttonColors != NULL) {
            lv_palette_t palette = buttonColors[i];
            if (palette == LV_PALETTE_NONE) {
                // Set to white
                lv_obj_set_style_bg_color(btn, lv_color_white(), 0);
                lv_obj_set_style_text_color(label, lv_color_black(), 0);
            } else {
                lv_obj_set_style_bg_color(btn, lv_palette_main(palette), 0);
            }
        }

        lv_group_add_obj(group, btn);
    }

    if (screenStack.size() == 1) {
        // We're on the top menu - include an exit button
        btn = lv_btn_create(scrollable);
        lv_obj_add_style(btn, &focusedButtonStyle, LV_STATE_FOCUSED);
        lv_obj_set_user_data(btn, this);
        lv_obj_set_width(btn, lv_pct(buttonWidthPercentage));
        lv_obj_add_event_cb(btn, handleExitButtonClicked, LV_EVENT_CLICKED, btn);
        label = lv_label_create(btn);
        lv_label_set_text_static(label, MENU_BTN_EXIT);
        lv_obj_set_style_text_align(btn, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_group_add_obj(group, btn);
    } else {
        // We're in a submenu - include a back button
        btn = lv_btn_create(scrollable);
        lv_obj_add_style(btn, &focusedButtonStyle, LV_STATE_FOCUSED);
        lv_obj_set_user_data(btn, this);
        lv_obj_set_width(btn, lv_pct(buttonWidthPercentage));
        lv_obj_add_event_cb(btn, handleBackButtonClicked, LV_EVENT_CLICKED, btn);
        label = lv_label_create(btn);
        lv_label_set_text_static(label, MENU_BTN_BACK);
        lv_obj_set_style_text_align(btn, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_group_add_obj(group, btn);
    }

    screenStack.push_back(scr); // Save the new screen on the stack
    lv_screen_load(scr);        // Activate the new screen
    lvgl_port_unlock();
}

void UserSettings::removeCurrentMenu() {
    if (!lvgl_port_lock(0)) {
        return;
    }

    lv_obj_t *currentScreen = screenStack.back();
    screenStack.pop_back();

    lv_obj_t *parentScreen = screenStack.back();
    lv_scr_load(parentScreen);      // Show the parent screen

    lv_obj_clean(currentScreen);    // Clean up the screen so memory is cleared from sub items
    lv_obj_del(currentScreen);      // Remove the old screen from memory

    // Restore the default group so it can be navigated with the foot switch
    lv_group_t *group = find_group_in_parent(parentScreen);
    if (group != NULL) {
        lv_group_set_default(group);
    }

    lvgl_port_unlock();
}

void UserSettings::createSlider(const char *sliderName, int32_t minRange, int32_t maxRange, lv_event_cb_t sliderCallback, float *sliderValue) {
    // Create a new screen, add the slider to it, add the screen to the stack,
    // and activate the new screen.
    if (!lvgl_port_lock(0)) {
        return;
    }

    lv_obj_t *scr = lv_obj_create(NULL);

    // Create a scrollable container
    lv_obj_t *scrollable = lv_obj_create(scr);
    lv_obj_remove_style_all(scrollable);
    lv_obj_set_size(scrollable, lv_pct(100), lv_pct(100)); // Full size of the parent
    lv_obj_set_flex_flow(scrollable, LV_FLEX_FLOW_COLUMN); // Arrange children in a vertical list
    lv_obj_set_scroll_dir(scrollable, LV_DIR_VER);         // Enable vertical scrolling
    lv_obj_set_scrollbar_mode(scrollable, LV_SCROLLBAR_MODE_AUTO); // Show scrollbar when scrolling
    lv_obj_set_style_pad_all(scrollable, 10, 0);           // Add padding for aesthetics
    lv_obj_set_style_bg_color(scrollable, lv_color_black(), 0); // Blank background color

    // Show the title of the screen at the top middle
    lv_obj_t *label = lv_label_create(scrollable);
    lv_label_set_text_static(label, sliderName);
    lv_obj_set_width(label, lv_pct(100));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *spacer = lv_obj_create(scrollable);
    lv_obj_remove_style_all(spacer);
    lv_obj_set_width(spacer, lv_pct(100));
    lv_obj_set_flex_grow(spacer, 2);

    // Create a slider in the center of the display
    lv_obj_t *slider = lv_slider_create(scr);
    lv_obj_center(slider);
    lv_obj_set_user_data(slider, this); // Send "this" as the user data of the slider
    lv_slider_set_value(slider, *sliderValue * 100, LV_ANIM_OFF);
    lv_obj_center(slider);
    lv_obj_add_event_cb(slider, sliderCallback, LV_EVENT_VALUE_CHANGED, sliderValue); // Send the slider value as the event user data
    lv_obj_set_style_anim_duration(slider, 2000, 0);

    // We're in a submenu - include a back button
    lv_obj_t *btn = lv_btn_create(scrollable);
    lv_obj_set_user_data(btn, this);
    lv_obj_set_width(btn, lv_pct(100));
    lv_obj_add_event_cb(btn, handleBackButtonClicked, LV_EVENT_CLICKED, btn);
    label = lv_label_create(btn);
    lv_label_set_text_static(label, MENU_BTN_BACK);
    lv_obj_set_style_text_align(btn, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    screenStack.push_back(scr); // Save the new screen on the stack
    lv_screen_load(scr);        // Activate the new screen
    lvgl_port_unlock();
}

void UserSettings::createRadioList(const char *title,
                                   const char *itemStrings[],
                                   int numOfItems,
                                   const lv_palette_t *itemColors,
                                   lv_event_cb_t radioCallback,
                                   uint8_t *radioValue,
                                   int valueOffset) {
    if (!lvgl_port_lock(0)) {
        return;
    }
    lv_obj_t *scr = lv_obj_create(NULL);

    // Adjust the settings value to be 0-based if needed.
    uint8_t zeroBasedValue = *radioValue;
    if (zeroBasedValue > numOfItems) {
        zeroBasedValue = 0;
    }
    if (zeroBasedValue > 0) {
        zeroBasedValue -= valueOffset;
    }

    lv_group_t *group = lv_group_create();
    lv_group_set_wrap(group, true);
    lv_group_set_default(group);

    // Show the title of the screen at the top middle
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text_static(label, title);
    lv_obj_set_width(label, lv_pct(100));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_pad_all(label, 10, 0);           // Add padding for aesthetics

    // Create a container
    lv_obj_t *cont1 = lv_obj_create(scr);
    lv_obj_set_size(cont1, LV_SIZE_CONTENT, lv_pct(60));
    lv_obj_set_flex_flow(cont1, LV_FLEX_FLOW_COLUMN); // Arrange children in a vertical list
    lv_obj_set_style_pad_all(cont1, 10, 0);           // Add padding for aesthetics
    lv_obj_set_style_bg_color(cont1, lv_color_black(), 0); // Optional background color
    lv_obj_add_event_cb(cont1, radioCallback, LV_EVENT_CLICKED, (void *)radioValue);
    lv_obj_align(cont1, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_user_data(cont1, this); // Pass UserSettings object to the event callback

    // lv_obj_t *scrollToItem = NULL;
    for (int i = 0; i < numOfItems; i++) {
        lv_obj_t * obj = lv_checkbox_create(cont1);
        lv_checkbox_set_text(obj, itemStrings[i]);
        lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_style(obj, &radioStyle, LV_PART_INDICATOR);
        lv_obj_add_style(obj, &radioCheckStyle, LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_group_add_obj(group, obj);
        lv_obj_add_style(obj, &focusedButtonStyle, LV_STATE_FOCUSED);
        if (i == zeroBasedValue) {
            lv_obj_add_state(obj, LV_STATE_CHECKED);
            // scrollToItem = obj;
        }
        if (itemColors != NULL) {
            // Set the color of the checkbox text
            if (itemColors[i] == LV_PALETTE_NONE) {
                lv_obj_set_style_text_color(obj, lv_color_white(), 0);
            } else {
                lv_obj_set_style_text_color(obj, lv_palette_main(itemColors[i]), 0);
            }
        }
    }

    // // Make sure the selected item is in view
    // if (scrollToItem != NULL) {
    //     lv_obj_scroll_to_view(scrollToItem, LV_ANIM_OFF);
    // }

    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_add_style(btn, &focusedButtonStyle, LV_STATE_FOCUSED);
    lv_group_add_obj(group, btn);
    lv_obj_set_user_data(btn, this);
    lv_obj_set_width(btn, lv_pct(100));
    lv_obj_add_event_cb(btn, handleBackButtonClicked, LV_EVENT_CLICKED, btn);
    label = lv_label_create(btn);
    lv_label_set_text_static(label, MENU_BTN_BACK);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_pad_all(btn, 10, 0);

    screenStack.push_back(scr); // Save the new screen on the stack
    lv_screen_load(scr);        // Activate the new screen
    lvgl_port_unlock();
}

/**
 * @brief This is a huge hack. We need some additional way to
 * know about the conversion factor inside of the lv_spinbox_increment_event_cb
 * and lv_spinbox_decrement_event_cb functions but we don't have them. For now
 * only one spinbox is on the screen at a time and so we can get away with this.
 * 
 * IMPORTANT: Make sure to set this when you create the spinbox!
 * 
 * Yuck!
 */
float spinboxConversionFactor = 1.0;

static void lv_spinbox_increment_event_cb(lv_event_t * e) {
    if (!lvgl_port_lock(0)) {
        return;
    }
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t *spinbox = (lv_obj_t *)lv_obj_get_user_data(btn);
    if(code == LV_EVENT_SHORT_CLICKED || code  == LV_EVENT_LONG_PRESSED_REPEAT) {
        lv_spinbox_increment(spinbox);

        float *spinboxValue = (float *)lv_event_get_user_data(e);
        int32_t newValue = lv_spinbox_get_value(spinbox);
        ESP_LOGI(TAG, "New spinbox value: %ld", newValue);
        *spinboxValue = newValue * spinboxConversionFactor;
        ESP_LOGI(TAG, "New settings value: %f", *spinboxValue);
    }
    lvgl_port_unlock();
}

static void lv_spinbox_decrement_event_cb(lv_event_t * e) {
    if (!lvgl_port_lock(0)) {
        return;
    }
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t *spinbox = (lv_obj_t *)lv_obj_get_user_data(btn);
    if(code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
        lv_spinbox_decrement(spinbox);

        float *spinboxValue = (float *)lv_event_get_user_data(e);
        int32_t newValue = lv_spinbox_get_value(spinbox);
        ESP_LOGI(TAG, "New spinbox value: %ld", newValue);
        *spinboxValue = newValue * spinboxConversionFactor;
        ESP_LOGI(TAG, "New settings value: %f", *spinboxValue);
    }
    lvgl_port_unlock();
}

void UserSettings::createSpinbox(const char *title, uint32_t minRange, uint32_t maxRange, uint32_t digitCount, uint32_t separatorPosition, float *spinboxValue, float conversionFactor) {
    if (!lvgl_port_lock(0)) {
        return;
    }
    spinboxConversionFactor = conversionFactor;
    lv_obj_t *scr = lv_obj_create(NULL);

    // lv_obj_set_style_pad_bottom(scr, 10, 0);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0); // Optional background color

    // Show the title of the screen at the top middle
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text_static(label, title);
    lv_obj_set_width(label, lv_pct(100));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t * spinbox = lv_spinbox_create(scr);
    lv_spinbox_set_range(spinbox, minRange, maxRange);
    lv_obj_set_style_text_font(spinbox, &lv_font_montserrat_18, 0);
    lv_spinbox_set_digit_format(spinbox, digitCount, separatorPosition);
    ESP_LOGI(TAG, "Setting initial spinbox value of: %f / %f", *spinboxValue, conversionFactor);
    lv_spinbox_set_value(spinbox, *spinboxValue / conversionFactor);
    lv_spinbox_step_prev(spinbox); // Moves the step (cursor)
    lv_obj_center(spinbox);

    int32_t h = lv_obj_get_height(spinbox);

    lv_obj_t * btn = lv_button_create(scr);
    lv_obj_set_user_data(btn, spinbox);
    lv_obj_set_size(btn, h, h);
    lv_obj_align_to(btn, spinbox, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_obj_set_style_bg_image_src(btn, LV_SYMBOL_PLUS, 0);
    lv_obj_add_event_cb(btn, lv_spinbox_increment_event_cb, LV_EVENT_ALL, spinboxValue);

    btn = lv_button_create(scr);
    lv_obj_set_user_data(btn, spinbox);
    lv_obj_set_size(btn, h, h);
    lv_obj_align_to(btn, spinbox, LV_ALIGN_OUT_LEFT_MID, -5, 0);
    lv_obj_set_style_bg_image_src(btn, LV_SYMBOL_MINUS, 0);
    lv_obj_add_event_cb(btn, lv_spinbox_decrement_event_cb, LV_EVENT_ALL, spinboxValue);

    btn = lv_btn_create(scr);
    lv_obj_set_user_data(btn, this);
    lv_obj_set_width(btn, lv_pct(100));
    lv_obj_add_event_cb(btn, handleBackButtonClicked, LV_EVENT_CLICKED, btn);
    label = lv_label_create(btn);
    lv_label_set_text_static(label, MENU_BTN_BACK);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, 0);

    screenStack.push_back(scr); // Save the new screen on the stack
    lv_screen_load(scr);        // Activate the new screen
    lvgl_port_unlock();
}

void UserSettings::exitSettings() {
    settingsWillExitCallback();

    // Remove all but the first item out of the screenStack.
    while (screenStack.size() > 1) {
        lv_obj_t *scr = screenStack.back();
        lv_obj_clean(scr);  // Clean up sub object memory
        lv_obj_del(scr);

        screenStack.pop_back();
    }

    lv_obj_t *main_screen = screenStack.back();
    lv_screen_load(main_screen);
}

void UserSettings::rotateScreenTo(TunerOrientation newRotation) {
    if (!lvgl_port_lock(0)) {
        return;
    }

    lv_display_rotation_t new_rotation = LV_DISPLAY_ROTATION_0;
    switch (newRotation)
    {
    case orientationNormal:
    ESP_LOGI(TAG, "Rotating to normal");
        new_rotation = LV_DISPLAY_ROTATION_180;
        break;
    case orientationLeft:
    ESP_LOGI(TAG, "Rotating to left");
        new_rotation = LV_DISPLAY_ROTATION_90;
        break;
    case orientationRight:
    ESP_LOGI(TAG, "Rotating to right");
        new_rotation = LV_DISPLAY_ROTATION_270;
        break;
    default:
    ESP_LOGI(TAG, "Rotating to upside down");
        new_rotation = LV_DISPLAY_ROTATION_0;
        break;
    }

    if (lv_display_get_rotation(lvglDisplay) != new_rotation) {
        ESP_ERROR_CHECK(lcd_display_rotate(lvglDisplay, new_rotation));

        // Save this off into user preferences.
        this->displayOrientation = newRotation;
        this->saveSettings();
    }

    lvgl_port_unlock();
}

static void handleExitButtonClicked(lv_event_t *e) {
    ESP_LOGI(TAG, "Exit button clicked");
    tunerController->setState(tunerStateTuning);
}

static void handleTunerButtonClicked(lv_event_t *e) {
    ESP_LOGI(TAG, "Tuner button clicked");
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();
    const char *buttonNames[] = {
        MENU_BTN_TUNER_MODE,
        MENU_BTN_IN_TUNE_THRESHOLD,
        MENU_BTN_MONITORING_MODE,
    };
    lv_event_cb_t callbackFunctions[] = {
        handleTunerModeButtonClicked,
        handleInTuneThresholdButtonClicked,
        handleMonitoringModeButtonClicked,
    };
    settings->createMenu(buttonNames, NULL, NULL, callbackFunctions, 3);
}

static void handleTunerModeButtonClicked(lv_event_t *e) {
    ESP_LOGI(TAG, "Tuner mode button clicked");
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();

    const char **buttonNames = (const char **)malloc(sizeof(const char *) * num_of_available_guis);

    for (int i = 0; i < num_of_available_guis; i++) {
        buttonNames[i] = available_guis[i].get_name();
    }

    settings->createRadioList((const char *)MENU_BTN_TUNER_MODE, 
                               buttonNames,
                               num_of_available_guis,
                               NULL,
                               handleTunerModeSelected,
                               &settings->tunerGUIIndex,
                               0); // This setting is 0-based

    free(buttonNames);
}

static void handleTunerModeSelected(lv_event_t *e) {
    ESP_LOGI(TAG, "Tuner mode clicked");
    if (!lvgl_port_lock(0)) {
        return;
    }
    uint8_t *tunerSetting = (uint8_t *)lv_event_get_user_data(e);

    lv_obj_t * cont = (lv_obj_t *)lv_event_get_current_target(e);
    lv_obj_t * act_cb = (lv_obj_t *)lv_event_get_target(e);
    char *button_text = lv_label_get_text(act_cb);
    lv_obj_t * old_cb = (lv_obj_t *)lv_obj_get_child(cont, *tunerSetting);
    lvgl_port_unlock();

    // Do nothing if the container was clicked
    if(act_cb == cont) return;

    lv_obj_remove_state(old_cb, LV_STATE_CHECKED);   /*Uncheck the previous radio button*/
    lv_obj_add_state(act_cb, LV_STATE_CHECKED);     /*Uncheck the current radio button*/

    for (int i = 0; i < num_of_available_guis; i++) {
        if (strcmp(available_guis[i].get_name(), button_text) == 0) {
            // This is the one!
            ESP_LOGI(TAG, "New Tuner GUI setting: %d", i);
            *tunerSetting = i;
            return;
        }
    }
}

static void handleInTuneThresholdButtonClicked(lv_event_t *e) {
    ESP_LOGI(TAG, "In Tune Threshold button clicked");
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();
    const char *buttonNames[] = {
        "+/- 1 cent",
        "+/- 2 cents",
        "+/- 3 cents",
        "+/- 4 cents",
        "+/- 5 cents",
        "+/- 6 cents"
    };
    settings->createRadioList((const char *)MENU_BTN_IN_TUNE_THRESHOLD, 
                               buttonNames,
                               sizeof(buttonNames) / sizeof(buttonNames[0]),
                               NULL,
                               handleInTuneThresholdRadio,
                               &settings->inTuneCentsWidth,
                               1); // this setting is 1-based instead of 0-based
}

static void handleMonitoringModeButtonClicked(lv_event_t *e) {
    ESP_LOGI(TAG, "Monitoring mode clicked");
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();
    const char *buttonNames[] = {
        MENU_BTN_MONITORING_OFF,
        MENU_BTN_MONITORING_ON,
    };
    settings->createRadioList((const char *)MENU_BTN_MONITORING_MODE, 
                               buttonNames,
                               sizeof(buttonNames) / sizeof(buttonNames[0]),
                               NULL,
                               handleMonitoringModeRadio,
                               &settings->monitoringMode,
                               0); // a 0-based setting
}

static void handleInTuneThresholdRadio(lv_event_t *e) {
    if (!lvgl_port_lock(0)) {
        return;
    }

    uint8_t *thresholdSetting = (uint8_t *)lv_event_get_user_data(e); // this is 1-based
    int32_t radioIndex = ((int32_t)*thresholdSetting) - 1;
    if (radioIndex < 0) {
        radioIndex = 0;
    }

    lv_obj_t * cont = (lv_obj_t *)lv_event_get_current_target(e);
    lv_obj_t * act_cb = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t * old_cb = (lv_obj_t *)lv_obj_get_child(cont, radioIndex);

    // Do nothing if the container was clicked
    if(act_cb == cont) return;

    lv_obj_remove_state(old_cb, LV_STATE_CHECKED);   /*Uncheck the previous radio button*/
    lv_obj_add_state(act_cb, LV_STATE_CHECKED);     /*Uncheck the current radio button*/

    *thresholdSetting = lv_obj_get_index(act_cb) + 1; // Adjust before storing into settings for 1-based values
    ESP_LOGI(TAG, "New In Tune Threshold setting: %d", *thresholdSetting);

    lvgl_port_unlock();
}

static void handleMonitoringModeRadio(lv_event_t *e) {
    if (!lvgl_port_lock(0)) {
        return;
    }

    uint8_t *monitoringSetting = (uint8_t *)lv_event_get_user_data(e);
    int32_t radioIndex = ((int32_t)*monitoringSetting);

    lv_obj_t * cont = (lv_obj_t *)lv_event_get_current_target(e);
    lv_obj_t * act_cb = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t * old_cb = (lv_obj_t *)lv_obj_get_child(cont, radioIndex);

    // Do nothing if the container was clicked
    if(act_cb == cont) return;

    lv_obj_remove_state(old_cb, LV_STATE_CHECKED);   /*Uncheck the previous radio button*/
    lv_obj_add_state(act_cb, LV_STATE_CHECKED);     /*Uncheck the current radio button*/

    *monitoringSetting = lv_obj_get_index(act_cb);
    ESP_LOGI(TAG, "New Monitoring Mode setting: %d", *monitoringSetting);

    lvgl_port_unlock();
}

static void handleDisplayButtonClicked(lv_event_t *e) {
    ESP_LOGI(TAG, "Display button clicked");
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();
    const char *buttonNames[] = {
        MENU_BTN_BRIGHTNESS,
        MENU_BTN_NOTE_COLOR,
        MENU_BTN_INITIAL_SCREEN,
        MENU_BTN_ROTATION,
    };
    lv_event_cb_t callbackFunctions[] = {
        handleBrightnessButtonClicked,
        handleNoteColorButtonClicked,
        handleInitialScreenButtonClicked,
        handleRotationButtonClicked,
    };
    settings->createMenu(buttonNames, NULL, NULL, callbackFunctions, 4);
}

static void handleBrightnessButtonClicked(lv_event_t *e) {
    ESP_LOGI(TAG, "Brightness button clicked");
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();
    const char *buttonNames[] = {
        "10%",
        "20%",
        "30%",
        "40%",
        "50%",
        "60%",
        "70%",
        "80%",
        "90%",
        "100%",
    };
    settings->createRadioList((const char *)MENU_BTN_BRIGHTNESS, 
                               buttonNames,
                               sizeof(buttonNames) / sizeof(buttonNames[0]),
                               NULL,
                               handleBrightnessSelected,
                               &settings->displayBrightness,
                               0); // a 0-based setting

}

static void handleBrightnessSelected(lv_event_t *e) {
    if (!lvgl_port_lock(0)) {
        return;
    }
    uint8_t *brightnessSetting = (uint8_t *)lv_event_get_user_data(e);
    int32_t radioIndex = ((int32_t)*brightnessSetting);
    if (radioIndex < 0) {
        radioIndex = 0;
    }

    lv_obj_t * cont = (lv_obj_t *)lv_event_get_current_target(e);
    lv_obj_t * act_cb = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t * old_cb = (lv_obj_t *)lv_obj_get_child(cont, radioIndex);

    // Do nothing if the container was clicked
    if(act_cb == cont) return;

    lv_obj_remove_state(old_cb, LV_STATE_CHECKED);   /*Uncheck the previous radio button*/
    lv_obj_add_state(act_cb, LV_STATE_CHECKED);     /*Uncheck the current radio button*/

    int32_t newIndex = lv_obj_get_index(act_cb);
    float brightnessValue = (float)newIndex * 10 + 10;

    if (lcd_display_brightness_set(brightnessValue) == ESP_OK) {
        *brightnessSetting = (uint8_t)newIndex;
        ESP_LOGI(TAG, "New display brightness setting is %ld (%f%%)", newIndex, brightnessValue);
    }

    lvgl_port_unlock();
}

static void handleNoteColorButtonClicked(lv_event_t *e) {
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();
    const char *buttonNames[] = {
        "White", // Default
        "Red",
        "Pink",
        "Purple",
        "Blue",
        "Green",
        "Orange",
        "Yellow",
    };
    lv_palette_t buttonColors[] = {
        LV_PALETTE_NONE, // Default
        LV_PALETTE_RED,
        LV_PALETTE_PINK,
        LV_PALETTE_PURPLE,
        LV_PALETTE_LIGHT_BLUE,
        LV_PALETTE_LIGHT_GREEN,
        LV_PALETTE_ORANGE,
        LV_PALETTE_YELLOW,
    };
    settings->currentSettingIndex = settingIndexForPalette(settings->noteNamePalette);
    settings->createRadioList((const char *)MENU_BTN_NOTE_COLOR, 
                               buttonNames,
                               sizeof(buttonNames) / sizeof(buttonNames[0]),
                               buttonColors,
                               handleNoteColorSelected,
                               &settings->currentSettingIndex,
                               0); // a 0-based setting
}

static lv_palette_t paletteForSettingIndex(uint8_t settingIndex) {
    switch (settingIndex) {
        case 0:
            return LV_PALETTE_NONE;
        case 1:
            return LV_PALETTE_RED;
        case 2:
            return LV_PALETTE_PINK;
        case 3:
            return LV_PALETTE_PURPLE;
        case 4:
            return LV_PALETTE_LIGHT_BLUE;
        case 5:
            return LV_PALETTE_LIGHT_GREEN;
        case 6:
            return LV_PALETTE_ORANGE;
        case 7:
            return LV_PALETTE_YELLOW;
        default:
            return LV_PALETTE_NONE;
    }
}

static uint8_t settingIndexForPalette(lv_palette_t palette) {
    switch (palette) {
        case LV_PALETTE_NONE:
            return 0;
        case LV_PALETTE_RED:
            return 1;
        case LV_PALETTE_PINK:
            return 2;
        case LV_PALETTE_PURPLE:
            return 3;
        case LV_PALETTE_LIGHT_BLUE:
            return 4;
        case LV_PALETTE_LIGHT_GREEN:
            return 5;
        case LV_PALETTE_ORANGE:
            return 6;
        case LV_PALETTE_YELLOW:
            return 7;
        default:
            return 0;
    }
}   

static void handleNoteColorSelected(lv_event_t *e) {
    ESP_LOGI(TAG, "Note Color Selection changed");
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    lv_obj_t * cont = (lv_obj_t *)lv_event_get_current_target(e);
    settings = (UserSettings *)lv_obj_get_user_data(cont);

    int32_t radioIndex = ((int32_t)settings->currentSettingIndex);
    if (radioIndex < 0) {
        radioIndex = 0;
    }

    lv_obj_t * act_cb = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t * old_cb = (lv_obj_t *)lv_obj_get_child(cont, radioIndex);

    // Do nothing if the container was clicked
    if(act_cb == cont) return;

    lv_obj_remove_state(old_cb, LV_STATE_CHECKED);   /*Uncheck the previous radio button*/
    lv_obj_add_state(act_cb, LV_STATE_CHECKED);     /*Uncheck the current radio button*/

    int32_t newIndex = lv_obj_get_index(act_cb);
    lv_palette_t palette = paletteForSettingIndex(newIndex);
    settings->noteNamePalette = palette;

    lvgl_port_unlock();
}

static void handleInitialScreenButtonClicked(lv_event_t *e) {
    ESP_LOGI(TAG, "Initial screen button clicked");
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();
    const char *buttonNames[] = {
        MENU_BTN_STANDBY,
        MENU_BTN_TUNING,
    };
    settings->currentSettingIndex = settingIndexForPalette(settings->noteNamePalette);
    settings->createRadioList((const char *)MENU_BTN_INITIAL_SCREEN, 
                               buttonNames,
                               sizeof(buttonNames) / sizeof(buttonNames[0]),
                               NULL,
                               handleInitialStateSelected,
                               (uint8_t *)&settings->initialState,
                               1); // a 1-based setting
}

static void handleInitialStateSelected(lv_event_t *e) {
    ESP_LOGI(TAG, "Set initial screen option selected");
    if (!lvgl_port_lock(0)) {
        return;
    }

    uint8_t *initialStateSetting = (uint8_t *)lv_event_get_user_data(e); // this is 1-based
    int32_t radioIndex = ((int32_t)*initialStateSetting) - 1;
    if (radioIndex < 0) {
        radioIndex = 0;
    }

    lv_obj_t * cont = (lv_obj_t *)lv_event_get_current_target(e);
    lv_obj_t * act_cb = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t * old_cb = (lv_obj_t *)lv_obj_get_child(cont, radioIndex);

    // Do nothing if the container was clicked
    if(act_cb == cont) return;

    lv_obj_remove_state(old_cb, LV_STATE_CHECKED);   /*Uncheck the previous radio button*/
    lv_obj_add_state(act_cb, LV_STATE_CHECKED);     /*Uncheck the current radio button*/

    *initialStateSetting = lv_obj_get_index(act_cb) + 1; // Adjust before storing into settings for 1-based values
    ESP_LOGI(TAG, "New initial screen setting: %d", *initialStateSetting);

    lvgl_port_unlock();
}

static void handleRotationButtonClicked(lv_event_t *e) {
    ESP_LOGI(TAG, "Rotation button clicked");
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();
    const char *buttonNames[] = {
        MENU_BTN_ROTATION_NORMAL,
        MENU_BTN_ROTATION_LEFT,
        MENU_BTN_ROTATION_RIGHT,
        MENU_BTN_ROTATION_UPSIDE_DN,
    };
    
    settings->createRadioList((const char *)MENU_BTN_ROTATION, 
                               buttonNames,
                               sizeof(buttonNames) / sizeof(buttonNames[0]),
                               NULL,
                               handleRotationSelected,
                               (uint8_t *)&settings->displayOrientation,
                               0); // a 0-based setting
}

static void handleRotationSelected(lv_event_t *e) {
    ESP_LOGI(TAG, "Rotation option selected");
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }

    uint8_t *rotationSetting = (uint8_t *)lv_event_get_user_data(e);
    int32_t radioIndex = ((int32_t)*rotationSetting);
    if (radioIndex < 0) {
        radioIndex = 0;
    }

    lv_obj_t * cont = (lv_obj_t *)lv_event_get_current_target(e);
    settings = (UserSettings *)lv_obj_get_user_data(cont);
    lv_obj_t * act_cb = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t * old_cb = (lv_obj_t *)lv_obj_get_child(cont, radioIndex);

    // Do nothing if the container was clicked
    if(act_cb == cont) return;

    lv_obj_remove_state(old_cb, LV_STATE_CHECKED);   /*Uncheck the previous radio button*/
    lv_obj_add_state(act_cb, LV_STATE_CHECKED);     /*Uncheck the current radio button*/

    lvgl_port_unlock();

    *rotationSetting = lv_obj_get_index(act_cb);

    // If the rotation is successful, it's saved in the settings
    settings->rotateScreenTo((TunerOrientation)*rotationSetting);
}

static void handleDebugButtonClicked(lv_event_t *e) {
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();

    const char *buttonNames[] = {
        MENU_BTN_EXP_SMOOTHING,
        MENU_BTN_1EU_BETA,
        MENU_BTN_NAME_DEBOUNCING,
        // MENU_BTN_MOVING_AVG,
    };
    lv_event_cb_t callbackFunctions[] = {
        handleExpSmoothingButtonClicked,
        handle1EUBetaButtonClicked,
        handleNameDebouncingButtonClicked,
        // handleMovingAvgButtonClicked,
    };
    settings->createMenu(buttonNames, NULL, NULL, callbackFunctions, 3);
}

static void handleExpSmoothingButtonClicked(lv_event_t *e) {
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();
    settings->createSpinbox(MENU_BTN_EXP_SMOOTHING, 0, 100, 3, 1, &settings->expSmoothing, 0.01);
}

static void handle1EUBetaButtonClicked(lv_event_t *e) {
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();
    ESP_LOGI(TAG, "Opening 1EU Spinbox with %f", settings->oneEUBeta);
    settings->createSpinbox(MENU_BTN_1EU_BETA, 0, 1000, 4, 1, &settings->oneEUBeta, 0.001);
}

static void handle1EUFilterFirstButtonClicked(lv_event_t *e) {
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();

}

// static void handleMovingAvgButtonClicked(lv_event_t *e) {
//     UserSettings *settings;
//     if (!lvgl_port_lock(0)) {
//         return;
//     }
//     settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
//     lvgl_port_unlock();
//     settings->createSpinbox(MENU_BTN_MOVING_AVG, 1, 1000, 4, 4, &settings->movingAvgWindow, 1);
// }

static void handleNameDebouncingButtonClicked(lv_event_t *e) {
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();
    settings->createSpinbox(MENU_BTN_NAME_DEBOUNCING, 100, 500, 3, 3, &settings->noteDebounceInterval, 1);
}

static void handleAboutButtonClicked(lv_event_t *e) {
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();
    static char versionString[32];
    sprintf(versionString, "Version %s", PROJECT_VERSION);
    const char *buttonNames[] = {
        versionString,
        MENU_BTN_FACTORY_RESET,
    };
    lv_event_cb_t callbackFunctions[] = {
        handleBackButtonClicked,
        handleFactoryResetButtonClicked,
    };
    settings->createMenu(buttonNames, NULL, NULL, callbackFunctions, 2);
}

static void handleFactoryResetChickenOutConfirmed(lv_event_t *e) {
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    lv_obj_t *mbox = (lv_obj_t *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    settings = (UserSettings *)lv_event_get_user_data(e);

    // Handle factory reset logic here
    ESP_LOGI(TAG, "Factory Reset initiated!");
    settings->restoreDefaultSettings();

    // Close the message box
    lv_obj_del(mbox);

    lvgl_port_unlock();
}

static void handleFactoryResetButtonClicked(lv_event_t *e) {
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    
    lv_group_t *group = lv_group_create();
    lv_group_set_wrap(group, true);
    lv_group_set_default(group);

    lv_obj_t * mbox = lv_msgbox_create(lv_scr_act());
    lv_obj_set_style_pad_all(mbox, 10, 0);           // Add padding for aesthetics
    lv_msgbox_add_title(mbox, "Factory Reset");
    lv_msgbox_add_text(mbox, "Reset to factory defaults?");
    lv_obj_t *btn = lv_msgbox_add_footer_button(mbox, "Cancel");
    lv_group_add_obj(group, btn);
    lv_obj_add_style(btn, &settings->focusedButtonStyle, LV_STATE_FOCUSED);
    lv_obj_add_event_cb(btn, [](lv_event_t *e) {
        if (!lvgl_port_lock(0)) {
            return;
        }
        lv_obj_t *mbox = (lv_obj_t *)lv_event_get_user_data(e);
        lv_obj_del(mbox); // closes the mbox

        // Restore the default group so the About menu can be navigated again
        lv_group_t *group = find_group_in_parent(lv_scr_act());
        if (group != NULL) {
            lv_group_set_default(group);
        }

        lvgl_port_unlock();
    }, LV_EVENT_CLICKED, mbox);
    btn = lv_msgbox_add_footer_button(mbox, "Reset");
    lv_group_add_obj(group, btn);
    lv_obj_add_style(btn, &settings->focusedButtonStyle, LV_STATE_FOCUSED);
    lv_obj_set_user_data(btn, mbox);
    lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_event_cb(btn, handleFactoryResetChickenOutConfirmed, LV_EVENT_CLICKED, settings);

    lv_obj_center(mbox);

    lvgl_port_unlock();
}

static void handleBackButtonClicked(lv_event_t *e) {
    ESP_LOGI(TAG, "Back button clicked");
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();
    settings->saveSettings(); // TODO: Figure out a better way of doing this than saving every time
    settings->removeCurrentMenu();
}

void UserSettings::footswitchPressed(FootswitchPress press) {
    switch (press) {
    case footswitchSinglePress:
        ESP_LOGI(TAG, "Settings: Single press");
        advanceToNextButton();
        break;
    case footswitchDoublePress:
        ESP_LOGI(TAG, "Settings: Double press");
        pressFocusedButton();
        break;
    case footswitchLongPress:
        ESP_LOGI(TAG, "Settings: Long press");
        if (screenStack.size() > 2) {
            ESP_LOGI(TAG, "0");
            saveSettings();
            removeCurrentMenu();
        } else {
            ESP_LOGI(TAG, "1");
            // saveSettings();
            tunerController->setState(tunerStateTuning);
        }
        break;
    }
}