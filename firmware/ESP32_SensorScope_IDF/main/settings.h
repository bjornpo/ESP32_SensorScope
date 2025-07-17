#pragma once
#include "serial_analyzer.h"

typedef struct {
    uint32_t version;
    SerialAnalyzerConfig serial_analyzer;
    //ModbusConfig modbus;
    //SDI12Config sdi12;
} AppSettings;

void nvs_init(void);
void nvs_save_settings();
void init_default_settings(AppSettings *cfg);

extern AppSettings g_settings;