#include "occupancy.h"
#include <Arduino.h>
#include "decision_tree.h"
#include "decision_tree_trainer.h"

const unsigned long LONG_PRESS_MS = 1500;
const unsigned long SAMPLE_INTERVAL_MS = 2000;
const unsigned long OCC_MOTION_HOLD_MS = 30000;

DecisionTreeTrainer<150, 4> occupancyTrainer;   // features: motionRecent, light, temperature, humidity
DecisionTreeClassifier* occupancyClassifier = nullptr;

enum OccupancyMode { CALIBRATING, RUNNING };
OccupancyMode occupancyMode = CALIBRATING;

uint8_t currentLabel = 0;  // 0 = empty, 1 = occupied - toggled by short button press
unsigned long lastSampleMillis = 0;
unsigned long buttonDownMillis = 0;
bool buttonWasDown = false;

unsigned long lastMotionMillis = 0;
bool everSeenMotion = false;

void handleCalibrationButton() {
  bool down = (digitalRead(CALIB_BUTTON_PIN) == LOW);

  if (down && !buttonWasDown) buttonDownMillis = millis();

  if (!down && buttonWasDown) {
    unsigned long heldFor = millis() - buttonDownMillis;
    if (heldFor >= LONG_PRESS_MS) {
      size_t n = occupancyTrainer.train(4, 3);
      if (n > 0) {
        if (occupancyClassifier != nullptr) delete occupancyClassifier;
        occupancyClassifier = new DecisionTreeClassifier(occupancyTrainer.nodes(), n);
        occupancyMode = RUNNING;
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
    // TODO: feed `occupied` into the automation rules below (e.g. only
    // let PIR-driven behaviour during bed/winddown act if occupied).
  }
}
