#include "LVGL_Driver.h"

static const char *TAG_LVGL = "LVGL";

void *buf1 = NULL;
void *buf2 = NULL;
// static lv_color_t buf1[ LVGL_BUF_LEN ];
// static lv_color_t buf2[ LVGL_BUF_LEN];
    

extern esp_lcd_panel_handle_t panel_handle;
extern lv_display_t *lvgl_display;
// lv_disp_draw_buf_t disp_buf;                                                 // contains internal graphic buffer(s) called draw buffer(s)
// lv_disp_drv_t disp_drv;                                                      // contains callback functions
// lv_indev_drv_t indev_drv;

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

/*Read the touchpad*/
// void touchpad_read( lv_indev_drv_t * drv, lv_indev_data_t * data )
// {
//     uint16_t touchpad_x[5] = {0};
//     uint16_t touchpad_y[5] = {0};
//     uint8_t touchpad_cnt = 0;

//     /* Read touch controller data */
//     esp_lcd_touch_read_data(drv->user_data);

//     /* Get coordinates */
//     bool touchpad_pressed = esp_lcd_touch_get_coordinates(drv->user_data, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 5);

//     // printf("CCCCCCCCCCCCC=%d  \r\n",touchpad_cnt);
//     if (touchpad_pressed && touchpad_cnt > 0) {
//         data->point.x = touchpad_x[0];
//         data->point.y = touchpad_y[0];
//         data->state = LV_INDEV_STATE_PR;
//         // printf("X=%u Y=%u num=%d \r\n", data->point.x, data->point.y,touchpad_cnt);
//     } else {
//         data->state = LV_INDEV_STATE_REL;
//     }
   
// }
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
    ESP_LOGI(TAG_LVGL, "Initialize LVGL library");
    lv_init();
    ESP_LOGI(TAG_LVGL, "Install LVGL tick timer");
    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    lvgl_display = lv_display_create(LCD_H_RES, LCD_V_RES);
    
    lv_display_set_flush_cb(lvgl_display, lvgl_flush_cb);

    buf1 = heap_caps_malloc(LVGL_BUF_LEN, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    assert(buf1);
    buf2 = heap_caps_malloc(LVGL_BUF_LEN, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    assert(buf2);
    // buf1 = heap_caps_malloc(LVGL_BUF_LEN , MALLOC_CAP_SPIRAM);
    // assert(buf1);
    // buf2 = heap_caps_malloc(LVGL_BUF_LEN , MALLOC_CAP_SPIRAM);    
    // assert(buf2);

    lv_display_set_buffers(lvgl_display, buf1, buf2, LVGL_BUF_LEN, LV_DISP_RENDER_MODE_PARTIAL);

    // TODO: figure out how to register touch panel
    
    // lv_indev_drv_init ( &indev_drv );
    // indev_drv.type = LV_INDEV_TYPE_POINTER;
    // indev_drv.disp = disp;
    // indev_drv.read_cb = touchpad_read;
    // indev_drv.user_data = tp;
    // lv_indev_drv_register( &indev_drv );

}
