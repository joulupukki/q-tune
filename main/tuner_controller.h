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
#if !defined(TUNER_STATE)
#define TUNER_STATE

#include "freertos/FreeRTOS.h"

enum TunerState: uint8_t {
    tunerStateBooting = 0,
    tunerStateStandby,      // Standby
    tunerStateTuning,       // Actively tuning (or monitored mode)
    tunerStateSettings,     // User settings are showing
};

enum FootswitchPress: uint8_t {
    footswitchSinglePress,
    footswitchDoublePress,
    footswitchLongPress,
};

/// @brief Called before the state of the tuner changes.
/// @param new_state Indicates the state that the tuner is changing to.
typedef void (*tuner_state_will_change_cb_t)(TunerState old_state, TunerState new_state);

/// @brief Called immediately after the state of the tuner changes.
/// @param new_state Indicates the state that the tuner changed to.
typedef void (*tuner_state_did_change_cb_t)(TunerState old_state, TunerState new_state);

/// @brief Called when the momentary foot switch is pressed.
/// @param press Indicates the type of press.
typedef void (*tuner_footswitch_pressed_cb_t)(FootswitchPress press);

class TunerController {

    TunerState tunerState;

    portMUX_TYPE tuner_state_mutex = portMUX_INITIALIZER_UNLOCKED;

    tuner_state_will_change_cb_t    stateWillChangeCallback;
    tuner_state_did_change_cb_t     stateDidChangeCallback;
    tuner_footswitch_pressed_cb_t   footswitchPressedCallback;

public:

    TunerController(tuner_state_will_change_cb_t willChange, tuner_state_did_change_cb_t didChange, tuner_footswitch_pressed_cb_t footswitchPressed);

    /// @brief Gets the tuner's current state (thread safe).
    /// @return The state.
    TunerState getState();

    /// @brief Sets the tuner state (thread safe).
    /// @param new_state The new state.
    void setState(TunerState new_state);

    /// @brief Called when the momentary foot switch is pressed.
    /// @param press Indicates the type of press.
    void footswitchPressed(FootswitchPress press);
};

#endif
