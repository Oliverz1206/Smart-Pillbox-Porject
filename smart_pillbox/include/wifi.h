#ifndef WIFI_H
#define WIFI_H

#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include "nvs.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "util.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

enum WIFI_STATUS {WIFI_DISCONNECTED, WIFI_CONNECTING, WIFI_CONNECTED, WIFI_SMARTCONFIG};

/**
 * @brief Start smartconfig application
 */
void smartconfig(void);

/**
 * @brief Start Wi-Fi service
 */
void wifi_connect(void);

/**
 * @brief Get wifi service status
 * @return 
 *    - WIFI_DISCONNECTED wifi disconnected
 *    - WIFI_CONNECTING   wifi trying to connect
 *    - WIFI_CONNECTED    wifi is connected
 *    - WIFI_SMARTCONFIG  wifi serivice is for smartconfig
 */
int get_wifi_status(void);

/**
 * @brief Get wifi ip infomation
 * @return 
 *    - esp_netif_ip_info_t* wifi information handler
 */
esp_netif_ip_info_t * get_wifi_info(void);

#endif /* WIFI_H */