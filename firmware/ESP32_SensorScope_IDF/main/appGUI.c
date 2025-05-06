//appGUI.c

#include "appGUI.h"
#include "lvgl.h"
//#include "lvgl_helpers.h"

#include "Circuitboard_dark_landscape.c"  // Include the image source

void lvgl_example_menu_3(void)
{
    //Set background image
    lv_obj_t *bg_img = lv_image_create(lv_screen_active());
    lv_image_set_src(bg_img, &Circuitboard_dark_landscape);

    lv_obj_set_size(bg_img, lv_display_get_horizontal_resolution(NULL), lv_display_get_vertical_resolution(NULL));
    lv_obj_center(bg_img);

    /*Create a menu object*/
    lv_obj_t * menu = lv_menu_create(lv_screen_active());
    lv_obj_set_style_bg_opa(menu, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_size(menu, lv_display_get_horizontal_resolution(NULL)-200, lv_display_get_vertical_resolution(NULL)-20-100);
    //lv_obj_center(menu);
    lv_obj_set_pos(menu, 0, 20);

    /*Modify the header*/
    lv_obj_t * back_btn = lv_menu_get_main_header_back_button(menu);
    lv_obj_t * back_button_label = lv_label_create(back_btn);
    lv_label_set_text(back_button_label, "Back");

    lv_obj_t * cont;
    lv_obj_t * label;

    /*Create sub pages*/
    lv_obj_t * sub_1_page = lv_menu_page_create(menu, "Hello World");

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