#include "adc.h"

#define SAMPLE_RATE 50 //Get an adc read every SAMPLE_RATE

static const char *TAG = "ADC";

adc_oneshot_unit_handle_t adc_handle;

#define ADC_UNIT        ADC_UNIT_1
#define ADC_BITWIDTH    ADC_BITWIDTH_12
#define ADC_ATTEN_DB    ADC_ATTEN_DB_12
#define ADC_CHANNEL     7

static void adc_task(void *pvParameters) {
    QueueHandle_t queue = (QueueHandle_t)pvParameters;
    int adc_value;
    while(1) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle,ADC_CHANNEL, &adc_value));
        ESP_LOGI(TAG,"Adc value: %d", adc_value);
        if(queue) {
            xQueueSend(queue, &adc_value, portMAX_DELAY);
        }
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_RATE));
    }
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

void adc_init(QueueHandle_t queue) {
    adc_cali_handle_t adc_cali_handle = NULL;
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
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &adc_os_cfg));

    xTaskCreate(adc_task,"adc task", 2048, (void *) queue, 5, NULL);
}