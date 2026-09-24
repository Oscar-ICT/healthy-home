#pragma once

void SetActuatorStates();

//MQTT Publish function
//Takes the readings already taken this loop() iteration instead of
//re-reading the DHT22 (loop() already validated they're not NaN).
void PublishDhtReadings(float temperature, float humidity);
void PublishPirReading(int pirValue);
void PublishLdrReading(int ldrValue);
void PublishServoState();
void PublishModeState();
