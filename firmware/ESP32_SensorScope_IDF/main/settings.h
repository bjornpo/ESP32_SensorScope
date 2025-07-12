#pragma once
#include "serial_analyzer.h"

typedef struct {
    uint32_t version;
    SerialAnalyzerConfig serial_analyzer;
    //ModbusConfig modbus;
    //SDI12Config sdi12;
} AppSettings;

extern AppSettings g_settings;