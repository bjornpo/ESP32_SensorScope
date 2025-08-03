#pragma once

#include "driver/rmt_rx.h"
#include "serial_analyzer.h"

enum BitFlags {
    BIT_LEVEL    = 1 << 0,  // logic level
    BIT_START    = 1 << 1,
    BIT_DATA     = 1 << 2,
    BIT_PARITY   = 1 << 3,
    BIT_STOP     = 1 << 4,
    BIT_ERROR    = 1 << 5,
    BIT_IDLE     = 1 << 6,
    BIT_TIMEOUT  = 1 << 7, 
};



void SerialAnalyzer_RMT_start(void);
void analyze_symbols(rmt_symbol_word_t *symbols, size_t num_symbols, uint8_t *bit_array, size_t size_bit_array, char *decoded_data, size_t size_decoded_data, Serial_analyzer_data_t *serial_analyzer_output);
uint32_t get_nearest_baud(int measured_baud);