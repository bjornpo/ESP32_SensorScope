//serial_analyzer
#pragma once
//#ifdef ESP_PLATFORM
//#include "lvgl.h"
//#endif
#include "lvgl.h"
#include "../main/shared_types.h"

typedef enum {
    OSCILLOSCOPE_MODE_ROLL,
    OSCILLOSCOPE_MODE_NORMAL,
    OSCILLOSCOPE_MODE_SINGLE,
} oscilloscope_mode_t;

typedef enum {
    OSCILLOSCOPE_RANGE_0_5V,
    OSCILLOSCOPE_RANGE_NEG5V_5V,
    OSCILLOSCOPE_RANGE_0_10V,
    OSCILLOSCOPE_RANGE_NEG10V_10V,
} oscilloscope_v_range_t;

typedef enum {
    OSCILLOSCOPE_CHANNEL_CH1,
    OSCILLOSCOPE_CHANNEL_CH2,
    OSCILLOSCOPE_CHANNEL_CH1_CH2,
}oscilloscope_ch_t;


typedef struct {
    oscilloscope_mode_t mode;
    oscilloscope_ch_t channel;
    oscilloscope_v_range_t v_range;
    uint32_t v_offset;
    int h_timebase_item; // Horizontal time base in microseconds
    uint32_t trigger_level_mv; // Trigger level in mV
    uint32_t sample_rate_hz; // Sample rate in Hz
} Oscilloscope_config_t;

// typedef struct {
//     uint8_t *buffer;
//     uint32_t size; // Size of the buffer in bytes
// } scope_buffer_t;

void oscilloscope_settings_screen(void);
void oscilloscope_recording_screen(void);
void oscilloscope_init_defaults(Oscilloscope_config_t *config);
