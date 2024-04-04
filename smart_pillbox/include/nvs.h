#ifndef NVS_H
#define NVS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"

#define SSID_SIZE          33
#define PWD_SIZE           65

#define WIFI_SSID_INITIAL  "YUNFAN6190"
#define WIFI_PWD_INITIAL   "1415926zyfA"

void nvs_init(void);
void nvs_read_wifi_info(char* ssid, char* pwd);
void nvs_write_wifi_info(const char* ssid, const char* pwd);
esp_err_t nvs_check_wifi_info(void);

#endif /* NVS_H */