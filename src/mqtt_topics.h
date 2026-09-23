#pragma once
// Node-RED / MQTT broker connection details and topic names.
//
// `constexpr` (not just `const`) matters here: this header is included
// by several .cpp files, and a plain `const char *X = "..."` only makes
// the pointee const, not the pointer itself, so it gets external linkage
// and the linker sees a duplicate definition in every extra .cpp that
// includes it. `constexpr` forces internal linkage, so each translation
// unit safely gets its own copy.

constexpr const char *WIFI_SSID = "Wokwi-GUEST";
constexpr const char *WIFI_PASSWORD = "";

constexpr const char *MQTT_SERVER = "broker.hivemq.com";
constexpr uint16_t MQTT_PORT = 1883;
constexpr const char *MQTT_COMMAND_TOPIC = "week6/HealthyHome/status/esp32";
constexpr const char *MQTT_STATUS_TOPIC = "week6/HealthyHome/status/esp32";
constexpr const char *MQTT_TEMPERATURE_TOPIC = "week6/HealthyHome/sensor/temperature";
constexpr const char *MQTT_HUMIDITY_TOPIC = "week6/HealthyHome/sensor/humidity";
constexpr const char *MQTT_SERVO_SET_TOPIC = "week6/HealthyHome/actuator/servo/set";
constexpr const char *MQTT_SERVO_STATE_TOPIC = "week6/HealthyHome/actuator/servo/state";
constexpr const char *MQTT_PIR_TOPIC = "week6/HealthyHome/sensor/pir";
constexpr const char *MQTT_LDR_TOPIC = "week6/HealthyHome/sensor/ldr";
constexpr const char *MQTT_PIR_LED_TOPIC = "week6/HealthyHome/actuator/pir_led/set";
constexpr const char *MQTT_HEAT_LED_TOPIC = "week6/HealthyHome/actuator/heat_led/set";
constexpr const char *MQTT_COOL_LED_TOPIC = "week6/HealthyHome/actuator/cool_led/set";
constexpr const char *MQTT_BRIGHT_LED_TOPIC = "week6/HealthyHome/actuator/bright_led/set";
constexpr const char *MQTT_BUZZER_TOPIC = "week6/HealthyHome/actuator/buzzer/set";
constexpr const char *MQTT_PIRLED_STATE_TOPIC = "week6/HealthyHome/actuator/pir_led/state";
constexpr const char *MQTT_HEATLED_STATE_TOPIC = "week6/HealthyHome/actuator/heat_led/state";
constexpr const char *MQTT_COOLLED_STATE_TOPIC = "week6/HealthyHome/actuator/cool_led/state";
constexpr const char *MQTT_BRIGHTLED_STATE_TOPIC = "week6/HealthyHome/actuator/bright_led/state";
constexpr const char *MQTT_BUZZER_STATE_TOPIC = "week6/HealthyHome/actuator/buzzer/state";
constexpr const char *MQTT_MODE_TOPIC = "week6/HealthyHome/mode/set";
