#pragma once
#include <WiFi.h>
#include <PubSubClient.h>

extern PubSubClient mqttClient;

//MQTT Communication
String CreateMqttClientId();
void SetupMqtt();
void ConnectToWiFi();
void ConnectToMqtt();
