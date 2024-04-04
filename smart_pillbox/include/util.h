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

#define SENSOR_ID           "S1"
#define BROKER_URL          "mqtt://broker.emqx.io"
#define TOPIC               "ECE590_Final_Project_G2"

#define DIRECT_MODIFY       true
#define WIFI_SSID_INITIAL   "YUNFAN6190"
#define WIFI_PWD_INITIAL    "1415926zyfA"

#define TEMP_LIMIT          40
#define HUMID_LIMIT         60

#define SAMPLE_TIME         60
#define NOTIFI_TIME         60
#define SCREEN_OFF_TIME     120 

#define DHT22_PIN           7
#define BUZZ_PIN            48
#define BUTTON_PIN          47
#define LEDarray ((gpio_num_t[]){GPIO_NUM_6, GPIO_NUM_5, GPIO_NUM_4, GPIO_NUM_2, GPIO_NUM_1})

enum STATUS {IDLE = 0, CONNECTING, SMARTCONFIG, NOTIFICATION};

void set_LED(uint32_t * status);
void clear_LED(uint32_t * status);
void start_buzz_alarm(void);
void stop_buzz_alarm(void);

#endif