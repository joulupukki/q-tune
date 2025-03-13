#include "waveshare.h"

#include "esp_lcd_touch.h"
#include "esp_lvgl_port.h"

static const char *TAG = "Waveshare";

lv_display_t *lvgl_display = NULL;

esp_lcd_panel_io_handle_t lcd_io = NULL;
esp_lcd_panel_handle_t lcd_panel = NULL;
esp_lcd_touch_handle_t tp = NULL;
// lvgl_port_touch_cfg_t touch_cfg = NULL;

void lvgl_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map);

esp_err_t waveshare_lcd_init() {
    LCD_Init();

    return ESP_OK;
}

esp_err_t waveshare_lvgl_init() {
    LVGL_Init();

    //     ESP_ERROR_CHECK(lcd_display_brightness_set(userSettings->displayBrightness * 10 + 10)); // Adjust for 0 - 10%, 1 - 20%, etc.
    //     ESP_ERROR_CHECK(lcd_display_rotate(lvgl_display, userSettings->getDisplayOrientation()));
    //     // ESP_ERROR_CHECK(lcd_display_rotate(lvgl_display, LV_DISPLAY_ROTATION_0)); // Upside Down
    //     lvgl_port_unlock();
    // }

    return ESP_OK;
}

esp_err_t lcd_display_brightness_set(uint8_t brightness) {
    ESP_LOGI(TAG, "TODO: Implement lcd_display_brightness_set()");
    return ESP_OK;
    // if (brightness > Backlight_MAX || brightness < 0) {
    //   ESP_LOGI(TAG, "Set Backlight parameters in the range of 0 to 100 ");
    // } else {
    //   ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, brightness*(1024/100)));    // Set duty
    //   ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));                 // Update duty to apply the new value
    // }
  
    // return ESP_OK;
  }
  
esp_err_t lcd_display_rotate(lv_display_t * lvgl_disp, lv_display_rotation_t dir) {
    ESP_LOGI(TAG, "TODO: Implement lcd_display_rotate()");
    return ESP_OK;
    // if (lvgl_disp) {
    //     lv_display_set_rotation(lvgl_disp, dir);
    //     return ESP_OK;
    // }

    // return ESP_FAIL;
}