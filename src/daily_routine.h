#pragma once
#include <RTClib.h>

#include "schedule.h"

// Time-of-day driven lighting/blinds/alarm/PIR rules: the rising and
// winddown fades, the wake alarm, LDR-driven daytime lighting, and
// PIR-driven night lighting.

void SetupDailyRoutine();

// Runs the automatic rules for the current time state. No-op while
// customMode is true - Node-RED is driving the actuators directly then.
// Always returns the latest PIR pin reading (held over from the last
// time this ran automatically, if customMode is true), for publishing.
int UpdateDailyRoutine(const DateTime& now, TimeState ts, int ldrValue, bool customMode);

// Current overhead-light PWM value, for the MQTT actuator-state publish.
int DailyRoutineBrightness();
