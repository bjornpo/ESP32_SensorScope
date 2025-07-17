//serial_analyzer
#pragma once
//#ifdef ESP_PLATFORM
//#include "lvgl.h"
//#endif
#include "lvgl.h"
#include "../main/shared_types.h"

typedef struct {
    enum comPort com_port;
    bool termination_enabled;
    bool bias_enabled;
    bool stop_when_bus_idle; // Stop capturing when bus is idle
    uint32_t sample_size;
} SerialAnalyzerConfig;

void serial_analyzer_init_defaults(SerialAnalyzerConfig *config);
void serial_analyzer_settings_screen();

#ifdef __cplusplus
extern "C" {
#endif
