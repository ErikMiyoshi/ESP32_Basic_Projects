#include "freertos/FreeRTOS.h"
#include "lvgl.h"

void func(void);

void display_init(QueueHandle_t queue);
void example_lvgl_demo_ui(lv_disp_t *disp, int sensor);