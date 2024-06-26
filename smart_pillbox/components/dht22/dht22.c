#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_check.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_rx.h"
#include "dht22.h"

static const char *TAG = "DHT22";

#define DHT22_RMT_RESOLUTION_HZ               (1 * 1000 * 1000) // RMT channel default resolution for 1-wire bus, 1MHz, 1tick = 1us
#define DHT22_RMT_DEFAULT_TRANS_QUEUE_SIZE    4

// the memory size of each RMT channel, in words (4 bytes)
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
#define DHT22_RMT_DEFAULT_MEM_BLOCK_SYMBOLS   64
#else
#define DHT22_RMT_DEFAULT_MEM_BLOCK_SYMBOLS   48
#endif

#define DHT22_RMT_RX_MEM_BLOCK_SIZE           DHT22_RMT_DEFAULT_MEM_BLOCK_SYMBOLS

#define DHT22_RMT_SYMBOLS_PER_TRANS          42 // 1 start signal, 1 response signal, 40 data bits

#define DHT22_START_PULSE_LOW_US             10000 // 0
#define DHT22_START_PULSE_HIGH_US            20    // 20us
#define DHT22_RESPONSE_PULSE_LOW_US          80    // 80us
#define DHT22_RESPONSE_PULSE_HIGH_US         80    // 80us
#define DHT22_BIT_ZERO_LOW_US                50    // 50us
#define DHT22_BIT_ZERO_HIGH_US               26    // 26us
#define DHT22_BIT_ONE_LOW_US                 50    // 50us
#define DHT22_BIT_ONE_HIGH_US                70    // 70us

#define DHT22_DECODE_SYMBOL_MARGIN_US        15 // error margin of 15us

typedef struct dht22_t {
    rmt_channel_handle_t tx_channel;      /*!< rmt tx channel handler */
    rmt_channel_handle_t rx_channel;      /*!< rmt rx channel handler */
    rmt_encoder_handle_t tx_copy_encoder; /*!< used to encode the start signal */
    rmt_symbol_word_t *rx_symbols_buf;    /*!< hold rmt raw symbols */
    size_t rx_symbols_buf_size;           /*!< size of rx_symbols_buf */
    QueueHandle_t receive_queue;          /*!< A queue used to send the raw data to the task from the ISR */
} dht22_t;

const static rmt_symbol_word_t dht22_start_pulse_symbol = {
    .level0 = 0,
    .duration0 = DHT22_START_PULSE_LOW_US,
    .level1 = 1,
    .duration1 = DHT22_START_PULSE_HIGH_US,
};

const static rmt_transmit_config_t dht22_rmt_tx_config = {
    .loop_count = 0,     // no transfer loop
    .flags.eot_level = 1 // should released the bus in the end
};

const static rmt_receive_config_t dht22_rmt_rx_config = {
    .signal_range_min_ns = 1000,
    .signal_range_max_ns = 20 * 1000 * 1000,
};

static inline bool dht22_check_in_range(uint32_t signal_duration, uint32_t expect_duration)
{
    return (signal_duration < (expect_duration + DHT22_DECODE_SYMBOL_MARGIN_US)) &&
           (signal_duration > (expect_duration - DHT22_DECODE_SYMBOL_MARGIN_US));
}

static bool dht22_parse_logic0(rmt_symbol_word_t *rmt_nec_symbols)
{
    return dht22_check_in_range(rmt_nec_symbols->duration0, DHT22_BIT_ZERO_LOW_US) &&
           dht22_check_in_range(rmt_nec_symbols->duration1, DHT22_BIT_ZERO_HIGH_US);
}

static bool dht22_parse_logic1(rmt_symbol_word_t *rmt_nec_symbols)
{
    return dht22_check_in_range(rmt_nec_symbols->duration0, DHT22_BIT_ONE_LOW_US) &&
           dht22_check_in_range(rmt_nec_symbols->duration1, DHT22_BIT_ONE_HIGH_US);
}

static bool dht22_rmt_rx_done_callback(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data)
{
    BaseType_t task_woken = pdFALSE;
    dht22_t *dht22 = (dht22_t *)user_data;

    xQueueSendFromISR(dht22->receive_queue, edata, &task_woken);

    return task_woken;
}

static esp_err_t dht22_rmt_decode_data(rmt_symbol_word_t *rmt_symbols, size_t symbol_num, float *temp, float *humi)
{
    bool integrity_check_pass = (symbol_num >= DHT22_RMT_SYMBOLS_PER_TRANS) &&
                                dht22_check_in_range(rmt_symbols[1].duration0, DHT22_RESPONSE_PULSE_LOW_US) &&
                                dht22_check_in_range(rmt_symbols[1].duration1, DHT22_RESPONSE_PULSE_HIGH_US);
    ESP_RETURN_ON_FALSE(integrity_check_pass, ESP_ERR_INVALID_RESPONSE, TAG, "invalid response signal");

    // parse the RMT symbols into temperature and humidity data, 16bit+16bit, MSB first
    uint16_t humi_int = 0;
    for (int i = 2; i < 18; i++) {
        humi_int <<= 1;
        if (dht22_parse_logic1(&rmt_symbols[i])) {
            humi_int |= 0x01;
        } else if (!dht22_parse_logic0(&rmt_symbols[i])) {
            ESP_RETURN_ON_FALSE(false, ESP_FAIL, TAG, "unknown data bit");
        }
    }
    uint16_t temp_int = 0;
    for (int i = 18; i < 34; i++) {
        temp_int <<= 1;
        if (dht22_parse_logic1(&rmt_symbols[i])) {
            temp_int |= 0x01;
        } else if (!dht22_parse_logic0(&rmt_symbols[i])) {
            ESP_RETURN_ON_FALSE(false, ESP_FAIL, TAG, "unknown data bit");
        }
    }
    // parse the checksum
    uint8_t checksum = 0;
    for (int i = 34; i < 42; i++) {
        checksum <<= 1;
        if (dht22_parse_logic1(&rmt_symbols[i])) {
            checksum |= 0x01;
        } else if (!dht22_parse_logic0(&rmt_symbols[i])) {
            ESP_RETURN_ON_FALSE(false, ESP_FAIL, TAG, "unknown data bit");
        }
    }

    // verify the checksum
    uint32_t checksum_calc = (temp_int >> 8) + (temp_int & 0xff) + (humi_int >> 8) + (humi_int & 0xff);
    integrity_check_pass = (checksum_calc & 0xff) == checksum;
    ESP_RETURN_ON_FALSE(integrity_check_pass, ESP_ERR_INVALID_CRC, TAG, "invalid checksum");

    // The highest bit of temp_int is the sign bit
    *temp = (temp_int & 0x7FFF) / 10.0f;
    if (temp_int & 0x8000) {
        *temp *= -1;
    }
    *humi = humi_int / 10.0f;

    return ESP_OK;
}

static esp_err_t dht22_destroy(dht22_t *dht22)
{
    if (dht22->tx_copy_encoder) {
        rmt_del_encoder(dht22->tx_copy_encoder);
    }
    if (dht22->rx_channel) {
        rmt_disable(dht22->rx_channel);
        rmt_del_channel(dht22->rx_channel);
    }
    if (dht22->tx_channel) {
        rmt_disable(dht22->tx_channel);
        rmt_del_channel(dht22->tx_channel);
    }
    if (dht22->receive_queue) {
        vQueueDelete(dht22->receive_queue);
    }
    if (dht22->rx_symbols_buf) {
        free(dht22->rx_symbols_buf);
    }
    free(dht22);
    return ESP_OK;
}

esp_err_t dht22_new_sensor(dht22_handle_t *ret_sensor, const gpio_num_t dht22_gpio) {
    esp_err_t ret = ESP_OK;
    dht22_t *dht22 = NULL;
    ESP_RETURN_ON_FALSE(dht22_gpio && ret_sensor, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    dht22 = calloc(1, sizeof(dht22_t));
    ESP_RETURN_ON_FALSE(dht22, ESP_ERR_NO_MEM, TAG, "no mem for dht22_t");

    // create rmt copy encoder to transmit start pulse
    rmt_copy_encoder_config_t copy_encoder_config = {};
    ESP_GOTO_ON_ERROR(rmt_new_copy_encoder(&copy_encoder_config, &dht22->tx_copy_encoder),
                      err, TAG, "create copy encoder failed");

    // Note: must create rmt rx channel before tx channel
    rmt_rx_channel_config_t rx_channel_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = DHT22_RMT_RESOLUTION_HZ,
        .gpio_num = dht22_gpio,
        .mem_block_symbols = DHT22_RMT_RX_MEM_BLOCK_SIZE,
    };
    ESP_GOTO_ON_ERROR(rmt_new_rx_channel(&rx_channel_cfg, &dht22->rx_channel),
                      err, TAG, "create rmt rx channel failed");

    // create rmt tx channel
    rmt_tx_channel_config_t tx_channel_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = DHT22_RMT_RESOLUTION_HZ,
        .gpio_num = dht22_gpio,
        .mem_block_symbols = DHT22_RMT_DEFAULT_MEM_BLOCK_SYMBOLS,
        .trans_queue_depth = DHT22_RMT_DEFAULT_TRANS_QUEUE_SIZE,
        .flags.io_loop_back = true, // make tx channel coexist with rx channel on the same gpio pin
        .flags.io_od_mode = true,   // enable open-drain mode for 1-wire bus
    };
    ESP_GOTO_ON_ERROR(rmt_new_tx_channel(&tx_channel_cfg, &dht22->tx_channel),
                      err, TAG, "create rmt tx channel failed");

    // allocate rmt rx symbol buffer, one RMT symbol represents one bit
    dht22->rx_symbols_buf_size = DHT22_RMT_SYMBOLS_PER_TRANS * 2 * sizeof(rmt_symbol_word_t);
    dht22->rx_symbols_buf = malloc(dht22->rx_symbols_buf_size);
    ESP_GOTO_ON_FALSE(dht22->rx_symbols_buf, ESP_ERR_NO_MEM, err, TAG, "no mem to store received RMT symbols");

    dht22->receive_queue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
    ESP_GOTO_ON_FALSE(dht22->receive_queue, ESP_ERR_NO_MEM, err, TAG, "receive queue creation failed");

    // register rmt rx done callback
    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = dht22_rmt_rx_done_callback
    };
    ESP_GOTO_ON_ERROR(rmt_rx_register_event_callbacks(dht22->rx_channel, &cbs, dht22),
                      err, TAG, "enable rmt rx channel failed");

    // enable rmt channels
    ESP_GOTO_ON_ERROR(rmt_enable(dht22->rx_channel), err, TAG, "enable rmt rx channel failed");
    ESP_GOTO_ON_ERROR(rmt_enable(dht22->tx_channel), err, TAG, "enable rmt tx channel failed");

    // release the bus by sending a special RMT symbol
    static const rmt_symbol_word_t release_symbol = {
        .level0 = 1,
        .duration0 = 1,
        .level1 = 1,
        .duration1 = 0,
    };
    ESP_GOTO_ON_ERROR(rmt_transmit(dht22->tx_channel, dht22->tx_copy_encoder, &release_symbol,
                                   sizeof(release_symbol), &dht22_rmt_tx_config), err, TAG, "release bus failed");

    *ret_sensor = dht22;
    return ret;

err:
    if (dht22) {
        dht22_destroy(dht22);
    }

    return ret;
}

esp_err_t dht22_del_sensor(dht22_handle_t sensor)
{
    ESP_RETURN_ON_FALSE(sensor, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    return dht22_destroy(sensor);
}

esp_err_t dht22_read_data(dht22_handle_t sensor, float *temp, float *humi)
{
    ESP_RETURN_ON_FALSE(sensor && temp && humi, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    ESP_RETURN_ON_ERROR(rmt_receive(sensor->rx_channel, sensor->rx_symbols_buf, sensor->rx_symbols_buf_size, &dht22_rmt_rx_config), TAG, "dht22 start receive failed");
    ESP_RETURN_ON_ERROR(rmt_transmit(sensor->tx_channel, sensor->tx_copy_encoder, &dht22_start_pulse_symbol, sizeof(dht22_start_pulse_symbol), &dht22_rmt_tx_config), TAG, "dht22 start transmit failed");

    // wait the transmission finishes and decode data
    rmt_rx_done_event_data_t rmt_rx_evt_data;
    ESP_RETURN_ON_FALSE(xQueueReceive(sensor->receive_queue, &rmt_rx_evt_data, pdMS_TO_TICKS(1000)) == pdPASS, ESP_ERR_TIMEOUT,
                        TAG, "dht22 receive timeout");
    ESP_RETURN_ON_ERROR(dht22_rmt_decode_data(rmt_rx_evt_data.received_symbols, rmt_rx_evt_data.num_symbols, temp, humi),
                        TAG, "dht22 decode data failed");

    return ESP_OK;
}
