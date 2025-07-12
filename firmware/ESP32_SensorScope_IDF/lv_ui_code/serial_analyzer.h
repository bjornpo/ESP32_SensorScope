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
    uint32_t sample_size;
} SerialAnalyzerConfig;

/*void serial_anlayzer_default_config(SerialAnalyzerConfig *config) {
    config->com_port = RS232; // Default com port
    config->termination_enabled = false; // Default termination state
    config->bias_enabled = false; // Default bias state
    config->sample_size = 100; // Default sample size
}*/
void serial_analyzer_settings_screen();

#ifdef __cplusplus
extern "C" {
#endif
