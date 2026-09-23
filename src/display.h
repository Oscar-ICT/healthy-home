#pragma once
#include <RTClib.h>

#include "schedule.h"

// OLED display: shows the (demo) clock + current time-of-day phase so the
// cycle can be followed at a glance during a demo instead of the serial
// monitor. Shares the I2C bus with the RTC (SDA=21, SCL=22).

// Brings up the I2C bus and the display. Safe to keep going if the
// display isn't found - updateDisplay() then just no-ops.
void SetupDisplay();

void updateDisplay(const DateTime& now, TimeState ts);
