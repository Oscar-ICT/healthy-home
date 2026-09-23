#include "daily_routine.h"

#include <Arduino.h>
#include <math.h> // Used for floor() function

#include "pins.h"
#include "servo_control.h"

namespace {

//Alarm
bool alarmFiredThisCycle = false; //ensures the wake alarm fires once per lap, not once per loop()

bool lightOn = false;

//Overhead light PWM settings
int pwmval = 0;

//PIR set up
int PIRSTATE = LOW;
int PIRval = 0;

}  // namespace

void SetupDailyRoutine() {
  pinMode(LDRPIN, INPUT);
  pinMode(BUZZERPIN, OUTPUT);
  pinMode(PIRLEDPIN, OUTPUT);
  pinMode(PIRPIN, INPUT);
  pinMode(PWMPIN, OUTPUT);
}

int UpdateDailyRoutine(const DateTime& now, TimeState ts, int ldrValue, bool customMode) {
  if (customMode) return PIRval;

  //Rising: fade the light up across the hour, driven by the clock
  //itself rather than a per-loop increment (see fadeLevel()).
  if (ts == rising) {
    alarmFiredThisCycle = false; //arm the wake alarm for this lap
    pwmval = fadeLevel(now, true);
    analogWrite(PWMPIN, pwmval);
    myServo.write(floor(pwmval / 1.41));
  }

  //Wake: alarm fires once, right as the state is entered.
  if (ts == wake && !alarmFiredThisCycle) {
    tone(BUZZERPIN, 500, 500);
    alarmFiredThisCycle = true;
  }

  //LDR Logic for wake/day time states
  Serial.print("LDR Value: ");
  Serial.println(ldrValue);

  if (!lightOn && ldrValue > LDR_THRESHOLD) {
    lightOn = true;
    Serial.println("Light ON");
  } else if (lightOn && ldrValue < LDR_THRESHOLD_OFF) {
    lightOn = false;
    Serial.println("Light OFF");
  }

  // LDR controlling light after rising
  if (ts == day || ts == wake) {
    if (lightOn) {
      analogWrite(PWMPIN, 255);
    } else {
      analogWrite(PWMPIN, 0);
    }
  }

  //Winddown: fade the light back down across the hour, same
  //clock-driven approach as the rising fade.
  if (ts == winddown) {
    pwmval = fadeLevel(now, false);
    analogWrite(PWMPIN, pwmval);
    myServo.write(floor(pwmval / 1.41));
  }

  if (ts == bed) {
    analogWrite(PWMPIN, 0);
    myServo.write(0);
    lightOn = false;
  }

  //PIR Motion Detector
  PIRval = digitalRead(PIRPIN);
  if (PIRval == HIGH && (ts == bed || ts == winddown)) {
    digitalWrite(PIRLEDPIN, HIGH);
    if (PIRSTATE == LOW) {
      Serial.println("Motion Detected"); //Motion sensor LED turns on
      PIRSTATE = HIGH;
    }
  } else {
    digitalWrite(PIRLEDPIN, LOW);
    if (PIRSTATE == HIGH) {
      Serial.println("Motion Ended");
    }
    PIRSTATE = LOW;
  }

  return PIRval;
}

int DailyRoutineBrightness() {
  return pwmval;
}
