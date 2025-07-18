//serial_analyzer.c

#include "Serial_analyzer.h"
#include "../main/settings.h"
#include "../main/shared_types.h"
#include "lvgl.h"
#include "appGUI.h"
#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#ifdef CONFIG_IDF_TARGET_ESP32S3
#include "../main/serial_analyzer_rmt.h"
#endif // CONFIG_IDF_TARGET_ESP32S3

//LV_FONT_DECLARE(Greek_alphabet_14);

static lv_obj_t * serial_termination_checkbox;
static lv_obj_t * serial_bias_checkbox;
static lv_obj_t * recording_num_symbols_label;

volatile bool analyzer_abort_requested = false; // Global variable to signal the analyzer task to abort

char SampleSizeDropdownString[] = "20\n50\n100\n500\n1000";

static uint32_t getIndexFromString(char * str, uint32_t num) //Used to find index from string in dropdown menu
{
    char buffer[8];
    snprintf(buffer, sizeof(buffer), "%"PRIu32, num);
    char *ptr = strstr(str, buffer);
    int index = 0;
    while(ptr >= str)
    {
        if(*ptr == '\n')
        {
            index++;
        }
        ptr--;
    }
    return index;
}


static void sample_help_button_cb()
{
    lv_obj_t * mbox1 = lv_msgbox_create(NULL);

    lv_msgbox_add_title(mbox1, "Number of samples");

    lv_msgbox_add_text(mbox1, "Then number of \"flanks\" to record before analyzing the data. A higher number will take longer time to record, but are less prone errors when analyzing the data.");
    lv_msgbox_add_close_button(mbox1);
}

static void bus_idle_checkbox_help_button_cb()
{
    lv_obj_t * mbox1 = lv_msgbox_create(NULL);

    lv_msgbox_add_title(mbox1, "Stop when bus is idle");

    lv_msgbox_add_text(mbox1, "If enabled, the analyzer will stop recording when the bus is idle for more then 100mS.");
    lv_msgbox_add_close_button(mbox1);
}

static void about_button_cb()
{
    lv_obj_t * mbox1 = lv_msgbox_create(NULL);

    lv_msgbox_add_title(mbox1, "About");

    lv_msgbox_add_text(mbox1, "This program records time between signal transitions, and tries to determine the baudrate, number of data bits, parity bit, and stop bits. It can also detect if the signal is inverted, and it the incoming data is binary or ascii.\nThe quality of this analysis is dependent on the variations in the incoming data. If incoming data just repeats a couple of bytes over and over, the algorithm might fail.");
    lv_msgbox_add_close_button(mbox1);
}

static void exit_button_cb()
{
    //load main menu
    create_menu_screen();
}

static void start_button_cb()
{
    // Start the serial analyzer
    //ESP_LOGI("SerialAnalyzer", "Starting serial analyzer with %d samples", g_settings.serial_analyzer.sample_size);
    #ifdef CONFIG_IDF_TARGET_ESP32S3
    SerialAnalyzer_RMT_start();
    #else
    // TODO: dummy RMT?
    #endif
    //create_menu_screen();
    serial_analyzer_recording_screen();
}

static void com_port_dropdown_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        char buf[32];
        lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
        g_settings.serial_analyzer.com_port = lv_dropdown_get_selected(obj);
        LV_LOG_USER("Option: %s", buf);
        if(g_settings.serial_analyzer.com_port == RS485)
        {
            LV_LOG_USER("RS485 selected");
            lv_obj_clear_state(serial_termination_checkbox, LV_STATE_DISABLED);
            lv_obj_clear_state(serial_bias_checkbox, LV_STATE_DISABLED);
        }
        else
        {
            lv_obj_add_state(serial_termination_checkbox, LV_STATE_DISABLED);
            lv_obj_add_state(serial_bias_checkbox, LV_STATE_DISABLED);
        }
    }
}

static void samples_dropdown_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        char buf[32];
        lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
        uint32_t samples = atoi(buf);
        if(samples > 0)
        {
            g_settings.serial_analyzer.sample_size = samples;
        }
    }
}

static void serial_termination_checkbox_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    if (lv_obj_has_state(obj, LV_STATE_CHECKED)) {
        g_settings.serial_analyzer.termination_enabled = true;
    }
    else{
        g_settings.serial_analyzer.termination_enabled = false;
    }
}

static void serial_bias_checkbox_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    if (lv_obj_has_state(obj, LV_STATE_CHECKED)) {
        g_settings.serial_analyzer.bias_enabled = true;
    }
    else{
        g_settings.serial_analyzer.bias_enabled = false;
    }
}

static void bus_idle_checkbox_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    if (lv_obj_has_state(obj, LV_STATE_CHECKED)) {
        g_settings.serial_analyzer.stop_when_bus_idle = true;
    }
    else{
        g_settings.serial_analyzer.stop_when_bus_idle = false;
    }
}


void serial_analyzer_settings_screen()
{
    lv_obj_clean(lv_scr_act());
    lv_obj_t * screen = lv_obj_create(NULL);
    lv_scr_load(screen);
    create_background_screen();

    //Label
    lv_obj_t *title = lv_label_create(lv_screen_active());
    lv_label_set_text(title, "Serial data analyzer");
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

    //COM group
    lv_obj_t *COM_group = lv_obj_create(group);  // Create container
    lv_obj_set_size(COM_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(COM_group, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(COM_group, LV_FLEX_FLOW_COLUMN);  // Side-by-side
    //lv_obj_clear_flag(COM_group, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(COM_group, LV_OPA_TRANSP, 0);  // Transparent
    lv_obj_set_style_pad_all(COM_group, 1, 1);  // Removes all internal padding
    lv_obj_set_style_border_width(COM_group, 1, 1);
    //COM-label
    lv_obj_t * com_port_label = lv_label_create(COM_group);
    lv_label_set_text(com_port_label, "Input source");
    //lv_obj_align(com_port_label, LV_ALIGN_TOP_LEFT, 0, 40);
    //COM dropdown
    lv_obj_t * com_port_dropdown = lv_dropdown_create(COM_group);
    lv_dropdown_set_options(com_port_dropdown, "RS232\n"
                            "RS485\n"
                            "SDI-12\n"
                            "UART"
                            );
    lv_dropdown_set_selected(com_port_dropdown, (int)g_settings.serial_analyzer.com_port); //set default value
    lv_obj_set_width(com_port_dropdown, 100);
    lv_obj_align(com_port_dropdown, LV_ALIGN_BOTTOM_LEFT, 0, 20);
    lv_obj_add_event_cb(com_port_dropdown, com_port_dropdown_cb, LV_EVENT_VALUE_CHANGED, NULL);
/*    //Add flex for additional serial settings
    COM_group_serial_settings = lv_obj_create(COM_group);  // Create container
    lv_obj_set_size(COM_group_serial_settings, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(COM_group_serial_settings, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(COM_group_serial_settings, LV_FLEX_FLOW_ROW);  // Side-by-side
    lv_obj_set_style_bg_opa(COM_group_serial_settings, LV_OPA_TRANSP, 0);  // Transparent
    lv_obj_set_style_pad_all(COM_group_serial_settings, 0, 0);  // Removes all internal padding
    lv_obj_set_style_border_width(COM_group_serial_settings, 0, 0);
    lv_obj_set_style_pad_column(COM_group_serial_settings, 0, 0);  // 1 pixels between rows*/
    serial_termination_checkbox = lv_checkbox_create(COM_group);
    //lv_obj_set_style_text_font(serial_termination_checkbox, &Greek_alphabet_14, LV_PART_MAIN);
    //lv_checkbox_set_text(serial_termination_checkbox, "120\xCE\xA9 term");
    lv_checkbox_set_text(serial_termination_checkbox, "120 Ohm term");
    //lv_font_t *font = lv_obj_get_style_text_font(serial_termination_checkbox, LV_PART_MAIN);
    //LV_LOG_USER("Font line height: %d\n", font->line_height);
    //LV_LOG_USER("Font base line:   %d\n", font->base_line);
    //if (font == &Greek_alphabet_14) {
    //LV_LOG_USER("Font is Montserrat 14\n");
//}
    lv_obj_set_style_opa(serial_termination_checkbox, LV_OPA_30, LV_PART_MAIN | LV_STATE_DISABLED);
    if(g_settings.serial_analyzer.termination_enabled)
    {
        lv_obj_add_state(serial_termination_checkbox, LV_STATE_CHECKED);
    }
    else
    {
        lv_obj_remove_state(serial_termination_checkbox, LV_STATE_CHECKED);
    }
    if(g_settings.serial_analyzer.com_port == RS485)
    {
        lv_obj_remove_state(serial_termination_checkbox, LV_STATE_DISABLED);
    }
    else
    {
        lv_obj_add_state(serial_termination_checkbox, LV_STATE_DISABLED);
    }
    lv_obj_add_event_cb(serial_termination_checkbox, serial_termination_checkbox_cb, LV_EVENT_VALUE_CHANGED, NULL);
    //
    serial_bias_checkbox = lv_checkbox_create(COM_group);
    lv_checkbox_set_text(serial_bias_checkbox, "bias resistors");
    lv_obj_set_style_opa(serial_bias_checkbox, LV_OPA_30, LV_PART_MAIN | LV_STATE_DISABLED);
    lv_obj_add_state(serial_bias_checkbox, LV_STATE_DISABLED);
    if(g_settings.serial_analyzer.bias_enabled)
    {
        lv_obj_add_state(serial_bias_checkbox, LV_STATE_CHECKED);
    }
    else
    {
        lv_obj_remove_state(serial_bias_checkbox, LV_STATE_CHECKED);
    }
    if(g_settings.serial_analyzer.com_port == RS485)
    {
        lv_obj_remove_state(serial_bias_checkbox, LV_STATE_DISABLED);
    }
    else
    {
        lv_obj_add_state(serial_bias_checkbox, LV_STATE_DISABLED);
    }
    lv_obj_add_event_cb(serial_bias_checkbox, serial_bias_checkbox_cb, LV_EVENT_VALUE_CHANGED, NULL);

    //Sample size group
    lv_obj_t *sample_group = lv_obj_create(group);  // Create container
    lv_obj_set_size(sample_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(sample_group, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(sample_group, LV_FLEX_FLOW_COLUMN);  // Side-by-side
    lv_obj_set_style_bg_opa(sample_group, LV_OPA_TRANSP, 0);  // Transparent
    lv_obj_set_style_pad_all(sample_group, 1, 1);  // Removes all internal padding
    lv_obj_set_style_border_width(sample_group, 1, 1);
    //Sample label
    lv_obj_t * sample_label = lv_label_create(sample_group);
    lv_label_set_text(sample_label, "Sample size");
    //select sample size
    lv_obj_t *dropdown_group = lv_obj_create(sample_group);
    lv_obj_set_size(dropdown_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(dropdown_group, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(dropdown_group, LV_FLEX_FLOW_ROW);  // Side-by-side
    lv_obj_set_style_pad_all(dropdown_group, 0, 0);  // Removes all internal padding
    lv_obj_set_style_border_width(dropdown_group, 0, 0);
    lv_obj_set_style_pad_column(dropdown_group, 0, 0);  // 1 pixels between rows
    lv_obj_set_style_bg_opa(dropdown_group, LV_OPA_TRANSP, 0);  // Transparent
    lv_obj_t * samples_dropdown = lv_dropdown_create(dropdown_group);
    lv_dropdown_set_options(samples_dropdown, SampleSizeDropdownString);
    /*Set a fixed text to display on the button of the drop-down list*/
    lv_dropdown_set_selected(samples_dropdown, getIndexFromString(SampleSizeDropdownString, g_settings.serial_analyzer.sample_size)); //set default value
    lv_obj_set_width(samples_dropdown, 100);
    lv_obj_add_event_cb(samples_dropdown, samples_dropdown_cb, LV_EVENT_VALUE_CHANGED, NULL);

    //help button
    lv_obj_t *sample_help_button = lv_button_create(dropdown_group);
    lv_obj_set_style_bg_color(sample_help_button, lv_color_hex(0x666666), LV_PART_MAIN);
    lv_obj_t *sample_help_button_label = lv_label_create(sample_help_button);
    lv_label_set_text(sample_help_button_label, "?");
    lv_obj_add_event_cb(sample_help_button, (lv_event_cb_t)sample_help_button_cb, LV_EVENT_CLICKED, NULL);

        //checkbox for stop when bus is idle
    lv_obj_t * bus_idle_checkbox_group = lv_obj_create(sample_group);
    lv_obj_set_size(bus_idle_checkbox_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(bus_idle_checkbox_group, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bus_idle_checkbox_group, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_bg_opa(bus_idle_checkbox_group, LV_OPA_TRANSP, 0);  // Transparent
    lv_obj_set_style_pad_all(bus_idle_checkbox_group, 0, 0);
    lv_obj_set_style_border_width(bus_idle_checkbox_group, 0, 0);
    lv_obj_set_style_pad_column(bus_idle_checkbox_group, 0, 0);
    lv_obj_t * bus_idle_checkbox = lv_checkbox_create(bus_idle_checkbox_group);
    lv_checkbox_set_text(bus_idle_checkbox, "Stop at\nbus idle");
    if(g_settings.serial_analyzer.stop_when_bus_idle)
    {
        lv_obj_add_state(bus_idle_checkbox, LV_STATE_CHECKED);
    }
    else
    {
        lv_obj_remove_state(bus_idle_checkbox, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(bus_idle_checkbox, (lv_event_cb_t)bus_idle_checkbox_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t * bus_idle_checkbox_help_button = lv_button_create(bus_idle_checkbox_group);
    lv_obj_set_style_bg_color(bus_idle_checkbox_help_button, lv_color_hex(0x666666), LV_PART_MAIN);
    lv_obj_t * bus_idle_checkbox_help_button_label = lv_label_create(bus_idle_checkbox_help_button);
    lv_label_set_text(bus_idle_checkbox_help_button_label, "?");
    lv_obj_add_event_cb(bus_idle_checkbox_help_button, (lv_event_cb_t)bus_idle_checkbox_help_button_cb, LV_EVENT_CLICKED, NULL);

    //Buttons
    lv_obj_t *button_group = lv_obj_create(group);
    lv_obj_set_size(button_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(button_group, 1);
    lv_obj_set_layout(button_group, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(button_group, LV_FLEX_FLOW_COLUMN);  // Side-by-side
    lv_obj_set_style_bg_opa(button_group, LV_OPA_TRANSP, 0);  // Transparent
    lv_obj_set_style_pad_all(button_group, 0, 0);  // Removes all internal padding
    lv_obj_set_style_border_width(button_group, 0, 0);
    lv_obj_set_style_pad_row(button_group, 1, 0);  // 1 pixels between rows
    //about button
    lv_obj_t *about_button = lv_button_create(button_group);
    lv_obj_set_style_bg_color(about_button, lv_color_hex(0x666666), LV_PART_MAIN);
    lv_obj_set_width(about_button, LV_PCT(90));
    lv_obj_t *about_button_label = lv_label_create(about_button);
    lv_label_set_text(about_button_label, "About");
    lv_obj_center(about_button_label);
    lv_obj_add_event_cb(about_button, (lv_event_cb_t)about_button_cb, LV_EVENT_CLICKED, NULL);
    //back button
    lv_obj_t *exit_button = lv_button_create(button_group);
    lv_obj_set_style_bg_color(exit_button, lv_color_hex(0xAA0000), LV_PART_MAIN);
    lv_obj_set_width(exit_button, LV_PCT(90));
    lv_obj_t *exit_button_label = lv_label_create(exit_button);
    lv_label_set_text(exit_button_label, "Exit");
    lv_obj_center(exit_button_label);
    lv_obj_add_event_cb(exit_button, (lv_event_cb_t)exit_button_cb, LV_EVENT_CLICKED, NULL);
    //start button
    lv_obj_t *start_button = lv_button_create(button_group);
    lv_obj_set_style_bg_color(start_button, lv_color_hex(0x00CC44), LV_PART_MAIN);
    lv_obj_set_width(start_button, LV_PCT(90));
    lv_obj_t *start_button_label = lv_label_create(start_button);
    lv_label_set_text(start_button_label, "Start");
    lv_obj_center(start_button_label);
    lv_obj_add_event_cb(start_button, (lv_event_cb_t)start_button_cb, LV_EVENT_CLICKED, NULL);
}

static void recording_cancel_button_cb()
{
    analyzer_abort_requested = true;
    serial_analyzer_settings_screen();
}

void serial_analyzer_recording_screen()
{
    lv_obj_clean(lv_scr_act());
    lv_obj_t * screen = lv_obj_create(NULL);
    lv_scr_load(screen);
    create_background_screen();

    static int32_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static int32_t row_dsc[] = {LV_GRID_FR(1), 20, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_t * cont = lv_obj_create(lv_screen_active());
    lv_obj_set_style_grid_column_dsc_array(cont, col_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(cont, row_dsc, 0);
    lv_obj_set_size(cont, 300, 220);
    lv_obj_center(cont);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);
    //Cell (0,0)
    lv_obj_t *title = lv_label_create(cont);
    lv_label_set_text(title, "Recording...");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_grid_cell(title, LV_GRID_ALIGN_START, 0, 1,
                             LV_GRID_ALIGN_CENTER, 0, 1);
    //cell (1,0)
    lv_obj_t * spinner = lv_spinner_create(cont);
    lv_obj_set_size(spinner, 50, 50);
    lv_obj_set_grid_cell(spinner, LV_GRID_ALIGN_START, 1, 1,
                             LV_GRID_ALIGN_CENTER, 0, 1);
    lv_spinner_set_anim_params(spinner, 1000, 200);

    //cell (0,1)
    lv_obj_t *received_symbols_label = lv_label_create(cont);
    lv_label_set_text(received_symbols_label, "Received symbols:");
    lv_obj_set_grid_cell(received_symbols_label, LV_GRID_ALIGN_END, 0, 1,
                             LV_GRID_ALIGN_START, 1, 1);

    recording_num_symbols_label = lv_label_create(cont);
    lv_label_set_text(recording_num_symbols_label, "0");
    lv_obj_set_grid_cell(recording_num_symbols_label, LV_GRID_ALIGN_START, 1, 1,
                             LV_GRID_ALIGN_START, 1, 1);

    //cell (0,2) cancel btn
    lv_obj_t * recording_cancel_button = lv_button_create(cont);
    lv_obj_set_style_bg_color(recording_cancel_button, lv_color_hex(0xAA0000), LV_PART_MAIN);
    lv_obj_set_width(recording_cancel_button, LV_PCT(90));
    lv_obj_t *recording_cancel_button_label = lv_label_create(recording_cancel_button);
    lv_label_set_text(recording_cancel_button_label, "Abort");
    lv_obj_set_style_text_font(recording_cancel_button_label, &lv_font_montserrat_20, LV_PART_MAIN);

    lv_obj_center(recording_cancel_button_label);
    lv_obj_set_grid_cell(recording_cancel_button, LV_GRID_ALIGN_STRETCH, 0, 2,
                             LV_GRID_ALIGN_STRETCH, 2, 1);
    lv_obj_add_event_cb(recording_cancel_button, (lv_event_cb_t)recording_cancel_button_cb, LV_EVENT_CLICKED, NULL);
}

void serial_analyzer_init_defaults(SerialAnalyzerConfig *config)
{
    if (config == NULL) return;  // Check for null pointer
    memset(config, 0, sizeof(SerialAnalyzerConfig));  // Clear the structure
    config->com_port = RS232;  // Default com port
    config->termination_enabled = false;  // Default termination state
    config->bias_enabled = false;  // Default bias state
    config->stop_when_bus_idle = false;
    config->sample_size = 100;  // Default sample size
}
