#include <string.h>
#include "oscilloscope.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_continuous.h"
#include "settings.h"
#include "driver/gpio.h"

//testing
#include "esp_timer.h"

#define CH1_PIN ADC_CHANNEL_3
#define CH2_PIN ADC_CHANNEL_4

#define OSCILLOSCOPE_TASK_STACK_SIZE (6*1024)//6*1024)
#define OSCILLOSCOPE_TASK_PRIORITY 20 //High priority for fast restart of RMT_receive

//TODO; adjust conv_frame based on sampler rate and number of channels so that one convframe is triggered for each LCD-frame update (20ms)
#define MAX_STORE_BUF_SIZE 1024
#define CONV_FRAME_SIZE 256

static TaskHandle_t s_task_handle; // Handle for the oscilloscope task

static const char *TAG = "Scope";

static _lock_t oscilloscope_buffer_lock;
void oscilloscope_lock(void) {
    _lock_acquire(&oscilloscope_buffer_lock);
}

void oscilloscope_unlock(void) {
    _lock_release(&oscilloscope_buffer_lock);
}

static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    //Notify that ADC continuous driver has done enough number of conversions
    vTaskNotifyGiveFromISR(s_task_handle, &mustYield);

    return (mustYield == pdTRUE);
}

static bool IRAM_ATTR s_adc_overflow_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    //ESP_LOGW(TAG, "ADC overflow occurred");
    gpio_set_level(GPIO_NUM_48, 0); // Turn off CH1 LED to indicate overflow
    return false; // No high priority task wakeup needed
}


void oscilloscope_task(void *pvParameters)
{
    osc_task_ctx_t *ctx = (osc_task_ctx_t *)pvParameters;
    if (ctx == NULL) {
        ESP_LOGE(TAG, "oscilloscope_task: Context is NULL");
        free(ctx); // Free the context itself
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Oscilloscope task starting. Sample rate: %"PRIu32" Hz, buffer: %"PRIi32" elements", g_settings.oscilloscope.sample_rate_hz, ctx->ch_buf_length);

    // Initialize the oscilloscope hardware and start sampling
    adc_continuous_handle_t handle = NULL;
    adc_continuous_handle_cfg_t adc_config = {
    .max_store_buf_size = MAX_STORE_BUF_SIZE,
    .conv_frame_size = CONV_FRAME_SIZE,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

    //SOC_ADC_SAMPLE_FREQ_THRES_HIGH          83333
    //SOC_ADC_SAMPLE_FREQ_THRES_LOW           611
    adc_digi_pattern_config_t adc_pattern[] = {
        {
            .atten = ADC_ATTEN_DB_0,
            .channel = CH1_PIN, // CH1
            .unit = ADC_UNIT_1,
            .bit_width = ADC_BITWIDTH_12,
        },
        // {
        //     .atten = ADC_ATTEN_DB_0,
        //     .channel = CH1_PIN, // CH2
        //     .unit = ADC_UNIT_1,
        //     .bit_width = ADC_BITWIDTH_12,
        // },
    };

    adc_continuous_config_t dig_cfg = {
    .pattern_num = 1, // one or two channels
    .adc_pattern = adc_pattern,
    .sample_freq_hz = 611,
    .conv_mode = ADC_CONV_SINGLE_UNIT_1,
    .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    };

    // Register the callback for conversion done
    adc_continuous_evt_cbs_t adc_cb = {
        .on_conv_done = s_conv_done_cb,
        .on_pool_ovf = s_adc_overflow_cb, // No overflow callback for now
    };

    // Get the current task handle to notify when conversion is done
    s_task_handle = xTaskGetCurrentTaskHandle();

    ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &adc_cb, NULL));
    ESP_ERROR_CHECK(adc_continuous_start(handle));


    // This is a placeholder for the actual implementation
    gpio_set_level(GPIO_NUM_48, 1);
    uint8_t conv_frame_buffer[CONV_FRAME_SIZE] = {0}; // Buffer to hold the conversion frame data
    memset(conv_frame_buffer, 0xCC, CONV_FRAME_SIZE); // Initialize the buffer to 0xCC for testing
    uint32_t ch1_index = 0;
    uint32_t ch2_index = 0;
    esp_err_t ret;
    while (1) {
        // Perform oscilloscope sampling here
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait for the conversion done notification
        while(1) {
            uint32_t conv_frame_size = 0;
            ret =adc_continuous_read(handle, conv_frame_buffer, CONV_FRAME_SIZE, &conv_frame_size, 0);
            //adc_digi_output_data_t i;
            if (ret == ESP_OK)
            {
                uint32_t lowest = UINT32_MAX;
                uint32_t highest = 0;
                for(int i = 0; i < conv_frame_size; i+= sizeof(int32_t)) {
                    adc_digi_output_data_t *p = (adc_digi_output_data_t*)&conv_frame_buffer[i];
                    uint32_t data = p->type2.data & 0xFFF; // Extract the data from the buffer
                    uint32_t channel = p->type2.channel; // Extract the channel from the buffer
                    if(channel == CH1_PIN) {
                        ctx->ch1_buf[ch1_index] = data; // Store the data in the buffer
                        ch1_index = (ch1_index + 1) % ctx->ch_buf_length; // Wrap around the index
                        //break; //Test single value
                        ESP_LOGI(TAG, "%d, %"PRId32"", i, data);
                        if(data<lowest){lowest = data;}
                        if(data>highest){highest = data;}
                        //highest = highest>data ? data : lowest;
                    } else if(channel == CH2_PIN) {
                        ctx->ch2_buf[ch2_index] = data; // Store the data in the buffer
                        ch2_index = (ch2_index + 1) % ctx->ch_buf_length; // Wrap around the index
                    }
                }
                ESP_LOGI(TAG, "low:%"PRIu32", high:%"PRIu32", delta:%"PRIu32"", lowest, highest, highest-lowest);
                //vTaskDelay(pdMS_TO_TICKS(300));
                //} else if (ret == ESP_ERR_TIMEOUT) {
            } else {
                //We try to read `EXAMPLE_READ_LEN` until API returns timeout, which means there's no available data
                break;
            }
        }
        // Update the chart buffers using memcpy
        size_t first_chunk = ctx->ch_buf_length - ch1_index;
        oscilloscope_lock(); // Lock the buffer to prevent concurrent access
        memcpy(ctx->ch1_chart_buf, &ctx->ch1_buf[ch1_index], first_chunk * sizeof(int32_t));
        memcpy(&ctx->ch1_chart_buf[first_chunk], ctx->ch1_buf, ch1_index * sizeof(int32_t));
        oscilloscope_unlock();
        ESP_LOGI(TAG, "ch1_index = %"PRIu32" test_value = %"PRIi32"", ch1_index, ctx->ch1_chart_buf[ctx->ch_buf_length-1]);
        
        //vTaskDelay(pdMS_TO_TICKS(300)); // Simulate processing time, adjust as needed
    }
    free(ctx); // Free the context itself
    vTaskDelete(NULL);
}


void oscilloscope_sampling_start(int32_t *ch1_buf, int32_t *ch1_chart_buf, int32_t *ch2_buf, int32_t *ch2_chart_buf, uint32_t ch_buf_length)
{
    osc_task_ctx_t * ctx = heap_caps_malloc(sizeof(osc_task_ctx_t), MALLOC_CAP_DEFAULT);
    if (ctx == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for osc_task_ctx_t");
        return;
    }
    ctx->ch1_buf = ch1_buf;
    ctx->ch1_chart_buf = ch1_chart_buf;
    ctx->ch2_buf = ch2_buf;
    ctx->ch2_chart_buf = ch2_chart_buf;
    ctx->ch_buf_length = ch_buf_length;
    xTaskCreatePinnedToCore(oscilloscope_task, "oscilloscope task", OSCILLOSCOPE_TASK_STACK_SIZE, ctx, OSCILLOSCOPE_TASK_PRIORITY, NULL, 1); // Run on core 1
}