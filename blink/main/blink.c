#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

void app_main(void)
{
    // gpio_config_t io_conf = {};
    // io_conf.intr_type = GPIO_INTR_DISABLE;
    // io_conf.mode = GPIO_MODE_OUTPUT;
    // io_conf.pin_bit_mask = (1ULL << GPIO_NUM_2);
    // io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    // io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    // gpio_config(&io_conf);
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_pull_mode(GPIO_NUM_2, GPIO_PULLDOWN_ONLY));

    ESP_ERROR_CHECK(gpio_dump_io_configuration(stdout, (1ULL << GPIO_NUM_2)));

    while(1){
        gpio_set_level(GPIO_NUM_2, 1);
        vTaskDelay( 500 / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_NUM_2, 0);
        vTaskDelay( 500 / portTICK_PERIOD_MS);
    }
}