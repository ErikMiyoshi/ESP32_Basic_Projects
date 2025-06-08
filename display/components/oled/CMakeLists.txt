idf_component_register(SRCS "oled.c" "example_demo_ui.c"
                    INCLUDE_DIRS "include"
                    REQUIRES i2c_cfg esp_lvgl_port driver)
