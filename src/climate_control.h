#pragma once

enum systemState {normal, cooling, heating, custom};

void SetupClimate();

// Runs the hysteresis state machine from the current temperature. While
// customMode is true the state is just forced to `custom` (actuators are
// then left to direct MQTT commands instead), matching the original.
void UpdateClimateState(float temperature, float mintemp, float maxtemp, float hysteresis, bool customMode);

// Drives the heat/cool proxy LEDs from the current state. No-op while
// customMode is true, so manual LED commands aren't overridden.
void ApplyClimateActuators(bool customMode);

systemState CurrentClimateState();

// Used by the MQTT mode/set handler when custom mode is switched off.
void ResetClimateToNormal();
