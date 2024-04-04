#include "util.h"

esp_timer_handle_t timer;

static const int beep[4] = {20, 25, 35, 40};
static int counter = 0;

static void buzzer_task() {
    counter++;
    if (counter == beep[0]) {
        gpio_set_level(BUZZ_PIN, 1);
    }
    else if (counter == beep[1]) {
        gpio_set_level(BUZZ_PIN, 0);
    }
    else if (counter == beep[2]) {
        gpio_set_level(BUZZ_PIN, 1);
    }
    else if (counter == beep[3]) {
        gpio_set_level(BUZZ_PIN, 0);
        counter = 0;
    }
}

void set_LED(uint32_t * status) {
    for (int i=0; i < 5; i++) {
        gpio_set_level(LEDarray[i], status[i]);
    }
}

void clear_LED(uint32_t * status) {
    for (int i=0; i < 5; i++) {
        gpio_set_level(LEDarray[i], 0);
        status[i] = 0;
    }
}

void start_buzz_alarm(void) {
    esp_timer_create_args_t timer_args = {
        .callback = &buzzer_task,
        .name = "buzzer_timer",
    };
    esp_timer_create(&timer_args, &timer);
    esp_timer_start_periodic(timer, 25e3);
}

void stop_buzz_alarm(void) {
    esp_timer_stop(timer);
    gpio_set_level(BUZZ_PIN, 0);
    counter = 0;
}
