#include "I2C_Driver.h"

#include "esp_err.h"

#define I2C_TRANS_BUF_MINIMUM_SIZE     (sizeof(i2c_cmd_desc_t) + \
                                        sizeof(i2c_cmd_link_t) * 8) /* It is required to have allocate one i2c_cmd_desc_t per command:
                                                                     * start + write (device address) + write buffer +
                                                                     * start + write (device address) + read buffer + read buffer for NACK +
                                                                     * stop */
static const char *I2C_TAG = "I2C";
/**
 * @brief i2c master initialization
 */
i2c_master_bus_handle_t i2c_bus_handle = NULL;

static esp_err_t i2c_master_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_SCL_IO,
        .sda_io_num = I2C_SDA_IO,
        .glitch_ignore_cnt = 7,
        .intr_priority = 1,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t ret = i2c_new_master_bus(&bus_config, &i2c_bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(I2C_TAG, "Failed to create I2C master bus: %s", esp_err_to_name(ret));
    }
    return ret;
}
void I2C_Init(void)
{
    /********************* I2C *********************/
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(I2C_TAG, "I2C initialized successfully");  
}


// // Reg addr is 8 bit
// esp_err_t I2C_Write(uint8_t Driver_addr, uint8_t Reg_addr, const uint8_t *Reg_data, uint32_t Length)
// {
//     uint8_t buf[Length+1];
    
//     buf[0] = Reg_addr;
//     // Copy Reg_data to buf starting at buf[1]
//     memcpy(&buf[1], Reg_data, Length);
//     return i2c_master_write_to_device(I2C_MASTER_NUM, Driver_addr, buf, Length+1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
// }



// esp_err_t I2C_Read(uint8_t Driver_addr, uint8_t Reg_addr, uint8_t *Reg_data, uint32_t Length)
// {
//     return i2c_master_write_read_device(I2C_MASTER_NUM, Driver_addr, &Reg_addr, 1, Reg_data, Length, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
// }
