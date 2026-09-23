#pragma once
#include <ESP32Servo.h>

// The blinds servo. Exposed directly (rather than only through
// SetServoAngle) because the daily-routine fades drive it every loop
// via myServo.write() without going through the angle-tracking/publish
// path - matches the original behaviour exactly.
extern Servo myServo;

void SetupServo();
void SetServoAngle(int angle);
int CurrentServoAngle();

// Whether the servo's angle has changed since it was last published to MQTT.
bool ServoStatePendingPublish();
void ClearServoStatePendingPublish();
