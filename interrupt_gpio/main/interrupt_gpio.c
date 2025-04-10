#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <esp_log.h>

static char tag[] = "Test_Intr";

static QueueHandle_t gpio_evt_queue = NULL;

int delay = 1000;

static void gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    if(gpio_num == GPIO_NUM_13) {
        delay = 500;
    } else if(gpio_num == GPIO_NUM_12) {
        delay = 200;
    } else {
        delay = 50;
    }

    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task(void* args){
    uint32_t io_num;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            ESP_LOGI(tag, "GPIO %d generate an interruption", (int) io_num);
            //printf("GPIO[%"PRIu32"] intr, val: %d\n", io_num, gpio_get_level(io_num));
        }
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_pull_mode(GPIO_NUM_2, GPIO_PULLDOWN_ONLY));

    gpio_config_t cfg = {};
    cfg.intr_type = GPIO_INTR_NEGEDGE;
    cfg.mode = GPIO_MODE_INPUT;
    cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    cfg.pin_bit_mask = (1ULL << GPIO_NUM_13) | (1ULL << GPIO_NUM_12) | (1ULL << GPIO_NUM_14);
    ESP_ERROR_CHECK(gpio_config(&cfg));

    ESP_ERROR_CHECK(gpio_dump_io_configuration(stdout, (1ULL << GPIO_NUM_2) | (1ULL << GPIO_NUM_12) | (1ULL << GPIO_NUM_14)));
    
    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_NUM_12, gpio_isr_handler, (void *) GPIO_NUM_12);
    gpio_isr_handler_add(GPIO_NUM_13, gpio_isr_handler, (void *) GPIO_NUM_13);
    gpio_isr_handler_add(GPIO_NUM_14, gpio_isr_handler, (void *) GPIO_NUM_14);
    
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
  
    xTaskCreate(gpio_task, "gpio_task_example", 2048, NULL, 10, NULL);

    while(1){
        gpio_set_level(GPIO_NUM_2, 1);
        vTaskDelay(delay / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_NUM_2, 0);
        vTaskDelay(delay / portTICK_PERIOD_MS);
    }

}
