#pragma once
#include <PubSubClient.h>

// The shared MQTT connection - defined here so mqtt_publish.cpp can
// publish on it directly.
extern PubSubClient mqttClient;

// True while Node-RED has switched the system into custom/manual mode
// (via the MQTT_MODE_TOPIC "true"/"false" messages). While true, every
// automatic rule in loop() is suspended so actuators only change in
// response to direct MQTT commands.
bool CustomModeActive();
void SetCustomMode(bool enabled);

void SetupMqtt();
void ConnectToWiFi();
void ConnectToMqtt();

// WiFi/MQTT reconnect housekeeping + mqttClient.loop(). Call every loop().
void MaintainMqttConnection();
