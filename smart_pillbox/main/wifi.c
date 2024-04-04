#include "wifi.h"

static const char *TAG = "WIFI";

static int wifi_status;
static char wifi_ssid[33] = {0};
static char wifi_password[65] = {0};
static int wifi_status = WIFI_DISCONNECTED;

static wifi_config_t wifi_config;
static esp_netif_ip_info_t *info;
static EventGroupHandle_t wifi_event_group;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    /* Record for retry times */
    static int retry_num = 0;

    /* Try to connet wifi using wifi info stored in NVS */
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        wifi_status = WIFI_CONNECTING;
        esp_wifi_connect();
    } 

    /* Retry connect to wifi if not successfully connected */
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (wifi_status != WIFI_SMARTCONFIG) {
            /* Retry MAXIMUM_RETRY times */
            wifi_status = (retry_num < MAXIMUM_RETRY) ? WIFI_CONNECTING : WIFI_DISCONNECTED;
            if (retry_num > MAXIMUM_RETRY) xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);

            esp_wifi_connect();
            retry_num++;
            ESP_LOGI(TAG, "Retry #%d: Retry to connect to the AP...", retry_num);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    } 

    /* If connected to wifi show the IP */
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        retry_num = 0;
        wifi_status = WIFI_CONNECTED;
        
        /* Print out the IP */
        *info = event->ip_info;
        ESP_LOGI(TAG, "Got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    }

    /* Smart Config Event for First Scanning */
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "Scan done");
    } 
    else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        ESP_LOGI(TAG, "Found channel");
    } 
    else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        ESP_LOGI(TAG, "Got SSID and password");

        /* Got the data from the smart device */
        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;

        /* Print out SSID and PWD */
        ESP_LOGI(TAG, "SSID:%s", evt->ssid);
        ESP_LOGI(TAG, "PASSWORD:%s", evt->password);

        /* Copy the data to wifi_config */
        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));

        /* Save SSID and PWD into NVS */
        memcpy(wifi_ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_password, evt->password, sizeof(wifi_config.sta.password));
        nvs_write_wifi_info(wifi_ssid, wifi_password);
        ESP_LOGI(TAG, "Smartconfig save wifi_cfg to NVS");

        /* Using New SSID and Password to connect to the WiFi */
        retry_num = 0;
        wifi_status = WIFI_CONNECTING;
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        esp_wifi_connect();
    } 
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        ESP_LOGI(TAG, "Smartconfig done");
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void smartconfig(void) {

    esp_wifi_disconnect();
    wifi_status = WIFI_SMARTCONFIG;
    xEventGroupClearBits(wifi_event_group, WIFI_FAIL_BIT);
    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    
    ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
    ESP_LOGI(TAG, "Smartconfig start .......");
    
    EventBits_t SCBits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
    if (SCBits & WIFI_CONNECTED_BIT) {
       ESP_LOGI(TAG, "Connected to AP SSID: %s password: %s", wifi_config.sta.ssid, wifi_config.sta.password);
        wifi_status = WIFI_CONNECTED;
    }
    else {
        ESP_LOGI(TAG, "Still Failed to connect to the AP");
        wifi_status = WIFI_DISCONNECTED;
    }
    esp_smartconfig_stop();
}

void wifi_connect(void) {
    // Create a info place
    info = malloc(sizeof(*info));

    // Init wifi event loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    // Init wifi config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_LOGI(TAG, "Finish WIFI init");

    /* Registing WIFI Event, IP Adress Event and Smart config Event to the handler */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    /* Read the SSID and Pwd from NVS and copy them to wifi_config */
    nvs_read_wifi_info(wifi_ssid, wifi_password);
    bzero(&wifi_config, sizeof(wifi_config_t));  /* Initialize wifi_config struct */
    memcpy(wifi_config.sta.ssid, wifi_ssid, sizeof(wifi_config.sta.ssid));
    memcpy(wifi_config.sta.password, wifi_password, sizeof(wifi_config.sta.password));

    // Start wifi service
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Finish WIFI STA Setting");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    wifi_event_group = xEventGroupCreate();
}

int get_wifi_status(void) {
    return wifi_status;
}

esp_netif_ip_info_t * get_wifi_info(void) {
    return info;
}
