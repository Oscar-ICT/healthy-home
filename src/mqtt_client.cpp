#include "mqtt_client.h"
#include "mqtt_topics.h"
#include "mqtt_commands.h"
#include "mqtt_publish.h"
#include "servo_control.h"

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

String CreateMqttClientId()
{
  const uint64_t chipId = ESP.getEfuseMac();
  const unsigned long high = static_cast<unsigned long>(chipId >> 32);
  const unsigned long low = static_cast<unsigned long>(chipId);

  char clientId[40];
  snprintf(clientId, sizeof(clientId), "ESP32Client-%08lX%08lX", high, low);
  return String(clientId);
}

void SetupMqtt()
{
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(CallbackMqtt);
}

void ConnectToWiFi()
{
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\nWi-Fi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void ConnectToMqtt()
{
  while (!mqttClient.connected())
  {
    const String clientId = CreateMqttClientId();

    Serial.print("Connecting to MQTT as ");
    Serial.print(clientId);
    Serial.print("... ");

    if (mqttClient.connect(clientId.c_str()))
    {
      Serial.println("connected.");

      mqttClient.publish(MQTT_STATUS_TOPIC, "online", true);
      Serial.print("Subscribed status to:");
      Serial.println(MQTT_STATUS_TOPIC);

      if (mqttClient.subscribe(MQTT_SERVO_SET_TOPIC))
      {
        Serial.print("Subscribed to: ");
        Serial.println(MQTT_SERVO_SET_TOPIC);
      }
      else
      {
        Serial.println("MQTT servo subscription failed.");
      }

      // Subscribe to Node-RED actuator commands

      if (mqttClient.subscribe(MQTT_PIR_LED_TOPIC))
      {
        Serial.print("Subscribed to: ");
        Serial.println(MQTT_PIR_LED_TOPIC);
      }
      else
      {
        Serial.println("MQTT PIR LED subscription failed.");
      }

      if (mqttClient.subscribe(MQTT_HEAT_LED_TOPIC))
      {
        Serial.print("Subscribed to: ");
        Serial.println(MQTT_HEAT_LED_TOPIC);
      }
      else
      {
        Serial.println("MQTT heat LED subscription failed.");
      }

      if (mqttClient.subscribe(MQTT_COOL_LED_TOPIC))
      {
        Serial.print("Subscribed to: ");
        Serial.println(MQTT_COOL_LED_TOPIC);
      }
      else
      {
        Serial.println("MQTT cool LED subscription failed.");
      }

      if (mqttClient.subscribe(MQTT_BRIGHT_LED_TOPIC))
      {
        Serial.print("Subscribed to: ");
        Serial.println(MQTT_BRIGHT_LED_TOPIC);
      }
      else
      {
        Serial.println("MQTT bright LED subscription failed.");
      }

      if (mqttClient.subscribe(MQTT_BUZZER_TOPIC))
      {
        Serial.print("Subscribed to: ");
        Serial.println(MQTT_BUZZER_TOPIC);
      }
      else
      {
        Serial.println("MQTT buzzer subscription failed.");
      }

      servoStateNeedsPublish = true;

      if (mqttClient.subscribe(MQTT_COMMAND_TOPIC))
      {
        Serial.print("Subscribed to: ");
        Serial.println(MQTT_COMMAND_TOPIC);
      }
      else
      {
        Serial.println("MQTT subscription failed.");
      }
      if (mqttClient.subscribe(MQTT_MODE_TOPIC))
      {
        Serial.print("Subscribed to: ");
        Serial.println(MQTT_MODE_TOPIC);
      }
      else
      {
        Serial.println("MQTT mode subscription failed.");
      }
    }
    else
    {
      Serial.print("failed, state=");
      Serial.println(mqttClient.state());
      Serial.println("Retrying in 5 seconds...");
      delay(5000);
    }
  }
  SetActuatorStates();
}
