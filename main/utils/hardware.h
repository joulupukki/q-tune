#pragma once

// // #define LCD_SPI_MISO       -1
// #define LCD_BUSY           (gpio_num_t) GPIO_NUM_NC


// #define TOUCH_X_RES_MIN 0
// #define TOUCH_X_RES_MAX 240
// #define TOUCH_Y_RES_MIN 0
// #define TOUCH_Y_RES_MAX 320

// #define TOUCH_CLOCK_HZ ESP_LCD_TOUCH_SPI_CLOCK_HZ
// #define TOUCH_SPI      SPI3_HOST
// #define TOUCH_SPI_CLK  (gpio_num_t) GPIO_NUM_25
// #define TOUCH_SPI_MOSI (gpio_num_t) GPIO_NUM_32
// #define TOUCH_SPI_MISO (gpio_num_t) GPIO_NUM_39
// #define TOUCH_CS       (gpio_num_t) GPIO_NUM_33
// #define TOUCH_DC       (gpio_num_t) GPIO_NUM_NC
// #define TOUCH_RST      (gpio_num_t) GPIO_NUM_NC
// #define TOUCH_IRQ      (gpio_num_t) GPIO_NUM_NC // GPIO_NUM_36


// Waveshare EBD4
#define LCD_H_RES          480
#define LCD_V_RES          640

#define LCD_RESET          0x01

#define LCD_BUF_LINES      30
#define LCD_DOUBLE_BUFFER  1
#define LCD_DRAWBUF_SIZE   (LCD_H_RES * LCD_BUF_LINES)
#define LCD_MIRROR_X       (true)
#define LCD_MIRROR_Y       (false)

#define LCD_BITS_PIXEL     16
#define LCD_CMD_BITS       (1)
#define LCD_PARAM_BITS     (8)
#define LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)
#define LCD_SPI_HOST       SPI2_HOST
#define LCD_SPI_CLK        (gpio_num_t) GPIO_NUM_2
#define LCD_SPI_MOSI       (gpio_num_t) GPIO_NUM_1

#define LCD_CS             -1      // Using EXIO
#define LCD_DC             (gpio_num_t) GPIO_NUM_2 // BHT: No idea what this should be yet

#define LCD_BACKLIGHT      (gpio_num_t) GPIO_NUM_6
#define LCD_BACKLIGHT_LEDC_CH  (0)

#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (30 * 1000 * 1000)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_HSYNC          38
#define EXAMPLE_PIN_NUM_VSYNC          39
#define EXAMPLE_PIN_NUM_DE             40
#define EXAMPLE_PIN_NUM_PCLK           41
#define EXAMPLE_PIN_NUM_DATA0          5  // B0
#define EXAMPLE_PIN_NUM_DATA1          45 // B1
#define EXAMPLE_PIN_NUM_DATA2          48 // B2
#define EXAMPLE_PIN_NUM_DATA3          47 // B3
#define EXAMPLE_PIN_NUM_DATA4          21 // B4
#define EXAMPLE_PIN_NUM_DATA5          14 // G0
#define EXAMPLE_PIN_NUM_DATA6          13 // G1
#define EXAMPLE_PIN_NUM_DATA7          12 // G2
#define EXAMPLE_PIN_NUM_DATA8          11 // G3
#define EXAMPLE_PIN_NUM_DATA9          10 // G4
#define EXAMPLE_PIN_NUM_DATA10         9  // G5
#define EXAMPLE_PIN_NUM_DATA11         46 // R0
#define EXAMPLE_PIN_NUM_DATA12         3  // R1
#define EXAMPLE_PIN_NUM_DATA13         8  // R2
#define EXAMPLE_PIN_NUM_DATA14         18 // R3
#define EXAMPLE_PIN_NUM_DATA15         17 // R4
#define EXAMPLE_PIN_NUM_DISP_EN        -1