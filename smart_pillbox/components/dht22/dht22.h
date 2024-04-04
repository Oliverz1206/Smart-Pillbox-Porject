/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "driver/rmt_types.h"

/**
 * @brief Type of DHT22 sensor handle
 */
typedef struct dht22_t *dht22_handle_t;

/**
 * @brief Create DHT22 sensor handle with RMT backend
 *
 * @note One sensor utilizes a pair of RMT TX and RX channels
 *
 * @param[in] dht22_gpio DHT22 specific configuration
 * @param[out] ret_sensor Returned sensor handle
 * @return
 *      - ESP_OK: create sensor handle successfully
 *      - ESP_ERR_INVALID_ARG: create sensor handle failed because of invalid argument
 *      - ESP_ERR_NO_MEM: create sensor handle failed because of out of memory
 *      - ESP_FAIL: create sensor handle failed because some other error
 */
esp_err_t dht22_new_sensor(dht22_handle_t *ret_sensor, const gpio_num_t dht22_gpio);

/**
 * @brief Delete the sensor handle
 *
 * @param[in] sensor Sensor handle returned from `dht22_new_sensor`
 * @return
 *      - ESP_OK: delete sensor handle successfully
 *      - ESP_ERR_INVALID_ARG: delete sensor handle failed because of invalid argument
 *      - ESP_FAIL: delete sensor handle failed because some other error
 */
esp_err_t dht22_del_sensor(dht22_handle_t sensor);

/**
 * @brief Read temperature and humidity from the sensor
 *
 * @param[in] sensor Sensor handle returned from `dht22_new_sensor`
 * @param[out] temp Temperature in degree Celsius
 * @param[out] humi Humidity in percentage
 * @return
 *      - ESP_OK: read temperature and humidity successfully
 *      - ESP_ERR_INVALID_ARG: read temperature and humidity failed because of invalid argument
 *      - ESP_FAIL: read temperature and humidity failed because some other error
 */
esp_err_t dht22_read_data(dht22_handle_t sensor, float *temp, float *humi);