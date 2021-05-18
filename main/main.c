#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "wifi.h"
#include "mqtt.h"
#include "gpio.h"

xSemaphoreHandle wifi_semaphore;

void mqtt_conection(void * params)
{
  while(true)
  {
    if(xSemaphoreTake(wifi_semaphore, portMAX_DELAY))
    {
      mqtt_start();
    }
  }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    wifi_semaphore = xSemaphoreCreateBinary();
    set_up_gpio();
    wifi_start();

    xTaskCreate(&mqtt_conection,  "Conexão ao MQTT", 4096, NULL, 1, NULL);
}
