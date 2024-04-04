#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_timer.h"

#define SENSOR_ID           "S1"                        // Snesor ID
#define BROKER_URL          "mqtt://broker.emqx.io"     // Broker
#define TOPIC               "ECE590_Final_Project_G2"   // Topic

#define DIRECT_MODIFY       true                        // Force the board to use initial wifi ssid and password
#define WIFI_SSID_INITIAL   "YUNFAN6190"                // Initial SSID
#define WIFI_PWD_INITIAL    "1415926zyf"                // Initial Password

#define MAXIMUM_RETRY       5                           // Maximum retry for wifi connection
#define SSID_SIZE           33                          // ssid maximum length
#define PWD_SIZE            65                          // password maximum length

#define TEMP_LIMIT          40                          // 40 degree limit
#define HUMID_LIMIT         60                          // 60 %

#define SAMPLE_TIME         60                          // 60 seconds per sample
#define NOTIFI_TIME         60                          // 60 seconds for continous notification
#define SCREEN_OFF_TIME     120                         // 120 seconds for screen to truen off

#define DHT22_PIN           GPIO_NUM_7                  // DHT22 1-wire pin
#define BUZZ_PIN            GPIO_NUM_48                 // Buzzer pin
#define BUTTON_PIN          GPIO_NUM_47                 // button pin
#define LEDarray ((gpio_num_t[]) {GPIO_NUM_6, GPIO_NUM_5, GPIO_NUM_4, GPIO_NUM_2, GPIO_NUM_1}) // LED pins

enum STATUS {IDLE = 0, CONNECTING, SMARTCONFIG, NOTIFICATION}; // system status

/**
 * @brief Set LEDs on and off
 * @param[in] status array of led status need to be set
 */
void set_LED(uint32_t * status);

/**
 * @brief Set all LED off
 * @param[in] status array of led status and the function would set them all to zeros at the same time
 */
void clear_LED(uint32_t * status);

/**
 * @brief Start the buzzer alarm
 */
void start_buzz_alarm(void);

/**
 * @brief Stop the buzzer alarm
 */
void stop_buzz_alarm(void);

#endif /* UTIL_H */