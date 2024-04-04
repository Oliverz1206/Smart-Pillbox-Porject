#ifndef WIFI_H
#define WIFI_H

#include <stdio.h>
#include <stdio.h>
#include <string.h>

#include "nvs.h"
#include "lvgl.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#define WIFI_AUTH_TYPE  WIFI_AUTH_WPA2_PSK
#define MAXIMUM_RETRY   5

#define SSID_SIZE          33
#define PWD_SIZE           65

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define WIFI_DONE_BIT      BIT2

enum WIFI_STATUS {WIFI_DISCONNECTED, WIFI_CONNECTING, WIFI_CONNECTED, WIFI_SMARTCONFIG};

int get_wifi_status(void);
void smartconfig(void);
void wifi_connect(void);
esp_netif_ip_info_t * get_wifi_info(void);

#endif