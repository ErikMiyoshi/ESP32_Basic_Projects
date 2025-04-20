#include <stdio.h>
#include "driver/gpio.h"
#include "esp_lcd_io_spi.h"
#include <esp_log.h>
#include "pcd8544.h"

void app_main(void)
{
    pcd8544_config_t config = {
        .spi_host = HSPI_HOST,
        .is_backlight_common_anode = true,
    };

    pcd8544_spi_pin_config_t spi_pin_config = {
        .mosi_io_num = GPIO_NUM_13,
        .sclk_io_num = GPIO_NUM_14,
        .spics_io_num = GPIO_NUM_26,
    };

    pcd8544_control_pin_config_t spi_control_config = {
        .dc_io_num = GPIO_NUM_27,
        .reset_io_num = GPIO_NUM_25,
    };
    config.spi_pin = &spi_pin_config;
    config.control_pin = &spi_control_config;

    pcd8544_init(&config);
    pcd8544_set_contrast(27); //Adjusting contrast, with default value 20 I almost couldn't see the text
    
    pcd8544_clear_display();
    pcd8544_finalize_frame_buf();
    pcd8544_puts("Hello,PCD8544!");
    pcd8544_sync_and_gc();
    pcd8544_free();
}
