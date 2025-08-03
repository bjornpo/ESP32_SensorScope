#pragma once

typedef struct {
    int32_t *ch1_buf; // Pointer to the buffer holding the chart data
    int32_t *ch1_chart_buf; // Pointer to the buffer holding the chart data
    int32_t *ch2_buf; // Pointer to the buffer holding the chart data
    int32_t *ch2_chart_buf; // Pointer to the buffer holding the chart data
    uint32_t ch_buf_length; // Length of the buffer in samples
} osc_task_ctx_t;


void oscilloscope_sampling_start(int32_t *ch1_buf, int32_t *ch1_chart_buf, int32_t *ch2_buf, int32_t *ch2_chart_buf, uint32_t ch_buf_length);
void oscilloscope_lock(void);
void oscilloscope_unlock(void);