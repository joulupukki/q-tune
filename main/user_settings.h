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
#if !defined(TUNER_USER_SETTINGS)
#define TUNER_USER_SETTINGS

#include <vector>
#include <cstdint>
#include <cstring>

#include "tuner_controller.h"

#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

extern "C" { // because these files are C and not C++
    #include "lcd.h"
}

#include "nvs_flash.h"
#include "nvs.h"

#include "defines.h"

enum TunerOrientation: uint8_t {
    orientationNormal,
    orientationLeft,
    orientationRight,
    orientationUpsideDown,
};

typedef void (*settings_will_show_cb_t)();
typedef void (*settings_changed_cb_t)();
typedef void (*settings_will_exit_cb_t)();

/// @brief A class used to display and manage user settings.
class UserSettings {
    /**
     * @brief Keeps track of the stack of screens currently being displayed.
     * 
     * This class is initialized with the main screen and that screen should
     * always be at position 0 in the stack. As new sub menus are added to
     * the UI, they should be added to `screenStack` and as menus exit, they
     * should be removed. The current screen is always the last item in the
     * `screenStack`.
     */
    std::vector<lv_obj_t*> screenStack;
    lv_display_t *lvglDisplay;

    nvs_handle_t    nvsHandle;
    bool isShowingMenu = false;

    lv_style_t focusedButtonStyle;
    lv_style_t radioStyle;
    lv_style_t radioCheckStyle;

    settings_will_show_cb_t settingsWillShowCallback;
    settings_changed_cb_t settingsChangedCallback;
    settings_will_exit_cb_t settingsWillExitCallback;

    /// @brief Loads settings from persistent storage.
    void loadSettings();

    /// @brief Set whether the menu is showing (thread safe).
    /// @param isShowing 
    void setIsShowingSettings(bool isShowing);

    void advanceToNextButton();
    void pressFocusedButton();

public:
    // User Setting Variables
    TunerState          initialState            = DEFAULT_INITIAL_STATE;
    uint8_t             standbyGUIIndex         = DEFAULT_STANDBY_GUI_INDEX;
    uint8_t             tunerGUIIndex           = DEFAULT_TUNER_GUI_INDEX; // The ID is also the index in the `available_guis` array.
    uint8_t             inTuneCentsWidth        = DEFAULT_IN_TUNE_CENTS_WIDTH;
    lv_palette_t        noteNamePalette         = DEFAULT_NOTE_NAME_PALETTE;
    TunerOrientation    displayOrientation      = DEFAULT_DISPLAY_ORIENTATION;
    float               displayBrightness       = DEFAULT_DISPLAY_BRIGHTNESS;

    float               expSmoothing            = DEFAULT_EXP_SMOOTHING;
    float               oneEUBeta               = DEFAULT_ONE_EU_BETA;
    float               noteDebounceInterval    = DEFAULT_NOTE_DEBOUNCE_INTERVAL;
    bool                use1EUFilterFirst       = DEFAULT_USE_1EU_FILTER_FIRST;
//    float               movingAvgWindow         = DEFAULT_MOVING_AVG_WINDOW;

    /**
     * @brief Create the settings object and sets its parameters
     */
    UserSettings(settings_will_show_cb_t showCallback, settings_changed_cb_t changedCallback, settings_will_exit_cb_t exitCallback);

    /// @brief Know if the settings menu is being shown (thread safe).
    /// @return Returns `true` if the settings menu is currently showing.
    bool isShowingSettings();
    portMUX_TYPE isShowingMenu_mutex = portMUX_INITIALIZER_UNLOCKED;

    /**
     * @brief Saves settings to persistent storage.
     */
    void saveSettings();

    void restoreDefaultSettings();
    
    /**
     * @brief Get the user setting for display orientation.
     */
    lv_display_rotation_t getDisplayOrientation();
    void setDisplayBrightness(float newBrightness);

    /**
     * @brief Gives UserSettings a handle to the main display and the main screen.
     * @param display Handle to the main display. Used for screen rotation.
     * @param screen Handle to the main screen. Used to close the settings menu.
     */
    void setDisplayAndScreen(lv_display_t *display, lv_obj_t *screen);

    /**
     * @brief Pause tuning or standby mode and show the settings menu/screen.
     */
    void showSettings();

    void createMenu(const char *buttonNames[], const char *buttonSymbols[], lv_palette_t *buttonColors, lv_event_cb_t eventCallbacks[], int numOfButtons);
    void removeCurrentMenu();
    void createSlider(const char *sliderName, int32_t minRange, int32_t maxRange, lv_event_cb_t sliderCallback, float *sliderValue);

    /// @brief Create a screen that shows a radio list.
    /// @param title The title of the radio list/setting.
    /// @param itemStrings The items to show in the radio list.
    /// @param numOfItems Number of items in the `itemStrings` array.
    /// @param radioCallback A callback function that will be called when a radio item is selected.
    /// @param radioValue Pointer to the user setting (1-based index).
    /// @param valueOffset The offset to add to the radio value when saving it to the user settings.
    void createRadioList(const char *title, const char *itemStrings[], int numOfItems, lv_event_cb_t radioCallback, uint8_t *radioValue, int valueOffset);
    void createRoller(const char *title, const char *itemsString, lv_event_cb_t rollerCallback, uint8_t *rollerValue);
    void createSpinbox(const char *title, uint32_t minRange, uint32_t maxRange, uint32_t digitCount, uint32_t separatorPosition, float *spinboxValue, float conversionFactor);

    /**
     * @brief Exit the settings menu/screen and resume tuning/standby mode.
     */
    void exitSettings();

    void rotateScreenTo(TunerOrientation newRotation);

    void footswitchPressed(FootswitchPress press);
};

#endif