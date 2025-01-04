#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include "FT6236.h"

// #I2C pins for touch controller
#define I2C_SDA 8
#define I2C_SCL 9
#define RST_N_PIN 2
#define INT_N_PIN 42

FT6236 ts = FT6236(); // touch controller


/*screen resolution and rotation*/
#define TFT_HOR_RES   320
#define TFT_VER_RES   480
#define TFT_ROTATION  LV_DISPLAY_ROTATION_270


/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 20 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

/* LVGL calls it when a rendered image needs to copied to the display*/
void my_disp_flush( lv_display_t *disp, const lv_area_t *area, uint8_t * px_map)
{
    Serial.println( "my_disp_flush" );
    lv_display_flush_ready(disp);
}

/*Read the touchpad*/
void my_touchpad_read( lv_indev_t * indev, lv_indev_data_t * data )
{
    if (ts.touched())
    {
        // Retrieve point
        int32_t x, y;
        TS_Point p = ts.getPoint();
        data->state = LV_INDEV_STATE_PRESSED;
        x = constrain((TFT_HOR_RES-p.x),0,TFT_HOR_RES-1);
        y = constrain((TFT_VER_RES-p.y),0,TFT_VER_RES-1);
        data->point.x = x;
        data->point.y = y;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/*use Arduinos millis() as tick source*/
static uint32_t my_tick(void)
{
    return millis();
}

void setup()
{   
    // Initialize the touch controller pins
    pinMode(INT_N_PIN, INPUT);
    pinMode(RST_N_PIN, OUTPUT);
    digitalWrite(RST_N_PIN, HIGH);

    Wire.setClock(400000); // Set I2C speed to 400 kHz (Fast Mode)
    if (!ts.begin(40, I2C_SDA, I2C_SCL)) //40 in this case represents the sensitivity. Try higer or lower for better response. 
    {
        Serial.println("Unable to start the capacitive touchscreen.");
    }

    Serial.begin( 115200 );
    Serial0.begin( 115200 );

    lv_init();
    Serial.println( "lv_init done" );
    //draw_buf = heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    /*Set a tick source so that LVGL will know how much time elapsed. */
    lv_tick_set_cb(my_tick);

    lv_display_t * disp;
    /*TFT_eSPI can be enabled lv_conf.h to initialize the display in a simple way*/
    disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, sizeof(draw_buf));
    lv_display_set_rotation(disp, TFT_ROTATION);

    /*Initialize the input device driver*/
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
    lv_indev_set_read_cb(indev, my_touchpad_read);

    //Simple example
    lv_obj_t *label = lv_label_create( lv_screen_active() );
    lv_label_set_text( label, "Hello World" );
    lv_obj_align( label, LV_ALIGN_CENTER, 0, 0 );

    Serial.println( "Setup done" );
}

void loop()
{
    lv_timer_handler(); /* let the GUI do its work */
    //delay(5);
}