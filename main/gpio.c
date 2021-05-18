#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "gpio.h"
#include "mqtt.h"
#include "esp_sleep.h"


void set_up_gpio() {
    gpio_pad_select_gpio(BUTTON);
    gpio_set_direction(BUTTON, GPIO_MODE_INPUT);
    gpio_pulldown_en(BUTTON);
    gpio_pullup_dis(BUTTON);

    esp_sleep_enable_ext0_wakeup(BUTTON, 0);
}
