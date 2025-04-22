#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"

static void event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data);

void wifi_init_sta(void);