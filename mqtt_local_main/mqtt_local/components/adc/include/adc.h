#include <stdio.h>
#include <string.h>

#include "hal/adc_types.h"
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/queue.h"

typedef struct {
    QueueHandle_t queue;
    int *channels;
    int channels_num;
} adc_task_params_t;

typedef struct {
    int *values;
    int *channels;
    int channel_num;
} adc_data_t;

void adc_init(QueueHandle_t queue, int channels[], int channels_num);