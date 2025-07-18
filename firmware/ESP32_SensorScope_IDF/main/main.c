// main.c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "display_driver.h"
#include "lvgl.h"
#include "driver/gpio.h"

#include "lv_examples.h"
#include "lv_demos.h"
#include "appGUI.h"
#include "settings.h"


static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "System start");

    // Initialize display hardware (SPI, backlight, LCD)
    display_driver_init_display();

    // load NVS settings
    nvs_init();

    // GPIO initialization  
    gpio_config_t test_gpio_config = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = 1ULL << 21
    };
    ESP_ERROR_CHECK(gpio_config(&test_gpio_config));

      gpio_config_t test2_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = 1ULL << 48
    };
    ESP_ERROR_CHECK(gpio_config(&test2_gpio_config));


    // Display LVGL content (e.g., demo widget)
    lvgl_lock();

    //lv_example_arc_1();  // Your chosen demo or UI setup
    //lv_demo_widgets();
    //lv_demo_benchmark();
    //lv_demo_scroll();
    //lvgl_example_menu_3();
    create_menu_screen();
    lvgl_unlock();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));  // Main loop idle
    }
}
