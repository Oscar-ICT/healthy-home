#pragma once

// Servo set up
#define SERVOPIN 23

//DHT22 Sensor SetUp
#define DHTPIN 15

//LDR setup
const int LDRPIN = 32;
const int LDR_THRESHOLD = 1300; // Threshold for light ON
const int LDR_THRESHOLD_OFF = 1200; // Threshold for light OFF

//Air Con SetUp
extern int HOTLEDPIN;
extern int COLDLEDPIN;

//PIR set up
extern int PIRLEDPIN;
extern int PIRPIN;

//Alarm
extern int BUZZERPIN;

//Overhead light PWM pin and settings
extern int PWMPIN;
extern int pwmval;
