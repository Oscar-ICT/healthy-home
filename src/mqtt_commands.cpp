#include "mqtt_commands.h"
#include "mqtt_topics.h"
#include "mqtt_publish.h"
#include "servo_control.h"
#include "shared_state.h"
#include "pins.h"

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

    // A manual angle only means something if the automatic time-of-day
    // logic in loop() isn't also driving the servo, so switch to custom
    // mode here rather than requiring a separate mode command first -
    // otherwise the automatic logic silently reverts this within ~2s.
    if (!customMode)
    {
      customMode = true;
      Serial.println("CUSTOM MODE ON (manual servo command)");
    }

    SetServoAngle(static_cast<int>(requestedAngle));

    return;
  }

  // PIR LED

  if (strcmp(topic, MQTT_PIR_LED_TOPIC) == 0)
  {
    if (strcmp(message, "1") == 0)
    {
      if (!customMode)
      {
        customMode = true;
        Serial.println("CUSTOM MODE ON (manual PIR LED command)");
      }
      digitalWrite(PIRLEDPIN, HIGH);
      Serial.println("PIR LED ON");
    }
    else if (strcmp(message, "0") == 0)
    {
      if (!customMode)
      {
        customMode = true;
        Serial.println("CUSTOM MODE ON (manual PIR LED command)");
      }
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
      if (!customMode)
      {
        customMode = true;
        Serial.println("CUSTOM MODE ON (manual heat LED command)");
      }
      digitalWrite(HOTLEDPIN, HIGH);
      Serial.println("Heat LED ON");
    }
    else if (strcmp(message, "0") == 0)
    {
      if (!customMode)
      {
        customMode = true;
        Serial.println("CUSTOM MODE ON (manual heat LED command)");
      }
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
      if (!customMode)
      {
        customMode = true;
        Serial.println("CUSTOM MODE ON (manual cool LED command)");
      }
      digitalWrite(COLDLEDPIN, HIGH);
      Serial.println("Cool LED ON");
    }
    else if (strcmp(message, "0") == 0)
    {
      if (!customMode)
      {
        customMode = true;
        Serial.println("CUSTOM MODE ON (manual cool LED command)");
      }
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
      if (!customMode)
      {
        customMode = true;
        Serial.println("CUSTOM MODE ON (manual bright LED command)");
      }
      analogWrite(PWMPIN, 255);
      Serial.println("Bright LED ON");
    }
    else if (strcmp(message, "0") == 0)
    {
      if (!customMode)
      {
        customMode = true;
        Serial.println("CUSTOM MODE ON (manual bright LED command)");
      }
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
      if (!customMode)
      {
        customMode = true;
        Serial.println("CUSTOM MODE ON (manual buzzer command)");
      }
      tone(BUZZERPIN, 500);
      Serial.println("Buzzer ON");
    }
    else if (strcmp(message, "0") == 0)
    {
      if (!customMode)
      {
        customMode = true;
        Serial.println("CUSTOM MODE ON (manual buzzer command)");
      }
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
      customMode = true;
      Serial.println("CUSTOM MODE ON");
    }
    else if (strcmp(message, "false") == 0)
    {
      customMode = false;

      state = normal;

      if (timeState == day || timeState == wake || timeState == rising) {
        SetServoAngle(180);
      } else if (timeState == bed || timeState == winddown) {
        SetServoAngle(0);
      }

      Serial.println("AUTOMATIC MODE ON");
    }

    return;
  }
}
