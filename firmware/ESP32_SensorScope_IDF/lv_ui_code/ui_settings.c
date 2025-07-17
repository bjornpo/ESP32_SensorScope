#include "../main/settings.h"
#include "lvgl.h"
#include "appGUI.h"
#include <string.h>

static void settings_exit_button_cb()
{
    create_menu_screen();
}

static void saveCfg_button_cb()
{
    // Save settings to NVS
    nvs_save_settings();
    // Exit to main menu
    settings_exit_button_cb();
}

static void load_default_Cfg_button_cb()
{
    init_default_settings(&g_settings);
    settings_exit_button_cb();
}

void settings_screen()
{
    lv_obj_t * screen = lv_obj_create(NULL);
    lv_scr_load(screen);
    create_background_screen();

    //Label
    lv_obj_t *title = lv_label_create(lv_screen_active());
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 5, 25);

    //Settings group
    lv_obj_t *group = lv_obj_create(lv_screen_active());
    lv_obj_set_size(group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_width(group, LV_PCT(100));
    lv_obj_align(group, LV_ALIGN_TOP_LEFT, 0, 55);
    lv_obj_set_layout(group, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(group, LV_FLEX_FLOW_ROW);  // Side-by-side
    lv_obj_set_style_pad_all(group, 0, 0);  // Removes all internal padding
    lv_obj_set_style_border_width(group, 0, 0);
    lv_obj_set_style_bg_opa(group, LV_OPA_TRANSP, 0);  // Transparent

    lv_obj_t *saveCfg_button = lv_button_create(group);
    lv_obj_set_size(saveCfg_button, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_t *saveCfg_button_label = lv_label_create(saveCfg_button);
    lv_label_set_text(saveCfg_button_label, "Save settings");
    lv_obj_center(saveCfg_button_label);
    lv_obj_add_event_cb(saveCfg_button, (lv_event_cb_t)saveCfg_button_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *load_default_Cfg_button = lv_button_create(group);
    lv_obj_set_size(load_default_Cfg_button, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_t *load_default_Cfg_button_label = lv_label_create(load_default_Cfg_button);
    lv_label_set_text(load_default_Cfg_button_label, "Load default settings");
    lv_obj_center(load_default_Cfg_button_label);
    lv_obj_add_event_cb(load_default_Cfg_button, (lv_event_cb_t)load_default_Cfg_button_cb, LV_EVENT_CLICKED, NULL);

    //back button
    lv_obj_t *exit_button = lv_button_create(group);
    lv_obj_set_style_bg_color(exit_button, lv_color_hex(0xAA0000), LV_PART_MAIN);
    lv_obj_set_size(exit_button, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_t *exit_button_label = lv_label_create(exit_button);
    lv_label_set_text(exit_button_label, "Exit");
    lv_obj_center(exit_button_label);
    lv_obj_add_event_cb(exit_button, (lv_event_cb_t)settings_exit_button_cb, LV_EVENT_CLICKED, NULL);
}
