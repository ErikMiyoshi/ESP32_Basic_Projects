#include "adc.h"

#define SAMPLE_RATE 1000 //Get an adc read every SAMPLE_RATE in ms

static const char *TAG = "ADC";

adc_oneshot_unit_handle_t adc_handle;
adc_cali_handle_t adc_cali_handle = NULL;

#define ADC_UNIT        ADC_UNIT_1
#define ADC_BITWIDTH    ADC_BITWIDTH_12
#define ADC_ATTEN_DB    ADC_ATTEN_DB_12


static void adc_task(void *pvParameters) {
    adc_task_params_t *params = (adc_task_params_t *)pvParameters;

    adc_data_t *data = malloc(sizeof(adc_data_t));
    if (data == NULL) {
        ESP_LOGI(TAG,"Failed to initialize ADC data structure");
        return;
    }
    data->channel_num = params->channels_num;
    data->channels = malloc(sizeof(int) * data->channel_num);
    data->values = malloc(sizeof(int) * data->channel_num);
    if (!data->channels || !data->values) {
        ESP_LOGE(TAG, "Failed to allocate channels or values");
        free(data->channels);
        free(data->values);
        free(data);
        vTaskDelay(pdMS_TO_TICKS(500));
    }


    while(1) {
        for(int i=0; i<params->channels_num; i++) {
            int raw;
            ESP_ERROR_CHECK(adc_oneshot_read(adc_handle,params->channels[i], &raw));
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle, raw, &data->values[i]));
            ESP_LOGI(TAG,"Adc raw value: %d, channel %d, Cali mV %d", raw, params->channels[i], data->values[i]);
            data->channels[i] = params->channels[i];
        }

        if(params->queue) {
            BaseType_t result = xQueueSend(params->queue, &data, portMAX_DELAY);
            if(result != pdPASS) {
                ESP_LOGI(TAG,"Failed to send to Queue");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_RATE));
    }

    free(params);
    vTaskDelete(NULL);

}

static void adc_calibrate(adc_cali_handle_t *out_handle) {
    adc_cali_scheme_ver_t cali_scheme;
    adc_cali_handle_t cali_handle = NULL;
    bool calibrated = false;
    esp_err_t ret;

    ESP_ERROR_CHECK(adc_cali_check_scheme(&cali_scheme));

    if(cali_scheme & 1) {
        ESP_LOGI(TAG, "Suport ADC_CALI_SCHEME_VER_LINE_FITTING");
    }
    if(cali_scheme & 2) {
        ESP_LOGI(TAG, "Suport ADC_CALI_SCHEME_VER_CURVE_FITTING");
    }
    
    if(!calibrated) {
        ESP_LOGI(TAG, "Calibrating ADC...");
        adc_cali_line_fitting_config_t cali_cfg = {
            .unit_id = ADC_UNIT,
            .atten = ADC_ATTEN_DB,
            .bitwidth = ADC_BITWIDTH,
        };

        ret = adc_cali_create_scheme_line_fitting(&cali_cfg, &cali_handle);
    }
    
    *out_handle = cali_handle;

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }
}

void adc_init(QueueHandle_t queue, int channels[], int channels_num) {
    adc_calibrate(&adc_cali_handle);

    adc_oneshot_unit_init_cfg_t adc_init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_init_config, &adc_handle));

    adc_oneshot_chan_cfg_t adc_os_cfg = {
        .atten = ADC_ATTEN_DB,
        .bitwidth = ADC_BITWIDTH,
    };

    for(int i=0; i < channels_num; i++) {
        ESP_LOGI(TAG,"Configuring ADC Channel %d", channels[i]);
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, channels[i], &adc_os_cfg));
    }

    adc_task_params_t *adc_task_params = malloc(sizeof(adc_task_params_t));
    adc_task_params->channels = channels;
    adc_task_params->channels_num = channels_num;
    adc_task_params->queue = queue;

    xTaskCreate(adc_task,"adc task", 2048, adc_task_params, 5, NULL);
}