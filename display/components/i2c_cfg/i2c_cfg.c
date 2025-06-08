
#include "i2c_cfg.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "driver/i2c_master.h"


#define I2C_MASTER_SDA_IO 27
#define I2C_MASTER_SCL_IO 14
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_NUM I2C_NUM_0
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ    (400 * 1000)

static const char *TAG = "i2c";


i2c_master_bus_handle_t init_i2c(void) {   
    ESP_LOGI(TAG, "Initialize I2C bus");

    i2c_master_bus_handle_t i2c_bus = NULL;
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));
    return i2c_bus;
}