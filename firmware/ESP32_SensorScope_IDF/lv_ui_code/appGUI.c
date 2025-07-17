//appGUI.c

#include "appGUI.h"
#include "lvgl.h"
#include "serial_analyzer.h"
#include "ui_settings.h"
//#include "lvgl_helpers.h"

//#include "Circuitboard_dark_landscape.c"  // Include the image source
LV_IMAGE_DECLARE(testImg_2);
LV_IMAGE_DECLARE(Circuitboard_dark_landscape);
LV_IMAGE_DECLARE(wand);
LV_IMAGE_DECLARE(oscilloscope_50);
LV_IMAGE_DECLARE(oscilloscope_70);

LV_IMAGE_DECLARE(oscilloscope_icon_65);
LV_IMAGE_DECLARE(analog_icon_65);
LV_IMAGE_DECLARE(modbus_icon_65);
LV_IMAGE_DECLARE(protocol_analyzer_icon_65);
LV_IMAGE_DECLARE(SDI_12_icon_65);
LV_IMAGE_DECLARE(serial_analyzer_icon_65);
LV_IMAGE_DECLARE(Settings_icon_65);
LV_IMAGE_DECLARE(terminal_icon_65);

const lv_img_dsc_t *background_image = &Circuitboard_dark_landscape;

lv_obj_t *status_bar;
lv_obj_t *bg_img;

void oscilloscope_settings_screen()
{

}

void menu_coming_soon()
{
    //lv_obj_t * mbox1 = lv_msgbox_create(lv_screen_active());
    lv_obj_t * mbox1 = lv_msgbox_create(NULL);

    lv_msgbox_add_title(mbox1, "Work in progress");

    lv_msgbox_add_text(mbox1, "This feature is not ready yet. Come back soon ;)");
    lv_msgbox_add_close_button(mbox1);
}

static const menu_entry_t main_menu_entries[] = {
    {"Scope", &oscilloscope_icon_65, oscilloscope_settings_screen},
    {"Terminal", &terminal_icon_65, menu_coming_soon},
    {"SDI-12", &SDI_12_icon_65, menu_coming_soon},
    {"Modbus", &modbus_icon_65, menu_coming_soon},
    {"Serial\nanalyzer", &serial_analyzer_icon_65, serial_analyzer_settings_screen},
    {"Protocol\nanalyzer", &protocol_analyzer_icon_65, menu_coming_soon},
    {"Analog\nmA/V", &analog_icon_65, menu_coming_soon},
    {"Settings", &Settings_icon_65, settings_screen},
    //{"About", &oscilloscope_50, oscilloscope_settings_screen},
};

void ui_create_statusbar()
{
    // Create it on the system top layer (not attached to any screen)
    status_bar = lv_obj_create(lv_layer_top());
    lv_obj_set_size(status_bar, 480, 20);  // width of screen, height of bar
    lv_obj_align(status_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);

    // Style it
    lv_obj_set_style_bg_color(status_bar, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(status_bar, LV_OPA_80, LV_PART_MAIN);

    // Add icons or labels
    lv_obj_t *battery_label = lv_label_create(status_bar);
    lv_label_set_text(battery_label, LV_SYMBOL_BATTERY_FULL);
    lv_obj_align(battery_label, LV_ALIGN_RIGHT_MID, -5, 0);

    lv_obj_t *Ext_Vsupply_label = lv_label_create(status_bar);
    lv_label_set_text(Ext_Vsupply_label, LV_SYMBOL_BATTERY_3);

    lv_label_set_text_fmt(Ext_Vsupply_label, "%s 12V", LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_color(Ext_Vsupply_label, lv_color_hex(0xFFFF00), LV_PART_MAIN);
    lv_obj_align(Ext_Vsupply_label, LV_ALIGN_RIGHT_MID, -35, 0);

    lv_obj_t *usb_label = lv_label_create(status_bar);
    lv_label_set_text(usb_label, LV_SYMBOL_USB);
    lv_obj_align(usb_label, LV_ALIGN_LEFT_MID, 5, 0);

    lv_obj_t *wifi_label = lv_label_create(status_bar);
    lv_label_set_text(wifi_label, LV_SYMBOL_WIFI);
    lv_obj_align(wifi_label, LV_ALIGN_LEFT_MID, 35, 0);
}

void create_background_screen()
{
    bg_img = lv_obj_create(lv_layer_bottom());
    lv_obj_t *bg_img = lv_image_create(lv_screen_active());
    lv_image_set_src(bg_img, background_image);

    //lv_obj_set_size(bg_img, lv_display_get_horizontal_resolution(NULL), lv_display_get_vertical_resolution(NULL));
    lv_obj_center(bg_img);
}


void create_menu_screen(void) {
    lv_obj_t * screen = lv_obj_create(NULL);
    lv_scr_load(screen);
    //Set background image
    create_background_screen();
    //Status bar
    ui_create_statusbar();

    //Main menu
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_flex_flow(&style, LV_FLEX_FLOW_ROW_WRAP);
    lv_style_set_flex_main_place(&style, LV_FLEX_ALIGN_SPACE_EVENLY);
    lv_style_set_layout(&style, LV_LAYOUT_FLEX);

    lv_obj_t * cont = lv_obj_create(lv_screen_active());
    lv_obj_set_size(cont, 480, 300);
    //lv_obj_center(cont);
    lv_obj_set_pos(cont, 0, 20);
    //lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_add_style(cont, &style, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, LV_PART_MAIN);
    //lv_obj_set_style_bg_opa(cont, LV_OPA_50, LV_PART_MAIN);

    for(size_t i = 0; i < (sizeof(main_menu_entries)/sizeof(main_menu_entries[0])); i++) {
        const menu_entry_t *entry = &main_menu_entries[i];

            // Create the button
        lv_obj_t *btn = lv_button_create(cont);  // parent could be your screen or container
        lv_obj_set_size(btn, 100, 120);
        lv_obj_center(btn); // or set layout positions manually
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, 3, LV_PART_MAIN);
        lv_obj_set_style_border_color(btn, lv_color_white(), LV_PART_MAIN);
        //lv_obj_set_style_pad_all(btn, 5, LV_PART_MAIN);

            // Add the icon (image) as a child
        lv_obj_t *icon = lv_image_create(btn);
        lv_image_set_src(icon, entry->icon);
        lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 5);  // Align to top inside button

        // Add the label as a child
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, entry->label);
        lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -5);  // Align to bottom inside button

        lv_obj_add_event_cb(btn, (lv_event_cb_t)entry->create_settings_screen, LV_EVENT_CLICKED, NULL);
    }

}

void lvgl_example_menu_3(void)
{
    //Set background image
    lv_obj_t *bg_img = lv_image_create(lv_screen_active());
    lv_image_set_src(bg_img, &Circuitboard_dark_landscape);

    //lv_obj_set_size(bg_img, lv_display_get_horizontal_resolution(NULL), lv_display_get_vertical_resolution(NULL));
    lv_obj_center(bg_img);

    /*Create a menu object*/
    lv_obj_t * menu = lv_menu_create(lv_screen_active());
    //lv_obj_set_style_bg_opa(menu, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(menu, LV_OPA_60, LV_PART_MAIN);
    lv_obj_set_size(menu, lv_display_get_horizontal_resolution(NULL)-200, lv_display_get_vertical_resolution(NULL)-20-100);
    //lv_obj_center(menu);
    lv_obj_set_pos(menu, 50, 30);

    /*Modify the header*/
    lv_obj_t * back_btn = lv_menu_get_main_header_back_button(menu);
    lv_obj_t * back_button_label = lv_label_create(back_btn);
    lv_label_set_text(back_button_label, "Back");

    lv_obj_t * cont;
    lv_obj_t * label;

    /*Create sub pages*/
    lv_obj_t * sub_1_page = lv_menu_page_create(menu, "Oscilloscope");

    cont = lv_menu_cont_create(sub_1_page);
    label = lv_label_create(cont);
    lv_label_set_text(label, "Hello, I am hiding here");

    lv_obj_t * sub_2_page = lv_menu_page_create(menu, "Page 2");

    cont = lv_menu_cont_create(sub_2_page);
    label = lv_label_create(cont);
    lv_label_set_text(label, "Hello, I am hiding here");

    lv_obj_t * sub_3_page = lv_menu_page_create(menu, "Page 3");

    cont = lv_menu_cont_create(sub_3_page);
    label = lv_label_create(cont);
    lv_label_set_text(label, "Hello, I am hiding here");

    /*Create a main page*/
    lv_obj_t * main_page = lv_menu_page_create(menu, NULL);

    /*cont = lv_menu_cont_create(main_page);
    label = lv_label_create(cont);
    lv_label_set_text(label, "Item 1 (Click me!)");
    lv_menu_set_load_page_event(menu, cont, sub_1_page);*/

    cont = lv_button_create(main_page);        // Create a button directly in the main page
    label = lv_label_create(cont);             // Add a label inside the button
    lv_label_set_text(label, "Oscilloscope");     // Set button label text

    // Attach event to the button (instead of the container)
    lv_menu_set_load_page_event(menu, cont, sub_1_page);

    cont = lv_button_create(main_page);
    label = lv_label_create(cont);
    lv_label_set_text(label, "Serial Monitor");
    lv_menu_set_load_page_event(menu, cont, sub_2_page);

    cont = lv_menu_cont_create(main_page);
    label = lv_label_create(cont);
    lv_label_set_text(label, "SDI-12");
    lv_menu_set_load_page_event(menu, cont, sub_3_page);

    lv_menu_set_page(menu, main_page);
    }
