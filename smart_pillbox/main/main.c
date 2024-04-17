#include <stdio.h>
#include "nvs.h"
#include "util.h"
#include "wifi.h"
#include "oled.h"
#include "dht22.h"
#include "mqtt.h"
#include "cJSON.h"
#include "esp_timer.h"

static const char *TAG = "Main";

char message[200] = {0};
float temperature = 0, humidity = 0;

static size_t length;
static lv_obj_t *label;
static dht22_handle_t sensor;
static esp_timer_handle_t timer;
static esp_netif_ip_info_t *wifi_info;
static uint32_t led_status[5] = {0, 0, 0, 0, 0};
static uint32_t pill_num[5] = {0, 0, 0, 0, 0};

static int dht22_counter = 0, screen_off_counter = 0, notification_counter = 0;
static int system_status = IDLE, last_system_status = IDLE, wifi_status = WIFI_DISCONNECTED, mqtt_status = MQTT_DISCONNECTED, message_status = false;
static bool refresh_required, sample_required, smartconfig_required, warning_flag = false;

static void IRAM_ATTR button_isr_handler() {
    switch (system_status) {
        case IDLE:
            screen_off_counter = SCREEN_OFF_TIME;
            break;
        case SMARTCONFIG:
            smartconfig_required = true;
            break;
        case NOTIFICATION:
            system_status = IDLE;
            stop_buzz_alarm();
            clear_LED(led_status);
            break;
        default: return;
    }
}

static void interrupt_task() {
    // Display refresh
    refresh_required = true;

    // Sensor refresh
    sample_required = (dht22_counter == 0) ? true : sample_required;
    dht22_counter = (dht22_counter == SAMPLE_TIME) ? 0 : dht22_counter + 1;

    switch (system_status) {
        case IDLE:
            if (screen_off_counter == 0) {
                turn_off_disp();
                refresh_required = false;
            } 
            else {
                turn_on_disp();
                screen_off_counter--;
            }
            break;
        case NOTIFICATION:
            turn_on_disp();
            notification_counter++;
            if (notification_counter == NOTIFI_TIME) {
                notification_counter = 0;
                system_status = IDLE;
                stop_buzz_alarm();
                clear_LED(led_status);
            }
            break;
        default:
            turn_on_disp();
    }
}

void app_main(void) {
    /* Hardware part startup. Reboot if failed to start. */
    // OLED
    lcd_init();
    label = create_label();
    display_text(label, "System Startup");

    // DHT22 Sensor
    ESP_ERROR_CHECK(dht22_new_sensor(&sensor, DHT22_PIN));

    // NVS startup
    nvs_init();

    // LED
    for (int i = 0; i < 5; i++) {
        gpio_config_t led_conf = {
            .pin_bit_mask = (1ULL << LEDarray[i]), 
            .mode = GPIO_MODE_OUTPUT
        };
        gpio_config(&led_conf);
    }
    set_LED(led_status);

    // BUZZ
    gpio_config_t buzz_conf = {
        .pin_bit_mask = (1ULL << BUZZ_PIN), 
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = true
    };
    gpio_config(&buzz_conf);
    gpio_set_level(BUZZ_PIN, false);
    gpio_set_drive_capability(BUZZ_PIN, GPIO_DRIVE_CAP_0);

    // BUTTON
    gpio_config_t button_io = {
        .pin_bit_mask = (1ULL << BUTTON_PIN), 
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_POSEDGE,
    };
    gpio_config(&button_io);
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, (void*)BUTTON_PIN);

    // Display Timer
    esp_timer_create_args_t timer_args = {
        .callback = &interrupt_task,
        .name = "timer",
    };
    esp_timer_create(&timer_args, &timer);
    
    /* Service part startup. Plot information for the reason. */
    // WIFI Connection startup
    wifi_connect();

    // MQTT Service startup
    mqtt_start();

    turn_on_disp();
    esp_timer_start_periodic(timer, 1e6);
    ESP_LOGI(TAG, "Initialzation complete. Start service...");
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    while (1) {
        wifi_status = get_wifi_status();
        mqtt_status = get_mqtt_status();
        message_status = get_message(message, &length);
        
        // Update system status FSM1
        last_system_status = system_status;
        if (wifi_status == WIFI_DISCONNECTED) {
            system_status = SMARTCONFIG;
        }
        else if (wifi_status == WIFI_CONNECTING || 
                (wifi_status == WIFI_CONNECTED  && (mqtt_status == MQTT_CONNECTING || mqtt_status == MQTT_DISCONNECTED))) {
            system_status = CONNECTING;
        }
        else if (wifi_status == WIFI_CONNECTED && mqtt_status == MQTT_CONNECTED && message_status == true) {
            system_status = NOTIFICATION;
        }
        else if (wifi_status == WIFI_CONNECTED && mqtt_status == MQTT_CONNECTED && system_status != NOTIFICATION) {
            system_status = IDLE;
        }
        else {
            system_status = system_status;
        }

        switch (system_status) {
            case IDLE:
                if (last_system_status != system_status) {
                    wifi_info = get_wifi_info();
                    screen_off_counter = SCREEN_OFF_TIME;
                }
                break;

            case SMARTCONFIG:
                if (smartconfig_required) {
                    display_interface1(label, WIFI_SMARTCONFIG, mqtt_status, temperature, humidity, false);
                    smartconfig();
                    smartconfig_required = false;
                }
                break;

            case NOTIFICATION:
                if (message_status) {
                    ESP_LOGI(TAG, "Alarm Length: %u, Alarm Contents: %s", length, message);
                    memset(led_status, 0, sizeof(led_status));
                    memset(pill_num, 0, sizeof(pill_num));
                    cJSON * alarm = cJSON_ParseWithLength(message, length);
                    if (alarm != NULL) {
                        int idx = 0;
                        for (idx = 0; idx < 5; idx++) {
                            char str[12] = {0};
                            sprintf(str, "Slot%d", idx + 1);
                            cJSON * slot = cJSON_GetObjectItemCaseSensitive(alarm, str);
                            if (!cJSON_IsBool(slot)) break;

                            sprintf(str, "PillNum%d", idx + 1);
                            cJSON * num = cJSON_GetObjectItemCaseSensitive(alarm, str);
                            if (!cJSON_IsNumber(num)) break;

                            led_status[idx] = slot->valueint;
                            pill_num[idx] = num->valueint;
                        }
                        if (idx == 5) { 
                            set_LED(led_status);
                            start_buzz_alarm();
                        }
                        else {
                            system_status = IDLE;
                        }
                    }
                    else ESP_LOGE(TAG, "Unknown Json.");
                }
                break;
        }

        if (sample_required) {
            dht22_read_data(sensor, &temperature, &humidity);
            ESP_LOGI(TAG, "Temp: %.1f, Humid: %.1f", temperature, humidity);        
            warning_flag = (temperature > TEMP_LIMIT || humidity > HUMID_LIMIT) ? true : false;
            sample_required = false;
        }

        if (refresh_required) {
            switch (system_status) {
                case IDLE:
                case CONNECTING:
                case SMARTCONFIG:
                    display_interface1(label, wifi_status, mqtt_status, temperature, humidity, warning_flag);
                    break;
                case NOTIFICATION:
                    display_interface2(label, pill_num);
                    break;
            }
            refresh_required = false;
        }
    }
}
