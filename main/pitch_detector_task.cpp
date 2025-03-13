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
#include "pitch_detector_task.h"

#include "defines.h"
#include "globals.h"
#include "user_settings.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
// #include "esp_adc/adc_continuous.h"
#include "esp_timer.h"

//
// ADS1015
//
#include "driver/i2c_master.h"
#include "ads101x.h"

//
// Q DSP Library for Pitch Detection
//
#include <q/pitch/pitch_detector.hpp>
#include <q/fx/dynamic.hpp>
#include <q/fx/clip.hpp>
#include <q/fx/signal_conditioner.hpp>
#include <q/support/decibel.hpp>
#include <q/support/duration.hpp>
#include <q/support/literals.hpp>
#include <q/support/pitch_names.hpp>
#include <q/support/pitch.hpp>

//
// Smoothing Filters
//
#include "exponential_smoother.hpp"
#include "OneEuroFilter.h"
// #include "MovingAverage.hpp"
#include "MedianFilter.hpp"

static const char *TAG = "PitchDetector";

namespace q = cycfi::q;
using namespace q::literals;
using std::fixed;

using namespace cycfi::q::pitch_names;
using frequency = cycfi::q::frequency;
using pitch = cycfi::q::pitch;
CONSTEXPR frequency low_fs = cycfi::q::pitch_names::C[1]; // 32.7 Hz
CONSTEXPR frequency high_fs = cycfi::q::pitch_names::C[7]; // 2.093 kHz - Setting this higher helps to catch the high harmonics

// 1EU Filter Initialization Params
static const double euFilterFreq = EU_FILTER_ESTIMATED_FREQ; // I believe this means no guess as to what the incoming frequency will initially be
static const double mincutoff = EU_FILTER_MIN_CUTOFF;
static const double dcutoff = EU_FILTER_DERIVATIVE_CUTOFF;

ExponentialSmoother smoother(DEFAULT_EXP_SMOOTHING);
OneEuroFilter oneEUFilter(euFilterFreq, mincutoff, DEFAULT_ONE_EU_BETA, dcutoff);
// MovingAverage movingAverage(DEFAULT_MOVING_AVG_WINDOW);
// MedianFilter medianMovingFilter(3, true);
MedianFilter medianFilter(5, false);

extern UserSettings *userSettings;
extern QueueHandle_t frequencyQueue;
extern i2c_master_bus_handle_t i2c_bus_handle;

//
// External ADS1015 ADC
//

// We use alternating buffers to read ADC values from the external ADS1015 ADC.
// The values are read in with an interrupt. The ADS1015 is put into continuous
// reading mode and each time a value is read, it alerts us with an interrupt.
// When the interrupt happens, we read the value and add it to the current adc
// buffer. When the buffer is filled, we pass that buffer to the pitch detection
// function and start filling the next buffer.
static ads101x_t adc;
static int16_t *adc_buffers[TUNER_ADC_NUM_OF_BUFFERS];
static int current_buffer_index = 0;
static int adc_values_read = 0;
static QueueHandle_t adcBufferQueue;

typedef struct {
    int16_t *data;
    int length;
    int64_t elapsedTime;
} adc_buffer_t;

void adc_delay_ms(uint32_t time) {
    vTaskDelay(pdMS_TO_TICKS(time));
}

int64_t lastTime = esp_timer_get_time();
int64_t elapsedTime = 0;
void buffer_fill_timer_callback(void *arg) {
    int16_t adc_result = 0;
    int16_t *current_buffer = adc_buffers[current_buffer_index];

    // Wait for a value to be ready
    if (ads101x_conversion_complete(&adc) != ESP_OK) {
        return;
    }

    esp_err_t e = ads101x_get_last_conversion_results(&adc, &adc_result);
    if (e == ESP_OK) {
        current_buffer[adc_values_read] = adc_result;
        adc_values_read++;
    }

    if (adc_values_read >= TUNER_ADC_BUFFER_SIZE) {
        int64_t currentTime = esp_timer_get_time();
        elapsedTime = currentTime - lastTime;
        lastTime = currentTime;

        // The buffer is full. Pass it to the pitch detector and switch to the
        // other buffer so continuous reading can keep happening.
        adc_buffer_t buffer = {
            .data = current_buffer,
            .length = TUNER_ADC_BUFFER_SIZE,
            .elapsedTime = elapsedTime,
        };
        xQueueSend(adcBufferQueue, &buffer, 0);
        adc_values_read = 0;
        current_buffer_index++;
        if (current_buffer_index >= TUNER_ADC_NUM_OF_BUFFERS) {
            current_buffer_index = 0;
        }
        memset(adc_buffers[current_buffer_index], 0x00, sizeof(int16_t) * TUNER_ADC_BUFFER_SIZE);
    }
}

/// @brief Initialize the external ADC chip.
esp_err_t adc_init() {
    ESP_LOGI(TAG, "Initializing the ADS1015 external ADC");
    ESP_ERROR_CHECK(ads101x_init(&adc, ADS101X_MODEL_5, i2c_bus_handle, ADS101X_I2C_ADDRESS, adc_delay_ms));
    ads101x_set_data_rate(&adc, ADS101X_DATA_RATE_3300SPS);
    ads101x_set_gain(&adc, ADS101X_GAIN_ONE);
    ads101x_interrupt_enable(&adc, TUNER_ADC_ALERT_PIN);

    // Allocate the buffers
    for (int i = 0; i < TUNER_ADC_NUM_OF_BUFFERS; i++) {
        adc_buffers[i] = (int16_t *)malloc(sizeof(int16_t) * TUNER_ADC_BUFFER_SIZE);
        memset(adc_buffers[i], 0x00, TUNER_ADC_BUFFER_SIZE);
    }
    
    // Set up a queue that we will use to signal the pitch detector to do work.
    // The external ADC notifies us on an interrupt when a value is ready to
    // read. The buffer fills up and when it's ready, this is signaled for the
    // pitch detector to read the buffer. The value passed is the pointer to
    // the buffer.
    adcBufferQueue = xQueueCreate(1, sizeof(adc_buffer_t));

    return ESP_OK;
}

esp_timer_handle_t adc_buffer_timer;

/// @brief Start continuous reading of the external ADC
esp_err_t adc_start() {
    // Start continuous differential reading of the difference between pins 0 and 1.
    esp_err_t e = ads101x_start_reading(&adc, ADS101X_REG_CONFIG_MUX_DIFF_0_1, ADS101X_MODE_CONTINUOUS);

    const esp_timer_create_args_t timer_args = {
        .callback = &buffer_fill_timer_callback,
        .name = "adc-buffer-timer"
    };

    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &adc_buffer_timer));
    e = esp_timer_start_periodic(adc_buffer_timer, TUNER_ADC_SAMPLE_INTERVAL);
    ESP_LOGI(TAG, "Timer start result: %s", esp_err_to_name(e));
    return e;
}

auto sc_conf = q::signal_conditioner::config{};
auto sig_cond = q::signal_conditioner{sc_conf, low_fs, high_fs, TUNER_ADC_SAMPLE_RATE};
static q::pitch_detector   pd(low_fs, high_fs, TUNER_ADC_SAMPLE_RATE, -40_dB);

void perform_pitch_detection(int16_t *buffer) {
    std::vector<float> in(TUNER_ADC_BUFFER_SIZE); // a vector of values to pass into qlib
    float maxVal = 0;
    float minVal = MAXFLOAT;

    // Go through the buffer, convert to float values, and get the min and max
    // values so they readings can be normalized.
    for (int i = 0; i < TUNER_ADC_BUFFER_SIZE; i++) {
        float value = (float)buffer[i]; // Convert from int16_t to float
        in[i] = value;

        // Track the min and max values we see so we can convert to values between -1.0f and +1.0f
        if (value > maxVal) {
            maxVal = value;
        }
        if (value < minVal) {
            minVal = value;
        }
    }

    // Bail out if the input does not meet the minimum criteria
    float range = maxVal - minVal;
    if (range < TUNER_READING_DIFF_MINIMUM) {
        float no_freq = -1;
        xQueueOverwrite(frequencyQueue, &no_freq);
        // set_current_frequency(-1); // Indicate to the UI that there's no frequency available
        oneEUFilter.reset(); // Reset the 1EU filter so the next frequency it detects will be as fast as possible
        smoother.reset();
        // movingAverage.reset();
        // medianMovingFilter.reset();
        medianFilter.reset();
        pd.reset();
        return;
    }

    // ESP_LOGI(TAG, "Min: %f, Max: %f, peak-to-peak: %f", minVal, maxVal, range);
    oneEUFilter.setBeta(userSettings->oneEUBeta);
    smoother.setAmount(userSettings->expSmoothing);

    // Normalize the values between -1.0 and +1.0 before processing with qlib.
    float midVal = range / 2;
    // ESP_LOGI(TAG, "min: %f, max: %f, range: %f, mid: %f", minVal, maxVal, range, midVal);
    // int crossings = 0;
    // bool currently_negative = false;
    // float items[10];
    for (auto i = 0; i < TUNER_ADC_BUFFER_SIZE; i++) {
        // ESP_LOGI("adc", "%f", in[i]);
        float newPosition = in[i] - midVal - minVal;
        float normalizedValue = newPosition / midVal;
        // ESP_LOGI("norm-val", "%f is now %f", in[i], normalizedValue);
        in[i] = normalizedValue;

        float s = in[i]; // input signal

        // if (i == 0) {
        //     currently_negative = s < 0;
        // }

        // if ((s > 0 && currently_negative) || (s < 0 && !currently_negative)) {
        //     crossings++;
        // }
        // currently_negative = s < 0;

        // s = medianMovingFilter.addValue(s);

        // I've got the signal conditioning commented out right now
        // because it actually is making the frequency readings
        // NOT work. They probably just need to be tweaked a little.

        // Signal Conditioner
        // s = sig_cond(s);
        // if (i < 10) { // This is just to get a good look at how it's functioning.
        //     items[i] = s;
        // }

        // // Bandpass filter
        // s = lp(s);
        // s -= lp2(s);

        // // Envelope
        // auto e = env(std::abs(static_cast<int>(s)));
        // auto e_db = q::lin_to_db(e);

        // if (e > threshold) {
        //     // Compressor + make-up gain + hard clip
        //     auto gain = cycfi::q::lin_float(comp(e_db)) * makeup_gain;
        //     s = clip(s * gain);
        //     threshold = release_threshold;
        // } else {
        //     s = 0.0f;
        //     threshold = onset_threshold;
        // }

        int64_t time_us = esp_timer_get_time(); // Get time in microseconds
        int64_t time_ms = time_us / 1000; // Convert to milliseconds
        int64_t time_seconds = time_us / 1000000;    // Convert to seconds
        // int64_t elapsedTimeSinceLastUpdate = time_us - lastFrequencyRecordedTime;

        // Pitch Detect
        // Send in each value into the pitch detector
        // if (pd(s) == true && elapsedTimeSinceLastUpdate >= minIntervalForFrequencyUpdate) { // calculated a frequency
        if (pd(s) == true) { // calculated a frequency
            auto f = pd.get_frequency();

            bool use1EUFilterFirst = true; // TODO: This may never be needed. Need to test which "feels" better for tuning
            if (use1EUFilterFirst) {
                // 1EU Filtering
                f = (float)oneEUFilter.filter((double)f, (TimeStamp)time_seconds);

                // Simple Exponential Smoothing
                f = smoother.smooth(f);
            } else {
                // Simple Expoential Smoothing
                f = smoother.smooth(f);

                // 1EU Filtering
                f = (float)oneEUFilter.filter((double)f, (TimeStamp)time_seconds);
            }

            // Moving average (makes it BAD!)
            // f = movingAverage.addValue(f);
            f = f / WEIRD_ESP32_WROOM_32_FREQ_FIX_FACTOR; // Use the weird factor only on ESP32-WROOM-32 (which the CYD is)

            // Median Filter
            f = medianFilter.addValue(f);
            if (f != -1.0f) {
                xQueueOverwrite(frequencyQueue, &f);

                // int8_t current_seconds = (int8_t)time_seconds;
                // if (current_seconds % 2 == 0) {
                //     ESP_LOGI(TAG, "Frequency: %f", f);
                // }
            }
        }
    }

    // ESP_LOGI(TAG, "Crossings: %d", crossings);
    // ESP_LOGI(TAG, "Items: %f %f %f %f %f %f %f %f %f %f",
    //     items[0],
    //     items[1],
    //     items[2],
    //     items[3],
    //     items[4],
    //     items[5],
    //     items[6],
    //     items[7],
    //     items[8],
    //     items[9]
    // );
    // for (int i = 0; i < TUNER_ADC_BUFFER_SIZE; i++) {
    //     printf("%d,", buffer[i]);
    // }
}

void pitch_detector_task(void *pvParameter) {
    // Prep ADC
    esp_err_t ret;
    uint32_t num_of_bytes_read = 0;

    ESP_ERROR_CHECK(adc_init());

    ESP_LOGI(TAG, "Starting continuous ADC reading");
    ESP_ERROR_CHECK(adc_start());

    ESP_LOGI(TAG, "Started continuous ADC reading");

    while(1) {
        adc_buffer_t buffer;
        if (xQueueReceive(adcBufferQueue, &buffer, 0)) {
            // ESP_LOGI(TAG, "Elapsed time: %lld", buffer.elapsedTime);

            // ESP_LOGI(TAG, "First items: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
            //     buffer.data[0], buffer.data[1], buffer.data[2], buffer.data[3], buffer.data[4],
            //     buffer.data[5], buffer.data[6], buffer.data[7], buffer.data[8], buffer.data[9],
            //     buffer.data[10], buffer.data[11], buffer.data[12], buffer.data[13], buffer.data[14],
            //     buffer.data[15], buffer.data[16], buffer.data[17], buffer.data[18], buffer.data[19]
            // );
            perform_pitch_detection(buffer.data);
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    for (int i = 0; i < TUNER_ADC_NUM_OF_BUFFERS; i++) {
        free(adc_buffers[i]);
    }

    esp_timer_stop(adc_buffer_timer);
    esp_timer_delete(adc_buffer_timer);
}
