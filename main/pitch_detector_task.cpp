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
#include "user_settings.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_adc/adc_continuous.h"
// #include "esp_adc/adc_cali.h"
// #include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_filter.h"
#include "esp_timer.h"

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
// #include "exponential_smoother.hpp"
#include "OneEuroFilter.h"
// #include "MovingAverage.hpp"
// #include "MedianFilter.hpp"

static const char *TAG = "PitchDetector";

namespace q = cycfi::q;
using namespace q::literals;
using std::fixed;

using namespace cycfi::q::pitch_names;
using frequency = cycfi::q::frequency;
using pitch = cycfi::q::pitch;
CONSTEXPR frequency low_fs = cycfi::q::pitch_names::B[0]; // Lowest string on a 5-string bass
CONSTEXPR frequency high_fs = cycfi::q::pitch_names::C[7]; // Setting this higher helps to catch the high harmonics

// static adc_channel_t channel[1] = {ADC_CHANNEL_7}; // ESP32-WROOM-32 CYD - GPIO 35 (ADC1_CH7)
// static adc_channel_t channel[1] = {ADC_CHANNEL_3}; // ESP32-S3 EBD4 - GPIO 4 (ADC1_CH3)
static adc_channel_t channel[1] = {TUNER_ADC_CHANNEL}; // ESP32-S3 EBD2 - GPIO 10 (ADC1_CH9)

// adc_cali_handle_t cali_handle = NULL;

// ExponentialSmoother smoother(EXP_SMOOTHING);
OneEuroFilter oneEUFilter(
    EU_FILTER_ESTIMATED_FREQ,
    EU_FILTER_MIN_CUTOFF,
    EU_FILTER_BETA,
    EU_FILTER_DERIVATIVE_CUTOFF);
OneEuroFilter oneEUFilter2(
    EU_FILTER_ESTIMATED_FREQ,
    EU_FILTER_MIN_CUTOFF_2,
    EU_FILTER_BETA_2,
    EU_FILTER_DERIVATIVE_CUTOFF_2);
// MovingAverage movingAverage(5);
// MedianFilter medianMovingFilter(3, true);
// MedianFilter medianFilter(5, false);

extern UserSettings *userSettings;
extern QueueHandle_t frequencyQueue;

static TaskHandle_t s_task_handle;
static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    //Notify that ADC continuous driver has done enough number of conversions
    vTaskNotifyGiveFromISR(s_task_handle, &mustYield);

    return (mustYield == pdTRUE);
}

static void continuous_adc_init(adc_channel_t *channel, uint8_t channel_count, adc_continuous_handle_t *out_handle, adc_iir_filter_handle_t *out_filter_handle)
{
    adc_continuous_handle_t handle = NULL;

    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = TUNER_ADC_BUFFER_POOL_SIZE,
        .conv_frame_size = TUNER_ADC_FRAME_SIZE,
        .flags = {
            // .flush_pool = false,
            .flush_pool = true,
        },
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    for (int i = 0; i < channel_count; i++) {
        adc_pattern[i].atten = TUNER_ADC_ATTEN;
        adc_pattern[i].channel = channel[i] & 0x0F;
        adc_pattern[i].unit = TUNER_ADC_UNIT;
        adc_pattern[i].bit_width = TUNER_ADC_BIT_WIDTH;

        ESP_LOGI(TAG, "adc_pattern[%d].atten:     %"PRIu8"", i, adc_pattern[i].atten);
        ESP_LOGI(TAG, "adc_pattern[%d].channel:   %"PRIu8"", i, adc_pattern[i].channel);
        ESP_LOGI(TAG, "adc_pattern[%d].unit:      %"PRIu8"", i, adc_pattern[i].unit);
        ESP_LOGI(TAG, "adc_pattern[%d].bit_width: %"PRIu8"", i, adc_pattern[i].bit_width);
    }

    // int adc_gpio_num = -1;
    // adc_continuous_channel_to_io(TUNER_ADC_UNIT, TUNER_ADC_CHANNEL, &adc_gpio_num);
    // ESP_LOGI(TAG, "ADC Channel 9 is on GPIO %d", adc_gpio_num);

    adc_continuous_config_t dig_cfg = {
        .pattern_num = channel_count,
        .adc_pattern = adc_pattern,
        .sample_freq_hz = TUNER_ADC_SAMPLE_RATE,
        .conv_mode = TUNER_ADC_CONV_MODE,
        .format = TUNER_ADC_OUTPUT_TYPE,
    };

    // dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

    // adc_cali_curve_fitting_config_t curve_cfg = {
    //     .unit_id = TUNER_ADC_UNIT,
    //     .chan = TUNER_ADC_CHANNEL,
    //     .atten = TUNER_ADC_ATTEN,
    //     .bitwidth = ADC_BITWIDTH_12,
    // };
    // ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&curve_cfg, &cali_handle));

    adc_continuous_iir_filter_config_t iir_filter_cfg = {
        .unit = TUNER_ADC_UNIT,
        .channel = TUNER_ADC_CHANNEL,
        .coeff = TUNER_ACD_FILTER_COEFF,
    };
    ESP_ERROR_CHECK(adc_new_continuous_iir_filter(handle, &iir_filter_cfg, out_filter_handle));

    *out_handle = handle;
}

/// @brief Function to compute the closest note and cent deviation
inline esp_err_t get_frequency_info(float input_freq, FrequencyInfo *freqInfo) { //float *target_frequency, TunerNoteName *target_note, int *target_octave, float *cents_off) {
    if (input_freq <= 0) {
        return ESP_FAIL;
    }
    
    // Calculate the number of semitones away from A4
    float semitone_offset = 12 * log2(input_freq / A4_FREQ);
    int closest_semitone = (int)round(semitone_offset);
    
    // Compute the closest note index (modulo 12 for chromatic scale)
    int note_index = (closest_semitone + 9) % 12;
    if (note_index < 0) {
        note_index += 12; // Ensure positive index
    }
    int octave = 4 + ((closest_semitone + 9) / 12); // Determine octave number
    
    // Compute the frequency of the closest note
    float closest_note_freq = A4_FREQ * pow(2.0, closest_semitone / 12.0);
    freqInfo->frequency = input_freq;
    freqInfo->targetFrequency = closest_note_freq;
    
    // Calculate the cent deviation
    // *cents_off = (int)round(1200 * log2(input_freq / closest_note_freq));
    freqInfo->cents = 1200 * log2(input_freq / closest_note_freq);

    freqInfo->targetNote = (TunerNoteName)note_index;

    freqInfo->targetOctave = octave;
    return ESP_OK;
}

void pitch_detector_task(void *pvParameter) {
    // Prep ADC
    esp_err_t ret;
    uint32_t num_of_bytes_read = 0;
    uint8_t *adc_buffer = (uint8_t *)malloc(TUNER_ADC_FRAME_SIZE);
    if (adc_buffer == NULL) {
        ESP_LOGI(TAG, "Failed to allocate memory for buffer");
        return;
    }
    memset(adc_buffer, 0xcc, TUNER_ADC_FRAME_SIZE);

    // Get the pitch detector ready
    q::pitch_detector   pd(low_fs, high_fs, TUNER_ADC_SAMPLE_RATE, -40_dB);

    // auto const&                 bits = pd.bits();
    // auto const&                 edges = pd.edges();
    // q::bitstream_acf<>          bacf{ bits };
    // auto                        min_period = as_float(high_fs.period()) * TUNER_ADC_SAMPLE_RATE;
    
    // q::peak_envelope_follower   env{ 30_ms, TUNER_ADC_SAMPLE_RATE };
    // q::one_pole_lowpass         lp{high_fs, TUNER_ADC_SAMPLE_RATE};
    // q::one_pole_lowpass         lp2(low_fs, TUNER_ADC_SAMPLE_RATE);

    // constexpr float             slope = 1.0f/2;
    // constexpr float             makeup_gain = 2;
    // q::compressor               comp{ -18_dB, slope };
    // q::clip                     clip;

    // float                       onset_threshold = lin_float(-28_dB);
    // float                       release_threshold = lin_float(-60_dB);
    // float                       threshold = onset_threshold;

    auto sc_conf = q::signal_conditioner::config{};
    auto sig_cond = q::signal_conditioner{sc_conf, low_fs, high_fs, TUNER_ADC_SAMPLE_RATE};

    s_task_handle = xTaskGetCurrentTaskHandle();
    
    adc_continuous_handle_t handle = NULL;
    adc_iir_filter_handle_t adc_filter = NULL;
    continuous_adc_init(channel, sizeof(channel) / sizeof(adc_channel_t), &handle, &adc_filter);

    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = s_conv_done_cb,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, NULL));
    ESP_ERROR_CHECK(adc_continuous_start(handle));

    TunerNoteName lastSeenNote = NOTE_NONE;
    int sameNoteSeenCount = 0;
    FrequencyInfo freqInfo;
    FrequencyInfo noFreq = {
        .frequency = -1,
        .cents = -1,
        .targetFrequency = -1,
        .targetNote = NOTE_NONE,
        .targetOctave = -1,
    };

    TickType_t ticksBetweenFreqDetection = pdMS_TO_TICKS(5);

    while (1) {
        /**
         * This is to show you the way to use the ADC continuous mode driver event callback.
         * This `ulTaskNotifyTake` will block when the data processing in the task is fast.
         * However in this example, the data processing (print) is slow, so you barely block here.
         *
         * Without using this event callback (to notify this task), you can still just call
         * `adc_continuous_read()` here in a loop, with/without a certain block timeout.
         */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        while (1) {
            if (userSettings == NULL) {
                // Things aren't yet initialized. Do nothing.
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }

            std::vector<float> in(TUNER_ADC_FRAME_SIZE); // a vector of values to pass into qlib

            ret = adc_continuous_read(handle, adc_buffer, TUNER_ADC_FRAME_SIZE, &num_of_bytes_read, portMAX_DELAY);
            if (ret == ESP_OK) {
                // ESP_LOGI(TAG, "ret is %x, num_of_bytes_read is %"PRIu32" bytes", ret, num_of_bytes_read);

                // Get the data out of the ADC Conversion Result.
                int valuesStored = 0;
                float maxVal = 0;
                float minVal = MAXFLOAT;
                // ESP_LOGI(TAG, "Bytes read: %ld", num_of_bytes_read);
                for (int i = 0; i < num_of_bytes_read; i += SOC_ADC_DIGI_RESULT_BYTES, valuesStored++) {
                    adc_digi_output_data_t *p = (adc_digi_output_data_t*)&adc_buffer[i];
                    // if (i == 20) {
                    //     ESP_LOGI(TAG, "read value is: %d", TUNER_ADC_GET_DATA(p));
                    // }

                    int value = TUNER_ADC_GET_DATA(p);
                    // int calibratedValue;
                    // esp_err_t r = adc_cali_raw_to_voltage(cali_handle, value, &calibratedValue);
                    // if (r == ESP_OK) {
                    //     value = calibratedValue;
                    // }

                    // Do a first pass by just storing the raw values into the float array
                    in[valuesStored] = value;

                    // Track the min and max values we see so we can convert to values between -1.0f and +1.0f
                    if (in[valuesStored] > maxVal) {
                        maxVal = in[valuesStored];
                    }
                    if (in[valuesStored] < minVal) {
                        minVal = in[valuesStored];
                    }
                }

                // Bail out if the input does not meet the minimum criteria
                float range = maxVal - minVal;
                if (range < TUNER_READING_DIFF_MINIMUM) {
                    xQueueOverwrite(frequencyQueue, &noFreq);
                    // set_current_frequency(-1); // Indicate to the UI that there's no frequency available
                    oneEUFilter.reset(); // Reset the 1EU filter so the next frequency it detects will be as fast as possible
                    oneEUFilter2.reset();
                    // smoother.reset();
                    // movingAverage.reset();
                    // medianMovingFilter.reset();
                    // medianFilter.reset();
                    pd.reset();

                    lastSeenNote = NOTE_NONE;
                    sameNoteSeenCount = 0;

                    vTaskDelay(ticksBetweenFreqDetection);
                    continue;
                }

                // oneEUFilter.setBeta(userSettings->oneEUBeta);
                // smoother.setAmount(userSettings->expSmoothing);

                // Normalize the values between -1.0 and +1.0 before processing with qlib.
                float midVal = range / 2;
                // ESP_LOGI(TAG, "min: %f  max: %f  range: %f  mid: %f", minVal, maxVal, range, midVal);
                for (auto i = 0; i < valuesStored; i++) {
                    // ESP_LOGI("adc", "%f", in[i]);
                    float newPosition = in[i] - midVal - minVal;
                    float normalizedValue = newPosition / midVal;
                    // ESP_LOGI("norm-val", "%f is now %f", in[i], normalizedValue);
                    // in[i] = normalizedValue;
                    // float s = in[i]; // input signal
                    float s = normalizedValue;

                    // s = medianMovingFilter.addValue(s);

                    // I've got the signal conditioning commented out right now
                    // because it actually is making the frequency readings
                    // NOT work. They probably just need to be tweaked a little.

                    // Signal Conditioner
                    s = sig_cond(s);
                    // s = s * 2.5;

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

                    // Pitch Detect
                    // Send in each value into the pitch detector
                    if (pd(s) == true) { // calculated a frequency
                        auto f = pd.get_frequency();

                        bool use1EUFilterFirst = true; // TODO: This may never be needed. Need to test which "feels" better for tuning
                        // if (use1EUFilterFirst) {
                            
                        // Simple Exponential Smoothing
                        // f = smoother.smooth(f);
                        
                        // 1EU Filtering
                        double time_seconds;
                        oneEUFilter.setFrequency(f);
                        time_seconds = (double)esp_timer_get_time() / 1000000;    // Convert to seconds
                        f = (float)oneEUFilter.filter((double)f, (TimeStamp)time_seconds);

                        // f = movingAverage.addValue(f);
                        // f = smoother.smooth(f);
                        
                        oneEUFilter2.setFrequency(f);
                        time_seconds = (double)esp_timer_get_time() / 1000000;
                        f = (float)oneEUFilter2.filter((double)f, (TimeStamp)time_seconds);
                        
                        // } else {
                        //     // Simple Expoential Smoothing
                        //     f = smoother.smooth(f);

                        //     // 1EU Filtering
                        //     f = (float)oneEUFilter.filter((double)f, (TimeStamp)time_seconds);
                        // }

                        // f = f / WEIRD_ESP32_WROOM_32_FREQ_FIX_FACTOR; // Use the weird factor only on ESP32-WROOM-32 (which the CYD is)

                        if (f != -1.0f) {
                            if (get_frequency_info(f, &freqInfo) == ESP_OK) {
                                // Only show frequency info if we've seen the
                                // same target note more than once in a row.
                                // Doing this seems to help prevent sporadic
                                // notes from appearing right as you pluck a
                                // string.

                                if (lastSeenNote == freqInfo.targetNote) {
                                    sameNoteSeenCount++;
                                } else {
                                    sameNoteSeenCount = 0;
                                }
                                lastSeenNote = freqInfo.targetNote;

                                if (sameNoteSeenCount > 1) {
                                    xQueueOverwrite(frequencyQueue, &freqInfo);
                                }
                            }
                        }
                    }
                }

                vTaskDelay(pdMS_TO_TICKS(ticksBetweenFreqDetection));
            } else if (ret == ESP_ERR_TIMEOUT) {
                //We try to read `EXAMPLE_READ_LEN` until API returns timeout, which means there's no available data
                break;
            }
        }
    }

    free(adc_buffer);

    ESP_ERROR_CHECK(adc_continuous_stop(handle));
    ESP_ERROR_CHECK(adc_continuous_deinit(handle));
}