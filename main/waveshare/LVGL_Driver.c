#include "LVGL_Driver.h"

#include "esp_lvgl_port.h"
#include "defines.h"

static const char *TAG = "LVGL";

extern esp_lcd_panel_io_handle_t lcd_io;
extern esp_lcd_panel_handle_t lcd_panel;
extern lv_display_t *lvgl_display;

void increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

bool lvgl_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_display_t *display = (lv_display_t *)user_ctx;
    if (!display) {
        return false;
    }
    lv_disp_flush_ready(display);
    return false;
}

void lvgl_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t * px_map)
{
    // copy a buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map);
    // lv_display_flush_ready(lvgl_display);
}

// /* Rotate display and touch, when rotated screen in LVGL. Called when driver parameters are updated. */
// void lvgl_port_update_callback(lv_disp_drv_t *drv)
// {
//     esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;

//     switch (drv->rotated) {
//     case LV_DISP_ROT_NONE:
//         // Rotate LCD display
//         esp_lcd_panel_swap_xy(panel_handle, false);
//         esp_lcd_panel_mirror(panel_handle, true, false);
//         break;
//     case LV_DISP_ROT_90:
//         // Rotate LCD display
//         esp_lcd_panel_swap_xy(panel_handle, true);
//         esp_lcd_panel_mirror(panel_handle, true, true);
//         break;
//     case LV_DISP_ROT_180:
//         // Rotate LCD display
//         esp_lcd_panel_swap_xy(panel_handle, false);
//         esp_lcd_panel_mirror(panel_handle, false, true);
//         break;
//     case LV_DISP_ROT_270:
//         // Rotate LCD display
//         esp_lcd_panel_swap_xy(panel_handle, true);
//         esp_lcd_panel_mirror(panel_handle, false, false);
//         break;
//     }
// }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
lv_disp_t *disp;
void LVGL_Init(void)
{
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 4096,
        .task_affinity = -1,
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5,
    };

    esp_err_t e = lvgl_port_init(&lvgl_cfg);
    if (e != ESP_OK) {
        ESP_LOGI(TAG, "lvgl_port_init() failed: %s", esp_err_to_name(e));
        return;
    }

    ESP_LOGI(TAG, "Adding LCD Screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = lcd_io,
        .panel_handle = lcd_panel,
        .buffer_size = LCD_DRAWBUF_SIZE * sizeof(uint16_t),
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
            .swap_bytes = false,
        },
    };

    lvgl_display = lvgl_port_add_disp(&disp_cfg);


}
