#ifndef MQTT_H
#define MQTT_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "mqtt_client.h"
#include "util.h"

enum MQTT_STATUS {MQTT_CONNECTED, MQTT_CONNECTING, MQTT_DISCONNECTED};

/**
 * @brief Start MQTT Service
 */
void mqtt_start(void);

/**
 * @brief Get MQTT message
 * @param[out] str message buffer
 * @param[out] len length of the message
 * @return
 *   - true: currently have MQTT message 
 *   - false: currently not received MQTT message
 */
bool get_message(char * str, size_t * len);

/**
 * @brief Get MQTT service status
 * @return
 *   - MQTT_Status: MQTT_CONNECTED    mqtt service started 
 *                  MQTT_CONNECTING   mqtt service is subscribing
 *                  MQTT_DISCONNECTED mqtt service do not working
 */
int get_mqtt_status(void);

#endif /* MQTT_H */