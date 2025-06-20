#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_st.h"
#include "cJSON.h"

#include "mqtt_client.h"
#include "adc.h"

static const char *TAG = "mqtt_example";

//Download the credentials for broker(this example uses hiveMQ)
//Put the file inside main folder (in this example, the file is "isrgrootx1_pem")
esp_mqtt_client_handle_t client;

char *create_mqtt_payload(adc_data_t *data) {
    cJSON *root = cJSON_CreateObject();
    cJSON *adc_array = cJSON_CreateArray();

    for (int i = 0; i < data->channel_num; i++) {
        cJSON *entry = cJSON_CreateObject();
        cJSON_AddNumberToObject(entry, "channel", data->channels[i]);
        cJSON_AddNumberToObject(entry, "value", data->values[i]);
        cJSON_AddItemToArray(adc_array, entry);
    }

    cJSON_AddItemToObject(root, "adc", adc_array);

    char *json_string = cJSON_PrintUnformatted(root);  // Don't forget to free this after sending
    ESP_LOGI(TAG,"%s", json_string);
    cJSON_Delete(root);  // Frees internal tree but not the string

    return json_string;
}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
    
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
                log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
                log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
                ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
    
            }
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
        }
}


static void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL, // mqtt://<IP>
        .broker.address.port = 1883,
    };

#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.broker.address.uri, "FROM_STDIN") == 0) {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128) {
            int c = fgetc(stdin);
            if (c == '\n') {
                line[count] = '\0';
                break;
            } else if (c > 0 && c < 127) {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.broker.address.uri = line;
        printf("Broker url: %s\n", line);
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);


    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    QueueHandle_t adc_queue = xQueueCreate(10,sizeof(adc_data_t *));
    if (adc_queue == NULL) {
        ESP_LOGI(TAG,"Failed to create Queue");
        return;
    }

    wifi_init_sta();
    mqtt_app_start();
    vTaskDelay( 3000 / portTICK_PERIOD_MS);
    
    int adc_channels[] = {ADC_CHANNEL_6, ADC_CHANNEL_7};
    adc_init(adc_queue, adc_channels, sizeof(adc_channels)/sizeof(adc_channels[0]));

    adc_data_t *sensor_data;
    while(1) {
        BaseType_t ret = xQueueReceive(adc_queue, &sensor_data, portMAX_DELAY);
        if (ret == pdTRUE) {
            ESP_LOGI(TAG,"Received data from queue");
            for(int i=0; i<sensor_data->channel_num; i++) {
                ESP_LOGI(TAG,"Received from channel %d, value %d", sensor_data->channels[i], sensor_data->values[i]);
            }

            char *payload = create_mqtt_payload(sensor_data);
            esp_mqtt_client_publish(client, "sensor/adc", payload, 0, 1, 0);

        } else {
            ESP_LOGI(TAG,"Data from queue not received");
        }
    }
}
