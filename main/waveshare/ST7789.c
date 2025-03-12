#include "ST7789.h"

// #include "esp_lcd_panel_st7789.h"

static const char *TAG_LCD = "ST7789";

extern lv_display_t *lvgl_display;

esp_lcd_panel_handle_t panel_handle = NULL;

void LCD_Init(void)
{
    ESP_LOGI(TAG_LCD, "Initialize SPI bus");                                            
    spi_bus_config_t buscfg = {                                                         
        .sclk_io_num = PIN_NUM_SCLK,                                            
        .mosi_io_num = PIN_NUM_MOSI,                                            
        .miso_io_num = PIN_NUM_MISO,                                            
        .quadwp_io_num = -1,                                                            
        .quadhd_io_num = -1,                                                            
        .max_transfer_sz = LCD_H_RES * LCD_V_RES * sizeof(uint16_t),    
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));            

    ESP_LOGI(TAG_LCD, "Install panel IO");                                              
    esp_lcd_panel_io_handle_t io_handle = NULL;                                         
    esp_lcd_panel_io_spi_config_t io_config = {                                             
        .dc_gpio_num = PIN_NUM_LCD_DC,
        .cs_gpio_num = PIN_NUM_LCD_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = lvgl_notify_lvgl_flush_ready,
        .user_ctx = lvgl_display,
    };
    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    // ESP_LOGI(TAG_LCD, "Install ST7789 panel driver");
    // esp_lcd_panel_dev_config_t panel_dev_config = {
    //     .reset_gpio_num = PIN_NUM_LCD_RST,
    //     .rgb_endian = LCD_RGB_ENDIAN_BGR,
    //     .bits_per_pixel = 16,
    //     .flags = {
    //         .reset_active_high = 1,
    //     },
    // };

    // ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_dev_config, &panel_handle));

    esp_lcd_panel_dev_st7789t_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_LCD_RST,
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789t(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));

    // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG_LCD, "Turn on LCD backlight");
    // gpio_set_level(PIN_NUM_BK_LIGHT, LCD_BK_LIGHT_ON_LEVEL);
    
    Backlight_Init();    
    // TOUCH_Init();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Backlight program

uint8_t LCD_Backlight = 70;
static ledc_channel_config_t ledc_channel;
void Backlight_Init(void)
{
    ESP_LOGI(TAG_LCD, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_NUM_BK_LIGHT
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 5000,
        .speed_mode = LEDC_LS_MODE,
        .timer_num = LEDC_HS_TIMER,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel.channel    = LEDC_HS_CH0_CHANNEL;
    ledc_channel.duty       = 0;
    ledc_channel.gpio_num   = PIN_NUM_BK_LIGHT;
    ledc_channel.speed_mode = LEDC_LS_MODE;
    ledc_channel.timer_sel  = LEDC_HS_TIMER;
    ledc_channel_config(&ledc_channel);
    ledc_fade_func_install(0);
    
    Set_Backlight(LCD_Backlight);      //0~100    
}
void Set_Backlight(uint8_t Light)
{   
    if(Light > Backlight_MAX) Light = Backlight_MAX;
    uint16_t Duty = LEDC_MAX_Duty-(81*(Backlight_MAX-Light));
    if(Light == 0) 
        Duty = 0;
    ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, Duty);
    ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
}
// end Backlight program