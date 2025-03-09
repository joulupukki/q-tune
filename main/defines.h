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
#if !defined(TUNER_GLOBAL_DEFINES)
#define TUNER_GLOBAL_DEFINES

//
// Foot Switch and Relay (GPIO)
//
#define FOOT_SWITCH_GPIO                GPIO_NUM_0 // BOOT button
// #define FOOT_SWITCH_GPIO                GPIO_NUM_44 // TXD on EBD4
#define RELAY_GPIO                      GPIO_NUM_43 // RXD on EBD4

#define LONG_PRESS_TIME_MS              1000 // milliseconds
#define DOUBLE_PRESS_TIME_MS            250 // milliseconds
#define DEBOUNCE_TIME_MS                50 // milliseconds

#define DISPLAY_BUFFER_ROWS             32
#define DISPLAY_BUFFER_SIZE             (480 * DISPLAY_BUFFER_ROWS)
#define LCD_DOUBLE_BUFFER               1
#define LCD_MIRROR_X                    (false)
#define LCD_MIRROR_Y                    (false)

//
// Default User Settings
//
#define DEFAULT_INITIAL_STATE           ((TunerState) tunerStateTuning)
#define DEFAULT_STANDBY_GUI_INDEX       (0)
#define DEFAULT_TUNER_GUI_INDEX         (0)
#define DEFAULT_IN_TUNE_CENTS_WIDTH     ((uint8_t) 2)
#define DEFAULT_MONITORING_MODE         (0)
#define DEFAULT_NOTE_NAME_PALETTE       ((lv_palette_t) LV_PALETTE_NONE)
#define DEFAULT_DISPLAY_ORIENTATION     ((TunerOrientation) orientationNormal);
#define DEFAULT_EXP_SMOOTHING           ((float) 0.09)
#define DEFAULT_ONE_EU_BETA             ((float) 0.003)
#define DEFAULT_NOTE_DEBOUNCE_INTERVAL  ((float) 115.0)
#define DEFAULT_USE_1EU_FILTER_FIRST    (true)
// #define DEFAULT_MOVING_AVG_WINDOW       ((float) 100)
#define DEFAULT_DISPLAY_BRIGHTNESS      ((uint8_t) 7) // equates to 80% brightness because we're storing the value as a 0-based integer (0 - 10%, 1 - 20%, etc.)

//
// Pitch Detector Related
//
#define TUNER_ADC_UNIT                    ADC_UNIT_1
#define _TUNER_ADC_UNIT_STR(unit)         #unit
#define TUNER_ADC_UNIT_STR(unit)          _TUNER_ADC_UNIT_STR(unit)
#define TUNER_ADC_CONV_MODE               ADC_CONV_SINGLE_UNIT_1
#define TUNER_ADC_ATTEN                   ADC_ATTEN_DB_12
#define TUNER_ADC_BIT_WIDTH               12

// #if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
// #define TUNER_ADC_OUTPUT_TYPE             ADC_DIGI_OUTPUT_FORMAT_TYPE1
// #define TUNER_ADC_GET_CHANNEL(p_data)     ((p_data)->type1.channel)
// #define TUNER_ADC_GET_DATA(p_data)        ((p_data)->type1.data)
// #else
#define TUNER_ADC_OUTPUT_TYPE             ADC_DIGI_OUTPUT_FORMAT_TYPE2
#define TUNER_ADC_GET_CHANNEL(p_data)     ((p_data)->type2.channel)
#define TUNER_ADC_GET_DATA(p_data)        ((p_data)->type2.data)
// #endif

// CYD @ 48kHz
// #define TUNER_ADC_FRAME_SIZE            1024
// #define TUNER_ADC_BUFFER_POOL_SIZE      4096
// #define TUNER_ADC_SAMPLE_RATE           (48 * 1000) // 48kHz

// EBD4 @ 5kHz
#define TUNER_ADC_FRAME_SIZE            (SOC_ADC_DIGI_DATA_BYTES_PER_CONV * 64)
#define TUNER_ADC_BUFFER_POOL_SIZE      (TUNER_ADC_FRAME_SIZE * 4)
#define TUNER_ADC_SAMPLE_RATE           (5 * 1000) // 5kHz 

/// @brief This factor is used to correct the incoming frequency readings on ESP32-WROOM-32 (which CYD is). This same weird behavior does not happen on ESP32-S2 or ESP32-S3.
// #define WEIRD_ESP32_WROOM_32_FREQ_FIX_FACTOR    1.2222222223 // 11/9 but using 11/9 gives completely incorrect results. Weird.
#define WEIRD_ESP32_WROOM_32_FREQ_FIX_FACTOR    1 // ESP32-S3 does not need this correction.

// HELTEC @ 20kHz
// #define TUNER_ADC_FRAME_SIZE            (SOC_ADC_DIGI_DATA_BYTES_PER_CONV * 256)
// #define TUNER_ADC_BUFFER_POOL_SIZE      (TUNER_ADC_FRAME_SIZE * 16)
// #define TUNER_ADC_SAMPLE_RATE           (20 * 1000) // 20kHz

// Original HELTEC
// #define TUNER_ADC_FRAME_SIZE            (SOC_ADC_DIGI_DATA_BYTES_PER_CONV * 64)
// #define TUNER_ADC_BUFFER_POOL_SIZE      (TUNER_ADC_FRAME_SIZE * 4)
// #define TUNER_ADC_SAMPLE_RATE           (5 * 1000) // 5kHz

// If the difference between the minimum and maximum input values
// is less than this value, discard the reading and do not evaluate
// the frequency. This should help cut down on the noise from the
// OLED and only attempt to read frequency information when an
// actual input signal is being read.
#define TUNER_READING_DIFF_MINIMUM      80 // approximately 80mV

//
// Smoothing
//

// 1EU Filter
#define EU_FILTER_ESTIMATED_FREQ        48000 // Same as https://github.com/bkshepherd/DaisySeedProjects/blob/main/Software/GuitarPedal/Util/frequency_detector_q.h
#define EU_FILTER_MIN_CUTOFF            0.5
#define EU_FILTER_DERIVATIVE_CUTOFF     1

//
// GUI Related
//

#define A4_FREQ                         440.0
#define CENTS_PER_SEMITONE              100

#define INDICATOR_SEGMENTS              100 // num of visual segments for showing tuning accuracy

#define GEAR_SYMBOL "\xEF\x80\x93"

//
// When the pitch stops being detected, the note can fade out. This is how long
// that animation is set to run for.
#define LAST_NOTE_FADE_INTERVAL_MS  2000

#endif