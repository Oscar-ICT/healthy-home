#pragma once
#include <DHT.h>
#include "pins.h"

//DHT22 Sensor SetUp
#define DHTTYPE DHT22

extern DHT dht;

void SetupDht();
