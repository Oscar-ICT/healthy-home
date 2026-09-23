#include "mqtt_publish.h"

#include <Arduino.h>

#include "daily_routine.h"
#include "mqtt_client.h"
#include "mqtt_topics.h"
#include "pins.h"
#include "servo_control.h"

namespace {

// Never actually set anywhere (the wake alarm and buzzer/set MQTT
// handler both just fire tone()/noTone() without recording it here) -
// the published buzzer state topic is therefore always "0". Preserved
// as-is from the original rather than fixed as part of this refactor.
bool buzzerState = false;

}  // namespace

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
    DailyRoutineBrightness() > 0 ? "1" : "0",
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
  if (!ServoStatePendingPublish())
  {
    return;
  }

  char servoAngleText[8];
  snprintf(servoAngleText, sizeof(servoAngleText), "%d", CurrentServoAngle());

  if (mqttClient.publish(MQTT_SERVO_STATE_TOPIC, servoAngleText, true))
  {
    ClearServoStatePendingPublish();

    Serial.print("Published servo state to MQTT: ");
    Serial.println(servoAngleText);
  }
}
