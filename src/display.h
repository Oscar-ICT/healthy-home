#pragma once
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include "time_state.h"

//OLED display: shows the (demo) clock + current time-of-day phase so the
//cycle can be followed at a glance during a demo instead of the serial
//monitor. Shares the I2C bus with the RTC (SDA=21, SCL=22).
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
extern Adafruit_SSD1306 display;
extern bool oledReady;

void updateDisplay(const DateTime& now, TimeState ts);
