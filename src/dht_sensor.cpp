#include "dht_sensor.h"

DHT dht(DHTPIN, DHTTYPE);

void SetupDht()
{
dht.begin();
}
