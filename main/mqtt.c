#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "mqtt.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "gpio.h"
#include "cJSON.h"

#include "esp_sleep.h"
#include "driver/rtc_io.h"

#define TAG "MQTT"

RTC_DATA_ATTR int tag = 0;

extern xSemaphoreHandle mqtt_semaphore;
esp_mqtt_client_handle_t client;
int state = 0;

void write_nvs() {
    nvs_handle partition;
    esp_err_t res_nvs = nvs_open("armazenamento", NVS_READWRITE, &partition);

    if(res_nvs == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE("NVS", "Namespace: armazenamento, não encontrado");
    }
    esp_err_t res = nvs_set_str(partition, "local", room);

    if(res != ESP_OK)
    {
        ESP_LOGE("NVS", "Não foi possível escrever no NVS (%s)", esp_err_to_name(res));
    }
    nvs_commit(partition);
    nvs_close(partition);
}

int32_t check_nvs() {
    nvs_handle partition;
    esp_err_t res_nvs = nvs_open("armazenamento", NVS_READONLY, &partition);
    size_t buf_len = sizeof(room);
    
    if(res_nvs == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE("NVS", "Namespace: armazenamento, não encontrado");
        return -1;
    }
    else
    {
        esp_err_t res = nvs_get_str(partition, "local", room, &buf_len);

        switch (res)
        {
        case ESP_OK:
            printf("Valor armazenado: %s\n", room);
            break;
        case ESP_ERR_NOT_FOUND:
            ESP_LOGE("NVS", "Valor não encontrado");
            return -1;
        default:
            ESP_LOGE("NVS", "Erro ao acessar o NVS (%s)", esp_err_to_name(res));
            return -1;
            break;
        }

        nvs_close(partition);
    }
    return 0;
}

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    uint8_t derived_mac_addr[6] = {0};
    char init_topic[50];
    cJSON *json = NULL;
    cJSON *content = NULL;
    int length;
    
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

            if (check_nvs() == -1) {
                ESP_ERROR_CHECK(esp_read_mac(derived_mac_addr, ESP_MAC_WIFI_STA));
                sprintf(init_topic, "fse2020/180033646/dispositivos/%d%d%d%d%d%d", derived_mac_addr[0],
                                                                                derived_mac_addr[1],
                                                                                derived_mac_addr[2],
                                                                                derived_mac_addr[3],
                                                                                derived_mac_addr[4],
                                                                                derived_mac_addr[5]); 
                esp_mqtt_client_publish(client, init_topic, "{\"init\": 0}", 0, 1, 0);
                esp_mqtt_client_subscribe(client, init_topic, 0);
            } else {
                if (tag > 0) {
                sprintf(init_topic, "fse2020/180033646/%s/estado", room);
                mqtt_send_message(init_topic, "{\"input\": 1}");
                }
                tag++;
                esp_deep_sleep_start();
            }
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            json = cJSON_ParseWithLength(event->data, event->data_len);
            content = cJSON_GetObjectItemCaseSensitive(json, "room");
            length = strlen(content->valuestring);
            memcpy(room, content->valuestring, length);
            room[length] = '\0';
            write_nvs();
            cJSON_Delete(json);
            tag++;
            esp_deep_sleep_start();
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

void mqtt_start()
{
    esp_mqtt_client_config_t mqtt_config = {
        .uri = "mqtt://test.mosquitto.org",
    };
    client = esp_mqtt_client_init(&mqtt_config);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

void mqtt_send_message(char * topic, char * message)
{
    int message_id = esp_mqtt_client_publish(client, topic, message, 0, 1, 0);
    ESP_LOGI(TAG, "Mensagem enviada, ID: %d", message_id);
}
