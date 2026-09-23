#include "mqtt_commands.h"

#include <Arduino.h>

#include "climate_control.h"
#include "mqtt_client.h"
#include "mqtt_publish.h"
#include "mqtt_topics.h"
#include "pins.h"
#include "servo_control.h"

void CallbackMqtt(char *topic, byte *payload, unsigned int length)
{
  char message[16];

  const unsigned int copyLength =
      min(length, sizeof(message) - 1);

  for (unsigned int i = 0; i < copyLength; i++)
  {
    message[i] = static_cast<char>(payload[i]);
  }

  message[copyLength] = '\0';

  Serial.print("Message received on ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(message);

  // SERVO

  if (strcmp(topic, MQTT_SERVO_SET_TOPIC) == 0)
  {
    char *endPointer = nullptr;

    const long requestedAngle =
        strtol(message, &endPointer, 10);

    if (endPointer == message || *endPointer != '\0')
    {
      Serial.println("Invalid servo angle.");
      return;
    }

    SetServoAngle(static_cast<int>(requestedAngle));

    return;
  }

  // PIR LED

  if (strcmp(topic, MQTT_PIR_LED_TOPIC) == 0)
  {
    if (strcmp(message, "1") == 0)
    {
      digitalWrite(PIRLEDPIN, HIGH);
      Serial.println("PIR LED ON");
    }
    else if (strcmp(message, "0") == 0)
    {
      digitalWrite(PIRLEDPIN, LOW);
      Serial.println("PIR LED OFF");
    }

    SetActuatorStates();

    return;
  }

  // HEAT LED

  if (strcmp(topic, MQTT_HEAT_LED_TOPIC) == 0)
  {
    if (strcmp(message, "1") == 0)
    {
      digitalWrite(HOTLEDPIN, HIGH);
      Serial.println("Heat LED ON");
    }
    else if (strcmp(message, "0") == 0)
    {
      digitalWrite(HOTLEDPIN, LOW);
      Serial.println("Heat LED OFF");
    }

    SetActuatorStates();

    return;
  }

  // COOL LED

  if (strcmp(topic, MQTT_COOL_LED_TOPIC) == 0)
  {
    if (strcmp(message, "1") == 0)
    {
      digitalWrite(COLDLEDPIN, HIGH);
      Serial.println("Cool LED ON");
    }
    else if (strcmp(message, "0") == 0)
    {
      digitalWrite(COLDLEDPIN, LOW);
      Serial.println("Cool LED OFF");
    }

    SetActuatorStates();

    return;
  }

  // BRIGHT LED

  if (strcmp(topic, MQTT_BRIGHT_LED_TOPIC) == 0)
  {
    if (strcmp(message, "1") == 0)
    {
      analogWrite(PWMPIN, 255);
      Serial.println("Bright LED ON");
    }
    else if (strcmp(message, "0") == 0)
    {
      analogWrite(PWMPIN, 0);
      Serial.println("Bright LED OFF");
    }

    SetActuatorStates();

    return;
  }

  // BUZZER

  if (strcmp(topic, MQTT_BUZZER_TOPIC) == 0)
  {
    if (strcmp(message, "1") == 0)
    {
      tone(BUZZERPIN, 500);
      Serial.println("Buzzer ON");
    }
    else if (strcmp(message, "0") == 0)
    {
      noTone(BUZZERPIN);
      Serial.println("Buzzer OFF");
    }

    SetActuatorStates();

    return;
  }

  // CUSTOM / AUTOMATIC MODE

  if (strcmp(topic, MQTT_MODE_TOPIC) == 0)
  {
    if (strcmp(message, "true") == 0)
    {
      SetCustomMode(true);
      Serial.println("CUSTOM MODE ON");
    }
    else if (strcmp(message, "false") == 0)
    {
      SetCustomMode(false);

      ResetClimateToNormal();

      Serial.println("AUTOMATIC MODE ON");
    }

    return;
  }
}
