#include "climate_control.h"

#include <Arduino.h>

#include "pins.h"

namespace {

systemState state = normal;

}  // namespace

void SetupClimate() {
  pinMode(HOTLEDPIN, OUTPUT);
  pinMode(COLDLEDPIN, OUTPUT);
}

void UpdateClimateState(float temperature, float mintemp, float maxtemp, float hysteresis, bool customMode) {
  if (!customMode) {
    switch (state) {
      case cooling:
        if (temperature < maxtemp - hysteresis) { //Cooling state that switches off at 23
          state = normal;
        }
        break;
      case heating:
        if (temperature > mintemp + hysteresis) { //Heating state that switches off at 19
          state = normal;
        }
        break;
      case normal:
        if (temperature > maxtemp) {
          state = cooling;
        }                                          //Normal state checking for temperature breach
        else if (temperature < mintemp) {
          state = heating;
        }
        break;
    }
  } else {
    state = custom;
  }
}

void ApplyClimateActuators(bool customMode) {
  if (customMode) return;

  if (state == cooling) {
    digitalWrite(COLDLEDPIN, HIGH);
    digitalWrite(HOTLEDPIN, LOW);
    Serial.println("Cooling Activated!");
  } else if (state == heating) {
    digitalWrite(HOTLEDPIN, HIGH);
    digitalWrite(COLDLEDPIN, LOW);
    Serial.println("Heating Activated!");
  } else {
    digitalWrite(COLDLEDPIN, LOW);
    digitalWrite(HOTLEDPIN, LOW);
    Serial.println("Good Temperature!");
  }
}

systemState CurrentClimateState() {
  return state;
}

void ResetClimateToNormal() {
  state = normal;
}
