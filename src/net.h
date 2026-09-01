#pragma once
#include <Arduino.h>

// One telemetry sample pushed to the ThingSpeak telemetry channel.
struct Telemetry {
  float temperature;   // degC
  float humidity;      // %
  float light;         // 0-100%
  int   climateState;  // 0 normal, 1 cooling, 2 heating
  int   timeState;     // 0 day, 1 rising, 2 wake, 3 winddown, 4 bed
  bool  motion;        // PIR
};

// Remote command last read from the ThingSpeak control channel.
// `valid` is false until the first successful poll; fields keep their
// previous value when a channel field comes back empty.
struct Command {
  int   mode;         // 0 auto, 1 force cool, 2 force heat, 3 climate off
  float desiredTemp;  // degC
  float minTemp;      // degC
  float maxTemp;      // degC
  int   blinds;       // 0 auto, 1 open, 2 closed
  int   lights;       // 0 auto, 1 force on, 2 force off
  bool  valid;
};

// Call once from setup().
void netBegin();

// Call every loop(). Non-blocking except for the throttled HTTPS
// requests (~<5 s worst case). Handles WiFi (re)connect, and the
// rate-limited telemetry upload + command poll. Returns the latest
// known command.
Command netTick(const Telemetry& t);

// Latest command without triggering a refresh.
Command netLastCommand();
