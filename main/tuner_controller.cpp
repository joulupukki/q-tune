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
#include "defines.h"

#include "esp_log.h"

static const char *TAG = "CONTROLLER";

TunerController::TunerController(tuner_state_will_change_cb_t willChange,
                tuner_state_did_change_cb_t didChange,
                tuner_footswitch_pressed_cb_t footswitchPressed) {
    stateWillChangeCallback = willChange;
    stateDidChangeCallback = didChange;
    footswitchPressedCallback = footswitchPressed;

    TunerState initialState = tunerStateBooting;
    tunerStateQueue = xQueueCreate(TUNER_STATE_QUEUE_LENGTH, TUNER_STATE_QUEUE_ITEM_SIZE);
    xQueueOverwrite(tunerStateQueue, &initialState);
}

TunerState TunerController::getState() {
    TunerState state;
    if (!xQueuePeek(tunerStateQueue, &state, 0)) {
        state = tunerStateStandby;
    }
    return state;
}

void TunerController::setState(TunerState new_state) {
    ESP_LOGI(TAG, "1");
    TunerState old_state = getState();
    ESP_LOGI(TAG, "2");
    stateWillChangeCallback(old_state, new_state);
    ESP_LOGI(TAG, "3");
    xQueueOverwrite(tunerStateQueue, &new_state);
    ESP_LOGI(TAG, "4");
    stateDidChangeCallback(old_state, new_state);
    ESP_LOGI(TAG, "5");
}

void TunerController::footswitchPressed(FootswitchPress press) {
    footswitchPressedCallback(press);
}