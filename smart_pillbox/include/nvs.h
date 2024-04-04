#ifndef NVS_H
#define NVS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"

/**
 * @brief Initialize NVS
 */
void nvs_init(void);

/**
 * @brief Read wifi ssid and password from the NVS
 * @param[out] ssid ssid string
 * @param[out] pwd password string
 */
void nvs_read_wifi_info(char* ssid, char* pwd);

/**
 * @brief Read wifi ssid and password from the NVS
 * @param[in] ssid ssid string to write
 * @param[in] pwd password string to write
 */
void nvs_write_wifi_info(const char* ssid, const char* pwd);

/**
 * @brief Check the ssid and password in the NVS
 * @return
 *   - ESP_OK: NVS have wifi info
 *   - ESP_FAIL: NVS do not have wifi info
 */
esp_err_t nvs_check_wifi_info(void);

#endif /* NVS_H */