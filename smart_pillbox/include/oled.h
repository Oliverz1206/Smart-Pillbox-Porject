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

/**
 * @brief Create LCD screen
 */
void lcd_init(void);

/**
 * @brief turn off the display
 */
void turn_off_disp(void);
/**
 * @brief turn on the display
 */
void turn_on_disp(void);

/**
 * @brief Clear the whole display
 */
void display_clear(void);

/**
 * @brief Display the text on screen
 * @param[in] label label handler
 * @param[in] text  context for display
 */
void display_text(lv_obj_t * label, const char * text);

/**
 * @brief Special interface 1 for the screen
 */
void display_interface1(lv_obj_t * label, int wifi_status, int mqtt_status, float temp, float humid, bool warning);

/**
 * @brief Special interface 2 for the screen
 */
void display_interface2(lv_obj_t * label, uint32_t * pill_num);

/**
 * @brief Create a label for the screen
 * @return
 *   - lv_obj_t: a label handler
 */
lv_obj_t * create_label(void);

#endif /* OLED_H */