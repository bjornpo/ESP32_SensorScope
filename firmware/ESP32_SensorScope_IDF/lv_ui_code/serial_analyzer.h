//serial_analyzer
#pragma once
//#ifdef ESP_PLATFORM
//#include "lvgl.h"
//#endif
#include "lvgl.h"
#include "../main/shared_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    enum comPort com_port;
    bool termination_enabled;
    bool bias_enabled;
    bool stop_when_bus_idle; // Stop capturing when bus is idle
    uint32_t sample_size;
} SerialAnalyzerConfig;

typedef enum {
    SERIAL_PARITY_NONE,
    SERIAL_PARITY_EVEN,
    SERIAL_PARITY_ODD,
    SERIAL_PARITY_SPACE,
}parity_type_t;

typedef struct {
    uint16_t num_symbols;
    uint32_t bit_duration_ns; // in nanoseconds
    uint32_t measured_baud_rate;
    uint32_t nearest_baud_rate;
    uint16_t num_frames;
    uint16_t num_framing_errors;
    uint16_t num_data_bits;
    uint16_t num_stop_bits;
    parity_type_t parity_type;
    bool is_ASCII;
    uint8_t *bit_array;
    uint16_t size_bit_array;
    char *decoded_data;
    uint16_t size_decoded_data;
} Serial_analyzer_data_t;

// Global settings for cancelling the analyzer and killing the task
extern volatile bool analyzer_abort_requested;
extern volatile bool analyzer_task_terminate_g;

void serial_analyzer_init_defaults(SerialAnalyzerConfig *config);
void serial_analyzer_settings_screen();
void serial_analyzer_recording_screen();
void serial_analyzer_recording_update_async(void *num_ptr); // Update the recording screen with the number of symbols received
void serial_analyzer_show_results_screen_async(void *num_ptr);
#ifndef ESP_PLATFORM
void fill_serial_analyzer_data(Serial_analyzer_data_t *data);
#endif // ESP_PLATFORM


#ifdef __cplusplus
}
#endif
