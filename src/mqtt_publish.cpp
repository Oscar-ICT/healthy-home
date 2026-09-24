#include "mqtt_publish.h"
#include "mqtt_client.h"
#include "mqtt_topics.h"
#include "pins.h"
#include "servo_control.h"
#include "shared_state.h"

bool buzzerState = false;

void SetActuatorStates()
{
  mqttClient.publish(
    MQTT_PIRLED_STATE_TOPIC,
    digitalRead(PIRLEDPIN) == HIGH ? "1" : "0",
    true
  );

  mqttClient.publish(
    MQTT_HEATLED_STATE_TOPIC,
    digitalRead(HOTLEDPIN) == HIGH ? "1" : "0",
    true
  );

  mqttClient.publish(
    MQTT_COOLLED_STATE_TOPIC,
    digitalRead(COLDLEDPIN) == HIGH ? "1" : "0",
    true
  );

  mqttClient.publish(
    MQTT_BRIGHTLED_STATE_TOPIC,
    pwmval > 0 ? "1" : "0",
    true
  );

  // Buzzer state
  mqttClient.publish(
  MQTT_BUZZER_STATE_TOPIC,
  buzzerState ? "1" : "0",
  true
  );
}

void PublishDhtReadings(float temperature, float humidity)
{
  char temperatureText[10];
  char humidityText[10];

  dtostrf(temperature, 1, 1, temperatureText);
  dtostrf(humidity, 1, 1, humidityText);

  const bool temperaturePublished = mqttClient.publish(
    MQTT_TEMPERATURE_TOPIC,
    temperatureText
  );
  const bool humidityPublished = mqttClient.publish(
    MQTT_HUMIDITY_TOPIC,
    humidityText
  );

  Serial.print("Temperature: ");
  Serial.print(temperatureText);
  Serial.print(" C, Humidity: ");
  Serial.print(humidityText);
  Serial.print(" %, MQTT publish: ");
  Serial.println((temperaturePublished && humidityPublished) ? "success" : "failed");
}

void PublishPirReading(int pirValue)
{
  const char *motionText = (pirValue == HIGH) ? "Motion Detected" : "No Motion";

  const char *motionValue = (pirValue == HIGH) ? "1" : "0";


  if (mqttClient.publish(MQTT_PIR_TOPIC, motionValue))
  {
    Serial.print("PIR: ");
    Serial.println(motionValue);
  }
  else
  {
    Serial.println("PIR MQTT publish failed.");
  }
}

void PublishLdrReading(int ldrValue)
{
  char ldrText[8];
  snprintf(ldrText, sizeof(ldrText), "%d", ldrValue);

  if (mqttClient.publish(MQTT_LDR_TOPIC, ldrText))
  {
    Serial.print("LDR: ");
    Serial.println(ldrText);
  }
  else
  {
    Serial.println("LDR MQTT publish failed.");
  }
}

void PublishServoState()
{
  if (!servoStateNeedsPublish)
  {
    return;
  }

  char servoAngleText[8];
  snprintf(servoAngleText, sizeof(servoAngleText), "%d", currentServoAngle);

  if (mqttClient.publish(MQTT_SERVO_STATE_TOPIC, servoAngleText, true))
  {
    servoStateNeedsPublish = false;

    Serial.print("Published servo state to MQTT: ");
    Serial.println(servoAngleText);
  }
}

void PublishModeState()
{
  // A manual actuator command flips customMode on its own, so the dashboard
  // can only stay in sync if the device announces the mode rather than
  // assuming its own switch caused it. Retained so a reconnecting dashboard
  // picks up the current mode immediately.
  static int lastPublished = -1;

  const int current = customMode ? 1 : 0;

  if (current == lastPublished)
  {
    return;
  }

  if (mqttClient.publish(MQTT_MODE_STATE_TOPIC, customMode ? "true" : "false", true))
  {
    lastPublished = current;

    Serial.print("Published mode state to MQTT: ");
    Serial.println(customMode ? "true" : "false");
  }
}
