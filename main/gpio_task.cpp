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
#include "gpio_task.h"

#include "defines.h"
#include "tuner_controller.h"
#include "user_settings.h"

#include "lvgl.h"
// #include "esp_lvgl_port.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

// #include "multi_button.h"

static const char *TAG = "GPIO";

extern TunerController *tunerController;
extern QueueHandle_t bypassTypeQueue;
extern QueueHandle_t bypassTypeSettingsScreenQeuue;

// Keep track of the relay states so the app doesn't have to set them over and over
// making calls to set it over and over.
uint32_t current_bypass_relay_level = 0; // Off at launch
uint32_t current_bypass_type_relay_level = 0; // Off at launch

int footswitch_last_state = 1; // Assume initial state is open (active-high logic and conveniently matches the physical position of the footswitch)
int footswitch_press_count = 0;
int64_t footswitch_start_time = 0;
int64_t footswitch_last_press_time = 0;
bool footswitch_long_press_triggered = false;
esp_timer_handle_t single_press_timer;

// struct Button footswitchButton;
// PressEvent footswitch_btn_state;

//
// Local Function Declarations
//
void configure_gpio_pins();
void handle_button_press();
void ensure_relay_state();
void handle_single_press(void *param);
void handle_double_press(void *param);
void handle_long_press(void *param);
void single_press_timer_callback(void* arg);
void start_single_press_timer();
void cancel_single_press_timer();

void gpio_task(void *pvParameter) {
    ESP_LOGI(TAG, "GPIO task started");
    configure_gpio_pins();

    // double last_time = 0.0;

    while(1) {
        handle_button_press();
        ensure_relay_state();

        // int64_t time_us = esp_timer_get_time(); // Get time in microseconds
        // double time_ms = time_us / 1000; // Convert to milliseconds
        // double time_seconds = time_us / 1000000;    // Convert to seconds
        // if ((int)time_seconds % 2 == 0 && (int)time_seconds != (int)last_time) {
        //     ESP_LOGI(TAG, "Time: %f", time_seconds);
        //     last_time = time_seconds;

        //     int current_footswitch_state = gpio_get_level(FOOT_SWITCH_GPIO);
        //     ESP_LOGI(TAG, "Footswitch State: %d", current_footswitch_state);
        // }

        vTaskDelay(pdMS_TO_TICKS(50)); // Small delay for debouncing
    }
    vTaskDelay(portMAX_DELAY);
}

void configure_gpio_pins() {
    // Configure the Footswitch GPIO as an input pin
    gpio_reset_pin(FOOT_SWITCH_GPIO);
    gpio_config_t foot_switch_gpio_conf = {
        .pin_bit_mask = (1ULL << FOOT_SWITCH_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,   // Enable internal pull-up resistor
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE      // Disable interrupt for this pin
    };

    gpio_config(&foot_switch_gpio_conf);

    // Configure the bypass relay GPIO as an output pin
    gpio_reset_pin(BYPASS_RELAY_GPIO);
    gpio_config_t relay_gpio_conf = {
        .pin_bit_mask = (1ULL << BYPASS_RELAY_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&relay_gpio_conf);

    // Configure the bypass type GPIO as an output pin
    gpio_reset_pin(BYPASS_TYPE_RELAY_GPIO);
    gpio_config_t bypass_type_gpio_conf = {
        .pin_bit_mask = (1ULL << BYPASS_TYPE_RELAY_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&bypass_type_gpio_conf);

    // TODO: Read the initial state of the foot switch. If it's a 0, that means
    // the user had it pressed at power up and we may want to do something
    // special.

    // Make sure the relays start out as off
    gpio_set_level(BYPASS_RELAY_GPIO, 0); // Turn off the relay GPIO
    current_bypass_relay_level = 0;
    gpio_set_level(BYPASS_TYPE_RELAY_GPIO, 0);
    current_bypass_type_relay_level = 0;
    ESP_LOGI(TAG, "GPIO Pins Configured");
}

void handle_button_press() {
    // Detect press, double-press, and long press.
    int current_footswitch_state = gpio_get_level(FOOT_SWITCH_GPIO);

    if (current_footswitch_state == 0 && footswitch_last_state == 1) {
        // Button pressed
        footswitch_start_time = esp_timer_get_time() / 1000; // Get time in ms
        int64_t now = footswitch_start_time;

        if ((now - footswitch_last_press_time) <= DOUBLE_PRESS_TIME_MS) {
            footswitch_press_count++;
        } else {
            footswitch_press_count = 1;
        }

        footswitch_last_press_time = now;
        footswitch_long_press_triggered = false; // Reset long press trigger when pressed
    }

    if (current_footswitch_state == 0) {
        // Button is being held down
        int64_t press_duration = (esp_timer_get_time() / 1000) - footswitch_start_time;

        if (press_duration >= LONG_PRESS_TIME_MS && !footswitch_long_press_triggered) {
            lv_async_call(handle_long_press, NULL);
            footswitch_long_press_triggered = true; // Ensure long press is only triggered once
            footswitch_press_count = 0; // Reset press count after a long press
            vTaskDelay(pdMS_TO_TICKS(200)); // Small delay for visual feedback
        }
    }

    if (current_footswitch_state == 1 && footswitch_last_state == 0) {
        // Button released
        if (!footswitch_long_press_triggered) {
            int64_t press_duration = (esp_timer_get_time() / 1000) - footswitch_start_time;

            if (footswitch_press_count == 2) {
                cancel_single_press_timer(); // kill the timer if it's running
                lv_async_call(handle_double_press, NULL);
                footswitch_press_count = 0; // Reset press count after double press
                vTaskDelay(pdMS_TO_TICKS(200)); // Small delay for visual feedback
            } else if (press_duration < LONG_PRESS_TIME_MS && footswitch_press_count == 1) {
                if (tunerController->getState() == tunerStateSettings) {
                    start_single_press_timer();
                } else {
                    // Fire a single press right away
                    single_press_timer_callback(NULL);
                }
            } else {
                footswitch_press_count = 0; // Reset press count if it ever hits this condition
            }
        }
    }

    footswitch_last_state = current_footswitch_state;    
}

/// The purpose of this function is to make sure the relay is in the correct
/// state according to the tuner's current state. This is really only needed
/// for initial startup. We want to keep the controlling of the relay all done
/// here in gpio_task but if the user has tuning set as their initial startup
/// screen, we need to make sure to turn the relay on.
void ensure_relay_state() {
    // Make sure the main bypass/tuning relay is in the correct state

    bool is_showing_bypass_type_settings_screen = false;
    TunerState current_state = tunerController->getState();
    if (current_state == tunerStateStandby && current_bypass_relay_level != 0) {
        gpio_set_level(BYPASS_RELAY_GPIO, 0); // Turn off relay
        current_bypass_relay_level = 0;
        ESP_LOGI(TAG, "Turning OFF the bypass relay (going to standby mode)");
    } else if (current_state == tunerStateTuning && current_bypass_relay_level != 1) {
        gpio_set_level(BYPASS_RELAY_GPIO, 1); // Turn on relay
        current_bypass_relay_level = 1;
        ESP_LOGI(TAG, "Turning ON the bypass relay (going to tuning mode)");
    } else if (current_state == tunerStateSettings && xQueuePeek(bypassTypeSettingsScreenQeuue, &is_showing_bypass_type_settings_screen, 0) == pdTRUE) {
        if (is_showing_bypass_type_settings_screen) {
            // If we're in the bypass type settings screen, we want to turn off the relay
            // so the user can hear the difference between true and buffered bypass.
            if (current_bypass_relay_level != 0) {
                gpio_set_level(BYPASS_RELAY_GPIO, 0); // Turn off relay
                current_bypass_relay_level = 0;
                ESP_LOGI(TAG, "Turning OFF the bypass relay (bypass type settings screen active)");
            }
        } else {
            // If we're not in the bypass type settings screen, we want to
            // turn on the relay so that output is muted.
            if (current_bypass_relay_level != 1) {
                gpio_set_level(BYPASS_RELAY_GPIO, 1); // Turn on relay
                current_bypass_relay_level = 1;
                ESP_LOGI(TAG, "Turning ON the bypass relay (bypass type settings screen NOT active)");
            }
        }
    }

    // Make sure the bypass type relay is in the correct state
    TunerBypassType current_bypass_type;
    if (xQueuePeek(bypassTypeQueue, &current_bypass_type, 0) == pdTRUE) {
        if (current_bypass_type == tunerBypassTypeTrue && current_bypass_type_relay_level != 0) {
            gpio_set_level(BYPASS_TYPE_RELAY_GPIO, 0);
            current_bypass_type_relay_level = 0;
            ESP_LOGI(TAG, "Turning OFF the bypass type relay (going to true bypass mode)");
        } else if (current_bypass_type == tunerBypassTypeBuffered && current_bypass_type_relay_level != 1) {
            gpio_set_level(BYPASS_TYPE_RELAY_GPIO, 1);
            current_bypass_type_relay_level = 1;
            ESP_LOGI(TAG, "Turning ON the bypass type relay (going to buffered bypass mode)");
        }
    }
}

// Called on the LVGL task thread (tuner_gui_task).
void handle_single_press(void *param) {
    ESP_LOGI(TAG, "NORMAL PRESS detected");

    TunerState state = tunerController->getState();
    switch (state) {
    case tunerStateStandby:
        ESP_LOGI(TAG, "Turning ON the relay and going to tuning mode");
        // Turn on the relay which should mute the output
        gpio_set_level(BYPASS_RELAY_GPIO, 1); // Turn on relay
        current_bypass_relay_level = 1;
        
        // Go to tuning mode
        tunerController->setState(tunerStateTuning);
        break;
    case tunerStateTuning:
        ESP_LOGI(TAG, "Turning OFF the relay and going to standby mode");
        // Turn off the relay which should unmute the output
        gpio_set_level(BYPASS_RELAY_GPIO, 0); // Turn off relay
        current_bypass_relay_level = 0;

        // Go to standby mode
        tunerController->setState(tunerStateStandby);
        break;
    case tunerStateSettings:
        tunerController->footswitchPressed(footswitchSinglePress);
        break;
    default:
        break;
    }
}

// Called on the LVGL task thread (tuner_gui_task).
void handle_double_press(void *param) {
    ESP_LOGI(TAG, "DOUBLE PRESS detected");
    tunerController->footswitchPressed(footswitchDoublePress);
}

// Called on the LVGL task thread (tuner_gui_task).
void handle_long_press(void *param) {
    ESP_LOGI(TAG, "LONG PRESS detected");
    tunerController->footswitchPressed(footswitchLongPress);
}

void single_press_timer_callback(void* arg) {
    lv_async_call(handle_single_press, NULL);
    footswitch_press_count = 0; // Reset press count after single press
    vTaskDelay(pdMS_TO_TICKS(200)); // Small delay for visual feedback
}

void start_single_press_timer() {
    if (single_press_timer != NULL) {
        esp_timer_stop(single_press_timer);
        esp_timer_delete(single_press_timer);
    }

    const esp_timer_create_args_t single_press_timer_args = {
        .callback = &single_press_timer_callback,
        .name = "single_press_timer"
    };

    esp_timer_create(&single_press_timer_args, &single_press_timer);
    esp_timer_start_once(single_press_timer, DOUBLE_PRESS_TIME_MS * 1000); // Convert ms to us
}

void cancel_single_press_timer() {
    if (single_press_timer != NULL) {
        esp_timer_stop(single_press_timer);
        esp_timer_delete(single_press_timer);
        single_press_timer = NULL;
    }
}
