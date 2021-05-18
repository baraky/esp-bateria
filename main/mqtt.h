#ifndef MQTT_H
#define MQTT_H

char room[16];

void mqtt_start();

void mqtt_send_message(char * topic, char * message);

#endif
