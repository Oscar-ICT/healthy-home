#include <Arduino.h>
#include <DHT.h>
#include <RTClib.h>

#include "climate_control.h"
#include "daily_routine.h"
#include "display.h"
#include "mqtt_client.h"
#include "mqtt_publish.h"
#include "net.h" // WiFi + ThingSpeak (telemetry upload + command poll)
#include "occupancy.h"
#include "pins.h"
#include "schedule.h"
#include "servo_control.h"

//DHT22 Sensor SetUp
DHT dht(DHTPIN, DHTTYPE);

void SetupDht()
{
  dht.begin();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  SetupDisplay();
  SetupClimate();
  SetupDailyRoutine();
  SetupOccupancy();

  //MQTT setup
  SetupDht();
  SetupServo();
  ConnectToWiFi();
  SetupMqtt();
  ConnectToMqtt();

  //RTC (or the sped-up demo clock) - see schedule.h for DEMO_MODE
  SetupSchedule();

  //Servo (redundant re-attach, kept as in the original)
  myServo.attach(SERVOPIN);

  //Network: WiFi + ThingSpeak (spawns the net task on core 0)
  netBegin();
}

void loop() {
  delay(2000); // this speeds up the simulation

  static unsigned long lastSensorPublishTime = 0;

  // Sensor readings

  //Gets value from DHT22 sensor
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  //User values can be changed to read from ThingSpeak channel
  float mintemp = 18.0;
  float maxtemp = 24.0;

  //Air con values
  float hysteresis = 1.0;

  //RTC (or the sped-up demo clock - see schedule.h for DEMO_MODE)
  DateTime now = GetScheduleTime();

  //LDR
  int ldrval = analogRead(LDRPIN);

  Serial.print(now.hour());
  Serial.print(":");

  if (now.minute() < 10) {
    Serial.print("0");
  }

  Serial.print(now.minute());
  Serial.print(":");

  if (now.second() < 10) {
    Serial.print("0");
  }

  Serial.println(now.second());

  // Manage States

  const bool customMode = CustomModeActive();

  //RTC time state management
  UpdateTimeState(now, customMode);
  TimeState timeState = CurrentTimeState();

  updateDisplay(now, timeState);

  // Sensor error checks (may need to include all sensors in the future)

  //DHT22 sensor error check
  if (isnan(humidity) || isnan(temperature)){ //Prevents read failure
    Serial.println("Failed to read from DHT22 sensor");
    return;
  }

  //Air con state management
  UpdateClimateState(temperature, mintemp, maxtemp, hysteresis, customMode);

  //Time Based Rules Using RTC: lighting, blinds, alarm and PIR motion
  int pirValue = UpdateDailyRoutine(now, timeState, ldrval, customMode);

  //Air con proxy lights signifying heating (red) and cooling (blue)
  ApplyClimateActuators(customMode);

  //MQTT Communication
  MaintainMqttConnection();

  PublishServoState();
  SetActuatorStates();

  const unsigned long nowMQTT = millis();
  if (nowMQTT - lastSensorPublishTime >= DHT_PUBLISH_INTERVAL_MS)
  {
    lastSensorPublishTime = nowMQTT;
    PublishDhtReadings(temperature, humidity);
    PublishPirReading(pirValue);
    PublishLdrReading(ldrval);
  }

  //Network layer: hand off the latest sample and read back the latest
  //command. Both are just mutex-protected struct copies now - the
  //actual blocking HTTPS calls happen on the net task on core 0, so
  //this never stalls the demo clock and the old MAX_UNCOMPENSATED_MS
  //compensation hack is no longer needed.
  Telemetry tele;
  tele.temperature  = temperature;
  tele.humidity     = humidity;
  tele.light        = 100.0f * ldrval / 4095.0f;
  tele.climateState = (int)CurrentClimateState();
  tele.timeState    = tsTimeState(timeState);
  tele.motion       = (pirValue == HIGH);

  netUpdateTelemetry(tele);
  Command cmd = netGetCommand();

  //TODO(firmware team): apply `cmd` (mode / setpoints / blinds / lights
  //overrides) to the control logic above once task #1 lands. For now the
  //remote command is fetched and logged only.
  (void)cmd;

  //Edge AI: occupancy calibration/prediction (button on CALIB_BUTTON_PIN)
  handleCalibrationButton();
  updateOccupancy(temperature, humidity, tele.light, pirValue == HIGH);
}
