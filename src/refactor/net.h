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

// Call once from setup(). Brings up WiFi if needed and spawns a
// dedicated FreeRTOS task (pinned to core 0) that owns every blocking
// HTTPS call to ThingSpeak. Nothing on the calling core ever blocks on
// the network after this returns.
void netBegin();

// Call every loop() iteration with the latest sensor sample. Just takes
// a mutex and copies a small struct - not the HTTPS call itself - so
// it's effectively instant and safe to call from the main loop every
// tick. The net task picks it up next time an upload is due.
void netUpdateTelemetry(const Telemetry& t);

// Thread-safe snapshot of the latest command received from the control
// channel. Also just a mutex + struct copy - safe to call every tick.
Command netGetCommand();