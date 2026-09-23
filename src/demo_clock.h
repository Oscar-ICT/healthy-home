#pragma once
#include <RTClib.h>

// ---- Sped-up demo clock ------------------------------------------------
// Compresses a full 24h day into DEMO_CYCLE_MINUTES of real time so every
// time-based behaviour (rising/wake/day/winddown/bed - fades, buzzer,
// servo, PIR night lighting) plays out inside a short demo/interview
// slot instead of waiting for a real day to pass. Loops back to 00:00
// automatically. Set DEMO_MODE to 0 to run on the real DS3231 RTC.
#define DEMO_MODE 1
const float DEMO_CYCLE_MINUTES = 4.0f;
const float DEMO_SPEED = (24.0f * 60.0f * 60.0f) / (DEMO_CYCLE_MINUTES * 60.0f); // simulated seconds per real second
const DateTime DEMO_START(2026, 8, 26, 0, 0, 0);
extern unsigned long demoClockBaseMillis; // shifted forward to "pause" the demo clock during blocking network calls
