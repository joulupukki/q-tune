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

static const char *TAG = "Settings";

extern TunerController *tunerController;
extern TunerGUIInterface available_guis[1]; // defined in tuner_gui_task.cpp
extern size_t num_of_available_guis;

#define MENU_BTN_TUNER              "Tuner"
#define MENU_BTN_TUNER_MODE         "Mode"
#define MENU_BTN_IN_TUNE_THRESHOLD  "In-Tune Threshold"

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
#define SETTING_KEY_NOTE_NAME_PALETTE       "note_nm_palette"
#define SETTING_KEY_DISPLAY_ORIENTATION     "display_orient"
#define SETTING_KEY_EXP_SMOOTHING           "exp_smoothing"
#define SETTING_KEY_ONE_EU_BETA             "one_eu_beta"
#define SETTING_KEY_NOTE_DEBOUNCE_INTERVAL  "note_debounce"
#define SETTING_KEY_USE_1EU_FILTER_FIRST    "oneEUFilter1st"
// #define SETTING_KEY_MOVING_AVG_WINDOW_SIZE  "movingAvgWindow"
#define SETTING_KEY_DISPLAY_BRIGHTNESS      "disp_brightness"

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
static void handleInTuneThresholdButtonValueClicked(lv_event_t *e);
static void handleInTuneThresholdRoller(lv_event_t *e);

static void handleDisplayButtonClicked(lv_event_t *e);
static void handleBrightnessButtonClicked(lv_event_t *e);
static void handleBrightnessSlider(lv_event_t *e);

static void handleNoteColorButtonClicked(lv_event_t *e);
static void handleNoteColorSelected(lv_event_t *e);
static void handleNoteColorWhiteSelected(lv_event_t *e);
static void handleNoteColorRedSelected(lv_event_t *e);
static void handleNoteColorPinkSelected(lv_event_t *e);
static void handleNoteColorPurpleSelected(lv_event_t *e);
static void handleNoteColorBlueSelected(lv_event_t *e);
static void handleNoteColorGreenSelected(lv_event_t *e);
static void handleNoteColorOrangeSelected(lv_event_t *e);
static void handleNoteColorYellowSelected(lv_event_t *e);

static void handleInitialScreenButtonClicked(lv_event_t *e);
static void handleInitialStandbyButtonClicked(lv_event_t *e);
static void handleInitialTuningButtonClicked(lv_event_t *e);

static void handleRotationButtonClicked(lv_event_t *e);
static void handleRotationNormalClicked(lv_event_t *e);
static void handleRotationLeftClicked(lv_event_t *e);
static void handleRotationRightClicked(lv_event_t *e);
static void handleRotationUpsideDnClicked(lv_event_t *e);

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

    if (nvs_get_u8(nvsHandle, SETTING_STANDBY_GUI_INDEX, &value) == ESP_OK) {
        standbyGUIIndex = value;
    } else {
        standbyGUIIndex = DEFAULT_STANDBY_GUI_INDEX;
    }

    if (nvs_get_u8(nvsHandle, SETTING_TUNER_GUI_INDEX, &value) == ESP_OK) {
        tunerGUIIndex = value;
    } else {
        tunerGUIIndex = DEFAULT_TUNER_GUI_INDEX;
    }

    if (nvs_get_u8(nvsHandle, SETTING_KEY_IN_TUNE_WIDTH, &value) == ESP_OK) {
        inTuneCentsWidth = value;
    } else {
        inTuneCentsWidth = DEFAULT_IN_TUNE_CENTS_WIDTH;
    }

    if (nvs_get_u8(nvsHandle, SETTING_KEY_NOTE_NAME_PALETTE, &value) == ESP_OK) {
        noteNamePalette = (lv_palette_t)value;
    } else {
        noteNamePalette = DEFAULT_NOTE_NAME_PALETTE;
    }

    if (nvs_get_u8(nvsHandle, SETTING_KEY_DISPLAY_ORIENTATION, &value) == ESP_OK) {
        displayOrientation = (TunerOrientation)value;
    } else {
        displayOrientation = DEFAULT_DISPLAY_ORIENTATION;
    }

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
        displayBrightness = ((float)value) * 0.01;
    } else {
        displayBrightness = DEFAULT_DISPLAY_BRIGHTNESS;
    }
}

void UserSettings::setIsShowingSettings(bool isShowing) {
    portENTER_CRITICAL(&isShowingMenu_mutex);
    isShowingMenu = isShowing;
    portEXIT_CRITICAL(&isShowingMenu_mutex);
}

void UserSettings::advanceToNextButton() {
    lv_obj_t *screen = screenStack.back();
    lv_obj_t *scrollable = lv_obj_get_child(screen, 0);
    ESP_LOGI(TAG, "advanceToNextButton");
    if (scrollable == NULL) {
        ESP_LOGI(TAG, "advanceToNextButton - scrollable is NULL");
        return;
    }
    int numOfChildren = lv_obj_get_child_cnt(scrollable);
    if (numOfChildren == 0) {
        ESP_LOGI(TAG, "advanceToNextButton - numOfChildren is 0");        
        return;
    }
    lv_obj_t *button = lv_obj_get_child(scrollable, selectedButton);
    if (button == NULL) {
        ESP_LOGI(TAG, "advanceToNextButton - button is NULL");
        return;
    }
    lv_obj_remove_state(button, LV_STATE_FOCUSED);
    selectedButton++;
    if (selectedButton >= numOfChildren) {
        // Go back to the top button
        selectedButton = 0;
    }
    button = lv_obj_get_child(scrollable, selectedButton);
    if (button == NULL) {
        ESP_LOGI(TAG, "advanceToNextButton - next button is NULL");
        return;
    }
    lv_obj_add_state(button, LV_STATE_FOCUSED);
    lv_obj_scroll_to_view(button, LV_ANIM_ON);  // Ensure it's visible
}

void UserSettings::pressFocusedButton() {
    lv_obj_t *screen = screenStack.back();
    lv_obj_t *scrollable = lv_obj_get_child(screen, 0);
    if (scrollable == NULL) {
        ESP_LOGI(TAG, "pressFocusedButton - scrollable is NULL");
        return;
    }
    int numOfChildren = lv_obj_get_child_cnt(scrollable);
    if (numOfChildren == 0) {
        ESP_LOGI(TAG, "pressFocusedButton - numOfChildren is 0");        
        return;
    }
    lv_obj_t *button = lv_obj_get_child(scrollable, selectedButton);
    if (button == NULL) {
        ESP_LOGI(TAG, "pressFocusedButton - button is NULL");
        return;
    }
    lv_obj_send_event(button, LV_EVENT_CLICKED, NULL);
}

//
// PUBLIC Methods
//

UserSettings::UserSettings(settings_will_show_cb_t showCallback, settings_changed_cb_t changedCallback, settings_will_exit_cb_t exitCallback) {
    settingsWillShowCallback = showCallback;
    settingsChangedCallback = changedCallback;
    settingsWillExitCallback = exitCallback;
    loadSettings();
}

bool UserSettings::isShowingSettings() {
    bool isShowing = false;
    portENTER_CRITICAL(&isShowingMenu_mutex);
    isShowing = isShowingMenu;
    portEXIT_CRITICAL(&isShowingMenu_mutex);
    return isShowing;
}

void UserSettings::saveSettings() {
    ESP_LOGI(TAG, "save settings");
    uint8_t value;
    uint32_t value32;

    value = initialState;
    nvs_set_u8(nvsHandle, SETTINGS_INITIAL_SCREEN, value);

    value = standbyGUIIndex;
    nvs_set_u8(nvsHandle, SETTING_STANDBY_GUI_INDEX, value);

    value = tunerGUIIndex;
    nvs_set_u8(nvsHandle, SETTING_TUNER_GUI_INDEX, value);

    value = inTuneCentsWidth;
    nvs_set_u8(nvsHandle, SETTING_KEY_IN_TUNE_WIDTH, value);

    value = (uint8_t)noteNamePalette;
    nvs_set_u8(nvsHandle, SETTING_KEY_NOTE_NAME_PALETTE, value);

    value = (uint8_t)displayOrientation;
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

    value = (uint8_t)(displayBrightness * 100);
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
    setIsShowingSettings(true);
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

    createMenu(buttonNames, symbolNames, NULL, callbackFunctions, 4);
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
    lv_obj_set_style_bg_color(scrollable, lv_palette_darken(LV_PALETTE_BLUE_GREY, 4), 0); // Optional background color

    lv_obj_t *btn;
    lv_obj_t *label;

    int32_t buttonWidthPercentage = 100;
    selectedButton = 0;

    for (int i = 0; i < numOfButtons; i++) {
        ESP_LOGI(TAG, "Creating menu item: %d of %d", i, numOfButtons);
        const char *buttonName = buttonNames[i];
        lv_event_cb_t eventCallback = eventCallbacks[i];
        btn = lv_btn_create(scrollable);
        if (i == selectedButton) {
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

    // Restore the selected button
    selectedButton = 0; // Set the selected button back to the top
    lv_obj_t *scrollable = lv_obj_get_child(parentScreen, 0);
    if (scrollable == NULL) {
        lvgl_port_unlock();
        return;
    }
    int numOfChildren = lv_obj_get_child_cnt(scrollable);
    if (numOfChildren == 0) {
        lvgl_port_unlock();
        return;
    }
    for (int i = 0; i < numOfChildren; i++) {
        // Find the button that was selected
        lv_obj_t *button = lv_obj_get_child(scrollable, i);
        if (button == NULL) {
            lvgl_port_unlock();
            return;
        }
        if (lv_obj_has_state(button, LV_STATE_FOCUSED)) {
            selectedButton = i;
            break;
        }
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
    lv_obj_set_style_bg_color(scrollable, lv_color_black(), 0); // Optional background color

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

void UserSettings::createRoller(const char *title, const char *itemsString, lv_event_cb_t rollerCallback, uint8_t *rollerValue) {
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
    lv_obj_set_style_bg_color(scrollable, lv_color_black(), 0); // Optional background color

    // Show the title of the screen at the top middle
    lv_obj_t *label = lv_label_create(scrollable);
    lv_label_set_text_static(label, title);
    lv_obj_set_width(label, lv_pct(100));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);

    // lv_obj_t *spacer = lv_obj_create(scrollable);
    // lv_obj_remove_style_all(spacer);
    // lv_obj_set_width(spacer, lv_pct(100));
    // lv_obj_set_flex_grow(spacer, 2);

    lv_obj_t * roller = lv_roller_create(scrollable);
    lv_obj_set_user_data(roller, this);
    lv_roller_set_options(roller, itemsString, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(roller, 4);
    lv_roller_set_selected(roller, *rollerValue - 1, LV_ANIM_OFF); // TODO: This "-1" is for the In-Tune Threshold and this function should be made to be more generic
    lv_obj_set_width(roller, lv_pct(100));
    lv_obj_set_flex_grow(roller, 2);
    lv_obj_add_event_cb(roller, rollerCallback, LV_EVENT_ALL, rollerValue);


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
    lv_obj_set_style_text_font(spinbox, &lv_font_montserrat_36, 0);
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
    lv_obj_t *mainScreen = screenStack.front();
    settingsWillExitCallback();

    // Remove all but the first item out of the screenStack.
    while (screenStack.size() > 1) {
        lv_obj_t *scr = screenStack.back();
        lv_obj_clean(scr);  // Clean up sub object memory
        lv_obj_del(scr);

        screenStack.pop_back();
    }

    setIsShowingSettings(false);
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
        new_rotation = LV_DISPLAY_ROTATION_180;
        break;
    case orientationLeft:
        new_rotation = LV_DISPLAY_ROTATION_90;
        break;
    case orientationRight:
        new_rotation = LV_DISPLAY_ROTATION_270;
        break;
    default:
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
    };
    lv_event_cb_t callbackFunctions[] = {
        handleTunerModeButtonClicked,
        handleInTuneThresholdButtonClicked,
    };
    settings->createMenu(buttonNames, NULL, NULL, callbackFunctions, 2);
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
    lv_event_cb_t *callbackFunctions = (lv_event_cb_t *)malloc(sizeof(lv_event_cb_t) * num_of_available_guis);

    for (int i = 0; i < num_of_available_guis; i++) {
        buttonNames[i] = available_guis[i].get_name();
        callbackFunctions[i] = handleTunerModeSelected;
    }

    settings->createMenu(buttonNames, NULL, NULL, callbackFunctions, num_of_available_guis);
    free(buttonNames);
    free(callbackFunctions);
}

static void handleTunerModeSelected(lv_event_t *e) {
    ESP_LOGI(TAG, "Tuner mode clicked");
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));

    // Determine which mode was selected by the name of the button selected
    lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    char *button_text = lv_label_get_text(label);

    lvgl_port_unlock();

    for (int i = 0; i < num_of_available_guis; i++) {
        if (strcmp(available_guis[i].get_name(), button_text) == 0) {
            // This is the one!
            settings->tunerGUIIndex = i;
            settings->removeCurrentMenu(); // Don't make the user click back
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
    settings->createRoller((const char *)MENU_BTN_IN_TUNE_THRESHOLD,
                           (const char *)"+/- 1 cent\n"
                           "+/- 2 cents\n"
                           "+/- 3 cents\n"
                           "+/- 4 cents\n"
                           "+/- 5 cents\n"
                           "+/- 6 cents",
                           handleInTuneThresholdRoller,
                           &settings->inTuneCentsWidth);
}

static void handleInTuneThresholdRoller(lv_event_t *e) {
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    lv_obj_t *roller = (lv_obj_t *)lv_event_get_target(e);
    uint8_t *rollerValue = (uint8_t *)lv_event_get_user_data(e);

    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        uint32_t selectedIndex = lv_roller_get_selected(roller);
        ESP_LOGI(TAG, "In Tune Threshold Roller index selected: %ld", selectedIndex);
        *rollerValue = selectedIndex + 1; // TODO: Make this work for other things too
    }

    lvgl_port_unlock();
}

static void handleInTuneThresholdButtonValueClicked(lv_event_t *e) {
    if (!lvgl_port_lock(0)) {
        return;
    }
    ESP_LOGI(TAG, "In Tune Threshold value clicked");
    lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    if (label == NULL) {
        ESP_LOGI(TAG, "Label is null");
        lvgl_port_unlock();
        return;
    }
    UserSettings *settings = (UserSettings *)lv_obj_get_user_data(btn);
    char *text = lv_label_get_text(label);
    if (strcmp(text, "1") == 0) {
        settings->inTuneCentsWidth = 1;
    } else if (strcmp(text, "2") == 0) {
        settings->inTuneCentsWidth = 2;
    } else if (strcmp(text, "3") == 0) {
        settings->inTuneCentsWidth = 3;
    } else if (strcmp(text, "4") == 0) {
        settings->inTuneCentsWidth = 4;
    } else if (strcmp(text, "5") == 0) {
        settings->inTuneCentsWidth = 5;
    } else if (strcmp(text, "6") == 0) {
        settings->inTuneCentsWidth = 6;
    } else if (strcmp(text, "7") == 0) {
        settings->inTuneCentsWidth = 7;
    } else if (strcmp(text, "8") == 0) {
        settings->inTuneCentsWidth = 8;
    }
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
    ESP_LOGI(TAG, "Brightness slider changed");
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();
    settings->createSlider(MENU_BTN_BRIGHTNESS, 10, 100, handleBrightnessSlider, &settings->displayBrightness);
}

static void handleBrightnessSlider(lv_event_t *e) {
    if (!lvgl_port_lock(0)) {
        return;
    }
    lv_obj_t * slider = (lv_obj_t *)lv_event_get_target(e);
    float *sliderValue = (float *)lv_event_get_user_data(e);
    UserSettings *settings = (UserSettings *)lv_obj_get_user_data(slider);

    uint8_t newValue = (uint8_t)lv_slider_get_value(slider);

    if (lcd_display_brightness_set(newValue) == ESP_OK) {
        *sliderValue = (float)newValue * 0.01;
        ESP_LOGI(TAG, "New slider value: %.2f", *sliderValue);
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
    lv_event_cb_t callbackFunctions[] = {
        handleNoteColorWhiteSelected,
        handleNoteColorRedSelected,
        handleNoteColorPinkSelected,
        handleNoteColorPurpleSelected,
        handleNoteColorBlueSelected,
        handleNoteColorGreenSelected,
        handleNoteColorOrangeSelected,
        handleNoteColorYellowSelected,
    };
    settings->createMenu(buttonNames, NULL, buttonColors, callbackFunctions, 8);
}

static void handleNoteColorSelected(lv_event_t *e, lv_palette_t palette) {
    ESP_LOGI(TAG, "Note Color Selection clicked");
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();
    settings->noteNamePalette = palette;
    settings->saveSettings();
    settings->removeCurrentMenu();
}

static void handleNoteColorWhiteSelected(lv_event_t *e) {
    handleNoteColorSelected(e, LV_PALETTE_NONE);
}

static void handleNoteColorRedSelected(lv_event_t *e) {
    handleNoteColorSelected(e, LV_PALETTE_RED);
}

static void handleNoteColorPinkSelected(lv_event_t *e) {
    handleNoteColorSelected(e, LV_PALETTE_PINK);
}

static void handleNoteColorPurpleSelected(lv_event_t *e) {
    handleNoteColorSelected(e, LV_PALETTE_PURPLE);
}

static void handleNoteColorBlueSelected(lv_event_t *e) {
    handleNoteColorSelected(e, LV_PALETTE_LIGHT_BLUE);
}

static void handleNoteColorGreenSelected(lv_event_t *e) {
    handleNoteColorSelected(e, LV_PALETTE_LIGHT_GREEN);
}

static void handleNoteColorOrangeSelected(lv_event_t *e) {
    handleNoteColorSelected(e, LV_PALETTE_ORANGE);
}

static void handleNoteColorYellowSelected(lv_event_t *e) {
    handleNoteColorSelected(e, LV_PALETTE_YELLOW);
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
    lv_event_cb_t callbackFunctions[] = {
        handleInitialStandbyButtonClicked,
        handleInitialTuningButtonClicked,
    };
    settings->createMenu(buttonNames, NULL, NULL, callbackFunctions, 2);
}

static void handleInitialStandbyButtonClicked(lv_event_t *e) {
    ESP_LOGI(TAG, "Set initial screen as standby button clicked");
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();
    settings->initialState = tunerStateStandby;
    settings->removeCurrentMenu(); // Automatically press the back button
}

static void handleInitialTuningButtonClicked(lv_event_t *e) {
    ESP_LOGI(TAG, "Set initial screen as tuning button clicked");
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();
    settings->initialState = tunerStateTuning;
    settings->removeCurrentMenu(); // Automatically press the back button
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
    lv_event_cb_t callbackFunctions[] = {
        handleRotationNormalClicked,
        handleRotationLeftClicked,
        handleRotationRightClicked,
        handleRotationUpsideDnClicked,
    };
    settings->createMenu(buttonNames, NULL, NULL, callbackFunctions, 4);
}

static void handleRotationNormalClicked(lv_event_t *e) {
    ESP_LOGI(TAG, "Rotation Normal clicked");
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();
    settings->rotateScreenTo(orientationNormal);
}

static void handleRotationLeftClicked(lv_event_t *e) {
    ESP_LOGI(TAG, "Rotation Left clicked");
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();
    settings->rotateScreenTo(orientationLeft);
}

static void handleRotationRightClicked(lv_event_t *e) {
    ESP_LOGI(TAG, "Rotation Right clicked");
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();
    settings->rotateScreenTo(orientationRight);
}

static void handleRotationUpsideDnClicked(lv_event_t *e) {
    ESP_LOGI(TAG, "Rotation Upside Down clicked");
    UserSettings *settings;
    if (!lvgl_port_lock(0)) {
        return;
    }
    settings = (UserSettings *)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
    lvgl_port_unlock();
    settings->rotateScreenTo(orientationUpsideDown);
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
    const char *buttonNames[] = {
        "Version 0.0.1", // TODO: Grab the version from somewhere else
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

    lv_obj_t * mbox = lv_msgbox_create(lv_scr_act());
    lv_obj_set_style_pad_all(mbox, 10, 0);           // Add padding for aesthetics
    lv_msgbox_add_title(mbox, "Factory Reset");
    lv_msgbox_add_text(mbox, "Reset to factory defaults?");
    lv_obj_t *btn = lv_msgbox_add_footer_button(mbox, "Reset");
    lv_obj_set_user_data(btn, mbox);
    lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_event_cb(btn, handleFactoryResetChickenOutConfirmed, LV_EVENT_CLICKED, settings);
    btn = lv_msgbox_add_footer_button(mbox, "Cancel");
    lv_obj_add_event_cb(btn, [](lv_event_t *e) {
        if (!lvgl_port_lock(0)) {
            return;
        }
        lv_obj_t *mbox = (lv_obj_t *)lv_event_get_user_data(e);
        lv_obj_del(mbox); // closes the mbox
        lvgl_port_unlock();
    }, LV_EVENT_CLICKED, mbox);

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
            removeCurrentMenu();
        } else {
            tunerController->setState(tunerStateTuning);
        }
        break;
    }
}