#ifndef OLED_H
#define OLED_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"
#include "driver/i2c.h"
#include "hal/lcd_types.h"
#include "lvgl.h"
#include "wifi.h"
#include "mqtt.h"
#include "util.h"

#define I2C_HOST                    0
#define LCD_PIXEL_CLOCK_HZ          (400 * 1000)
#define PIN_NUM_SDA                 17
#define PIN_NUM_SCL                 18
#define PIN_NUM_RST                 21
#define I2C_HW_ADDR                 0x3C
#define LCD_CMD_BITS                8
#define LCD_PARAM_BITS              8

#define LCD_H_RES                   128
#define LCD_V_RES                   64

void lcd_init(void);
lv_obj_t * create_label(void);

void turn_off_disp(void);
void turn_on_disp(void);

void display_clear(void);
void display_text(lv_obj_t * label, const char * text);
void display_interface1(lv_obj_t * label, int wifi_status, int mqtt_status, float temp, float humid, bool warning);
void display_interface2(lv_obj_t * label, uint32_t * pill_num);

#endif