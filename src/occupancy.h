#pragma once

// ---- Occupancy edge-AI: calibration + live prediction -------------------
// GPIO4 -> button -> GND (uses the internal pull-up, so idle = HIGH,
// pressed = LOW). Short press toggles which label new samples get;
// holding it down for LONG_PRESS_MS trains the tree from whatever's
// been collected and switches from CALIBRATING to RUNNING.

void SetupOccupancy();

// Short press: toggle which label new samples get. Long press: stop
// collecting and train the tree from everything gathered so far.
void handleCalibrationButton();

// Call once per loop() with the current sensor readings. While
// CALIBRATING, banks a labeled sample every SAMPLE_INTERVAL_MS. Once
// RUNNING, predicts occupancy from the trained tree.
void updateOccupancy(float temperature, float humidity, float lightPct, bool pirHigh);
