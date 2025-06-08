#include <stdio.h>
#include "oled.h"
#include "hal/lcd_types.h"
#include "i2c_cfg.h"

#include "esp_err.h"
#include "esp_log.h"

#include "esp_lcd_io_i2c.h"
#include "esp_lcd_panel_ssd1306.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

static const char *TAG = "display";

#define EXAMPLE_LCD_PIXEL_CLOCK_HZ    (400 * 1000)
#define EXAMPLE_I2C_HW_ADDR           0x3C
// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8

#define EXAMPLE_PIN_NUM_RST           -1
#define EXAMPLE_LCD_H_RES              128
#define EXAMPLE_LCD_V_RES              64

void func(void)
{

}
lv_disp_t *disp;
lv_obj_t *my_label;

QueueHandle_t queue_adc;

void update_ui_task(void *arg) {
    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    lv_obj_t *label = lv_label_create(scr);

    lv_label_set_text(label, "Counting...");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    int counter = 0;
    char buf[32];

    while (1) {

        int sensor_value;
        if (xQueueReceive(queue_adc, &sensor_value, portMAX_DELAY)) {
            ESP_LOGI(TAG,"Received from sensor: %d", sensor_value);
        }

        if (lvgl_port_lock(0)) {
            snprintf(buf, sizeof(buf), "Sensor: %d", sensor_value);
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
            .mirror_x = true,
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

    queue_adc = queue;
    if (lvgl_port_lock(0)) {
        /* Rotation of the screen */
        lv_disp_set_rotation(disp, LV_DISPLAY_ROTATION_0);

        // Release the mutex
        lvgl_port_unlock();
    }
    xTaskCreate(update_ui_task, "ui_update", 4096, disp, 1, NULL);

}
