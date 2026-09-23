#pragma once
#include <Arduino.h>

void CallbackMqtt(char *topic, byte *payload, unsigned int length);
