#pragma once
#include <Arduino.h>

// Node Red Connection
extern const char *WIFI_SSID;
extern const char *WIFI_PASSWORD;

extern const char *MQTT_SERVER;
extern const uint16_t MQTT_PORT;
extern const char *MQTT_COMMAND_TOPIC;
extern const char *MQTT_STATUS_TOPIC;
extern const char *MQTT_TEMPERATURE_TOPIC;
extern const char *MQTT_HUMIDITY_TOPIC;
extern const char *MQTT_SERVO_SET_TOPIC;
extern const char *MQTT_SERVO_STATE_TOPIC;
extern const char *MQTT_PIR_TOPIC;
extern const char *MQTT_LDR_TOPIC;
extern const char *MQTT_PIR_LED_TOPIC;
extern const char *MQTT_HEAT_LED_TOPIC;
extern const char *MQTT_COOL_LED_TOPIC;
extern const char *MQTT_BRIGHT_LED_TOPIC;
extern const char *MQTT_BUZZER_TOPIC;
extern const char *MQTT_PIRLED_STATE_TOPIC;
extern const char *MQTT_HEATLED_STATE_TOPIC;
extern const char *MQTT_COOLLED_STATE_TOPIC;
extern const char *MQTT_BRIGHTLED_STATE_TOPIC;
extern const char *MQTT_BUZZER_STATE_TOPIC;
extern const char *MQTT_MODE_TOPIC;
