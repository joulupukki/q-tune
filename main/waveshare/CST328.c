#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"

#include "CST328.h"

static const char *TAG = "CST328";

extern esp_lcd_touch_handle_t tp;
i2c_master_bus_handle_t touch_i2c_bus_handle;

/*******************************************************************************
* Function definitions
*******************************************************************************/
static esp_err_t esp_lcd_touch_cst328_read_data(esp_lcd_touch_handle_t tp);
static bool esp_lcd_touch_cst328_get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num);
static esp_err_t esp_lcd_touch_cst328_del(esp_lcd_touch_handle_t tp);

/* I2C read/write */
static esp_err_t touch_cst328_i2c_read(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t *data, uint8_t len);
static esp_err_t touch_cst328_i2c_write(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t *data, uint8_t len);

/* CST328 reset */
static esp_err_t touch_cst328_reset(esp_lcd_touch_handle_t tp);

/* Read status and config register */
static void touch_cst328_read_cfg(esp_lcd_touch_handle_t tp);

static i2c_master_dev_handle_t touch_dev_handle;

/*******************************************************************************
* Public API functions
*******************************************************************************/

esp_err_t esp_lcd_touch_new_i2c_cst328(const esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *config, esp_lcd_touch_handle_t *out_touch)
{
    esp_err_t ret = ESP_OK;

    assert(io != NULL);
    assert(config != NULL);
    assert(out_touch != NULL);

    /* Prepare main structure */
    esp_lcd_touch_handle_t esp_lcd_touch_cst328 = heap_caps_calloc(1, sizeof(esp_lcd_touch_t), MALLOC_CAP_DEFAULT);
    ESP_GOTO_ON_FALSE(esp_lcd_touch_cst328, ESP_ERR_NO_MEM, err, TAG, "no mem for CST328 controller");
    /* Communication interface */
    esp_lcd_touch_cst328->io = io;

    /* Only supported callbacks are set */
    esp_lcd_touch_cst328->read_data = esp_lcd_touch_cst328_read_data;
    esp_lcd_touch_cst328->get_xy = esp_lcd_touch_cst328_get_xy;
    esp_lcd_touch_cst328->del = esp_lcd_touch_cst328_del;

    /* Mutex */
    esp_lcd_touch_cst328->data.lock.owner = portMUX_FREE_VAL;

    /* Save config */
    memcpy(&esp_lcd_touch_cst328->config, config, sizeof(esp_lcd_touch_config_t));

    /* Prepare pin for touch interrupt */
    if (esp_lcd_touch_cst328->config.int_gpio_num != GPIO_NUM_NC) {
        const gpio_config_t int_gpio_config = {
            .mode = GPIO_MODE_INPUT,
            .intr_type = GPIO_INTR_NEGEDGE,
            .pin_bit_mask = BIT64(esp_lcd_touch_cst328->config.int_gpio_num)
        };
        ret = gpio_config(&int_gpio_config);
        ESP_GOTO_ON_ERROR(ret, err, TAG, "GPIO config failed");
        
        // /* Register interrupt callback */
        // if (esp_lcd_touch_cst328->config.interrupt_callback) {
        //     esp_lcd_touch_register_interrupt_callback(esp_lcd_touch_cst328, esp_lcd_touch_cst328->config.interrupt_callback);
        // }
    }

    /* Reset controller */
    ret = touch_cst328_reset(esp_lcd_touch_cst328);
    ESP_GOTO_ON_ERROR(ret, err, TAG, "CST328 reset failed");
    touch_cst328_read_cfg(esp_lcd_touch_cst328);

err:
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error (0x%x)! Touch controller CST328 initialization failed!", ret);
        if (esp_lcd_touch_cst328) {
            esp_lcd_touch_cst328_del(esp_lcd_touch_cst328);
        }
    }

    *out_touch = esp_lcd_touch_cst328;

    return ret;
}


static esp_err_t esp_lcd_touch_cst328_read_data(esp_lcd_touch_handle_t tp)
{
    esp_err_t err;
    uint8_t buf[41];
    uint8_t touch_cnt = 0;
    uint8_t clear = 0;
    size_t i = 0,num=0;

    assert(tp != NULL);

    err = touch_cst328_i2c_read(tp, ESP_LCD_TOUCH_CST328_READ_Number_REG, buf, 1);
    ESP_RETURN_ON_ERROR(err, TAG, "I2C read error!");
    // touch_cst328_i2c_write(tp, ESP_LCD_TOUCH_CST328_READ_XY_REG, &Over, 1);
    /* Any touch data? */
    if ((buf[0] & 0x0F) == 0x00) {
        touch_cst328_i2c_write(tp, ESP_LCD_TOUCH_CST328_READ_Number_REG, &clear, 1);  // No touch data
    } else {
        /* Count of touched points */
        touch_cnt = buf[0] & 0x0F;
        if (touch_cnt > 5 || touch_cnt == 0) {
            touch_cst328_i2c_write(tp, ESP_LCD_TOUCH_CST328_READ_Number_REG, &clear, 1);
            return ESP_OK;
        }
        // printf("BBBBBBBBBBBBBBBBBB = %d\r\n",touch_cnt);

        /* Read all points */
        // err = touch_cst328_i2c_read(tp, ESP_LCD_TOUCH_CST328_READ_XY_REG, &buf[1], touch_cnt * 5 + 2);
        err = touch_cst328_i2c_read(tp, ESP_LCD_TOUCH_CST328_READ_XY_REG, &buf[1], 27);
        ESP_RETURN_ON_ERROR(err, TAG, "I2C read error!");
        // touch_cst328_i2c_write(tp, ESP_LCD_TOUCH_CST328_READ_XY_REG, &Over, 1);

        /* Clear all */
        err = touch_cst328_i2c_write(tp, ESP_LCD_TOUCH_CST328_READ_Number_REG, &clear, 1);
        ESP_RETURN_ON_ERROR(err, TAG, "I2C read error!");

        taskENTER_CRITICAL(&tp->data.lock);

        /* Number of touched points */
        if(touch_cnt > CONFIG_ESP_LCD_TOUCH_MAX_POINTS)
            touch_cnt = CONFIG_ESP_LCD_TOUCH_MAX_POINTS;
        tp->data.points = (uint8_t)touch_cnt;
        
        /* Fill all coordinates */
        for (i = 0; i < touch_cnt; i++) {
            if(i>0) num = 2;
            tp->data.coords[i].x = (uint16_t)(((uint16_t)buf[(i * 5) + 2 + num] << 4) + ((buf[(i * 5) + 4 + num] & 0xF0)>> 4));               
            tp->data.coords[i].y = (uint16_t)(((uint16_t)buf[(i * 5) + 3 + num] << 4) + ( buf[(i * 5) + 4 + num] & 0x0F));
            tp->data.coords[i].strength = ((uint16_t)buf[(i * 5) + 5 + num]);
        }

        taskEXIT_CRITICAL(&tp->data.lock);
    }

    return ESP_OK;
}

static bool esp_lcd_touch_cst328_get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num)
{
    assert(tp != NULL);
    assert(x != NULL);
    assert(y != NULL);
    assert(point_num != NULL);
    assert(max_point_num > 0);

    taskENTER_CRITICAL(&tp->data.lock);

    /* Count of points */
    if(tp->data.points > max_point_num)
        tp->data.points = max_point_num;

    for (size_t i = 0; i < tp->data.points; i++) {
        x[i] = tp->data.coords[i].x;
        y[i] = tp->data.coords[i].y;

        if (strength) {
            strength[i] = tp->data.coords[i].strength;
        }
    }
    *point_num = tp->data.points;
    /* Invalidate */
    tp->data.points = 0;


    taskEXIT_CRITICAL(&tp->data.lock);
    return (*point_num > 0);
}

static esp_err_t esp_lcd_touch_cst328_del(esp_lcd_touch_handle_t tp)
{
    assert(tp != NULL);

    /* Reset GPIO pin settings */
    if (tp->config.int_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.int_gpio_num);
    }

    /* Reset GPIO pin settings */
    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.rst_gpio_num);
    }

    free(tp);

    return ESP_OK;
}

/*******************************************************************************
* Private API function
*******************************************************************************/

/* Reset controller */
static esp_err_t touch_cst328_reset(esp_lcd_touch_handle_t tp)
{
    assert(tp != NULL);

    ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, !tp->config.levels.reset), TAG, "GPIO set level error!");
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, tp->config.levels.reset), TAG, "GPIO set level error!");
    vTaskDelay(pdMS_TO_TICKS(10));

    return ESP_OK;
}

static void touch_cst328_read_cfg(esp_lcd_touch_handle_t tp)
{
    uint8_t buf[24];

    assert(tp != NULL);
    touch_cst328_i2c_write(tp, CST328_REG_DEBUG_INFO_MODE, buf, 0);
    touch_cst328_i2c_read(tp, CST328_REG_DEBUG_INFO_BOOT_TIME, (uint8_t *)&buf[0], 4);
    ESP_LOGI(TAG, "TouchPad_ID:0x%02x,0x%02x,0x%02x,0x%02x", buf[0], buf[1], buf[2], buf[3]);

    touch_cst328_i2c_read(tp, CST328_REG_DEBUG_INFO_RES_X, (uint8_t *)&buf[0], 1);
    touch_cst328_i2c_read(tp, CST328_REG_DEBUG_INFO_RES_X+1, (uint8_t *)&buf[1], 1);
    ESP_LOGI(TAG, "TouchPad_X_MAX:%d", buf[1]*256+buf[0]);
    touch_cst328_i2c_read(tp, CST328_REG_DEBUG_INFO_RES_Y, (uint8_t *)&buf[2], 1);
    touch_cst328_i2c_read(tp, CST328_REG_DEBUG_INFO_RES_Y+1, (uint8_t *)&buf[3], 1);
    ESP_LOGI(TAG, "TouchPad_Y_MAX:%d", buf[3]*256+buf[2]);
    
    touch_cst328_i2c_read(tp, CST328_REG_DEBUG_INFO_TP_NTX, buf, 24);
    ESP_LOGI(TAG, "D1F4:0x%02x,0x%02x,0x%02x,0x%02x", buf[0], buf[1], buf[2], buf[3]);
    ESP_LOGI(TAG, "D1F8:0x%02x,0x%02x,0x%02x,0x%02x", buf[4], buf[5], buf[6], buf[7]);
    ESP_LOGI(TAG, "D1FC:0x%02x,0x%02x,0x%02x,0x%02x", buf[8], buf[9], buf[10], buf[11]);
    ESP_LOGI(TAG, "D200:0x%02x,0x%02x,0x%02x,0x%02x", buf[12], buf[13], buf[14], buf[15]);
    ESP_LOGI(TAG, "D204:0x%02x,0x%02x,0x%02x,0x%02x", buf[16], buf[17], buf[18], buf[19]);
    ESP_LOGI(TAG, "D208:0x%02x,0x%02x,0x%02x,0x%02x", buf[20], buf[21], buf[22], buf[23]);

    ESP_ERROR_CHECK(touch_cst328_i2c_write(tp, CST328_REG_NORMAL_MODE, buf, 0));
}

static esp_err_t touch_cst328_i2c_read(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t *data, uint8_t len)
{
    assert(tp != NULL);
    assert(data != NULL);

    /* Read data */
    // return i2c_master_transmit_receive(touch_dev_handle, &reg, sizeof(uint8_t), data, len, -1);
    return esp_lcd_panel_io_rx_param(tp->io, reg, data, len);
}

static esp_err_t touch_cst328_i2c_write(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t* data, uint8_t len)
{
    assert(tp != NULL);

    // *INDENT-OFF*
    /* Write data */
    // return i2c_master_transmit_receive(touch_dev_handle, &reg, sizeof(uint8_t), data, sizeof(uint8_t), -1);
    // return i2c_master_transmit(touch_dev_handle, data, sizeof(uint8_t), -1);
    return esp_lcd_panel_io_tx_param(tp->io, reg, data, len);
    // *INDENT-ON*
}


/*Read the touchpad*/
// void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
//     uint16_t touchpad_x[5] = {0};
//     uint16_t touchpad_y[5] = {0};
//     uint8_t touchpad_cnt = 0;

//     /* Read touch controller data */
//     esp_lcd_touch_read_data(indev->user_data);

//     /* Get coordinates */
//     bool touchpad_pressed = esp_lcd_touch_get_coordinates(indev->user_data, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 5);

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

/**
 * @brief i2c master initialization
 */
esp_err_t Touch_I2C_Init(void)
{
    i2c_master_bus_config_t touch_bus_config = {
        .i2c_port = -1, // auto-assign
        .sda_io_num = I2C_Touch_SDA_IO,
        .scl_io_num = I2C_Touch_SCL_IO,
        // .clk_source = I2C_CLK_SRC_DEFAULT,
        .clk_source = I2C_CLK_SRC_RC_FAST,
        .glitch_ignore_cnt = 7,
        .intr_priority = 1,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t err = i2c_new_master_bus(&touch_bus_config, &touch_i2c_bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create the I2C master touch bus: %s", esp_err_to_name(err));
        return err;
    }

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ESP_LCD_TOUCH_IO_I2C_CST328_ADDRESS,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    err = i2c_master_bus_add_device(touch_i2c_bus_handle, &dev_config, &touch_dev_handle);
    ESP_LOGI(TAG, "Adding touch device result: %s", esp_err_to_name(err));
    return err;
}
void TOUCH_Init(void)
{
    ESP_ERROR_CHECK(Touch_I2C_Init());
    ESP_LOGI(TAG, "I2C initialized successfully");
    
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    // esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_CST328_CONFIG();
    esp_lcd_panel_io_i2c_config_t tp_io_config = {
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_CST328_ADDRESS,
        .control_phase_bytes = 1,
        .dc_bit_offset = 0,
        .lcd_cmd_bits = 16,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
        .flags.disable_control_phase = 1,
    };
    ESP_LOGI(TAG, "Initialize touch IO (I2C)");
    /* Touch IO handle */
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(touch_i2c_bus_handle, &tp_io_config, &tp_io_handle));
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_V_RES,
        .y_max = LCD_H_RES,
        .rst_gpio_num = I2C_Touch_RST_IO,
        .int_gpio_num = I2C_Touch_INT_IO,
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    /* Initialize touch */
    ESP_LOGI(TAG, "Initialize touch controller CST328");
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_cst328(tp_io_handle, &tp_cfg, &tp));
}