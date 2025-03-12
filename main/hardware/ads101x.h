/**
  ******************************************************************************
  * @file           : ads101x.h
  * @author         : Mauricio Barroso Benavides
  * @date           : Nov 6, 2023
  * @brief          : todo: write brief 
  ******************************************************************************
  * @attention
  *
  * MIT License
  *
  * Copyright (c) 2023 Mauricio Barroso Benavides
  *
  * Permission is hereby granted, free of charge, to any person obtaining a copy
  * of this software and associated documentation files (the "Software"), to
  * deal in the Software without restriction, including without limitation the
  * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
  * sell copies of the Software, and to permit persons to whom the Software is
  * furnished to do so, subject to the following conditions:
  *
  * The above copyright notice and this permission notice shall be included in
  * all copies or substantial portions of the Software.
  * 
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  * IN THE SOFTWARE.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef ADS101X_H_
#define ADS101X_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "driver/i2c_master.h"
#include "driver/gpio.h"

/* Exported Macros -----------------------------------------------------------*/
/* ADS101x I2C device address */
#define ADS101X_I2C_ADDRESS	0x48 /* ADDR pin connected to GND */

/* Pointer registers */
#define ADS101X_REG_POINTER_MASK			0x03 /* Point mask */
#define ADS101X_REG_POINTER_CONVERT		0x00 /* Conversion */
#define ADS101X_REG_POINTER_CONFIG		0x01 /* Configuration */
#define ADS101X_REG_POINTER_LOWTHRESH	0x02 /* Low threshold */
#define ADS101X_REG_POINTER_HITHRESH	0x03 /* High threshold */

/* Configuration registers */
#define ADS101X_REG_CONFIG_OS_MASK			0x8000 /* OS Mask */
#define ADS101X_REG_CONFIG_OS_SINGLE		0x8000 /* Write: Set to start a
																									single-conversion */
#define ADS101X_REG_CONFIG_OS_BUSY			0x0000 /* Read: Bit = 0 when conversion
																									is in progress */
#define ADS101X_REG_CONFIG_OS_NOTBUSY		0x8000 /* Read: Bit = 1 when device is
																									not performing a conversion */
#define ADS101X_REG_CONFIG_MUX_MASK			0x7000 /* Mux Mask */
#define ADS101X_REG_CONFIG_MUX_DIFF_0_1	0x0000 /* Differential P = AIN0,
																									N = AIN1 (default) */
#define ADS101X_REG_CONFIG_MUX_DIFF_0_3	0x1000 /* Differential P = AIN0,
																									N = AIN3 */
#define ADS101X_REG_CONFIG_MUX_DIFF_1_3	0x2000 /* Differential P = AIN1,
																									N = AIN3 */
#define ADS101X_REG_CONFIG_MUX_DIFF_2_3	0x3000 /* Differential P = AIN2,
																									N = AIN3 */
#define ADS101X_REG_CONFIG_MUX_SINGLE_0	0x4000 /* Single-ended AIN0 */
#define ADS101X_REG_CONFIG_MUX_SINGLE_1	0x5000 /* Single-ended AIN1 */
#define ADS101X_REG_CONFIG_MUX_SINGLE_2	0x6000 /* Single-ended AIN2 */
#define ADS101X_REG_CONFIG_MUX_SINGLE_3	0x7000 /* Single-ended AIN3 */
#define ADS101X_REG_CONFIG_PGA_MASK			0x0E00 /* PGA Mask */
#define ADS101X_REG_CONFIG_PGA_6_144V		0x0000 /* +/-6.144V range = Gain 2/3 */
#define ADS101X_REG_CONFIG_PGA_4_096V		0x0200 /* +/-4.096V range = Gain 1 */
#define ADS101X_REG_CONFIG_PGA_2_048V		0x0400 /* +/-2.048V range = Gain 2 */
#define ADS101X_REG_CONFIG_PGA_1_024V		0x0600 /* +/-1.024V range = Gain 4 */
#define ADS101X_REG_CONFIG_PGA_0_512V		0x0800 /* +/-0.512V range = Gain 8 */
#define ADS101X_REG_CONFIG_PGA_0_256V		0x0A00 /* +/-0.256V range = Gain 16 */
#define ADS101X_REG_CONFIG_MODE_MASK		0x0100   /* Mode Mask */
#define ADS101X_REG_CONFIG_MODE_CONTINUOUS	0x0000 /* Continuous conversion mode */
#define ADS101X_REG_CONFIG_MODE_SINGLE	0x0100 /* Power-down single-shot mode */
#define ADS101X_REG_CONFIG_RATE_MASK		0x00E0 /* Data Rate Mask */
#define ADS101X_REG_CONFIG_CMODE_MASK		0x0010 /* CMode Mask */
#define ADS101X_REG_CONFIG_CMODE_TRAD		0x0000 /* Traditional comparator with
                                                  hysteresis (default) */
#define ADS101X_REG_CONFIG_CMODE_WINDOW	0x0010 /* Window comparator */
#define ADS101X_REG_CONFIG_CPOL_MASK		0x0008 /* CPol Mask */
#define ADS101X_REG_CONFIG_CPOL_ACTVLOW	0x0000 /* ALERT/RDY pin is low when
																									active */
#define ADS101X_REG_CONFIG_CPOL_ACTVHI	0x0008 /* ALERT/RDY pin is high when
																									active */
#define ADS101X_REG_CONFIG_CLAT_MASK		0x0004 /* Determines if ALERT/RDY pin
																									latches once asserted */
#define ADS101X_REG_CONFIG_CLAT_NONLAT	0x0000 /* Non-latching comparator */
#define ADS101X_REG_CONFIG_CLAT_LATCH		0x0004 /* Latching comparator */
#define ADS101X_REG_CONFIG_CQUE_MASK		0x0003 /* CQue Mask */
#define ADS101X_REG_CONFIG_CQUE_1CONV		0x0000 /* Assert ALERT/RDY after one
																									conversions */
#define ADS101X_REG_CONFIG_CQUE_2CONV		0x0001 /* Assert ALERT/RDY after two
																									conversions */
#define ADS101X_REG_CONFIG_CQUE_4CONV		0x0002 /* Assert ALERT/RDY after four
																									conversions */
#define ADS101X_REG_CONFIG_CQUE_NONE		0x0003 /* Disable the comparator and put
																									ALERT/RDY in high state */

/* Exported typedef ----------------------------------------------------------*/
typedef enum {
	ADS101X_MODEL_3, /* ADS1013 model */
	ADS101X_MODEL_4, /* ADS1014 model */
	ADS101X_MODEL_5, /* ADS1015 model */
} ads101x_model_t;

/* Gain settings */
typedef enum {
	ADS101X_GAIN_TWOTHIRDS = ADS101X_REG_CONFIG_PGA_6_144V,
	ADS101X_GAIN_ONE       = ADS101X_REG_CONFIG_PGA_4_096V,
	ADS101X_GAIN_TWO       = ADS101X_REG_CONFIG_PGA_2_048V,
	ADS101X_GAIN_FOUR      = ADS101X_REG_CONFIG_PGA_1_024V,
	ADS101X_GAIN_EIGHT     = ADS101X_REG_CONFIG_PGA_0_512V,
	ADS101X_GAIN_SIXTEEN   = ADS101X_REG_CONFIG_PGA_0_256V
} ads101x_gain_t;

/* Data rates */
typedef enum {
	ADS101X_DATA_RATE_128SPS  =	0x0000,
	ADS101X_DATA_RATE_250SPS  =	0x0020,
	ADS101X_DATA_RATE_490SPS  =	0x0040,
	ADS101X_DATA_RATE_920SPS  =	0x0060,
	ADS101X_DATA_RATE_1600SPS =	0x0080,
	ADS101X_DATA_RATE_2400SPS =	0x00A0,
	ADS101X_DATA_RATE_3300SPS =	0x00C0
} ads101x_data_rate_t;

/* Mode setting */
typedef enum {
	ADS101X_MODE_ONESHOT,
	ADS101X_MODE_CONTINUOUS,
} ads101x_mode_t;

/* Channels */
typedef enum {
	ADS101X_CHANNEL_0 = ADS101X_REG_CONFIG_MUX_SINGLE_0,
	ADS101X_CHANNEL_1 = ADS101X_REG_CONFIG_MUX_SINGLE_1,
	ADS101X_CHANNEL_2 = ADS101X_REG_CONFIG_MUX_SINGLE_2,
	ADS101X_CHANNEL_3 = ADS101X_REG_CONFIG_MUX_SINGLE_3
} ads101x_channel_t;

/* ADS101x instance structure */
typedef struct {
	i2c_master_dev_handle_t i2c_dev;
	ads101x_model_t model;
	ads101x_gain_t gain;
	ads101x_data_rate_t data_rate;
	uint8_t bit_shift;
	bool int_en;
	uint32_t int_pin;
	bool is_complete;
	void (* delay_ms)(uint32_t);
} ads101x_t;

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
/**
 * @brief Function that initializes a ADS101x instance
 *
 * @param me       : Pointer to a ads101x_t instance
 * @param model    : ADS101x model
 * @param i2c_bus  : Pointer to a structure with the data to initialize the I2C device
 * @param dev_addr : I2C device address
 * @param delay_ms : Pointer to delay function in ms
 *
 * @return ESP_OK on success
 */
esp_err_t ads101x_init(ads101x_t *const me, ads101x_model_t model, i2c_master_bus_handle_t i2c_bus_handle, uint8_t dev_addr,void (* delay_ms)(uint32_t));

/**
 * @brief Function that reads a specific single-ended ADC channel.
 *
 * @param me         : Pointer to a ads101x_t instance
 * @param channel    : ADC channel to be read
 * @param adc_result : ADC conversion result
 *
 * @return ESP_OK on success
 */
esp_err_t ads101x_read_single_ended(ads101x_t *const me,
		                                ads101x_channel_t channel,
																		int16_t *adc_result);

/**
 * @brief Function that reads the voltage difference between the P (AIN0) and N
 *				(AIN1) input.
 *
 * @param me         : Pointer to a ads101x_t instance
 * @param adc_result : ADC conversion result
 *
 * @return ESP_OK on success
 */
esp_err_t ads101x_read_differential_0_1(ads101x_t *const me,
		                                    int16_t *adc_result);

/**
 * @brief Function that reads the voltage difference between the P (AIN0) and N
 *				(AIN3) input.
 *
 * @param me         : Pointer to a ads101x_t instance
 * @param adc_result : ADC conversion result
 *
 * @return ESP_OK on success
 */
esp_err_t ads101x_read_differential_0_3(ads101x_t *const me,
		                                    int16_t *adc_result);

/**
 * @brief Function that reads the voltage difference between the P (AIN1) and N
 *				(AIN3) input.
 *
 * @param me         : Pointer to a ads101x_t instance
 * @param adc_result : ADC conversion result
 *
 * @return ESP_OK on success
 */
esp_err_t ads101x_read_differential_1_3(ads101x_t *const me,
		                                    int16_t *adc_result);

/**
 * @brief Function that reads the voltage difference between the P (AIN2) and N
 *				(AIN3) input.
 *
 * @param me         : Pointer to a ads101x_t instance
 * @param adc_result : ADC conversion result
 *
 * @return ESP_OK on success
 */
esp_err_t ads101x_read_differential_2_3(ads101x_t *const me,
		                                    int16_t *adc_result);

/**
 * @brief Function that sets up the comparator to operator in basic mode,
 *        causing the ALRT/RDY pin to assert when the ADC value exceeeds the
 *        specified value.
 *
 * @param me        : Pointer to a ads101x_t instance
 * @param channel   : ADC channel to be read
 * @param threshold : Comparator threshold value
 *
 * @return ESP_OK on success
 */
esp_err_t ads101x_start_comparator_single_ended(ads101x_t *const me,
		                                            ads101x_channel_t channel,
																								int16_t threshold);

/**
 * @brief Function that reads the last conversion results without changin the
 *        configuration value.
 *
 * @param me         : Pointer to a ads101x_t instance
 * @param adc_result : ADC conversion result
 *
 * @return ESP_OK on success
 */
esp_err_t ads101x_get_last_conversion_results(ads101x_t *const me,
		                                          int16_t *adc_result);

/**
 * @brief Function that computes the voltage value from ADC raw value
 *
 * @param me     : Pointer to a ads101x_t instance
 * @param counts : ADC reading in raw counts
 *
 * @return Voltage value
 */
float ads101x_compute_volts(ads101x_t *const me, int16_t counts);

/**
 * @brief Function that sets the ADS101x gain
 *
 * @param me   : Pointer to a ads101x_t instance
 * @param gain : ADS101x gain: 2/3, 1, 2, 4, 8 and 16
 */
void ads101x_set_gain(ads101x_t *const me, ads101x_gain_t gain);

/**
 * @brief Function that gets the ADS101x gain
 *
 * @param me : Pointer to a ads101x_t instance
 *
 * @return ADS101x gain
 */
ads101x_gain_t ads101x_get_gain(ads101x_t *const me);

/**
 * @brief Function that sets the ADS101x data rate
 *
 * @param me        : Pointer to a ads101x_t instance
 * @param data_rate : ADS101x data rate: 128, 250, 490, 920, 1600, 2400, 3300 SPS
 */
void ads101x_set_data_rate(ads101x_t *const me, ads101x_data_rate_t data_rate);

/**
 * @brief Function that gets ADS101x data rate
 *
 * @param me : Pointer to a ads101x_t instance
 *
 * @return ADS101x data rate
 */
ads101x_data_rate_t ads101x_get_data_rate(ads101x_t *const me);

/**
 * @brief Function that stars the conversion function
 *
 * @param me   : Pointer to a ads101x_t instance
 * @param mux  : MUX field value
 * @param mode : ADS101x reading mode
 *
 * @return ESP_OK on success
 */
esp_err_t ads101x_start_reading(ads101x_t *const me, uint16_t mux,
		                            ads101x_mode_t mode);

/**
 * @brief Function to check if ADC conversion was completed
 *
 * @param me : Pointer to a ads101x_t instance
 *
 * @return ESP_OK on success
 */
esp_err_t ads101x_conversion_complete(ads101x_t *const me);

/**
 * @brief Function to enable ADS101x interrupt pin. This pin is used to indicante a complete ADC conversion
 *
 * @param me      : Pointer to a ads101x_t instance
 * @param int_pin : Host interrupt pin to connect ADS101x interrupt pin 
 *
 * @return ESP_OK on success
 */
esp_err_t ads101x_interrupt_enable(ads101x_t *const me, uint32_t int_pin);

/**
 * @brief Function to disnable ADS101x interrupt pin. This pin is used to indicante a complete ADC conversion
 *
 * @param me          : Pointer to a ads101x_t instance
 *
 * @return ESP_OK on success
 */
esp_err_t ads101x_interrupt_disable(ads101x_t *const me);


#ifdef __cplusplus
}
#endif

#endif /* ADS101X_H_ */

/***************************** END OF FILE ************************************/
