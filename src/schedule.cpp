#include "schedule.h"

#include <Arduino.h>
#include <RTClib.h>

namespace {

const float DEMO_CYCLE_MINUTES = 4.0f;
const float DEMO_SPEED = (24.0f * 60.0f * 60.0f) / (DEMO_CYCLE_MINUTES * 60.0f); // simulated seconds per real second
const DateTime DEMO_START(2026, 8, 26, 0, 0, 0);
unsigned long demoClockBaseMillis = 0; // shifted forward to "pause" the demo clock during blocking network calls

RTC_DS3231 rtc;

TimeState timeState = day;

}  // namespace

const char* timeStateName(TimeState s) {
  switch (s) {
    case rising:   return "RISING";
    case wake:     return "WAKE";
    case winddown: return "WINDDOWN";
    case bed:      return "BED";
    case day:      return "DAY";
  }
  return "?";
}

int fadeLevel(const DateTime& now, bool rampUp) {
  float fraction = (now.minute() * 60 + now.second()) / 3600.0f;
  if (fraction > 1.0f) fraction = 1.0f;
  int level = (int)(fraction * 255.0f);
  return rampUp ? level : (255 - level);
}

int tsTimeState(TimeState s) {
  switch (s) {
    case day:      return 0;
    case rising:   return 1;
    case wake:     return 2;
    case winddown: return 3;
    case bed:      return 4;
  }
  return 0;
}

void SetupSchedule() {
#if DEMO_MODE
  // Demo mode drives the clock from millis(), not the DS3231 - no RTC
  // hardware dependency, so a missing/faulty RTC can't hang the demo.
  demoClockBaseMillis = millis();
  Serial.print("DEMO MODE: 24h compressed into ");
  Serial.print(DEMO_CYCLE_MINUTES);
  Serial.print(" min (");
  Serial.print(DEMO_SPEED, 0);
  Serial.println("x speed), loops back to 00:00 automatically");
#else
  //RTC connection check
  if (!rtc.begin()) {
    while (1);
  }
  //RTC powerloss
  if (rtc.lostPower()) {
    Serial.println("RTC lost power");
    rtc.adjust(DateTime(__DATE__, __TIME__));
  }
  //rtc.adjust(DateTime(2026,8,26,1,0,0)); //Un comment for 1am (bed)
  rtc.adjust(DateTime(2026,8,26,7,0,0)); //Un comment for 7:00am (rising)
  //rtc.adjust(DateTime(2026,8,26,8,0,0)); //Un comment for 8:01am (wake)
  //rtc.adjust(DateTime(2026,8,26,9,0,0)); //Un comment for 9am (day)
  //rtc.adjust(DateTime(2026,8,26,20,0,0)); //Un comment for 8:01pm (winddown)
  Serial.println("RTC initialised");
#endif
}

DateTime GetScheduleTime() {
#if DEMO_MODE
  unsigned long elapsedRealMs = millis() - demoClockBaseMillis;
  uint32_t simSeconds = (uint32_t)((elapsedRealMs / 1000.0f) * DEMO_SPEED);
  simSeconds %= 86400UL; // wraps back to 00:00 after one simulated day
  return DEMO_START + TimeSpan((int32_t)simSeconds);
#else
  return rtc.now();
#endif
}

void UpdateTimeState(const DateTime& now, bool customMode) {
  if (customMode) return;

  if (now.hour() == 7) {
    timeState = rising;
  } else if (now.hour() == 8) {
    timeState = wake;
  } else if (now.hour() == 20) {
    timeState = winddown;
  } else if (now.hour() >= 21 || now.hour() < 7) {
    timeState = bed;
  } else {
    timeState = day;
  }
}

TimeState CurrentTimeState() {
  return timeState;
}
