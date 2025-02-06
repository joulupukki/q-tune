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
#include "tuner_controller.h"

#include "esp_log.h"

static const char *TAG = "CONTROLLER";

TunerController::TunerController(tuner_state_will_change_cb_t willChange,
                tuner_state_did_change_cb_t didChange,
                tuner_footswitch_pressed_cb_t footswitchPressed) {
    tunerState = tunerStateBooting;
    stateWillChangeCallback = willChange;
    stateDidChangeCallback = didChange;
    footswitchPressedCallback = footswitchPressed;
}

TunerState TunerController::getState() {
    TunerState state = tunerStateStandby;
    portENTER_CRITICAL(&tuner_state_mutex);
    state = tunerState;
    portEXIT_CRITICAL(&tuner_state_mutex);
    return state;
}

void TunerController::setState(TunerState new_state) {
    TunerState old_state = getState();
    stateWillChangeCallback(old_state, new_state);
    portENTER_CRITICAL(&tuner_state_mutex);
    tunerState = new_state;
    portEXIT_CRITICAL(&tuner_state_mutex);
    stateDidChangeCallback(old_state, new_state);
}

void TunerController::footswitchPressed(FootswitchPress press) {
    footswitchPressedCallback(press);
}