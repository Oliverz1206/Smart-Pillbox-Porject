# Smart Pillbox Edge Program

## Description
The ESP32 for smart pillbox involves handling connections like WiFi and MQTT, managing non-volatile storage (NVS), and interfacing with an OLED display. It provides functionalities like displaying system statuses, interacting with sensors, and alert systems through utility functions.

## File Structure
- `main.c`: Contains the main application logic, initializing system components and handling the main loop where system tasks are managed.
- `mqtt.c`, `mqtt.h`: Handles MQTT protocol operations, enabling the device to communicate with MQTT brokers for sending and receiving messages.
- `nvs.c`, `nvs.h`: Manages non-volatile storage to save and retrieve configurations or states, ensuring data persistence across reboots.
- `oled.c`, `oled.h`: Controls the OLED display, providing functions to initialize the display, show messages, and update screen content dynamically.
- `util.c`, `util.h`: Includes utility functions such as LED controls and buzzer alarms to provide feedback or alerts based on system status or external events.
- `wifi.c`, `wifi.h`: Manages WiFi connectivity, providing functions to connect to available networks, handle reconnections, and maintain network status.
- `dht22.c`, `dht22.h`: Implements the interface and logic for reading temperature and humidity data from a DHT22 sensor using RMT interface.

## Setup and Configuration
### Hardware Requirements:
- ESP32 module
- OLED display
- DHT22 Sensor
- 1 Button, 5 LEDs

### Software Dependencies:
- ESP-IDF SDK
- Libraries for MQTT, WiFi, NVS, and OLED handling (details on specific libraries to install)
