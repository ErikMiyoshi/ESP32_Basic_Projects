/* The example of ESP-IDF
 *
 * This sample code is in the public domain.
 */

#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "lora.h"

#include "hal/lcd_types.h"

#include "esp_err.h"
#include "esp_log.h"

#include "esp_lcd_io_i2c.h"
#include "esp_lcd_panel_ssd1306.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

#include "driver/i2c_master.h"

static const char *TAG = "display";

#define EXAMPLE_LCD_PIXEL_CLOCK_HZ    (400 * 1000)
#define EXAMPLE_I2C_HW_ADDR           0x3C
// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8

#define EXAMPLE_PIN_NUM_RST           -1
#define EXAMPLE_LCD_H_RES              128
#define EXAMPLE_LCD_V_RES              64

#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_NUM I2C_NUM_0
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ    (400 * 1000)

static const char *TAGI2C = "i2c";


lv_disp_t *disp;
lv_obj_t *my_label;

QueueHandle_t queue;

i2c_master_bus_handle_t init_i2c(void);
void display_init(QueueHandle_t queue);
void update_ui_task(void *arg);

typedef struct loraMessage_t {
	uint8_t buffer[255];
	int Rxlen;	
} loraMessage_t;

// #if CONFIG_SENDER
// void task_tx(void *pvParameters)
// {
// 	ESP_LOGI(pcTaskGetName(NULL), "Start");
// 	uint8_t buf[255]; // Maximum Payload size of SX1276/77/78/79 is 255
// 	while(1) {
// 		TickType_t nowTick = xTaskGetTickCount();
// 		int send_len = sprintf((char *)buf,"Hello World!! %"PRIu32, nowTick);
// 		lora_send_packet(buf, send_len);
// 		ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent...", send_len);
// 		int lost = lora_packet_lost();
// 		if (lost != 0) {
// 			ESP_LOGW(pcTaskGetName(NULL), "%d packets lost", lost);
// 		}
// 		vTaskDelay(pdMS_TO_TICKS(1000));
// 	} // end while

// 	// never reach here
// 	vTaskDelete( NULL );
// }
// #endif // CONFIG_SENDER

//#if CONFIG_RECEIVER
void task_rx(void *pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	loraMessage_t message;
	//uint8_t buf[255]; // Maximum Payload size of SX1276/77/78/79 is 255
	while(1) {
		lora_receive(); // put into receive mode
		if (lora_received()) {
			message.Rxlen = lora_receive_packet(message.buffer, sizeof(message.buffer));
			ESP_LOGI(pcTaskGetName(NULL), "%d byte packet received:[%.*s]", message.Rxlen, message.Rxlen, message.buffer);
			xQueueSend(queue, &message, portMAX_DELAY);
		}
		vTaskDelay(1); // Avoid WatchDog alerts
	} // end while

	// never reach here
	vTaskDelete( NULL );
}
//#endif // CONFIG_RECEIVER

void app_main()
{
	// Initialize LoRa
	if (lora_init() == 0) {
		ESP_LOGE(pcTaskGetName(NULL), "Does not recognize the module");
		while(1) {
			vTaskDelay(1);
		}
	}

#if CONFIG_433MHZ
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 433MHz");
	lora_set_frequency(433e6); // 433MHz
#elif CONFIG_866MHZ
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 866MHz");
	lora_set_frequency(866e6); // 866MHz
#elif CONFIG_915MHZ
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 915MHz");
	lora_set_frequency(915e6); // 915MHz
#elif CONFIG_OTHER
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is %dMHz", CONFIG_OTHER_FREQUENCY);
	long frequency = CONFIG_OTHER_FREQUENCY * 1000000;
	lora_set_frequency(frequency);
#endif

	lora_enable_crc();

	int cr = 1;
	int bw = 7;
	int sf = 7;
#if CONFIG_ADVANCED
	cr = CONFIG_CODING_RATE;
	bw = CONFIG_BANDWIDTH;
	sf = CONFIG_SF_RATE;
#endif

	lora_set_coding_rate(cr);
	//lora_set_coding_rate(CONFIG_CODING_RATE);
	//cr = lora_get_coding_rate();
	ESP_LOGI(pcTaskGetName(NULL), "coding_rate=%d", cr);

	lora_set_bandwidth(bw);
	//lora_set_bandwidth(CONFIG_BANDWIDTH);
	//int bw = lora_get_bandwidth();
	ESP_LOGI(pcTaskGetName(NULL), "bandwidth=%d", bw);

	lora_set_spreading_factor(sf);
	//lora_set_spreading_factor(CONFIG_SF_RATE);
	//int sf = lora_get_spreading_factor();
	ESP_LOGI(pcTaskGetName(NULL), "spreading_factor=%d", sf);

// #if CONFIG_SENDER
// 	xTaskCreate(&task_tx, "TX", 1024*3, NULL, 5, NULL);
// #endif
// #if CONFIG_RECEIVER
	xTaskCreate(&task_rx, "RX", 1024*3, NULL, 5, NULL);
// #endif
	queue = xQueueCreate(10,sizeof(loraMessage_t));
	display_init(queue);
}


void update_ui_task(void *arg) {
    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    lv_obj_t *label = lv_label_create(scr);

    lv_label_set_text(label, "Counting...");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    int counter = 0;
    char buf[255];
    while (1) {

		loraMessage_t rxMessage;
        if (xQueueReceive(queue, &rxMessage, portMAX_DELAY)) {
            ESP_LOGI(TAG,"Received from lora: %.*s", rxMessage.Rxlen,rxMessage.buffer);
        }

        if (lvgl_port_lock(0)) {
            snprintf(buf, sizeof(buf), "%.*s", rxMessage.Rxlen,rxMessage.buffer);
            lv_label_set_text(label, buf);
            lvgl_port_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}


void display_init(QueueHandle_t queue) {
    i2c_master_bus_handle_t i2c_bus = NULL;
    i2c_bus = init_i2c();

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = EXAMPLE_I2C_HW_ADDR,
        .scl_speed_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
        .control_phase_bytes = 1,               // According to SSD1306 datasheet
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,   // According to SSD1306 datasheet
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS, // According to SSD1306 datasheet
    #if CONFIG_EXAMPLE_LCD_CONTROLLER_SSD1306
        .dc_bit_offset = 6,                     // According to SSD1306 datasheet
    #elif CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
        .dc_bit_offset = 0,                     // According to SH1107 datasheet
        .flags =
        {
            .disable_control_phase = 1,
        }
    #endif
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &io_handle));

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
    #if (ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5,0,0))
        .color_space = ESP_LCD_COLOR_SPACE_MONOCHROME,
    #endif
    };
    #if CONFIG_EXAMPLE_LCD_CONTROLLER_SSD1306
    #if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,3,0))
    esp_lcd_panel_ssd1306_config_t ssd1306_config = {
        .height = EXAMPLE_LCD_V_RES,
    };
    panel_config.vendor_config = &ssd1306_config;
    #endif
    ESP_LOGI(TAG, "Install SSD1306 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));
    #elif CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
    ESP_LOGI(TAG, "Install SH1107 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_sh1107(io_handle, &panel_config, &panel_handle));
    #endif

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    #if CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
#endif

    ESP_LOGI(TAG, "Initialize LVGL");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES,
        .double_buffer = true,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .monochrome = true,
#if LVGL_VERSION_MAJOR >= 9
        .color_format = LV_COLOR_FORMAT_RGB565,
#endif
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
#if LVGL_VERSION_MAJOR >= 9
            .swap_bytes = false,
#endif
            .sw_rotate = false,
        }
    };
    disp = lvgl_port_add_disp(&disp_cfg);

    ESP_LOGI(TAG, "Display LVGL Scroll Text");
    // Lock the mutex due to the LVGL APIs are not thread-safe

    if (lvgl_port_lock(0)) {
        /* Rotation of the screen */
        lv_disp_set_rotation(disp, LV_DISPLAY_ROTATION_0);

        // Release the mutex
        lvgl_port_unlock();
    }
    xTaskCreate(update_ui_task, "ui_update", 4096, disp, 1, NULL);

}

i2c_master_bus_handle_t init_i2c(void) {   
    ESP_LOGI(TAGI2C, "Initialize I2C bus");

    i2c_master_bus_handle_t i2c_bus = NULL;
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));
    return i2c_bus;
}