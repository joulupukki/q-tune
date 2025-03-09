#include <stdio.h>
#include <math.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>

#include <esp_system.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_check.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <esp_timer.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <driver/spi_master.h>
// #include <esp_lcd_ili9341.h>
#include <esp_lcd_st7701.h>

#include <lvgl.h>
#include <esp_lvgl_port.h>

#include "hardware.h"
#include "TCA9554PWR.h"
#include "ST7701S.h"

static const char *TAG="lcd";

esp_err_t lcd_display_brightness_init(void)
{
    const ledc_channel_config_t LCD_backlight_channel = {
        .gpio_num = LCD_BACKLIGHT,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LCD_BACKLIGHT_LEDC_CH,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = 0,
        .duty = 0,
        .hpoint = 0,
        // .flags.output_invert = true // potentially for CYD2USB
        .flags.output_invert = false
    };
 
    const ledc_timer_config_t LCD_backlight_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = 0,
        .freq_hz = 30000, // Moves the PWM backlight control frequency out of the audible range
        .clk_cfg = LEDC_AUTO_CLK
    };

    ESP_ERROR_CHECK(ledc_timer_config(&LCD_backlight_timer));
    ESP_ERROR_CHECK(ledc_channel_config(&LCD_backlight_channel));
 
    return ESP_OK;
}

esp_err_t lcd_display_brightness_set(int brightness_percent)
{
    if (brightness_percent > 100) {
        brightness_percent = 100;
    }
    if (brightness_percent < 0) {
        brightness_percent = 0;
    }

    ESP_LOGI(TAG, "Setting LCD backlight: %d%%", brightness_percent);

    uint32_t duty_cycle = brightness_percent * (8192 / 100);

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LCD_BACKLIGHT_LEDC_CH, duty_cycle));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LCD_BACKLIGHT_LEDC_CH));

    return ESP_OK;
}

esp_err_t lcd_display_backlight_off(void)
{
    return lcd_display_brightness_set(0);
}

esp_err_t lcd_display_backlight_on(void)
{
    return lcd_display_brightness_set(100);
}

esp_err_t lcd_display_rotate(lv_display_t *lvgl_disp, lv_display_rotation_t dir)
{
    if (lvgl_disp)
    {
        lv_display_set_rotation(lvgl_disp, dir);
        return ESP_OK;
    }

    return ESP_FAIL;
}

esp_err_t app_lcd_init(esp_lcd_panel_io_handle_t *lcd_io, esp_lcd_panel_handle_t *lcd_panel) {
    ESP_LOGI(TAG, "Initializing LCD IO");

    ST7701S_reset();
    ST7701S_CS_EN();
    vTaskDelay(pdMS_TO_TICKS(100));
    ST7701S_handle st7701s = ST7701S_newObject(LCD_MOSI, LCD_SCLK, LCD_CS, SPI2_HOST, SPI_METHOD);
    
    ST7701S_screen_init(st7701s, 1);

    // RGB LCD Panel Driver
    ESP_LOGI(TAG, "Install RGB LCD panel driver");
    esp_lcd_rgb_panel_config_t panel_config = {
        .data_width = 16, // RGB565 in parallel mode, thus 16bit in width
        .psram_trans_align = 64,
        .num_fbs = EXAMPLE_LCD_NUM_FB,
// #if CONFIG_EXAMPLE_USE_BOUNCE_BUFFER
        .bounce_buffer_size_px = 10 * EXAMPLE_LCD_H_RES,
// #endif
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .disp_gpio_num = EXAMPLE_PIN_NUM_DISP_EN,
        .pclk_gpio_num = EXAMPLE_PIN_NUM_PCLK,
        .vsync_gpio_num = EXAMPLE_PIN_NUM_VSYNC,
        .hsync_gpio_num = EXAMPLE_PIN_NUM_HSYNC,
        .de_gpio_num = EXAMPLE_PIN_NUM_DE,
        .data_gpio_nums = {
            EXAMPLE_PIN_NUM_DATA0,
            EXAMPLE_PIN_NUM_DATA1,
            EXAMPLE_PIN_NUM_DATA2,
            EXAMPLE_PIN_NUM_DATA3,
            EXAMPLE_PIN_NUM_DATA4,
            EXAMPLE_PIN_NUM_DATA5,
            EXAMPLE_PIN_NUM_DATA6,
            EXAMPLE_PIN_NUM_DATA7,
            EXAMPLE_PIN_NUM_DATA8,
            EXAMPLE_PIN_NUM_DATA9,
            EXAMPLE_PIN_NUM_DATA10,
            EXAMPLE_PIN_NUM_DATA11,
            EXAMPLE_PIN_NUM_DATA12,
            EXAMPLE_PIN_NUM_DATA13,
            EXAMPLE_PIN_NUM_DATA14,
            EXAMPLE_PIN_NUM_DATA15,
        },
        .timings = {
            .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
            .h_res = EXAMPLE_LCD_H_RES,
            .v_res = EXAMPLE_LCD_V_RES, 
            .hsync_back_porch = 10,
            .hsync_front_porch = 50,
            .hsync_pulse_width = 8,
            .vsync_back_porch = 18,
            .vsync_front_porch = 8,
            .vsync_pulse_width = 2,
            .flags.pclk_active_neg = false,
        },
        .flags.fb_in_psram = true, // allocate frame buffer in PSRAM
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &panel_config, lcd_io));
    // esp_lcd_new_panel_st7701(*lcd_io, &panel_config, panel_handle);    

    
    // st7701_vendor_config_t vendor_config = {
    //     .rgb_config = &rgb_config,
    //     // .init_cmds = lcd_init_cmds,      // Uncomment these line if use custom initialization commands
    //     // .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(st7701_lcd_init_cmd_t),
    //     .flags = {
    //         .mirror_by_cmd = 1,             // Only work when `enable_io_multiplex` is set to 0
    //         .enable_io_multiplex = 0,         /**
    //                                          * Set to 1 if panel IO is no longer needed after LCD initialization.
    //                                          * If the panel IO pins are sharing other pins of the RGB interface to save GPIOs,
    //                                          * Please set it to 1 to release the pins.
    //                                          */
    //     },
    // };
    // ESP_LOGI(TAG, "4");

    // const esp_lcd_panel_dev_config_t panel_config = {
    //     .reset_gpio_num = LCD_RESET,
    //     .color_space = ESP_LCD_COLOR_SPACE_BGR,
    //     .bits_per_pixel = LCD_BITS_PIXEL,
    //     .vendor_config = &vendor_config,
    // };

    ESP_LOGI(TAG, "5");
    // esp_err_t r = esp_lcd_new_panel_st7789(*lcd_io, &panel_config, lcd_panel); // For CYD2USB
    // esp_err_t r = esp_lcd_new_panel_ili9341(*lcd_io, &panel_config, lcd_panel);
    esp_err_t r = esp_lcd_new_panel_st7701(*lcd_io, &panel_config, lcd_panel);
    if (r != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_new_panel_st7701() failed: %s", esp_err_to_name(r));
        return r;
    }
    ESP_LOGI(TAG, "6");

    // esp_lcd_panel_reset(*lcd_panel);
    // ESP_LOGI(TAG, "7");
    // esp_lcd_panel_init(*lcd_panel);
    // ESP_LOGI(TAG, "8");

    // esp_lcd_panel_mirror(*lcd_panel, LCD_MIRROR_X, LCD_MIRROR_Y);
    // ESP_LOGI(TAG, "9");
    // esp_lcd_panel_disp_on_off(*lcd_panel, true);
    // ESP_LOGI(TAG, "10");

    ESP_ERROR_CHECK(esp_lcd_panel_reset(*lcd_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(*lcd_panel));
    ST7701S_CS_Dis();
    Backlight_Init();

    return r;
}

lv_display_t *app_lvgl_init(esp_lcd_panel_io_handle_t lcd_io, esp_lcd_panel_handle_t lcd_panel)
{
    ESP_LOGI(TAG, "Initializing LVGL");
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 4096,
        .task_affinity = -1,
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5
    };

    esp_err_t e = lvgl_port_init(&lvgl_cfg);
    ESP_LOGI(TAG, "lvgl_port_init() returned %s", esp_err_to_name(e));

    if (e != ESP_OK)
    {
        ESP_LOGI(TAG, "lvgl_port_init() failed: %s", esp_err_to_name(e));

        return NULL;
    }

    ESP_LOGI(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = lcd_io,
        .panel_handle = lcd_panel,
        .buffer_size = LCD_DRAWBUF_SIZE,
        .double_buffer = LCD_DOUBLE_BUFFER,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = LCD_MIRROR_X,
            .mirror_y = LCD_MIRROR_Y,
        },
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
            .swap_bytes = true,
        }
    };

    return lvgl_port_add_disp(&disp_cfg);;
}
