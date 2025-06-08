/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "lvgl.h"
#include "stdio.h"

void example_lvgl_demo_ui(lv_disp_t *disp, int sensor)
{
    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP); /* Circular scroll */

    char s[10];

    sprintf(s,"%d", sensor);

    lv_label_set_text(label, s);
    /* Size of the screen (if you use rotation 90 or 270, please set disp->driver->ver_res) */
#if LVGL_VERSION_MAJOR >= 9
    lv_obj_set_width(label, lv_display_get_physical_horizontal_resolution(disp));
#else
    lv_obj_set_width(label, disp->driver->hor_res);
#endif
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
}