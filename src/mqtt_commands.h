#pragma once
#include <Arduino.h>

// PubSubClient message callback - handles inbound Node-RED commands
// (servo angle, LED/buzzer overrides, custom-mode toggle). Registered
// with mqttClient.setCallback() in mqtt_client.cpp.
void CallbackMqtt(char *topic, byte *payload, unsigned int length);
