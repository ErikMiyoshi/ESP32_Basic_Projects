#include <stdio.h>
#include <string.h>

#include "hal/adc_types.h"
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/queue.h"

void adc_init(QueueHandle_t queue);