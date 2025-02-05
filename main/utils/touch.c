#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include <esp_log.h>
#include <esp_err.h>
#include <esp_timer.h>
#include <esp_lcd_touch.h>
#include <esp_lcd_touch_xpt2046.h>

#include <driver/spi_master.h>
#include <driver/gpio.h>

#include "esp_lvgl_port.h"
#include "esp_log.h"

#include "hardware.h"
#include "touch.h"

#include "nvs_flash.h"
#include "nvs.h"

#define TOUCH_NUM_OF_CALIB_POINTS 3
#define MAX_TOUCH_DIFF_ALLOWED 50

// NVS currently doesn't support storing floating point values so we have to
// first convert to int before storing (and vice versa for reading).
#define TOUCH_CALIB_PRECISION_MULTIPLIER 100000 // 5 decimal points of precision

static const char *TAG = "Touch";

// Calibration data structure
// typedef struct {
//     int32_t xScale;
//     int32_t xOffset;
//     int32_t xDelta;
//     int32_t yScale;
//     int32_t yOffset;
//     int32_t yDelta;
// } touch_calibration_data_t;

// static touch_calibration_data_t calibration_data;
// static touch_calibration_data_t nvs_calibration_data;
static float transform_matrix[6]; // Transformation matrix coefficients

static lv_point_t screen_points[TOUCH_NUM_OF_CALIB_POINTS] = {
    {50, 50},
    {220, 50},
    {120, 180}
};
static lv_point_t touch_points[TOUCH_NUM_OF_CALIB_POINTS];

static nvs_handle_t nvsHandle;

uint8_t current_calib_idx;

// static lv_point_t offsets = {0, 0};
static bool calibration_done = false;

static void calculate_transformation_matrix(void) {
    int32_t dx1 = screen_points[1].x - screen_points[0].x;
    int32_t dy1 = screen_points[1].y - screen_points[0].y;
    int32_t dx2 = screen_points[2].x - screen_points[0].x;
    int32_t dy2 = screen_points[2].y - screen_points[0].y;

    int32_t tx1 = touch_points[1].x - touch_points[0].x;
    int32_t ty1 = touch_points[1].y - touch_points[0].y;
    int32_t tx2 = touch_points[2].x - touch_points[0].x;
    int32_t ty2 = touch_points[2].y - touch_points[0].y;

    float denominator = (float)(dx1 * dy2 - dx2 * dy1);

    transform_matrix[0] = (tx1 * dy2 - tx2 * dy1) / denominator;
    transform_matrix[1] = (dx1 * tx2 - dx2 * tx1) / denominator;
    transform_matrix[2] = touch_points[0].x - 
                          (transform_matrix[0] * screen_points[0].x + transform_matrix[1] * screen_points[0].y);

    transform_matrix[3] = (ty1 * dy2 - ty2 * dy1) / denominator;
    transform_matrix[4] = (dx1 * ty2 - dx2 * ty1) / denominator;
    transform_matrix[5] = touch_points[0].y - 
                          (transform_matrix[3] * screen_points[0].x + transform_matrix[4] * screen_points[0].y);

    // const float divisor = (
    //       (float)touch_points[0].x * ((float)touch_points[2].y - (float)touch_points[1].y)
    //     - (float)touch_points[1].x * (float)touch_points[2].y
    //     + (float)touch_points[1].y * (float)touch_points[2].x
    //     + (float)touch_points[0].y * ((float)touch_points[1].x - (float)touch_points[2].x)
    // );

    // transform_matrix[0] = ((float)screen_points[0].x * ((float)touch_points[2].y - (float)touch_points[1].y)
    //         - (float)screen_points[1].x * (float)touch_points[2].y
    //         + (float)screen_points[2].x * (float)touch_points[1].y
    //         + ((float)screen_points[1].x - (float)screen_points[2].x) * (float)touch_points[0].y
    //     ) / divisor;
    // transform_matrix[1] = ((float)screen_points[0].x * ((float)touch_points[2].x - (float)touch_points[1].x)
    //         - (float)screen_points[1].x * (float)touch_points[2].x
    //         + (float)screen_points[2].x * (float)touch_points[1].x
    //         + ((float)screen_points[1].x - (float)screen_points[2].x) * (float)touch_points[0].x
    //     ) / divisor;
    // transform_matrix[2] = ((float)screen_points[0].x * ((float)touch_points[1].y * (float)touch_points[2].x - (float)touch_points[1].x * (float)touch_points[2].y)
    //         + (float)touch_points[0].x * ((float)screen_points[1].x * (float)touch_points[2].y - (float)screen_points[2].x * (float)touch_points[1].y)
    //         + (float)touch_points[0].y * ((float)screen_points[2].x * (float)touch_points[1].x - (float)screen_points[1].x * (float)touch_points[2].x)
    //     ) / divisor;
    // transform_matrix[3] = ((float)screen_points[0].y * ((float)touch_points[2].y - (float)touch_points[1].y)
    //         - (float)screen_points[1].y * (float)touch_points[2].y
    //         + (float)screen_points[2].y * (float)touch_points[1].y
    //         + ((float)screen_points[1].y - (float)screen_points[2].y) * (float)touch_points[0].y
    //     ) / divisor;
    // transform_matrix[4] = ((float)screen_points[0].y * ((float)touch_points[2].x - (float)touch_points[1].x)
    //         - (float)screen_points[1].y * (float)touch_points[2].x
    //         + (float)screen_points[2].y * (float)touch_points[1].x
    //         + ((float)screen_points[1].y - (float)screen_points[2].y) * (float)touch_points[0].x
    //     ) / divisor;
    // transform_matrix[5] = ((float)screen_points[0].y * ((float)touch_points[1].y * (float)touch_points[2].x - (float)touch_points[1].x * (float)touch_points[2].y)
    //         + (float)touch_points[0].x * ((float)screen_points[1].y * (float)touch_points[2].y - (float)screen_points[2].y * (float)touch_points[1].y)
    //         + (float)touch_points[0].y * ((float)screen_points[2].y * (float)touch_points[1].x - (float)screen_points[1].y * (float)touch_points[2].x)
    //     ) / divisor;
    // ESP_LOGI(TAG, "Transformation matrix calculated: [%f, %f, %f, %f, %f, %f]",
    //          transform_matrix[0], transform_matrix[1], transform_matrix[2],
    //          transform_matrix[3], transform_matrix[4], transform_matrix[5]);
    
    // Store floats as Int32 (*100 to get 2 points of the decimals)
    nvs_set_i32(nvsHandle, "touch_cal_1", (int32_t)(transform_matrix[1] * TOUCH_CALIB_PRECISION_MULTIPLIER));
    nvs_set_i32(nvsHandle, "touch_cal_2", (int32_t)(transform_matrix[2] * TOUCH_CALIB_PRECISION_MULTIPLIER));
    nvs_set_i32(nvsHandle, "touch_cal_3", (int32_t)(transform_matrix[3] * TOUCH_CALIB_PRECISION_MULTIPLIER));
    nvs_set_i32(nvsHandle, "touch_cal_0", (int32_t)(transform_matrix[0] * TOUCH_CALIB_PRECISION_MULTIPLIER));
    nvs_set_i32(nvsHandle, "touch_cal_4", (int32_t)(transform_matrix[4] * TOUCH_CALIB_PRECISION_MULTIPLIER));
    nvs_set_i32(nvsHandle, "touch_cal_5", (int32_t)(transform_matrix[5] * TOUCH_CALIB_PRECISION_MULTIPLIER));
    nvs_commit(nvsHandle);
    calibration_done = true;
}

static uint16_t map(uint16_t n, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) {
    uint16_t value = (n - in_min) * (out_max - out_min) / (in_max - in_min);
    return (value < out_min) ? out_min : ((value > out_max) ? out_max : value);
}

static void process_coordinates(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num) {
    uint16_t actualX = *x;
    uint16_t actualY = *y;

    // if (calibration_done) {
    //     // actualX = (uint16_t)(transform_matrix[0] * actualX + transform_matrix[1] * actualY + transform_matrix[2]);
    //     // actualY = (uint16_t)(transform_matrix[3] * actualX + transform_matrix[4] * actualY + transform_matrix[5]);

    //     actualX = (uint16_t)(actualX * transform_matrix[0] + actualY * transform_matrix[1] + transform_matrix[2]);
    //     actualY = (uint16_t)(actualY * transform_matrix[3] + actualY * transform_matrix[4] + transform_matrix[5]);
    // }
    *x = map(actualX, TOUCH_X_RES_MIN, TOUCH_X_RES_MAX, 0, LCD_H_RES);
    *y = map(actualY, TOUCH_Y_RES_MIN, TOUCH_Y_RES_MAX, 0, LCD_V_RES);
}

void display_instructions(lv_obj_t *parent) {
    // Create a text label aligned center: https://docs.lvgl.io/master/widgets/label.html
    lv_obj_t * text_label = lv_label_create(parent);
    lv_label_set_text(text_label, "Tap each crosshair until it disappears.");
    lv_obj_align(text_label, LV_ALIGN_CENTER, 0, 0);

    // Set font type and font size. More information: https://docs.lvgl.io/master/overview/font.html
    static lv_style_t style_text_label;
    lv_style_init(&style_text_label);
    lv_style_set_text_font(&style_text_label, &lv_font_montserrat_14);
    lv_style_set_text_color(&style_text_label, lv_color_black());
    lv_obj_add_style(text_label, &style_text_label, 0);
}

static void draw_calibration_point(lv_obj_t *parent, lv_point_t point) {
    static lv_point_precise_t h_line_points[] = { {0, 0}, {10, 0} };
    static lv_point_precise_t v_line_points[] = { {0, 0}, {0, 10} };

    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_line_width(&style_line, 2);
    lv_style_set_line_color(&style_line, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_line_rounded(&style_line, true);

    // Create crosshair lines
    lv_obj_t* crosshair_h = lv_line_create(parent);
    lv_obj_t* crosshair_v = lv_line_create(parent);

    lv_line_set_points(crosshair_h, h_line_points, 2); // Set the coordinates for the crosshair_h line
    lv_obj_add_style(crosshair_h, &style_line, 0);

    lv_line_set_points(crosshair_v, v_line_points, 2); // Set the coordinates for the crosshair_h line
    lv_obj_add_style(crosshair_v, &style_line, 0);

    lv_obj_set_pos(crosshair_h, point.x - 5, point.y);
    lv_obj_set_pos(crosshair_v, point.x, point.y - 5);

    // lv_obj_t *circle = lv_obj_create(parent);
    // lv_obj_set_size(circle, 20, 20);
    // lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);
    // lv_obj_set_style_bg_color(circle, lv_palette_main(LV_PALETTE_RED), 0);
    // lv_obj_align(circle, LV_ALIGN_TOP_LEFT, point.x - 10, point.y - 10); // -10 on each to account for center of a 20x20 circle
}

// Prompt user to touch a specific point
static void calibrate_point() {
    if (!lvgl_port_lock(0)) {
        return;
    }
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr); // Clear screen
    display_instructions(scr);
    draw_calibration_point(scr, screen_points[current_calib_idx]);
    lvgl_port_unlock();

    ESP_LOGI(TAG, "Touch the point at (%ld, %ld).", screen_points[current_calib_idx].x, screen_points[current_calib_idx].y);
}

lv_obj_t *menu_screen;
extern esp_lcd_touch_handle_t touch_handle;

/// Allows us to save off the original callback so we can restore it after
/// performing touch screen calibration.
lv_indev_read_cb_t orig_read_cb = NULL;

lv_display_t *display = NULL;

void finish_calibration() {
    // Calculate the offsets and save them to NVS.
    // int32_t diffXTotal = 0;
    // int32_t diffYTotal = 0;
    // for (int i = 0; i < TOUCH_NUM_OF_CALIB_POINTS; i++) {
    //     diffXTotal += touch_points[i].x;
    //     diffYTotal += touch_points[i].y;
    // }
    // offsets.x = diffXTotal / TOUCH_NUM_OF_CALIB_POINTS;
    // offsets.y = diffYTotal / TOUCH_NUM_OF_CALIB_POINTS;
    calculate_transformation_matrix();

    // int32_t dx1 = touch_points[1].x - touch_points[0].x;
    // int32_t dy1 = touch_points[1].y - touch_points[0].y;
    // int32_t dx2 = touch_points[2].x - touch_points[0].x;
    // int32_t dy2 = touch_points[2].y - touch_points[0].y;

    // int32_t dpx1 = screen_points[1].x - screen_points[0].x;
    // int32_t dpy1 = screen_points[1].y - screen_points[0].y;
    // int32_t dpx2 = screen_points[2].x - screen_points[0].x;
    // int32_t dpy2 = screen_points[2].y - screen_points[0].y;

    // calibration_data.xScale = (dpx1 * dy2 - dpy1 * dx2) / (dx1 * dy2 - dy1 * dx2);
    // calibration_data.xOffset = screen_points[0].x - (calibration_data.xScale * touch_points[0].x);
    // calibration_data.yScale = abs((dpx2 * dy1 - dpy2 * dx1) / (dx1 * dy2 - dy1 * dx2));
    // calibration_data.yOffset = screen_points[0].y - (calibration_data.yScale * touch_points[0].y);

    // nvs_set_i32(nvsHandle, "xScale", calibration_data.xScale);
    // nvs_set_i32(nvsHandle, "xOffset", calibration_data.xOffset);
    // nvs_set_i32(nvsHandle, "yScale", calibration_data.xScale);
    // nvs_set_i32(nvsHandle, "yOffset", calibration_data.xOffset);
    // nvs_commit(nvsHandle);
    // calibration_done = true;
    // ESP_LOGI(TAG, "xScale: %ld, xOffset: %ld, yScale: %ld, yOffset; %ld", calibration_data.xScale, calibration_data.xOffset, calibration_data.yScale, calibration_data.yOffset);

    // ESP_LOGI(TAG, "avg offset: (%ld, %ld)", offsets.x, offsets.y);

    if (!lvgl_port_lock(0)) {
        return;
    }

    // Restore the original touch handler so the UI works
    lv_indev_t *touch_indev = lv_indev_get_next(NULL);
    if (touch_indev != NULL && orig_read_cb != NULL) {
        lv_indev_set_read_cb(touch_indev, orig_read_cb);
    }

    lv_obj_t *calib_screen = lv_scr_act();

    lv_screen_load(menu_screen);

    lv_obj_clean(calib_screen);
    lv_obj_del(calib_screen);

    lvgl_port_unlock();
}

void touch_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    ESP_LOGI(TAG, "touch occurred");

    int32_t raw_x = 0, raw_y = 0, strength = 0;
    uint8_t point_num;
    bool pressed = false;

    // These two "last_n" variables help to debounce touches
    static int32_t last_x = 0;
    static int32_t last_y = 0;

    while (!pressed) {
        esp_lcd_touch_read_data(touch_handle); // Poll touch panel

        if (esp_lcd_touch_get_coordinates(touch_handle, &raw_x, &raw_y, &strength, &point_num, 1) && (raw_x != last_x && raw_y != last_y)) { // Get first touch point
            ESP_LOGI(TAG, "raw_x: %ld, raw_y: %ld", raw_x, raw_y);
            int32_t pointX = 0;
            int32_t pointY = 0;
            // touch_points[point_idx].x = raw_x;
            // touch_points[point_idx].y = raw_y;
            // ESP_LOGI(TAG, "Recorded raw touch: (%d, %d)", raw_x, raw_y);
            // ESP_LOGI(TAG, "Touch the point at (%ld, %ld).", touch_points[point_idx].x, touch_points[point_idx].y);
            
            // Convert the raw point to a screen coordinate if needed (based on
            // screen rotation).
            lv_display_rotation_t current_rotation = lv_disp_get_rotation(display);
            switch (current_rotation) {
            case LV_DISPLAY_ROTATION_0:
                pointX = raw_x;
                pointY = raw_y;
                break;
            case LV_DISPLAY_ROTATION_90:
                pointX = LV_HOR_RES - raw_y - 1;
                pointY = raw_x;
                break;
            case LV_DISPLAY_ROTATION_180:
                pointX = LV_HOR_RES - raw_x - 1;
                pointY = LV_VER_RES - raw_y - 1;
                break;
            case LV_DISPLAY_ROTATION_270:
                pointX = raw_y;
                pointY = LV_VER_RES - raw_x - 1;
                break;
            }

            int32_t diffX = pointX - screen_points[current_calib_idx].x;
            int32_t diffY = pointY - screen_points[current_calib_idx].y;
            ESP_LOGI(TAG, "pointX: %ld, diffX: %ld, pointY: %ld, diffY: %ld", pointX, diffX, pointY, diffY);

            if (abs(diffX) > MAX_TOUCH_DIFF_ALLOWED || abs(diffY) > MAX_TOUCH_DIFF_ALLOWED) {
                continue;
            }
            ESP_LOGI(TAG, "PRESSED");
            pressed = true;
            touch_points[current_calib_idx].x = pointX;
            touch_points[current_calib_idx].y = pointY;

            ESP_LOGI(TAG, "Recorded point at (%ld, %ld).", pointX, pointY);

            current_calib_idx++;
            if (current_calib_idx >= TOUCH_NUM_OF_CALIB_POINTS) {
                finish_calibration();
                vTaskDelay(pdMS_TO_TICKS(300));
                return;
            }
            calibrate_point();
            vTaskDelay(pdMS_TO_TICKS(300));
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void calibrate_touch_screen(esp_lcd_touch_handle_t tp) {
    ESP_LOGI(TAG, "Starting touch calibration...");

    if (!lvgl_port_lock(0)) {
        return;
    }

    menu_screen = lv_scr_act();
    lv_obj_t *calib_screen = lv_obj_create(NULL);
    lv_obj_set_size(calib_screen, lv_pct(100), lv_pct(100));
    lv_obj_set_scrollbar_mode(calib_screen, LV_SCROLLBAR_MODE_OFF);
    // lv_obj_set_style_bg_color(calib_screen, lv_palette_darken(LV_PALETTE_BLUE_GREY, 4), 0); // Optional background color
    lv_obj_set_style_bg_color(calib_screen, lv_color_white(), 0); // Optional background color
    ESP_LOGI(TAG, "c");
    lv_screen_load(calib_screen); // Show the new calibration screen
    lvgl_port_unlock();
    vTaskDelay(pdMS_TO_TICKS(50));
    ESP_LOGI(TAG, "d");

    lv_indev_t *touch_indev = lv_indev_get_next(NULL);
    if (touch_indev != NULL) {
        ESP_LOGI(TAG, "Got an indev");
        orig_read_cb = lv_indev_get_read_cb(touch_indev);
        lv_indev_set_read_cb(touch_indev, touch_cb);
    }
    current_calib_idx = 0;
    calibration_done = false;

    calibrate_point();
}

esp_err_t touch_init(esp_lcd_touch_handle_t *tp, lv_display_t *disp) {
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    display = disp; // used later to know screen rotation

    // Use saved touch screen calibration info.
    nvs_open("touch-calib", NVS_READWRITE, &nvsHandle);
    int32_t cal0, cal1, cal2, cal3, cal4, cal5;

    if (nvs_get_i32(nvsHandle, "touch_cal_0", &cal0) == ESP_OK && 
            nvs_get_i32(nvsHandle, "touch_cal_1", &cal1) == ESP_OK &&
            nvs_get_i32(nvsHandle, "touch_cal_2", &cal2) == ESP_OK &&
            nvs_get_i32(nvsHandle, "touch_cal_3", &cal3) == ESP_OK &&
            nvs_get_i32(nvsHandle, "touch_cal_4", &cal4) == ESP_OK && 
            nvs_get_i32(nvsHandle, "touch_cal_5", &cal5) == ESP_OK) {
        transform_matrix[0] = ((float)cal0) / TOUCH_CALIB_PRECISION_MULTIPLIER;
        transform_matrix[1] = ((float)cal1) / TOUCH_CALIB_PRECISION_MULTIPLIER;
        transform_matrix[2] = ((float)cal2) / TOUCH_CALIB_PRECISION_MULTIPLIER;
        transform_matrix[3] = ((float)cal3) / TOUCH_CALIB_PRECISION_MULTIPLIER;
        transform_matrix[4] = ((float)cal4) / TOUCH_CALIB_PRECISION_MULTIPLIER;
        transform_matrix[5] = ((float)cal5) / TOUCH_CALIB_PRECISION_MULTIPLIER;
        calibration_done = true; // This allows process_coordinates to work

        ESP_LOGI(TAG, "Transformation matrix loaded from NVS: [%f, %f, %f, %f, %f, %f]",
             transform_matrix[0], transform_matrix[1], transform_matrix[2],
             transform_matrix[3], transform_matrix[4], transform_matrix[5]);
    }

    const esp_lcd_panel_io_spi_config_t tp_io_config = { .cs_gpio_num = TOUCH_CS,
        .dc_gpio_num = TOUCH_DC,
        .spi_mode = 0,
        .pclk_hz = TOUCH_CLOCK_HZ,
        .trans_queue_depth = 3,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .flags = { .dc_low_on_data = 0, .octal_mode = 0, .sio_mode = 0, .lsb_first = 0, .cs_high_active = 0 } };

    static const int SPI_MAX_TRANSFER_SIZE = 32768;
    const spi_bus_config_t buscfg_touch = { .mosi_io_num = TOUCH_SPI_MOSI,
        .miso_io_num = TOUCH_SPI_MISO,
        .sclk_io_num = TOUCH_SPI_CLK,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .data4_io_num = GPIO_NUM_NC,
        .data5_io_num = GPIO_NUM_NC,
        .data6_io_num = GPIO_NUM_NC,
        .data7_io_num = GPIO_NUM_NC,
        .max_transfer_sz = SPI_MAX_TRANSFER_SIZE,
        .flags = SPICOMMON_BUSFLAG_SCLK | SPICOMMON_BUSFLAG_MISO | SPICOMMON_BUSFLAG_MOSI | SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS,
        .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
        .intr_flags = ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_IRAM };

    esp_lcd_touch_config_t tp_cfg = {.x_max = LCD_H_RES,
                                   .y_max = LCD_V_RES,
                                   .rst_gpio_num = TOUCH_RST,
                                   .int_gpio_num = TOUCH_IRQ,
                                   .levels = {.reset = 0, .interrupt = 0},
                                   .flags =
                                       {
                                           .swap_xy = false,
                                           .mirror_x = LCD_MIRROR_X,
                                           .mirror_y = LCD_MIRROR_Y,
                                       },
                                   .process_coordinates = process_coordinates,
                                   .interrupt_callback = NULL};

    ESP_ERROR_CHECK(spi_bus_initialize(TOUCH_SPI, &buscfg_touch, SPI_DMA_CH_AUTO));

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)TOUCH_SPI, &tp_io_config, &tp_io_handle));
    ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &tp_cfg, tp));

    return ESP_OK;
}