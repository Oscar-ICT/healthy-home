#pragma once
// Pin assignments and the small hardware constants tied to them. No
// logic here - just wiring, so every module can pull in the numbers it
// needs without dragging in whatever module happens to "own" a pin.

// ---- Servo --------------------------------------------------------------
#define SERVOPIN 23

// ---- DHT22 sensor ---------------------------------------------------------
#define DHTPIN 15
#define DHTTYPE DHT22

// ---- Air con LEDs -----------------------------------------------------
const int HOTLEDPIN = 13;
const int COLDLEDPIN = 14;

// ---- LDR ----------------------------------------------------------------
const int LDRPIN = 32;
const int LDR_THRESHOLD = 1300;     // Threshold for light ON
const int LDR_THRESHOLD_OFF = 1200; // Threshold for light OFF

// ---- PIR ----------------------------------------------------------------
const int PIRLEDPIN = 26;
const int PIRPIN = 18;

// ---- Alarm ----------------------------------------------------------------
const int BUZZERPIN = 25;

// ---- Overhead light PWM ---------------------------------------------------
const int PWMPIN = 27;

// ---- OLED display -----------------------------------------------------
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

// ---- Occupancy calibration button -----------------------------------------
// GPIO4 -> button -> GND (uses the internal pull-up, so idle = HIGH,
// pressed = LOW). Short press toggles which label new samples get;
// holding it down for LONG_PRESS_MS trains the tree from whatever's
// been collected and switches from CALIBRATING to RUNNING.
#define CALIB_BUTTON_PIN 4
const unsigned long LONG_PRESS_MS = 1500;
const unsigned long SAMPLE_INTERVAL_MS = 2000;
const unsigned long OCC_MOTION_HOLD_MS = 30000;

// ---- Publish cadence --------------------------------------------------------
const unsigned long DHT_PUBLISH_INTERVAL_MS = 5000;
