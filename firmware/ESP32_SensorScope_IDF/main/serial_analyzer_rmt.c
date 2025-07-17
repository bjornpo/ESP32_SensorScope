#include "serial_analyzer_rmt.h"
#include "settings.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "driver/rmt.h"
#include "driver/rmt_rx.h"
#include "driver/gpio.h"

#define RMT_ANALYZER_TASK_STACK_SIZE (6*1024)
#define RMT_ANALYZER_TASK_PRIORITY 20

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
    //ESP_LOGI(TAG, "Capture done with %d symbols", edata->num_symbols);
    gpio_set_level(GPIO_NUM_48, 1);
    BaseType_t task_awoken = pdFALSE;
    bool is_last = false;

    if(g_settings.serial_analyzer.stop_when_bus_idle) {
        is_last = true;
    }
    else if(edata->num_symbols > (g_settings.serial_analyzer.sample_size - 10 - (g_settings.serial_analyzer.sample_size*20)/100)) {
        is_last = true;
    }

    rmt_event_t evt = {
        .num_symbols = edata->num_symbols,
        .is_last = is_last
    };

    xQueueSendFromISR(rmt_event_queue, &evt, &task_awoken);

    return task_awoken == pdTRUE;
}

void rmt_analyzer_task(void *arg)
{
    ESP_LOGI(TAG, "RMT Analyzer task started. Sample size: %d", (int)g_settings.serial_analyzer.sample_size);

    rmt_event_queue = xQueueCreate(4, sizeof(rmt_event_t));

    // Configure RMT RX channel
    rmt_rx_channel_config_t rx_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = RMT_RX_GPIO,
        .mem_block_symbols = g_settings.serial_analyzer.sample_size,   // max symbols/flanks to capture
        .resolution_hz = 1000 * 1000, // 1 µs resolution
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_config, &rx_channel));

    // Register callback
    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = on_rmt_rx_done,
    };
    ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(rx_channel, &cbs, NULL));

    // Enable channel
    ESP_ERROR_CHECK(rmt_enable(rx_channel));

    rmt_receive_config_t receive_config = {
        .signal_range_min_ns = 3000,     // bit-length at 23200 baud
        .signal_range_max_ns = 10000000, // the longest duration for NEC signal is 9000 µs, 12000000 ns > 9000 µs, the receive does not stop early
        .flags.en_partial_rx = false,     // Enable partial reception if buffer is not enough
    };

    // Allocate symbol buffer
    //rmt_symbol_word_t *rx_symbols = (rmt_symbol_word_t *)heap_caps_malloc(
    //    g_settings.serial_analyzer.sample_size * sizeof(rmt_symbol_word_t),
    //    MALLOC_CAP_INTERNAL
    //);
    //if (!rx_symbols) {
    //    ESP_LOGE(TAG, "Failed to allocate rx_symbols buffer in internal RAM");
    //    vTaskDelete(NULL);
    //    return;
    //}
    rmt_symbol_word_t rx_symbols[g_settings.serial_analyzer.sample_size];

    // Start receiving
    ESP_LOGI(TAG, "Waiting for signal...");
    ESP_ERROR_CHECK(rmt_receive(rx_channel, rx_symbols, sizeof(rx_symbols), &receive_config));
    //ESP_ERROR_CHECK(rmt_receive(rx_channel, rx_symbols, sizeof(rx_symbols), NULL));

    size_t true_num_symbols = 0; //rmt_rx_done_event_data_t->num_symbols get reset after each receive
    while (1)
    {
        rmt_event_t evt;
        if (xQueueReceive(rmt_event_queue, &evt, portMAX_DELAY)) {
            gpio_set_level(GPIO_NUM_48, 0); // Turn off GPIO after receiving
            true_num_symbols += evt.num_symbols;
            if(!evt.is_last) {
                // If not the last part, we can continue receiving
                size_t used_bytes = true_num_symbols * sizeof(rmt_symbol_word_t);
                ESP_ERROR_CHECK(rmt_receive(rx_channel, rx_symbols+true_num_symbols, sizeof(rx_symbols)-used_bytes, &receive_config));
                ESP_LOGI(TAG, "Received %d symbols, waiting for more...", evt.num_symbols);
                continue;
            }
            else
            {
                ESP_LOGI(TAG, "Received %d symbols", true_num_symbols);
                // Process only the valid symbols in rx_symbols[]
                for (int i = 0; i < true_num_symbols; i++) 
                {
                    rmt_symbol_word_t *sym = &rx_symbols[i];
                    ESP_LOGI(TAG, "Symbol %d: level0=%d, duration0=%d, level1=%d, duration1=%d",
                         i, sym->level0, sym->duration0, sym->level1, sym->duration1);
                }
            }
        }
        else {
            ESP_LOGW(TAG, "No event received");
        }
        //TODO: wait for the reception to complete

        //TODO: process the received symbols

        // For testing. remove later
        ESP_ERROR_CHECK(rmt_receive(rx_channel, rx_symbols, sizeof(rx_symbols), &receive_config));
    }
}

void SerialAnalyzer_RMT_start(void)
{
    xTaskCreatePinnedToCore(rmt_analyzer_task, "RMT Analyzer", RMT_ANALYZER_TASK_STACK_SIZE, NULL, RMT_ANALYZER_TASK_PRIORITY, NULL, 1); // Run on core 1

}