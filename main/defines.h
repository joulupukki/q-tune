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

#include "driver/gpio.h"
#include "hal/adc_types.h"

typedef enum {
    NOTE_C = 0,
    NOTE_C_SHARP,
    NOTE_D,
    NOTE_D_SHARP,
    NOTE_E,
    NOTE_F,
    NOTE_F_SHARP,
    NOTE_G,
    NOTE_G_SHARP,
    NOTE_A,
    NOTE_A_SHARP,
    NOTE_B,
    NOTE_NONE
} TunerNoteName;

inline const char *name_for_note(TunerNoteName note) { 
    switch (note) {
        case NOTE_C:
            return "C";
        case NOTE_C_SHARP:
            return "C#";
        case NOTE_D:
            return "D";
        case NOTE_D_SHARP:
            return "D#";
        case NOTE_E:
            return "E";
        case NOTE_F:
            return "F";
        case NOTE_F_SHARP:
            return "F#";
        case NOTE_G:
            return "G";
        case NOTE_G_SHARP:
            return "G#";
        case NOTE_A:
            return "A";
        case NOTE_A_SHARP:
            return "A#";
        case NOTE_B:
            return "B";
        case NOTE_NONE:
            return "-";
        default:
            return "Unknown";
    }
}

typedef struct {
    float frequency;
    float cents;
    float targetFrequency;
    TunerNoteName targetNote;
    int targetOctave;
} FrequencyInfo;

typedef enum : uint8_t {
    tunerBypassTypeTrue = 0,
    tunerBypassTypeBuffered,
} TunerBypassType;

//
// RTOS Queues
//

// The pitch detector task will always write the latest value of detection on
// the queue. Use a length of 1 so we can use xQueueOverwrite and xQueuePeek so
// the very latest frequency info is always available to anywhere.

#define FREQUENCY_QUEUE_LENGTH 1
#define FREQUENCY_QUEUE_ITEM_SIZE sizeof(FrequencyInfo)

#define TUNER_STATE_QUEUE_LENGTH 1
#define TUNER_STATE_QUEUE_ITEM_SIZE sizeof(uint8_t)

//
// Foot Switch and Relay (GPIO)
//
#define FOOT_SWITCH_GPIO                GPIO_NUM_44 // RXD on EBD2
#define BYPASS_RELAY_GPIO               GPIO_NUM_43 // TXD on EBD2
#define BYPASS_TYPE_RELAY_GPIO          GPIO_NUM_15 // True Bypass (3.3V "On" state) or Buffered Bypass (0V "Off" state)

#define LONG_PRESS_TIME_MS              1000 // milliseconds
#define DOUBLE_PRESS_TIME_MS            250 // milliseconds
#define DEBOUNCE_TIME_MS                50 // milliseconds

//
// LCD
//
#define LCD_H_RES                       240
#define LCD_V_RES                       320
#define LCD_BITS_PIXEL                  16
#define LCD_BUF_LINES                   30
#define LCD_DOUBLE_BUFFER               1
#define LCD_DRAWBUF_SIZE                (LCD_H_RES * LCD_BUF_LINES)
#define LCD_MIRROR_X                    (true)
#define LCD_MIRROR_Y                    (false)

//
// Default User Settings
//
#define DEFAULT_INITIAL_STATE           ((TunerState) tunerStateTuning)
#define DEFAULT_BYPASS_TYPE             ((TunerBypassType) tunerBypassTypeTrue)
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
#define TUNER_ADC_UNIT                  ADC_UNIT_2
// #define TUNER_ADC_CHANNEL               ADC_CHANNEL_4 // GPIO15 - According to the ESP32-S3 datasheet, GPIO15 is a strapping pin and shouldn't be used
#define TUNER_ADC_CHANNEL               ADC_CHANNEL_7 // GPIO18
#define TUNER_ADC_CONV_MODE             ADC_CONV_SINGLE_UNIT_2
#define _TUNER_ADC_UNIT_STR(unit)       #unit
#define TUNER_ADC_UNIT_STR(unit)        _TUNER_ADC_UNIT_STR(unit)
#define TUNER_ADC_ATTEN                 ADC_ATTEN_DB_12
#define TUNER_ADC_BIT_WIDTH             12
#define TUNER_ACD_FILTER_COEFF          ADC_DIGI_IIR_FILTER_COEFF_64

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
// #define TUNER_ADC_FRAME_SIZE            (SOC_ADC_DIGI_DATA_BYTES_PER_CONV * 64)
// #define TUNER_ADC_BUFFER_POOL_SIZE      (TUNER_ADC_FRAME_SIZE * 4)
// #define TUNER_ADC_SAMPLE_RATE           (5 * 1000) // 5kHz 

// EBD2 @ 5kHz
#define TUNER_ADC_FRAME_SIZE            (SOC_ADC_DIGI_DATA_BYTES_PER_CONV * 64)
#define TUNER_ADC_BUFFER_POOL_SIZE      (TUNER_ADC_FRAME_SIZE * 4)
#define TUNER_ADC_SAMPLE_RATE           (5 * 1000) // 5kHz

/*
    ADC_DIGI_IIR_FILTER_COEFF_2,     ///< The filter coefficient is 2
    ADC_DIGI_IIR_FILTER_COEFF_4,     ///< The filter coefficient is 4
    ADC_DIGI_IIR_FILTER_COEFF_8,     ///< The filter coefficient is 8
    ADC_DIGI_IIR_FILTER_COEFF_16,    ///< The filter coefficient is 16
    ADC_DIGI_IIR_FILTER_COEFF_64,    ///< The filter coefficient is 64
*/

// EBD2 @ 3.3kHz with ADS1015 (External ADC)
// #define TUNER_ADC_SAMPLE_RATE               (3.3 * 1000) // 3.3kHz 
// #define TUNER_ADC_BUFFER_SIZE               300
// #define TUNER_ADC_NUM_OF_BUFFERS            6
// #define TUNER_ADC_ALERT_PIN                 GPIO_NUM_18
// #define TUNER_ADC_SAMPLE_INTERVAL           (1000000 / TUNER_ADC_SAMPLE_RATE) // microseconds to read one sample

/// @brief This factor is used to correct the incoming frequency readings on ESP32-WROOM-32 (which CYD is). This same weird behavior does not happen on ESP32-S2 or ESP32-S3.
// #define WEIRD_ESP32_WROOM_32_FREQ_FIX_FACTOR    1.2222222223 // 11/9 but using 11/9 gives completely incorrect results. Weird.
// #define WEIRD_ESP32_WROOM_32_FREQ_FIX_FACTOR    1 // ESP32-S3 does not need this correction.

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
// #define TUNER_READING_DIFF_MINIMUM      80
// #define TUNER_READING_DIFF_MINIMUM      100
#define TUNER_READING_DIFF_MINIMUM      600

//
// Smoothing
//

// 1EU Filter
#define EU_FILTER_ESTIMATED_FREQ        110

#define EU_FILTER_MIN_CUTOFF            1.0
#define EU_FILTER_BETA                  ((float) 0.05)
#define EU_FILTER_DERIVATIVE_CUTOFF     1.0

#define EU_FILTER_MIN_CUTOFF_2          0.5
#define EU_FILTER_BETA_2                ((float) 0.05)
#define EU_FILTER_DERIVATIVE_CUTOFF_2   1.0

// Exponential Smoothing
#define EXP_SMOOTHING                  ((float) 0.5)

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