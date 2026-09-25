#include "occupancy.h"
#include <Arduino.h>
#include "decision_tree.h"
#include "decision_tree_trainer.h"
#include "pins.h"
#include "shared_state.h"

const unsigned long LONG_PRESS_MS = 1500;
const unsigned long SAMPLE_INTERVAL_MS = 2000;
const unsigned long OCC_MOTION_HOLD_MS = 30000;
const unsigned long DEBOUNCE_MS = 50;

DecisionTreeTrainer<150, 4> occupancyTrainer;   // features: motionRecent, light, temperature, humidity
DecisionTreeClassifier* occupancyClassifier = nullptr;

enum OccupancyMode { CALIBRATING, RUNNING };
OccupancyMode occupancyMode = CALIBRATING;

uint8_t currentLabel = 0;  // 0 = empty, 1 = occupied - toggled by short button press
unsigned long lastSampleMillis = 0;
unsigned long buttonDownMillis = 0;
bool buttonWasDown = false;

unsigned long lastDebounceMillis = 0;
bool debouncedButtonState = false;

unsigned long lastMotionMillis = 0;
bool everSeenMotion = false;

void handleCalibrationButton() {
  bool rawDown = (digitalRead(CALIB_BUTTON_PIN) == LOW);
  unsigned long nowMs = millis();

  // Debounce: only trust a state change once it's been stable for
  // DEBOUNCE_MS. Rapid electrical bounce during a press/release gets
  // ignored instead of being read as several separate presses.
  if (rawDown != debouncedButtonState) {
    if (nowMs - lastDebounceMillis >= DEBOUNCE_MS) {
      debouncedButtonState = rawDown;
      lastDebounceMillis = nowMs;
    }
  } else {
    lastDebounceMillis = nowMs;
  }

  bool down = debouncedButtonState;

  static bool longPressAnnounced = false;

  if (down && !buttonWasDown) {
    buttonDownMillis = millis();
    longPressAnnounced = false;
  }

  // Give feedback the moment a hold crosses the long-press threshold,
  // rather than staying silent until release + training finishes.
  if (down && buttonWasDown && !longPressAnnounced) {
    if (millis() - buttonDownMillis >= LONG_PRESS_MS) {
      Serial.println("[occupancy] long press detected - training...");
      longPressAnnounced = true;
    }
  }

  if (!down && buttonWasDown) {
    unsigned long heldFor = millis() - buttonDownMillis;
    if (heldFor >= LONG_PRESS_MS) {
      size_t n = occupancyTrainer.train(4, 3);
      if (n > 0) {
        if (occupancyClassifier != nullptr) delete occupancyClassifier;
        occupancyClassifier = new DecisionTreeClassifier(occupancyTrainer.nodes(), n);
        occupancyMode = RUNNING;
        aiMode = true;
        Serial.print("[occupancy] trained, "); Serial.print(n); Serial.println(" nodes. Now RUNNING.");
      } else {
        Serial.println("[occupancy] not enough samples yet - keep calibrating.");
      }
    } else {
      currentLabel = 1 - currentLabel;
      Serial.print("[occupancy] labeling as: ");
      Serial.println(currentLabel == 1 ? "OCCUPIED" : "EMPTY");
    }
  }
  buttonWasDown = down;
}

void updateOccupancy(float temperature, float humidity, float lightPct, bool pirHigh) {
  if (pirHigh) { lastMotionMillis = millis(); everSeenMotion = true; }
  float motionRecent = (everSeenMotion && (millis() - lastMotionMillis < OCC_MOTION_HOLD_MS)) ? 1.0f : 0.0f;
  float features[4] = { motionRecent, lightPct, temperature, humidity };

  if (occupancyMode == CALIBRATING) {
    unsigned long nowMs = millis();
    if (nowMs - lastSampleMillis >= SAMPLE_INTERVAL_MS) {
      lastSampleMillis = nowMs;
      bool ok = occupancyTrainer.addSample(features, currentLabel);
      Serial.print("[occupancy] sample #"); Serial.print(occupancyTrainer.sampleCount());
      Serial.print(" label="); Serial.println(currentLabel == 1 ? "OCCUPIED" : "EMPTY");
      if (!ok) Serial.println("[occupancy] buffer full - hold button to train now.");
    }
  } else {
    bool occupied = (occupancyClassifier->predict(features) == 1);
    Serial.print("[occupancy] prediction: "); Serial.println(occupied ? "OCCUPIED" : "EMPTY");
    analogWrite(PWMPIN, occupied ? 255 : 0);
  }
}