#pragma once
#include <ESP32Servo.h>
#include "pins.h"

// Servo set up
extern Servo myServo;
extern int currentServoAngle;
extern bool servoStateNeedsPublish;

void SetServoAngle(int angle);
void SetupServo();
