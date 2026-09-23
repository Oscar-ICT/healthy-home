#pragma once

// Publishes the actual pin/PWM state of every actuator to MQTT (as
// opposed to just echoing back the last command received) - called
// after any actuator changes, whether from an MQTT command or from the
// automatic rules in loop().
void SetActuatorStates();

//Takes the readings already taken this loop() iteration instead of
//re-reading the DHT22 (loop() already validated they're not NaN).
void PublishDhtReadings(float temperature, float humidity);

void PublishPirReading(int pirValue);
void PublishLdrReading(int ldrValue);
void PublishServoState();
