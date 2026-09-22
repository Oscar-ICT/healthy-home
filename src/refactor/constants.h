// Node Red Connection
const char *WIFI_SSID = "Wokwi-GUEST";
const char *WIFI_PASSWORD = "";

const char *MQTT_SERVER = "broker.hivemq.com";
const uint16_t MQTT_PORT = 1883;
const char *MQTT_COMMAND_TOPIC = "week6/HealthyHome/status/esp32";
const char *MQTT_STATUS_TOPIC = "week6/HealthyHome/status/esp32";
const char *MQTT_TEMPERATURE_TOPIC = "week6/HealthyHome/sensor/temperature";
const char *MQTT_HUMIDITY_TOPIC = "week6/HealthyHome/sensor/humidity";
const char *MQTT_SERVO_SET_TOPIC = "week6/HealthyHome/actuator/servo/set";
const char *MQTT_SERVO_STATE_TOPIC = "week6/HealthyHome/actuator/servo/state";
const char *MQTT_PIR_TOPIC = "week6/HealthyHome/sensor/pir";
const char *MQTT_LDR_TOPIC = "week6/HealthyHome/sensor/ldr";
const char *MQTT_PIR_LED_TOPIC = "week6/HealthyHome/actuator/pir_led/set";
const char *MQTT_HEAT_LED_TOPIC = "week6/HealthyHome/actuator/heat_led/set";
const char *MQTT_COOL_LED_TOPIC = "week6/HealthyHome/actuator/cool_led/set";
const char *MQTT_BRIGHT_LED_TOPIC = "week6/HealthyHome/actuator/bright_led/set";
const char *MQTT_BUZZER_TOPIC = "week6/HealthyHome/actuator/buzzer/set";
const char *MQTT_PIRLED_STATE_TOPIC = "week6/HealthyHome/actuator/pir_led/state";
const char *MQTT_HEATLED_STATE_TOPIC = "week6/HealthyHome/actuator/heat_led/state";
const char *MQTT_COOLLED_STATE_TOPIC = "week6/HealthyHome/actuator/cool_led/state";
const char *MQTT_BRIGHTLED_STATE_TOPIC = "week6/HealthyHome/actuator/bright_led/state";
const char *MQTT_BUZZER_STATE_TOPIC = "week6/HealthyHome/actuator/buzzer/state";
const char *MQTT_MODE_TOPIC = "week6/HealthyHome/mode/set";

const unsigned long DHT_PUBLISH_INTERVAL_MS = 5000;
const unsigned long MQTT_RETRY_INTERVAL_MS = 5000;