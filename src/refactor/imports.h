// all of the imports 
#include <Arduino.h>
#include <DHT.h>
#include <RTClib.h>
#include <ESP32Servo.h>
#include <math.h> // Used for floor() function
#include "net.h"  // WiFi + ThingSpeak (telemetry upload + command poll)
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* THIS IS ALL OF THE STUFF WE'vVE WRITTEN */

// Everything "edge ai" related for the marks here
#include "../decision_tree.h"
#include "../decision_tree_trainer.h"

// all of the refactor files
#include "constants.h"