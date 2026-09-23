#pragma once
#include <RTClib.h>

//Air Con SetUp
enum systemState {normal, cooling, heating, custom};

//Time states
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
