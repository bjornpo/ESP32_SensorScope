#include "oscilloscope_ui.h"
#include <lvgl.h>
#include <string.h>
#include "../main/shared_types.h"
#include "../main/settings.h"
#include "appGUI.h"
#ifdef ESP_PLATFORM
#include "oscilloscope.h"
#include "esp_heap_caps.h"
#define OSCILLOSCOPE_LOCK() oscilloscope_lock()
#define OSCILLOSCOPE_UNLOCK() oscilloscope_unlock()
#else
#define OSCILLOSCOPE_LOCK() ((void)0)
#define OSCILLOSCOPE_UNLOCK() ((void)0)
#endif // ESP_PLATFORM

#define MAX_SAMPLES 100

const char TimebaseDropdownString[] = "60 S\n30 S\n5 S\n1 S\n500 mS\n100 mS\n50 mS\n10 mS\n5 mS";

//TimebaseDropdownItems and TimebaseDropdownValues_ms must match
const char *TimebaseDropdownItems =
    "60 S\n"
    "20 S\n"
    "5 S\n"
    "1 S\n"
    "500 mS\n"
    "100 mS\n"
    "50 mS\n"
    "10 mS\n"
    "5 mS";

const uint32_t TimebaseDropdownValues_ms[] = {
    60000,
    20000,
    5000,
    1000,
    500,
    100,
    50,
    10,
    5
};

int find_timebase_index(uint32_t value_ms) {
    for (size_t i = 0; i < (sizeof(TimebaseDropdownValues_ms) / sizeof((TimebaseDropdownValues_ms)[0])); i++) {
        if (TimebaseDropdownValues_ms[i] == value_ms) {
            return (int)i; // return index
        }
    }
    return 0; // not found
}

static void chart_update(lv_timer_t * t)
{
    lv_obj_t * chart = lv_timer_get_user_data(t);
    OSCILLOSCOPE_LOCK(); // Dont update chart while buffer is updating
    lv_chart_refresh(chart);
    OSCILLOSCOPE_UNLOCK();
}

static void exit_button_cb()
{
    //load main menu
    create_menu_screen();
}

static void start_button_cb()
{
    //load main menu
    //create_menu_screen();
    oscilloscope_recording_screen();
}

static void oscilloscope_channel_dropdown_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED)
    {
        g_settings.oscilloscope.channel = lv_dropdown_get_selected(obj);
    }
}

static void oscilloscope_mode_dropdown_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED)
    {
        g_settings.oscilloscope.mode = lv_dropdown_get_selected(obj);
        if(g_settings.oscilloscope.mode == OSCILLOSCOPE_MODE_ROLL && TimebaseDropdownValues_ms[g_settings.oscilloscope.h_timebase_item] < 1000)
        {
            g_settings.oscilloscope.h_timebase_item = find_timebase_index(1000);
            oscilloscope_settings_screen(); //Reload the screen to update timebase dirty hack ;(
        }
    }
}

static void oscilloscope_timebase_roller_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED)
    {
        if(g_settings.oscilloscope.mode == OSCILLOSCOPE_MODE_ROLL && TimebaseDropdownValues_ms[lv_roller_get_selected(obj)] < 1000)
        {
            lv_roller_set_selected(obj, find_timebase_index(1000), LV_ANIM_ON); //Forse timebase to 1000 mS
            g_settings.oscilloscope.h_timebase_item = find_timebase_index(1000);
            LV_LOG_USER("short timebase not supportet in Roll mode");
        }
        else{
            g_settings.oscilloscope.h_timebase_item = lv_roller_get_selected(obj);
        }
    }
}

void oscilloscope_settings_screen()
{
    lv_obj_clean(lv_scr_act());
    lv_obj_t * screen = lv_obj_create(NULL);
    lv_scr_load(screen);
    create_background_screen();

    //Label
    lv_obj_t *title = lv_label_create(lv_screen_active());
    lv_label_set_text(title, "Oscilloscope");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 5, 25);

    //Settings group
    lv_obj_t *group = lv_obj_create(lv_screen_active());
    lv_obj_set_size(group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(group, LV_ALIGN_TOP_LEFT, 0, 55);
    lv_obj_set_layout(group, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(group, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(group, 1, 1);  // Removes all internal padding
    lv_obj_set_style_border_width(group, 1, 1);
    lv_obj_set_style_bg_opa(group, LV_OPA_TRANSP, 0);  // Transparent

    //Mode select
    lv_obj_t * label = lv_label_create(group);
    lv_label_set_text(label, "Mode");
    lv_obj_t * dropdown = lv_dropdown_create(group);
    lv_dropdown_set_options(dropdown, "Roll\n"
                            "Normal\n"
                            "Single"
                            );
    lv_dropdown_set_selected(dropdown, (int)g_settings.oscilloscope.mode); //set default value
    //lv_obj_set_width(dropdown, LV_PCT(90));
    lv_obj_set_width(dropdown, 100);
    lv_obj_add_event_cb(dropdown, oscilloscope_mode_dropdown_cb, LV_EVENT_VALUE_CHANGED, NULL);

    //Channel select
    label = lv_label_create(group);
    lv_label_set_text(label, "Channel");
    dropdown = lv_dropdown_create(group);
    lv_dropdown_set_options(dropdown, "Ch1\n"
                            "Ch2\n"
                            "Ch1+Ch2"
                            );
    lv_dropdown_set_selected(dropdown, (int)g_settings.oscilloscope.channel); //set default value
    //lv_obj_set_width(dropdown, LV_PCT(90));
    lv_obj_set_width(dropdown, 100);
    lv_obj_add_event_cb(dropdown, oscilloscope_channel_dropdown_cb, LV_EVENT_VALUE_CHANGED, NULL);

    //Range select
        //Channel select
    label = lv_label_create(group);
    lv_label_set_text(label, "Measuring range");
    dropdown = lv_dropdown_create(group);
    lv_dropdown_set_options(dropdown, "0-5V\n"
                            "+-5V\n"
                            "0-10V\n"
                            "+-10V"
                            );
    //lv_dropdown_set_selected(dropdown, (int)g_settings.serial_analyzer.com_port); //set default value
    //lv_obj_set_width(dropdown, LV_PCT(90));
    lv_obj_set_width(dropdown, 100);
    //lv_obj_add_event_cb(dropdown, oscilloscope_mode_dropdown_cb, LV_EVENT_VALUE_CHANGED, NULL);

    //Timing selection
    group = lv_obj_create(lv_screen_active());
    lv_obj_set_size(group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(group, LV_ALIGN_TOP_LEFT, 160, 55);
    lv_obj_set_layout(group, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(group, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(group, 1, 1);  // Removes all internal padding
    lv_obj_set_style_border_width(group, 1, 1);
    lv_obj_set_style_bg_opa(group, LV_OPA_TRANSP, 0);  // Transparent

    label = lv_label_create(group);
    lv_label_set_text(label, "Timebase");

    lv_obj_t * roller1 = lv_roller_create(group);
    lv_roller_set_options(roller1, TimebaseDropdownItems, LV_ROLLER_MODE_NORMAL);

    lv_roller_set_visible_row_count(roller1, 4);
    lv_roller_set_selected(roller1, g_settings.oscilloscope.h_timebase_item, LV_ANIM_OFF);
    //lv_obj_center(roller1);
    lv_obj_add_event_cb(roller1, oscilloscope_timebase_roller_cb, LV_EVENT_VALUE_CHANGED, NULL);

    label = lv_label_create(group);
    lv_label_set_text(label, "Samplerate:\n1000 kHz");

    //buttons
    group = lv_obj_create(lv_screen_active());
    lv_obj_set_size(group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(group, LV_ALIGN_TOP_LEFT, 280, 55);
    lv_obj_set_layout(group, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(group, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(group, 1, 1);  // Removes all internal padding
    lv_obj_set_style_border_width(group, 1, 1);
    lv_obj_set_style_bg_opa(group, LV_OPA_TRANSP, 0);  // Transparent

    lv_obj_t *button = lv_button_create(group);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x00CC44), LV_PART_MAIN);
    //lv_obj_set_width(button, LV_PCT(90));
    lv_obj_set_width(button, 150);
    lv_obj_t *button_label = lv_label_create(button);
    lv_label_set_text(button_label, "Start");
    lv_obj_center(button_label);
    lv_obj_add_event_cb(button, (lv_event_cb_t)start_button_cb, LV_EVENT_CLICKED, NULL);

    button = lv_button_create(group);
    lv_obj_set_style_bg_color(button, lv_color_hex(0xAA0000), LV_PART_MAIN);
    //lv_obj_set_width(button, LV_PCT(90));
    lv_obj_set_width(button, 150);
    button_label = lv_label_create(button);
    lv_label_set_text(button_label, "exit");
    lv_obj_center(button_label);
    lv_obj_add_event_cb(button, (lv_event_cb_t)exit_button_cb, LV_EVENT_CLICKED, NULL);
}

void oscilloscope_recording_screen()
{
    lv_obj_clean(lv_scr_act());
    lv_obj_t * screen = lv_obj_create(NULL);
    lv_scr_load(screen);
    create_background_screen();

    int32_t *ch1_buf = NULL; //circular buffer for CH1
    int32_t *ch2_buf = NULL; //circular buffer for CH2
    int32_t *ch1_chart_buf = NULL; // linear buffer for CH1 chart
    int32_t *ch2_chart_buf = NULL; // linear buffer for CH2 chart

    #ifdef ESP_PLATFORM
    //Allocate memory in SPIRAM
    ch1_buf = heap_caps_malloc(MAX_SAMPLES * sizeof(int32_t), MALLOC_CAP_SPIRAM);
    ch2_buf = heap_caps_malloc(MAX_SAMPLES * sizeof(int32_t), MALLOC_CAP_SPIRAM);
    ch1_chart_buf = heap_caps_malloc(MAX_SAMPLES * sizeof(int32_t), MALLOC_CAP_SPIRAM);
    ch2_chart_buf = heap_caps_malloc(MAX_SAMPLES * sizeof(int32_t), MALLOC_CAP_SPIRAM);
    #else
    //Allocate memory in DRAM
    ch1_buf = malloc(MAX_SAMPLES * sizeof(int32_t));
    ch2_buf = malloc(MAX_SAMPLES * sizeof(int32_t));
    ch1_chart_buf = malloc(MAX_SAMPLES * sizeof(int32_t));
    ch2_chart_buf = malloc(MAX_SAMPLES * sizeof(int32_t));
    #endif // ESP_PLATFORM
    if (ch1_buf == NULL || ch2_buf == NULL || ch1_chart_buf == NULL || ch2_chart_buf == NULL) {
        LV_LOG_ERROR("Failed to allocate memory for oscilloscope buffers");
        return;
    }

    memset(ch1_chart_buf, LV_CHART_POINT_NONE, MAX_SAMPLES * sizeof(int32_t));

    lv_obj_t *chart_parent = lv_obj_create(lv_scr_act());
    lv_obj_set_size(chart_parent, 480, 250);
    lv_obj_align(chart_parent, LV_ALIGN_TOP_LEFT, 0, 20);
    lv_obj_set_style_pad_all(chart_parent, 0, 0);  // Removes all internal padding

    lv_obj_t *chart = lv_chart_create(chart_parent);
    //lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);
    //lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_CIRCULAR);
    lv_chart_set_point_count(chart, MAX_SAMPLES);
    lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR); //dots?
    lv_chart_series_t *ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 4100);
    lv_chart_set_ext_y_array(chart, ser, ch1_chart_buf);
    lv_obj_set_size(chart, 470, 240);
    lv_obj_center(chart);

    lv_timer_create(chart_update, 40, chart);
    lv_chart_refresh(chart);

    // start sampling task
    #ifdef CONFIG_IDF_TARGET_ESP32S3
    oscilloscope_sampling_start(ch1_buf, ch1_chart_buf, ch2_buf, ch2_chart_buf, MAX_SAMPLES);
    #endif // CONFIG_IDF_TARGET_ESP32S3
}

void oscilloscope_init_defaults(Oscilloscope_config_t *config)
{
    if (config == NULL) return;  // Check for null pointer
    memset(config, 0, sizeof(Oscilloscope_config_t));  // Clear the structure
    config->mode = OSCILLOSCOPE_MODE_ROLL;
    config->channel = OSCILLOSCOPE_CHANNEL_CH1;
    config->v_range = OSCILLOSCOPE_RANGE_NEG10V_10V;
    config->h_timebase_item = 3;
    config->trigger_level_mv = 2500;
    config->sample_rate_hz = 100; // Default sample rate in Hz
}
