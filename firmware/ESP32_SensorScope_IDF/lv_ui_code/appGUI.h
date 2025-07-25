//appGUI.h
#pragma once
//#ifdef ESP_PLATFORM
//#include "lvgl.h"
//#endif
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *label;
    const lv_img_dsc_t *icon;
    void (*create_settings_screen)(void);
} menu_entry_t;

void create_menu_screen(void);
void create_background_screen(void);

#ifdef __cplusplus
}
#endif
