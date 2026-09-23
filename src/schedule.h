#pragma once
#include <RTClib.h>

// ---- Sped-up demo clock ------------------------------------------------
// Compresses a full 24h day into DEMO_CYCLE_MINUTES of real time so every
// time-based behaviour (rising/wake/day/winddown/bed - fades, buzzer,
// servo, PIR night lighting) plays out inside a short demo/interview
// slot instead of waiting for a real day to pass. Loops back to 00:00
// automatically. Set DEMO_MODE to 0 to run on the real DS3231 RTC.
#define DEMO_MODE 1

enum TimeState {rising, wake, winddown, bed, day};

const char* timeStateName(TimeState s);

//Computes an 8-bit brightness that ramps across one hour: 0 at the top
//of the hour up to 255 by the end for a rising fade, or the reverse for
//a winddown fade. Driven by the clock's own minute/second instead of
//counting +-1 per loop() call, so it completes correctly whether the
//hour is 3600 real seconds (normal RTC) or ~10 real seconds
//(DEMO_MODE) - a fixed per-loop step could only ever manage the
//former.
int fadeLevel(const DateTime& now, bool rampUp);

//Maps the local TimeState enum to the ThingSpeak field5 encoding
//(0 day, 1 rising, 2 wake, 3 winddown, 4 bed).
int tsTimeState(TimeState s);

// Brings up the demo clock (DEMO_MODE) or the DS3231 RTC. Call once from setup().
void SetupSchedule();

// Current time, from whichever clock source is active.
DateTime GetScheduleTime();

// Updates the time-of-day state from `now`. No-op while customMode is
// true, so the last automatic time state is held during manual control.
void UpdateTimeState(const DateTime& now, bool customMode);

TimeState CurrentTimeState();
