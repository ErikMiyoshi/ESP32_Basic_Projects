#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "adc.h"
#include "oled.h"
#include "i2c_cfg.h"

static const char *TAG = "display_example";

QueueHandle_t queue;

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    queue = xQueueCreate(10,sizeof(int));

    vTaskDelay( 1000 / portTICK_PERIOD_MS);
    adc_init(queue);

    display_init(queue);

    while(1) {
    }
}
