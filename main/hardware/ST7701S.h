// This file was taken from the Waveshare demo and adapted to be used with LVGL 9

#pragma once

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/ledc.h"

#include "TCA9554PWR.h"
#include <lvgl.h>

#define SPI_METHOD 1
#define IOEXPANDER_METHOD 0

/********************* LCD *********************/

#define LCD_MOSI 1
#define LCD_SCLK 2
#define LCD_CS  -1      // Using EXIO
// The pixel number in horizontal and vertical
#define LCD_V_RES              640
#define LCD_H_RES              480

// #define LCD_PIXEL_CLOCK_HZ     (30 * 1000 * 1000) // original as defined by Waveshare
#define LCD_PIXEL_CLOCK_HZ     (28 * 1000 * 1000) // makes things work without glitches
#define LCD_BK_LIGHT_ON_LEVEL  1
#define LCD_BK_LIGHT_OFF_LEVEL !LCD_BK_LIGHT_ON_LEVEL
#define PIN_NUM_BK_LIGHT       6
#define PIN_NUM_HSYNC          38
#define PIN_NUM_VSYNC          39
#define PIN_NUM_DE             40
#define PIN_NUM_PCLK           41
#define PIN_NUM_DATA0          5  // B0
#define PIN_NUM_DATA1          45 // B1
#define PIN_NUM_DATA2          48 // B2
#define PIN_NUM_DATA3          47 // B3
#define PIN_NUM_DATA4          21 // B4
#define PIN_NUM_DATA5          14 // G0
#define PIN_NUM_DATA6          13 // G1
#define PIN_NUM_DATA7          12 // G2
#define PIN_NUM_DATA8          11 // G3
#define PIN_NUM_DATA9          10 // G4
#define PIN_NUM_DATA10         9  // G5
#define PIN_NUM_DATA11         46 // R0
#define PIN_NUM_DATA12         3  // R1
#define PIN_NUM_DATA13         8  // R2
#define PIN_NUM_DATA14         18 // R3
#define PIN_NUM_DATA15         17 // R4
#define PIN_NUM_DISP_EN        -1

#define LCD_NUM_OF_FRAME_BUFFERS        2

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          PIN_NUM_BK_LIGHT      // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_10_BIT // Set duty resolution to 10 bits
#define LEDC_DUTY               (512)    // Set duty to 50%. (2^10) * 50% = 512
#define LEDC_FREQUENCY          (30000) // Frequency in Hertz. Set frequency at 30 kHz to keep it out of the audible range.
#define Backlight_MAX           100

#define CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM 0 // With this set to 1 the refreshes get MUCH slower

#if CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM
extern SemaphoreHandle_t sem_vsync_end;
extern SemaphoreHandle_t sem_gui_ready;
#endif

extern uint8_t LCD_Backlight; 
extern esp_lcd_panel_handle_t panel_handle;

typedef struct{
    char method_select;
    //SPI config_t
    spi_device_handle_t spi_device;
    spi_bus_config_t spi_io_config_t;
    spi_device_interface_config_t st7701s_protocol_config_t;
} ST7701S;

typedef ST7701S * ST7701S_handle;

ST7701S_handle ST7701S_newObject(int SDA, int SCL, int CS, char channel_select, char method_select);//Create new object
void ST7701S_screen_init(ST7701S_handle St7701S_handle, unsigned char type);//Screen initialization
void ST7701S_delObject(ST7701S_handle St7701S_handle);//Delete object
void ST7701S_WriteCommand(ST7701S_handle St7701S_handle, uint8_t cmd);//SPI write instruction
void ST7701S_WriteData(ST7701S_handle St7701S_handle, uint8_t data);//SPI write data
esp_err_t ST7701S_CS_EN(void);//Enables SPI CS
esp_err_t ST7701S_CS_Dis(void);//Disable SPI CS
esp_err_t ST7701S_reset(void);// LCD Reset


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
esp_err_t LCD_Init(esp_lcd_panel_handle_t *);

/********************* BackLight *********************/
void Backlight_Init(void);

/// @brief Set the LCD Backlight Brightness.
/// @param  A value between 0 and 100.
/// @return ESP_OK if successful.
esp_err_t lcd_display_brightness_set(uint8_t brightness);

esp_err_t lcd_display_rotate(lv_display_t * lvgl_disp, lv_display_rotation_t dir);
