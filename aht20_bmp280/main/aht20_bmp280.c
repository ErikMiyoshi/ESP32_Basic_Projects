#include <stdio.h>
#include "driver/i2c_master.h"
#include "unity.h"
#include "esp_log.h"

#include "aht20.h"
#include "bmx280.h"


#define I2C_MASTER_SDA_IO 27
#define I2C_MASTER_SCL_IO 14
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_NUM I2C_NUM_0

static const char *TAG = "aht20_bmp280";

static aht20_dev_handle_t aht20_handle = NULL;
static bmx280_t* bmx280 = NULL;
static i2c_master_bus_handle_t i2c_bushandle = NULL;

static void i2c_bus_init(void)
{
    ESP_LOGI("I2C", "Initializing i2c...");
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c_bushandle));
    ESP_LOGI("I2C", "i2c master initialized...");
}

static void i2c_sensor_ath20_init(void)
{    
    i2c_aht20_config_t aht20_i2c_config = {
        .i2c_config.device_address = AHT20_ADDRESS_0,
        .i2c_config.scl_speed_hz = I2C_MASTER_FREQ_HZ,
        .i2c_timeout = 100,
    };
    TEST_ASSERT_FALSE_MESSAGE(aht20_new_sensor(i2c_bushandle, &aht20_i2c_config, &aht20_handle), "aht20_new_sensor fail");
    TEST_ASSERT_NOT_NULL_MESSAGE(aht20_handle, "AHT20 create returned NULL");
}

static void bmp280_init(void)
{   
    bmx280 = bmx280_create_master(i2c_bushandle);

    if (!bmx280) { 
        ESP_LOGE("test", "Could not create bmx280 driver.");
    }
    ESP_ERROR_CHECK(bmx280_init(bmx280));

    bmx280_config_t bmx_cfg = BMX280_DEFAULT_CONFIG;
    ESP_ERROR_CHECK(bmx280_configure(bmx280, &bmx_cfg));
}

TEST_CASE("sensor aht20 test", "[aht20][iot][sensor]")
{
    esp_err_t ret = ESP_OK;
    int16_t temperature_i16;
    int16_t humidity_i16;
    float temperature;
    float humidity;

    i2c_sensor_ath20_init();

    TEST_ASSERT(ESP_OK == aht20_read_float(aht20_handle, &temperature, &humidity));
    ESP_LOGI(TAG, "%-20s: %2.2fdegC", "temperature is", temperature);
    ESP_LOGI(TAG, "%-20s: %2.2f%%", "humidity is", humidity);
    
    TEST_ASSERT(ESP_OK == aht20_read_i16(aht20_handle, &temperature_i16, &humidity_i16));
    ESP_LOGI(TAG, "%-20s: %d", "temperature is", temperature_i16);
    ESP_LOGI(TAG, "%-20s: %d", "humidity is", humidity_i16);

    aht20_del_sensor(&aht20_handle);
    //ret = i2c_del_master_bus(i2c_bushandle);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

TEST_CASE("sensor bmp280 test", "[bmp280][iot][sensor]")
{
    bmp280_init();

    ESP_ERROR_CHECK(bmx280_setMode(bmx280, BMX280_MODE_CYCLE));
    float temp = 0, pres = 0, hum = 0;
    for(int i = 0; i < 3; i++)
    {
        do {
            vTaskDelay(pdMS_TO_TICKS(1));
        } while(bmx280_isSampling(bmx280));

        ESP_ERROR_CHECK(bmx280_readoutFloat(bmx280, &temp, &pres, &hum));
        ESP_LOGI("test", "Read Values: temp = %f, pres = %f, hum = %f", temp, pres, hum);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    bmx280_close(bmx280);
}

void app_main(void)
{
    i2c_bus_init();
    unity_run_menu();
}
