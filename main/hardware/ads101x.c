/**
  ******************************************************************************
  * @file           : ads101x.c
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

/* Includes ------------------------------------------------------------------*/
#include "ads101x.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "hal/gpio_types.h"
#include <sys/_intsup.h>

/* Private macros ------------------------------------------------------------*/

/* External variables --------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static const char *TAG = "ads101x";

/* Private function prototypes -----------------------------------------------*/
/**
 * @brief Function that implements the default I2C read transaction
 *
 * @param reg_addr : Register address to be read
 * @param reg_data : Pointer to the data to be read from reg_addr
 * @param data_len : Length of the data transfer
 * @param intf     : Pointer to the interface descriptor
 *
 * @return 0 if successful, non-zero otherwise
 */
static int8_t i2c_read(uint8_t reg_addr, uint16_t *reg_data, void *intf);

/**
 * @brief Function that implements the default I2C write transaction
 *
 * @param reg_addr : Register address to be written
 * @param reg_data : Data to be written to reg_addr
 * @param data_len : Length of the data transfer
 * @param intf     : Pointer to the interface descriptor
 *
 * @return 0 if successful, non-zero otherwise
 */
static int8_t i2c_write(uint8_t reg_addr, const uint16_t reg_data,
		                    void *intf);

/**
 * @brief Function that implements a micro seconds delay
 *
 * @param arg: todo: write
 */
static void isr_handler(void *arg);

/* Exported functions definitions --------------------------------------------*/
/**
 * @brief Function to initialize a ADS101x instance
 */
esp_err_t ads101x_init(ads101x_t *const me, ads101x_model_t model, i2c_master_bus_handle_t i2c_bus_handle, uint8_t dev_addr, void (* delay_ms)(uint32_t)) 
{
	/* Print initializing message */
	ESP_LOGI(TAG, "Initializing instance...");

	/* Variable to return error code */
	esp_err_t ret = ESP_OK;

	/**/
	me->model = model;
	me->bit_shift = 4;
	me->gain = ADS101X_GAIN_TWOTHIRDS;
	me->data_rate = ADS101X_DATA_RATE_1600SPS;
	me->is_complete = false;
	me->int_en = false;
	
	if (delay_ms == NULL) {
		return ESP_FAIL;
	}
	
	me->delay_ms = delay_ms;

	/* Add device to I2C bus */
	i2c_device_config_t i2c_dev_conf = {
			.scl_speed_hz = 400000,
			.device_address = dev_addr
	};

	if (i2c_master_bus_add_device(i2c_bus_handle, &i2c_dev_conf, &me->i2c_dev) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to add device to I2C bus");
		return ret;
	}

	/* Print successful initialization message */
	ESP_LOGI(TAG, "Instance initialized successfully");

	/* Return ESP_OK */
	return ret;
}

/**
 * @brief Function that reads a specific single-ended ADC channel.
 */
esp_err_t ads101x_read_single_ended(ads101x_t *const me,
		                                ads101x_channel_t channel,
																		int16_t *adc_result) {
	/* Variable to return error code */
	esp_err_t ret = ESP_OK;

	/* Check channel argument */
	if (me->model == ADS101X_MODEL_5) { /* ADS1015 */
		if (channel > ADS101X_CHANNEL_3) {
			ESP_LOGE(TAG, "Failed to select channel, must be less than 3");
			return ESP_FAIL;
		}
	}
	else { /* ADS1013 and ADS1014 */
		if (channel > ADS101X_CHANNEL_1) {
			ESP_LOGE(TAG, "Failed to select channel, must be less than 1");
			return ESP_FAIL;
		}
	}

	/* Perform a oneshot ADC reading */
	ret = ads101x_start_reading(me, channel, ADS101X_MODE_ONESHOT);

	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Check if the conversion is complete */
	ret = ads101x_conversion_complete(me);
	
	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	me->is_complete = false;

	/* Get the las ADC conversion result */
	ret = ads101x_get_last_conversion_results(me, adc_result);

	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Return ESP_OK */
	return ret;
}

/**
 * @brief Function that reads the voltage difference between the P (AIN0) and N
 *				(AIN1) input.
 */
esp_err_t ads101x_read_differential_0_1(ads101x_t *const me,
		                                    int16_t *adc_result) {
	/* Variable to return error code */
	esp_err_t ret = ESP_OK;

	/* Perform a oneshot ADC reading */
	ret = ads101x_start_reading(me, ADS101X_REG_CONFIG_MUX_DIFF_0_1, ADS101X_MODE_ONESHOT);

	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Check if the conversion is complete */
	ret = ads101x_conversion_complete(me);
	
	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Get the las ADC conversion result */
	ret = ads101x_get_last_conversion_results(me, adc_result);

	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Return ESP_OK */
	return ret;
}

/**
 * @brief Function that reads the voltage difference between the P (AIN0) and N
 *				(AIN3) input.
 */
esp_err_t ads101x_read_differential_0_3(ads101x_t *const me,
		                                    int16_t *adc_result) {
	/* Variable to return error code */
	esp_err_t ret = ESP_OK;

	/* Perform a oneshot ADC reading */
	ret = ads101x_start_reading(me, ADS101X_REG_CONFIG_MUX_DIFF_0_3, ADS101X_MODE_ONESHOT);

	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Check if the conversion is complete */
	ret = ads101x_conversion_complete(me);
	
	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Get the las ADC conversion result */
	ret = ads101x_get_last_conversion_results(me, adc_result);

	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Return ESP_OK */
	return ret;
}

/**
 * @brief Function that reads the voltage difference between the P (AIN1) and N
 *				(AIN3) input.
 */
esp_err_t ads101x_read_differential_1_3(ads101x_t *const me,
		                                    int16_t *adc_result) {
	/* Variable to return error code */
	esp_err_t ret = ESP_OK;

	/* Perform a oneshot ADC reading */
	ret = ads101x_start_reading(me, ADS101X_REG_CONFIG_MUX_DIFF_1_3, ADS101X_MODE_ONESHOT);

	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Check if the conversion is complete */
	ret = ads101x_conversion_complete(me);
	
	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Get the las ADC conversion result */
	ret = ads101x_get_last_conversion_results(me, adc_result);

	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Return ESP_OK */
	return ret;
}

/**
 * @brief Function that reads the voltage difference between the P (AIN2) and N
 *				(AIN3) input.
 */
esp_err_t ads101x_read_differential_2_3(ads101x_t *const me,
		                                    int16_t *adc_result) {
	/* Variable to return error code */
	esp_err_t ret = ESP_OK;

	/* Perform a oneshot ADC reading */
	ret = ads101x_start_reading(me, ADS101X_REG_CONFIG_MUX_DIFF_2_3, ADS101X_MODE_ONESHOT);

	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Check if the conversion is complete */
	ret = ads101x_conversion_complete(me);
	
	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Get the las ADC conversion result */
	ret = ads101x_get_last_conversion_results(me, adc_result);

	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Return ESP_OK */
	return ret;
}

/**
 * @brief Function that sets up the comparator to operator in basic mode,
 *        causing the ALRT/RDY pin to assert when the ADC value exceeeds the
 *        specified value.
 */
esp_err_t ads101x_start_comparator_single_ended(ads101x_t *const me,
		                                            ads101x_channel_t channel,
																								int16_t threshold) {
	/* Variable to return error code */
	esp_err_t ret = ESP_OK;

	/* Fill the ADS101x configuration value */
	uint16_t config = ADS101X_REG_CONFIG_CQUE_1CONV |   /* Comparator enabled and
	                                                       asserts on 1 match */
			              ADS101X_REG_CONFIG_CLAT_LATCH |   /* Latching mode */
										ADS101X_REG_CONFIG_CPOL_ACTVLOW | /* Alert/ready active low */
										ADS101X_REG_CONFIG_CMODE_TRAD |   /* Traditional comparator */
										ADS101X_REG_CONFIG_MODE_CONTINUOUS;   /* Continuous conversion
										                                     mode */

	/* Set PGA/voltage range */
	config |= me->gain;

	/* Set data rate */
	config |= me->data_rate;

	/* Set channel */
	config |= channel;

	/* Set the high threshold register */
	if (i2c_write(ADS101X_REG_POINTER_HITHRESH, threshold << me->bit_shift, me->i2c_dev) < 0) {
		return ESP_FAIL;
	}

	/* Set the new ADC configuration */
	if (i2c_write(ADS101X_REG_POINTER_CONFIG, config, me->i2c_dev) < 0) {
		return ESP_FAIL;
	}

	/* Return ESP_OK */
	return ret;
}

/**
 * @brief Function that reads the last conversion results without changin the
 *        configuration value.
 */
esp_err_t ads101x_get_last_conversion_results(ads101x_t *const me,
		                                          int16_t *adc_result) {
	/* Variable to return error code */
	esp_err_t ret = ESP_OK;

	/* Read the conversion result */
	uint16_t result = 0;

	if (i2c_read(ADS101X_REG_POINTER_CONVERT, &result, me->i2c_dev) < 0) {
		return ESP_FAIL;
	}

	result >>= me->bit_shift;

	if (me->bit_shift == 0) {
		*adc_result = (int16_t)result;
	}
	else {
		/* Shift 12-bit results right 4 bits for the ADS101x,
		 * making sure we keep the sign bit intact */
		if (result > 0x07FF) {
			/* Negative number, extend the sign to 16th bit */
			result |= 0xF000;
		}

		*adc_result = (int16_t)result;
	}

	/* Return ESP_OK */
	return ret;
}

/**
 * @brief Function that computes the voltage value from ADC raw value
 */
float ads101x_compute_volts(ads101x_t *const me, int16_t counts) {
	float fs_range;

	switch (me->gain) {
		case ADS101X_GAIN_TWOTHIRDS:
			fs_range = 6.144f;
			break;
		case ADS101X_GAIN_ONE:
			fs_range = 4.096f;
			break;
		case ADS101X_GAIN_TWO:
			fs_range = 2.048f;
			break;
		case ADS101X_GAIN_FOUR:
			fs_range = 1.024f;
			break;
		case ADS101X_GAIN_EIGHT:
			fs_range = 0.512f;
			break;
		case ADS101X_GAIN_SIXTEEN:
			fs_range = 0.256f;
			break;
		default:
			fs_range = 0.0f;
	}

	return (counts * (fs_range / (32768 >> me->bit_shift)));
}


/**
 * @brief Function that sets the ADS101x gain
 */
void ads101x_set_gain(ads101x_t *const me, ads101x_gain_t gain) {
	me->gain = gain;
}

/**
 * @brief Function that get the ADS101x gain
 */
ads101x_gain_t ads101x_get_gain(ads101x_t *const me) {
	return me->gain;
}

/**
 * @brief Function that sets the ADS101x data rate
 */
void ads101x_set_data_rate(ads101x_t *const me, ads101x_data_rate_t data_rate) {
	me->data_rate = data_rate;
}

/**
 * @brief Function that gets ADS101x data rate
 */
ads101x_data_rate_t ads101x_get_data_rate(ads101x_t *const me) {
	return me->data_rate;
}

/**
 * @brief Function that stars the conversion function
 */
esp_err_t ads101x_start_reading(ads101x_t *const me, uint16_t mux,
		                            ads101x_mode_t mode) {
	/* Variable to return error code */
	esp_err_t ret = ESP_OK;

	/* Fill the ADS101x configuration value */
	uint16_t config = ADS101X_REG_CONFIG_CQUE_1CONV |   /* Set CQUE to any value
	                                                       other than None so we
	                                                       can use it in RDY mode
	                                                       */
			              ADS101X_REG_CONFIG_CLAT_NONLAT |  /* Non-latching */
										ADS101X_REG_CONFIG_CPOL_ACTVLOW | /* ALERT/RDY active low */
										ADS101X_REG_CONFIG_CMODE_TRAD;    /* Traditional comparator */

	/* Configure the reading mode */
	if (mode == ADS101X_MODE_CONTINUOUS) {
		config |= ADS101X_REG_CONFIG_MODE_CONTINUOUS;
	}
	else {
		config |= ADS101X_REG_CONFIG_MODE_SINGLE;
	}

	/* Set PGA/voltage range */
	config |= me->gain;

	/* Set data rate */
	config |= me->data_rate;

	/* Set channel */
	config |= mux;

	/* Set to start a single-conversion */
	config |= ADS101X_REG_CONFIG_OS_SINGLE;

	/* Set the new ADC configuration */
	if (i2c_write(ADS101X_REG_POINTER_CONFIG, config, me->i2c_dev) < 0) {
		return ESP_FAIL;
	}

	/* Set ALERT/RDY to RDY mode */
	if (i2c_write(ADS101X_REG_POINTER_HITHRESH, 0x8000, me->i2c_dev) < 0) {
		return ESP_FAIL;
	}

	if (i2c_write(ADS101X_REG_POINTER_LOWTHRESH, 0x0000, me->i2c_dev) < 0) {
		return ESP_FAIL;
	}

	/* Return ESP_OK */
	return ret;
}

/**
 * @brief Function to check if ADC conversion was completed
 */
esp_err_t ads101x_conversion_complete(ads101x_t *const me) {

	/* Variable to return error code */
	esp_err_t ret = ESP_OK;
	
	/* Wait until is_complete is true */
	while (!me->is_complete) {
		if (!me->int_en) {
			/* Check if the device is performing a conversion */
			uint16_t rx_data = 0;
		
			if (i2c_read(ADS101X_REG_POINTER_CONFIG, &rx_data, me->i2c_dev) < 0) {
				return ESP_FAIL;
			}
		
			me->is_complete = (bool)(rx_data & 0x8000);		
		}
		
		/* Wait for 1 ms to avoid WDT */
		me->delay_ms(1);
	}

	/* Return ESP_OK */
	return ret;
}

/**
 * @brief Function to enable ADS101x interrupt pin. This pin is used to indicante a complete ADC conversion
 */
esp_err_t ads101x_interrupt_enable(ads101x_t *const me, uint32_t int_pin)
{
	/* Variable to return error code */
	esp_err_t ret = ESP_OK;
	
	me->int_pin = int_pin;
	
	/* Configure interrupt pin */
	gpio_config_t gpio_conf;
	gpio_conf.intr_type = GPIO_INTR_NEGEDGE;
	gpio_conf.mode = GPIO_MODE_INPUT;
	gpio_conf.pin_bit_mask = 1ULL << me->int_pin;
	gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;

	ret = gpio_config(&gpio_conf);
	
	if (ret != ESP_OK) {
		return ret;
	}
	
	ret = gpio_install_isr_service(0);
	
	if (ret != ESP_OK) {
		return ret;
	}
	
	ret = gpio_isr_handler_add(me->int_pin, isr_handler, (void *)me);
	
	if (ret != ESP_OK) {
		return ret;
	}
	
	/* Set to true interrupt enable field */
	me->int_en = true;
	
	/* Return ESP_OK */
	return ret;	
}

/**
 * @brief Function to disnable ADS101x interrupt pin. This pin is used to indicante a complete ADC conversion
 */
esp_err_t ads101x_interrupt_disable(ads101x_t *const me)
{
	/* Variable to return error code */ 
	esp_err_t ret = ESP_OK;
	
	ret = gpio_intr_disable(me->int_pin);
	
	if (ret != ESP_OK) {
		return ret;
	}
	
	/* Set to false interrupt enable field */
	me->int_en = false;	
	
	/* Return ESP_OK */
	return ret;	
}

/* Private function definitions ----------------------------------------------*/

/**
 * @brief Function that implements the default I2C read transaction
 */
static int8_t i2c_read(uint8_t reg_addr, uint16_t *reg_data, void *intf) {
	i2c_master_dev_handle_t i2c_dev = (i2c_master_dev_handle_t)intf;

	uint8_t buffer[2] = {0};

	if (i2c_master_transmit_receive(i2c_dev, &reg_addr, 1, buffer, 2, -1) != ESP_OK) {
		return -1;
	}

	*reg_data = (uint16_t)((buffer[0] << 8) | buffer[1]);

	return 0;
}
/**
 * @brief Function that implements the default I2C write transaction
 */
static int8_t i2c_write(uint8_t reg_addr, const uint16_t reg_data,
		                    void *intf) {
//	int8_t ret = 0;

	i2c_master_dev_handle_t i2c_dev = (i2c_master_dev_handle_t)intf;

	uint8_t buffer[32] = {0};

	/* Copy the register address to buffer */
	uint8_t addr_len = sizeof(reg_addr);

	for (uint8_t i = 0; i < addr_len; i++) {
		buffer[i] = (reg_addr & (0xFF << ((addr_len - 1 - i) * 8))) >> ((addr_len - 1 - i) * 8);
	}

	/* Copy the data to buffer */
	uint8_t data_len = sizeof(reg_data);

	for (uint8_t i = 0; i < data_len; i++) {
		buffer[i + addr_len] = (reg_data & (0xFF << ((data_len - 1 - i) * 8))) >> ((data_len - 1 - i) * 8);
	}

	/* Transmit buffer */
	if (i2c_master_transmit(i2c_dev, buffer, addr_len + data_len, -1) != ESP_OK) {
		return -1;
	}

	return 0;
}

static void isr_handler(void *arg) {
	ads101x_t *ads101x = (ads101x_t *)arg;
	ads101x->is_complete = true;
}
/***************************** END OF FILE ************************************/
