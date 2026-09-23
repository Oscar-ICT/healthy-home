#include "time_state.h"

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
