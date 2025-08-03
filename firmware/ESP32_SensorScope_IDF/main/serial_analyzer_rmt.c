#include <string.h>
#include "serial_analyzer_rmt.h"
#include "display_driver.h" // For display driver mutex
#include "serial_analyzer.h"
#include "settings.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "driver/rmt.h"
#include "driver/rmt_rx.h"
#include "driver/gpio.h"
#include <lvgl.h>


#define RMT_ANALYZER_TASK_STACK_SIZE (12*1024)//6*1024)
#define RMT_ANALYZER_TASK_PRIORITY 20 //High priority for fast restart of RMT_receive
#define RMT_SAMPLE_HZ 1000 * 1000 // 1 µs resolution
#define SIZE_BIT_ARRAY 5000 // Size of the bit array for the analyzer
#define SIZE_OUTPUT_BUFFER SIZE_BIT_ARRAY/3 // Size of the output buffer for the analyzer
#define RMT_TIMEOUT_TO_BITS 10 // Replaces a timeout with a number of dummy bits

#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct {
    size_t num_symbols;
    bool is_last; // Indicating if the current received data are the last part of the transaction
} rmt_event_t;


QueueHandle_t rmt_event_queue;

#define RMT_RX_GPIO GPIO_NUM_44  // GPIO for RMT RX 44

static rmt_channel_handle_t rx_channel = NULL;

static const char *TAG = "RMT_Analyzer";

static bool IRAM_ATTR on_rmt_rx_done(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data)
{
    // This is called in ISR context when capture completes
    gpio_set_level(GPIO_NUM_48, 1);
    BaseType_t task_awoken = pdFALSE;
    size_t * counter = (size_t *)user_data;

    bool is_last = g_settings.serial_analyzer.stop_when_bus_idle ||
                   ((edata->num_symbols + *counter) > (g_settings.serial_analyzer.sample_size - 10 - (g_settings.serial_analyzer.sample_size * 20) / 100));

    rmt_event_t evt = {
        .num_symbols = edata->num_symbols,
        .is_last = is_last,
    };

    xQueueSendFromISR(rmt_event_queue, &evt, &task_awoken);

    return task_awoken == pdTRUE;
}

void rmt_analyzer_task(void *arg)
{
    ESP_LOGI(TAG, "RMT Analyzer task started. Sample size: %d", (int)g_settings.serial_analyzer.sample_size);

    rmt_event_queue = xQueueCreate(4, sizeof(rmt_event_t));
    size_t true_num_symbols = 0; //rmt_rx_done_event_data_t->num_symbols get reset after each receive
    // Todo: Configure GPIOS for the input source and termination

    // Configure RMT RX channel
    rmt_rx_channel_config_t rx_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = RMT_RX_GPIO,
        .mem_block_symbols = g_settings.serial_analyzer.sample_size,   // max symbols/flanks to capture
        .resolution_hz = RMT_SAMPLE_HZ,
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_config, &rx_channel));

    // Register callback
    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = on_rmt_rx_done,
    };
    ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(rx_channel, &cbs, &true_num_symbols));

    // Enable channel
    ESP_ERROR_CHECK(rmt_enable(rx_channel));

    rmt_receive_config_t receive_config = {
        .signal_range_min_ns = 3000,     // bit-length at 23200 baud
        .signal_range_max_ns = 10000000, // the longest duration for NEC signal is 9000 µs, 12000000 ns > 9000 µs, the receive does not stop early
        .flags.en_partial_rx = false,     // Enable partial reception if buffer is not enough
    };


    rmt_symbol_word_t rx_symbols[g_settings.serial_analyzer.sample_size];

    // Start receiving
    ESP_LOGI(TAG, "Waiting for signal...");
    ESP_ERROR_CHECK(rmt_receive(rx_channel, rx_symbols, sizeof(rx_symbols), &receive_config));
    //ESP_ERROR_CHECK(rmt_receive(rx_channel, rx_symbols, sizeof(rx_symbols), NULL));

    //bool task_done = false;
    analyzer_abort_requested = false; // Reset the abort request
    //Serial_analyzer_rmt_data_t serial_analyzer_output;

    // allocate memory for the output data
    uint8_t *bit_array = heap_caps_malloc(SIZE_BIT_ARRAY, MALLOC_CAP_SPIRAM);
    if (bit_array == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for bit_array");
    }
    memset(bit_array, 0, SIZE_BIT_ARRAY); // Initialize the bit array to 0

    char *decoded_data = heap_caps_malloc(SIZE_OUTPUT_BUFFER, MALLOC_CAP_SPIRAM);
        if (decoded_data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for decoded data");
    }

    Serial_analyzer_data_t serial_analyzer_output;
    // Serial_analyzer_data_t *serial_analyzer_output = heap_caps_malloc(sizeof(Serial_analyzer_data_t), MALLOC_CAP_SPIRAM);
    // if (!serial_analyzer_output) {
    //     ESP_LOGE(TAG, "Failed to allocate memory for output struct");
    //     return;
    // }

    while (1)
    {
        rmt_event_t evt;        
        if (xQueueReceive(rmt_event_queue, &evt, pdMS_TO_TICKS(100))) {
            true_num_symbols += evt.num_symbols;
            if(!evt.is_last) {
                // If not the last part, we can continue receiving
                size_t used_bytes = true_num_symbols * sizeof(rmt_symbol_word_t);
                ESP_ERROR_CHECK(rmt_receive(rx_channel, rx_symbols+true_num_symbols, sizeof(rx_symbols)-used_bytes, &receive_config));
                ESP_LOGI(TAG, "Received %d symbols, total received symbols is %d waiting for more...", evt.num_symbols, true_num_symbols);
                // Update the recording screen with the number of symbols received
                lvgl_lock();
                lv_async_call(serial_analyzer_recording_update_async, &true_num_symbols);
                lvgl_unlock();
                continue;
            }
            else
            {
                analyze_symbols(rx_symbols, true_num_symbols, bit_array, SIZE_BIT_ARRAY, decoded_data, SIZE_OUTPUT_BUFFER, &serial_analyzer_output);
                //task_done = true;
                //analyzer_task_terminate_g = true; // Signal the task to terminate
            }
        }
        else if(analyzer_abort_requested)
        {
            analyzer_abort_requested = false;
            ESP_LOGI(TAG, "Analyzer task aborted by user. num of symbols = %d ", true_num_symbols);
            //if(true_num_symbols > 10) {
                // If we have received some symbols, we can process them
                analyze_symbols(rx_symbols, true_num_symbols, bit_array, SIZE_BIT_ARRAY, decoded_data, SIZE_OUTPUT_BUFFER, &serial_analyzer_output);
                //TODO: modify to return to the settings screen
            //}
            //analyzer_task_terminate_g = true; // Signal the task to terminate
            //task_done = true;
        }

        if(analyzer_task_terminate_g) {
            analyzer_task_terminate_g = false;
            ESP_LOGI(TAG, "Analyzer task completed, closing task");
            //small delay to allow the UI to update
            vTaskDelay(pdMS_TO_TICKS(100));
            heap_caps_free(bit_array);
            heap_caps_free(decoded_data);
            //heap_caps_free(serial_analyzer_output); // Free the struct after use  
            ESP_ERROR_CHECK(rmt_disable(rx_channel));
            ESP_ERROR_CHECK(rmt_del_channel(rx_channel));
            vQueueDelete(rmt_event_queue);
            // Todo: Release GPIOS for the input source and termination
            vTaskDelete(NULL);
        }
    }
}


void analyze_symbols(rmt_symbol_word_t *symbols, size_t num_symbols, uint8_t *bit_array, size_t size_bit_array, char *decoded_data, size_t size_decoded_data, Serial_analyzer_data_t *serial_analyzer_output)
{
    if (num_symbols < 10) {
        ESP_LOGI(TAG, "Not enough symbols received");
        lvgl_lock();
        serial_analyzer_output->num_symbols = 0;
        lv_async_call(serial_analyzer_show_results_screen_async, serial_analyzer_output);
        lvgl_unlock();
        return;
    }
    ESP_LOGI(TAG, "Received %d symbols", num_symbols);
    // Process only the valid symbols in rx_symbols[]
    // for (int i = 0; i < num_symbols; i++) 
    // {
    //     rmt_symbol_word_t *sym = &symbols[i];
    //     ESP_LOGI(TAG, "Symbol %d: level0=%d, duration0=%d, level1=%d, duration1=%d",
    //     i, sym->level0, sym->duration0, sym->level1, sym->duration1);
    //     vTaskDelay(pdMS_TO_TICKS(5)); // Small delay to allowidle task to run
    // }
    ESP_LOGI(TAG, "Detecting baud rate...");
    //find shotest duration0 or duration1 that is not zero. also find the longest duration0 or duration1
    uint32_t min_duration_us = UINT32_MAX;
    uint32_t max_duration0 = 0;
    uint32_t max_duration1 = 0;
    for (int i = 0; i < num_symbols; i++) {
        rmt_symbol_word_t *sym = &symbols[i];
        if (sym->duration0 > 0 && sym->duration0 < min_duration_us) {
            min_duration_us = sym->duration0;
        }
        if (sym->duration1 > 0 && sym->duration1 < min_duration_us) {
            min_duration_us = sym->duration1;
        }
        max_duration0 = MAX(max_duration0, sym->duration0);
        max_duration1 = MAX(max_duration1, sym->duration1);
    }
    //ESP_LOGI(TAG, "Minimum duration found: %"PRIu32" uS", min_duration_us);
    //ESP_LOGI(TAG, "Maximum duration0 found: %"PRIu32" uS\t maximum duration1 found: %"PRIu32" uS", max_duration0, max_duration1);
    // find average bit duration
    uint32_t max_bit_duration = min_duration_us + (min_duration_us / 2); // Allow some tolerance
    uint32_t bit_count = 0;
    uint32_t total_duration_us = 0;
    for (int i = 0; i < num_symbols; i++) {
        rmt_symbol_word_t *sym = &symbols[i];
        if(sym->duration0 > 0 && sym->duration0 <= max_bit_duration) {
            bit_count++;
            total_duration_us += (sym->duration0 > 0) ? sym->duration0 : sym->duration1;
        }
        if(sym->duration1 > 0 && sym->duration1 <= max_bit_duration) {
            bit_count++;
            total_duration_us += (sym->duration1 > 0) ? sym->duration1 : sym->duration0;
        }
    }
    uint32_t bit_duration_ns = (bit_count > 0) ? (uint32_t)(((uint64_t)total_duration_us * 1000) / bit_count) : min_duration_us * 1000; // Convert to nanoseconds
    // Divide by 1000 later if you want microseconds, or keep as nanoseconds for more precision
    ESP_LOGI(TAG, "Average bit duration: %"PRIu32" nS", bit_duration_ns);
    // Calculate baud rate
    uint32_t measured_baud_rate = (bit_duration_ns > 0) ? (1000000000 / bit_duration_ns) : 0; // 1 second = 1000000000 nanoseconds
    ESP_LOGI(TAG, "Measured baud rate: %"PRIu32" bps", measured_baud_rate);
    ESP_LOGI(TAG, "Nearest common baud rate: %"PRIu32" bps", get_nearest_baud(measured_baud_rate));
    // look for bus idle
    uint32_t bus_idle_trigger = min_duration_us * 12;
    int32_t bus_idle_level = -1; // -1 means no bus idle detected yet
    for(int i = 0; i < num_symbols; i++) {
        rmt_symbol_word_t *sym = &symbols[i];
        if (sym->duration0 > bus_idle_trigger || sym->duration0 ==0) {
            bus_idle_level = sym->level0;
            //ESP_LOGI(TAG, "Bus idle detected at symbol %d with level0=%d", i, sym->level0);
            break;
        }
        else if (sym->duration1 > bus_idle_trigger || sym->duration1 == 0) {
            bus_idle_level = sym->level1;
            //ESP_LOGI(TAG, "Bus idle detected at symbol %d with level1=%d", i, sym->level1);
            break;
        }
    }
    if(bus_idle_level == -1) {
        ESP_LOGI(TAG, "No bus idle detected. try a larger sample size. Assuming first symbol is the bus idle level.");
        bus_idle_level = symbols[0].level0; // Assume the first symbol is the bus idle level
    }
    if(bus_idle_level == 0) {
        ESP_LOGI(TAG, "Bus idle level is 0, Inverting signals");
        // Invert the symbols
        for(int i = 0; i < num_symbols; i++) {
            rmt_symbol_word_t *sym = &symbols[i];
            // Swap level0 and level1
            uint16_t temp_level = sym->level0;
            sym->level0 = sym->level1;
            sym->level1 = temp_level;
        }
    }
    //convert to bit array
    //int16_t bit_length = bit_duration_ns / 1000; // Convert to microseconds

    size_t bit_index = 0;
    for(int i = 0; i < num_symbols; i++) {
        rmt_symbol_word_t *sym = &symbols[i];
        if (sym->duration0 == 0){ //Duration 0 means timeout/idle
            for(int j = 0; j < RMT_TIMEOUT_TO_BITS && bit_index < SIZE_BIT_ARRAY; j++) {
                bit_array[bit_index++] = sym->level0 | BIT_TIMEOUT; // mark data as timeout/idle
            }
        }
        else {
           int num_bits = (sym->duration0*1000 + bit_duration_ns/2) / bit_duration_ns; // Calculate number of bits for duration0
           for (int j = 0; j < num_bits && bit_index < SIZE_BIT_ARRAY; j++) {
                bit_array[bit_index++] = sym->level0;
            }
        }
        if (sym->duration1 == 0){ //Duration 0 means timeout/idle
            for(int j = 0; j < RMT_TIMEOUT_TO_BITS && bit_index < SIZE_BIT_ARRAY; j++) {
                bit_array[bit_index++] = sym->level1 | BIT_TIMEOUT; // Fill with bus idle level
            }
        }
        else {
           int num_bits = (sym->duration1*1000 + bit_duration_ns/2) / bit_duration_ns; // Calculate number of bits for duration1
           for (int j = 0; j < num_bits && bit_index < SIZE_BIT_ARRAY; j++) {
                bit_array[bit_index++] = sym->level1;
            }
        }
    }
    //Create ascii plot of the bit array
    char *str = heap_caps_malloc(bit_index + 1, MALLOC_CAP_SPIRAM);
    if (str == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for str");
    }
    for(int i = 0; i < bit_index; i++) {
        if(bit_array[i] & 0x01) {
        str[i] = '-';
        }
        else {
        str[i] = '_';
        }
    }
    str[bit_index] = '\0';
    //ESP_LOGI(TAG, "Used %d bytes out of %d of bit_index", bit_index, SIZE_BIT_ARRAY);
    ESP_LOGI(TAG, "Ascii plot: %s", str);
    heap_caps_free(str);

    // Find position of the first stop bit in the frame
    const int SHORTEST_FRAME_SIZE = 7;
    const int LONGEST_FRAME_SIZE = 13;
    int16_t max_matches = INT16_MIN;
    uint16_t best_frame_size = SHORTEST_FRAME_SIZE; // Start with the shortest frame size
    
    for(int j = SHORTEST_FRAME_SIZE; j < LONGEST_FRAME_SIZE; j++){ //For every possible frame size
        int16_t stop_matches = 0;
        uint8_t previous_bit = 0x01; // Assume the first bit is 1 (idle)
        //TODO: what if idle level is 0?
        for(int i = 0; i < bit_index -(j-1); i++) {
            if(!(bit_array[i] & 0x01) && (previous_bit & 0x01)) { //possible start bit
                bool invalid_data = false;
                for(int k = 0; k < j-3; k++) { // check if Timeouts-flags exists in data part of the frame
                    if(bit_array[i+k] & BIT_TIMEOUT) { 
                        invalid_data = true;
                        break;
                    }
                }
                if(!invalid_data){
                    i += (j-1); // Skip to stop bit
                    if(bit_array[i] & 0x01 && !invalid_data) { // Check for stop bit
                        stop_matches++;
                    }
                    else {
                        stop_matches-= 2;
                    }
                }
            }
            previous_bit = bit_array[i];
        }
        if(stop_matches > max_matches) {
            max_matches = stop_matches;
            best_frame_size = j;
        }
        //ESP_LOGI(TAG, "%d bit frame: Stop bit match: %d", j, stop_matches);
    }
    //ESP_LOGI(TAG, "First stop bit is present at bit %d, with %d stop bit matches", best_frame_size, max_matches);

    // Mark start and stop bits in the bit array
    uint8_t previous_bit = 0x01; // Assume the first bit is 1 (idle)
    //TODO: what if idle level is 0?
    uint16_t num_frames = 0;
    uint16_t framing_errors = 0;
    for(int i = 0; i < bit_index-(best_frame_size-1); i++) {
        if(!(bit_array[i] & 0x01) && (previous_bit & 0x01)) { //possible start bit
            bool invalid_data = false;
            for(int k = 0; k < best_frame_size-3; k++) { // check for Timeouts-flags exists in data part of the frame
                if(bit_array[i+k] & BIT_TIMEOUT) { 
                    invalid_data = true;
                    break;
                }
            }
            if(!invalid_data){
                i += (best_frame_size-1); // Skip to stop bit
                if(bit_array[i] & 0x01 && !invalid_data) { // Check for stop bit
                    bit_array[i] |= BIT_STOP; // Mark stop bit
                    bit_array[i - (best_frame_size - 1)] |= BIT_START; // Mark start bit
                    num_frames++;
                }
            }
            else {
                framing_errors++;
                ESP_LOGW(TAG, "Framing error at bit %d, invalid data in frame", i);
            }
        }
        previous_bit = bit_array[i];
    }
    ESP_LOGI(TAG, "Found %d frames with start and stop bits", num_frames);

    // check for parity bit
    uint16_t num_even_parity_framing_errors = 0;
    uint16_t num_odd_parity_framing_errors = 0;
    uint16_t num_space_parity_framing_errors = 0;
    parity_type_t parity;
    uint16_t num_data_bits;
    for(int i = 0; i < bit_index; i++)
    {
        if(bit_array[i] & BIT_START) { // Start of a frame
            int parity_bit = 0;
            for(int j = 1; j < best_frame_size-1; j++) { // Check data bits
                if(bit_array[i+j] & 0x01) {
                    parity_bit++;
                }
            }
            if(parity_bit % 2){ //if numbers of 1s is odd
                num_even_parity_framing_errors++;
            }
            else { //if numbers of 1s is even
                num_odd_parity_framing_errors++;
            }
            if(bit_array[i+best_frame_size-2] & 0x01) { // count space parity errors
                num_space_parity_framing_errors++;
            }
        }
    }
    //ESP_LOGI(TAG, "Even parity framing errors: %d", num_even_parity_framing_errors);
    //ESP_LOGI(TAG, "Odd parity framing errors: %d", num_odd_parity_framing_errors);
    //ESP_LOGI(TAG, "Space parity framing errors: %d", num_space_parity_framing_errors);

    if(num_even_parity_framing_errors == 0){
        parity = SERIAL_PARITY_EVEN;
        ESP_LOGI(TAG, "Detected even parity");
        num_data_bits = best_frame_size - 3; // 1 start bit, 1 stop bit, 1 parity bit
    }
    else if(num_odd_parity_framing_errors == 0) {
        parity = SERIAL_PARITY_ODD;
        ESP_LOGI(TAG, "Detected odd parity");
        num_data_bits = best_frame_size - 3; // 1 start bit, 1 stop bit, 1 parity bit
    }
    //else if(num_space_parity_framing_errors == 0) {
    //    parity = PARITY_SPACE;
    //}
    //TODO: detect space parity
    else {
        parity = SERIAL_PARITY_NONE; // No clear parity detected
        ESP_LOGI(TAG, "No clear parity detected");
        num_data_bits = best_frame_size - 2; // 1 start bit, 1 stop bit
    }
    // Data and parity bits in bit_array
    for(int i = 0; i < bit_index; i++)
    {
        if(bit_array[i] & BIT_START) { // Start of a frame
            for(int j = 1; j < num_data_bits+1; j++) { // Check data bits
                bit_array[i+j] |= BIT_DATA; // Mark data bits
            }
            if(parity == SERIAL_PARITY_EVEN || parity == SERIAL_PARITY_ODD || parity == SERIAL_PARITY_SPACE) {
                bit_array[i+num_data_bits+2] |= BIT_PARITY; // Mark parity bit
            }
        }
    }
    ESP_LOGI(TAG, "Number of data bits: %d", num_data_bits);
    // check for multiple stop bits
    uint16_t num_stop_bits = UINT16_MAX;
    for(int i = 0; i < bit_index; i++)
    {
        if(bit_array[i] & BIT_STOP) {
            //cont bits until next start bit
            int j = 1;
            while(j+i <= bit_index && !(bit_array[j+i] & BIT_START)) {
                j++;
            }
            if(j < num_stop_bits) {
                    num_stop_bits = j;
            }
        }
    }
    ESP_LOGI(TAG, "Number of stop bits: %d", num_stop_bits);
    if(num_stop_bits > 2){
        ESP_LOGI(TAG, "Warning: More than 2 stop bits detected, due to bus idle. impossible to dertermine number of stop bits. assuming 1.");
        num_stop_bits = 1; // Assume 1 stop bit if more than 2 stop bits detected
    }
    if(num_stop_bits == 2) {
        for(int i = 0; i < bit_index-1; i++)
        {
            if(bit_array[i] & BIT_STOP) {
                //cont bits until next start bit
                bit_array[i+1] |= BIT_STOP; // Mark next bit as stop bit
                i++; // Skip next bit
            }
        }
    }

    //decode data
    uint16_t byte_index = 0;
    for(int i = 0; i < bit_index; i++) {
        if(bit_array[i] & BIT_START && (byte_index < SIZE_OUTPUT_BUFFER - 1)) { // Start of a frame
            int data_byte = 0;
            for(int j = num_data_bits; j >= 1; j--) { // Check data bits in reverse for MSB first
                if(bit_array[i+j] & BIT_DATA) {
                    
                    data_byte <<= 1; // Shift left
                    if(bit_array[i+j] & 0x01) {
                        data_byte |= 1; // Set LSB if bit is 1
                    }
                }
            }
            decoded_data[byte_index++] = (char)data_byte; // Store decoded byte
        }
    }
    decoded_data[byte_index] = '\0';

    // check if data is ascii or binary
    bool is_ascii = true;
    for(int i = 0; i < num_frames; i++) {
        if(decoded_data[i] > 126) { // ASCII printable range
            is_ascii = false;
            break;
        }
    }

    if(is_ascii) {
        ESP_LOGI(TAG, "Decoded data is ASCII");
        ESP_LOGI(TAG, "Decoded data: \"%s\"", decoded_data);
    } else {
        ESP_LOGI(TAG, "Decoded data is binary");
        // Print decoded data in hex format
        char hex_str[num_frames * 3 + 1]; // 2 chars per byte + space + null terminator
        int pos = 0;
        for(int i = 0; i < num_frames; i++) {
            pos += sprintf(&hex_str[pos], "%02X ", (unsigned char)decoded_data[i]);
        }
        hex_str[pos] = '\0';
        ESP_LOGI(TAG, "Decoded data (hex): %s", hex_str);
    }

    serial_analyzer_output->num_symbols = num_symbols;
    serial_analyzer_output->bit_duration_ns = bit_duration_ns;
    serial_analyzer_output->measured_baud_rate = measured_baud_rate;
    serial_analyzer_output->nearest_baud_rate = get_nearest_baud(measured_baud_rate);
    serial_analyzer_output->num_frames = num_frames;
    serial_analyzer_output->num_framing_errors = framing_errors;
    serial_analyzer_output->num_data_bits = num_data_bits;
    serial_analyzer_output->num_stop_bits = num_stop_bits;
    serial_analyzer_output->parity_type = parity;
    serial_analyzer_output->is_ASCII = is_ascii;
    serial_analyzer_output->bit_array = bit_array; // Store the bit array for further processing
    serial_analyzer_output->size_bit_array = bit_index;
    serial_analyzer_output->decoded_data = decoded_data;
    serial_analyzer_output->size_decoded_data = num_frames;

    lvgl_lock();
    lv_async_call(serial_analyzer_show_results_screen_async, serial_analyzer_output);
    lvgl_unlock();


    //Todo: invert the symbols if the bus idle level is 0?
    //todo: detect if the data is binary or ascii





}

uint32_t get_nearest_baud(int measured_baud)
{
    const int common_baudrates[] = {
        1200, 2400, 4800, 9600,
        14400, 19200, 28800, 38400, 57600, 76800,
        115200, 230400, 250000, 460800,
        500000, 576000, 921600, 1000000
    };

    const size_t num_rates = sizeof(common_baudrates) / sizeof(common_baudrates[0]);
    int closest = common_baudrates[0];
    int min_diff = INT_MAX;

    for (size_t i = 0; i < num_rates; i++) {
        int diff = abs(measured_baud - common_baudrates[i]);
        if (diff < min_diff) {
            min_diff = diff;
            closest = common_baudrates[i];
        }
    }

    return closest;
}

void SerialAnalyzer_RMT_start(void)
{
    xTaskCreatePinnedToCore(rmt_analyzer_task, "RMT Analyzer", RMT_ANALYZER_TASK_STACK_SIZE, NULL, RMT_ANALYZER_TASK_PRIORITY, NULL, 1); // Run on core 1
}