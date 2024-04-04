#include "nvs.h"

static const char * TAG = "NVS";

static size_t length;
static nvs_handle nvs_handler;

void nvs_init(void) {
    /* NVS Memory Check */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    /* Rrite Initial Wifi Info into NVS if not exist */
    if (nvs_check_wifi_info() == ESP_FAIL) {
        nvs_write_wifi_info(WIFI_SSID_INITIAL, WIFI_PWD_INITIAL);
        ESP_LOGI(TAG, "No NVS wifi info found, write initial wifi ssid and password.");
    }

    ESP_LOGI(TAG, "Initialize Successfully");
}

esp_err_t nvs_check_wifi_info(void) {
    if (nvs_open("WiFi_cfg", NVS_READONLY, &nvs_handler) != ESP_OK || 
        nvs_get_str(nvs_handler, "wifi_ssid", NULL, &length) != ESP_OK ||
        nvs_get_str(nvs_handler, "wifi_passwd", NULL, &length) != ESP_OK) {
        return ESP_FAIL;
    }
    else {
        return ESP_OK;
    }
}

void nvs_read_wifi_info(char * ssid, char * pwd)
{
    /* Open NVS */
    ESP_ERROR_CHECK(nvs_open("WiFi_cfg", NVS_READONLY, &nvs_handler));

    /* Get ssid from NVS */
    length = SSID_SIZE;
    ESP_ERROR_CHECK(nvs_get_str(nvs_handler, "wifi_ssid", ssid, &length));

    /* Get password from NVS */
    length = PWD_SIZE;
    ESP_ERROR_CHECK(nvs_get_str(nvs_handler, "wifi_passwd", pwd, &length));

    /* Commit Change */
    ESP_ERROR_CHECK(nvs_commit(nvs_handler));
    nvs_close(nvs_handler);

    ESP_LOGI(TAG, "Read SSID: %s, PWD: %s", ssid, pwd);
}

void nvs_write_wifi_info(const char* ssid, const char* pwd)
{
    /* Open NVS */
    ESP_ERROR_CHECK(nvs_open("WiFi_cfg", NVS_READWRITE, &nvs_handler));

    /* Set ssid to NVS */
    ESP_ERROR_CHECK(nvs_set_str(nvs_handler,"wifi_ssid", ssid));

    /* Set password to NVS */
    ESP_ERROR_CHECK(nvs_set_str(nvs_handler,"wifi_passwd", pwd));

    /* Commit Change */
    ESP_ERROR_CHECK(nvs_commit(nvs_handler)); 
    nvs_close(nvs_handler); 

    ESP_LOGI(TAG, "Write SSID: %s, PWD: %s", ssid, pwd);          
}
