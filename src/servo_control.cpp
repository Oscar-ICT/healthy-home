#include "servo_control.h"

Servo myServo;
int currentServoAngle = 0;
bool servoStateNeedsPublish = true;

void SetServoAngle(int angle)
{
  currentServoAngle = constrain(angle, 0, 180);
  myServo.write(currentServoAngle);
  servoStateNeedsPublish = true;
}

void SetupServo()
{
  myServo.setPeriodHertz(50);
  myServo.attach(SERVOPIN, 500, 2400);
  SetServoAngle(currentServoAngle);
}
