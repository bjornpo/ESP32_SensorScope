// display_driver.h
#pragma once

#include "lvgl.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_touch_ft5x06.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize display hardware (SPI bus, LCD panel, backlight)
void display_driver_init_display(void);

// Initialize LVGL and return the LVGL display handle
//lv_display_t *display_driver_init_lvgl(void);

// Initialize touch controller and link it to the given LVGL display
//void display_driver_init_touch(lv_display_t *display);

// Lock and unlock LVGL API access (for thread safety)
void lvgl_lock(void);
void lvgl_unlock(void);

// Accessors for hardware handles (for advanced configurations)
//esp_lcd_panel_handle_t display_driver_get_panel_handle(void);
//esp_lcd_touch_handle_t display_driver_get_touch_handle(void);

#ifdef __cplusplus
}
#endif
