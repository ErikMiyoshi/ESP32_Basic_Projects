#include <stdio.h>
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include <esp_log.h>

TaskHandle_t fade_task_handle = NULL;

const char tag[] = "pwm";

static IRAM_ATTR bool ledc_fade_cb(const ledc_cb_param_t *param, void *user_arg) {
    BaseType_t higher_priority_task_woken = pdFALSE;

    if (param->event == LEDC_FADE_END_EVT) {
        vTaskNotifyGiveFromISR(fade_task_handle, &higher_priority_task_woken);
    }

    return higher_priority_task_woken == pdTRUE;
}

void fade_task(void* pvParameters) {
    while(1) {
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);

        ESP_LOGI(tag, "Fade Finished");
    }
}



void app_main(void)
{
    ledc_timer_config_t cfg_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_12_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 4000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&cfg_timer));

    ledc_channel_config_t cfg_channel = {
        .gpio_num = GPIO_NUM_2,
        .speed_mode = LEDC_LOW_SPEED_MODE,   
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0, //2^12 / 2
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&cfg_channel));

    ESP_ERROR_CHECK(ledc_fade_func_install(0));

    xTaskCreate(fade_task, "fade_task", 2048, NULL, 5, &fade_task_handle);

    ledc_cbs_t ledc_cb = {.fade_cb = ledc_fade_cb};
    ledc_cb_register(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0, &ledc_cb, NULL);

    ESP_ERROR_CHECK(ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,4000,5000));
    ESP_ERROR_CHECK(ledc_fade_start(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,LEDC_FADE_WAIT_DONE));

    while(1){
        ESP_LOGI(tag,"Increasing...");
        ESP_ERROR_CHECK(ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,4000,3000));
        ESP_ERROR_CHECK(ledc_fade_start(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,LEDC_FADE_WAIT_DONE));
        vTaskDelay(100 / portTICK_PERIOD_MS);
        ESP_LOGI(tag,"Decreasing...");
        ESP_ERROR_CHECK(ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,0,3000));
        ESP_ERROR_CHECK(ledc_fade_start(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,LEDC_FADE_WAIT_DONE));
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
