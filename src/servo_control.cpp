#include "servo_control.h"

#include <Arduino.h>

#include "pins.h"

Servo myServo;

namespace {

int currentServoAngle = 0;
bool servoStateNeedsPublish = true;

}  // namespace

void SetupServo() {
  myServo.setPeriodHertz(50);
  myServo.attach(SERVOPIN, 500, 2400);
  SetServoAngle(currentServoAngle);
}

void SetServoAngle(int angle) {
  currentServoAngle = constrain(angle, 0, 180);
  myServo.write(currentServoAngle);
  servoStateNeedsPublish = true;
}

int CurrentServoAngle() {
  return currentServoAngle;
}

bool ServoStatePendingPublish() {
  return servoStateNeedsPublish;
}

void ClearServoStatePendingPublish() {
  servoStateNeedsPublish = false;
}
