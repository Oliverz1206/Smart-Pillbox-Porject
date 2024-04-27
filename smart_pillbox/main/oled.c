#include "oled.h"

static const char *TAG = "OLED Setup";

static lv_disp_t * disp = NULL;
static lv_obj_t * screen = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_panel_io_handle_t io_handle = NULL;

void lcd_init(void) {
    // Initial I2C BUS
    ESP_LOGI(TAG, "Initialize I2C bus");
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = PIN_NUM_SDA,
        .scl_io_num = PIN_NUM_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = LCD_PIXEL_CLOCK_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_HOST, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_HOST, I2C_MODE_MASTER, 0, 0, 0));
    vTaskDelay(200 / portTICK_PERIOD_MS);

    // Install the IO for OLED display module (Gneral Config)
    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = I2C_HW_ADDR,
        .control_phase_bytes = 1,               // According to SSD1306 datasheet
        .dc_bit_offset = 6,                     // According to SSD1306 datasheet
        .lcd_cmd_bits = LCD_CMD_BITS,           // According to SSD1306 datasheet
        .lcd_param_bits = LCD_PARAM_BITS,       // According to SSD1306 datasheet
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_HOST, &io_config, &io_handle));
    vTaskDelay(200 / portTICK_PERIOD_MS);


    // Intall SSD1306 diplay module (Specific Config)
    ESP_LOGI(TAG, "Install SSD1306 panel driver");
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = PIN_NUM_RST,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, false));
    vTaskDelay(200 / portTICK_PERIOD_MS);

    // Install LVGL Library
    ESP_LOGI(TAG, "Initialize LVGL");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LCD_H_RES * LCD_V_RES,
        .double_buffer = true,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = true,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
        }
    };
    disp = lvgl_port_add_disp(&disp_cfg);
    lv_disp_set_rotation(disp, LV_DISP_ROT_180);
    screen = lv_disp_get_scr_act(disp);
    ESP_LOGI(TAG, "LCD Init Finish");
    vTaskDelay(200 / portTICK_PERIOD_MS);
}

void display_clear(void) {
    lvgl_port_lock(0);
    lv_obj_clean(screen);
    lvgl_port_unlock();
}

lv_obj_t * create_label(void) {
    lvgl_port_lock(0);
    lv_obj_t * label = lv_label_create(screen);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
    lvgl_port_unlock();
    return label;
}

void display_text(lv_obj_t * label, const char * text) {
    lvgl_port_lock(0);
    lv_label_set_text(label, text);
    lvgl_port_unlock();
}

void turn_off_disp(void) {
    esp_lcd_panel_disp_on_off(panel_handle, false);
}

void turn_on_disp(void) {
    esp_lcd_panel_disp_on_off(panel_handle, true);
}

void display_interface1(lv_obj_t * label, int wifi_status, int mqtt_status, float temp, float humid, bool warning) {
    lvgl_port_lock(0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    switch (wifi_status) {
        case WIFI_DISCONNECTED:
            lv_label_set_text(label, "Wifi " LV_SYMBOL_CLOSE "\nPress button\nto smartconfig.");
            break;
        case WIFI_CONNECTING:
            lv_label_set_text(label, "Wifi " LV_SYMBOL_REFRESH "\nConnecting. . .");
            break;
        case WIFI_SMARTCONFIG:
            lv_label_set_text(label, "Wifi " LV_SYMBOL_WIFI "\nSmartconfig. . .");
            break;
        case WIFI_CONNECTED:
            char str[80];
            switch(mqtt_status) {
                case MQTT_CONNECTED:
                    sprintf(str, "Wifi " LV_SYMBOL_OK "\nT: %.1f°C H: %.1f%%\nSensorID: %s\n", temp, humid, SENSOR_ID);
                    break;
                default:
                    sprintf(str, "MQTT " LV_SYMBOL_REFRESH "\nT: %.1f°C H: %.1f%%\nSensorID: %s\n", temp, humid, SENSOR_ID);
            }
            if (warning) {
                strcat(str, LV_SYMBOL_WARNING " Storage Cond.");
            }
            else {
                strcat(str, LV_SYMBOL_OK " Storage Cond.");
            }
            lv_label_set_text(label, str);
            break;
    }
    lvgl_port_unlock();
}

void display_interface2(lv_obj_t * label, uint32_t * pill_num) {
    char str[140];
    lvgl_port_lock(0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, -5);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    sprintf(str, "================\n||  %02ld  ||  %02ld  ||  %02ld  ||\n-----------------------\n||     %02ld     ||     %02ld      ||\n================", 
            pill_num[4], pill_num[1], pill_num[3], pill_num[0], pill_num[2]);
    lv_label_set_text(label, str);
    lvgl_port_unlock();
}
