#include <Arduino.h>
#include <RTClib.h>
#include <math.h> // Used for floor() function
#include "net.h"  // WiFi + ThingSpeak (telemetry upload + command poll)
#include <WiFi.h>
#include <DHTesp.h>
#include <Wire.h>

#include "pins.h"
#include "time_state.h"
#include "shared_state.h"
#include "demo_clock.h"
#include "display.h"
#include "dht_sensor.h"
#include "servo_control.h"
#include "mqtt_client.h"
#include "mqtt_publish.h"
#include "occupancy.h"

const unsigned long DHT_PUBLISH_INTERVAL_MS = 5000;
const unsigned long MQTT_RETRY_INTERVAL_MS = 5000;

unsigned long lastSensorPublishTime = 0;

//RTC set up
RTC_DS3231 rtc;

//LDR setup
bool lightOn = false;

//PIR set up
int PIRSTATE = LOW;
int PIRval = 0;

//Time states
TimeState timeState = day;

//Alarm
bool alarmFiredThisCycle = false; //ensures the wake alarm fires once per lap, not once per loop()

void setup() {
  Serial.begin(115200);
  delay(1000);

  //OLED display (shares the I2C bus with the RTC - SDA=21, SCL=22)
  Wire.begin();
  oledReady = display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (!oledReady) {
    Serial.println("SSD1306 init failed - continuing without display");
  } else {
    display.clearDisplay();
    display.display();
  }

  //Air Conditioner Pins
  pinMode(HOTLEDPIN, OUTPUT);
  pinMode(COLDLEDPIN, OUTPUT);

  //LDR pins
  pinMode(LDRPIN, INPUT);

  //Buzzer pin. tone() defaults to LEDC channel 0, which is the same channel
  //ESP32Servo hands the servo - firing the alarm re-routes channel 0's output
  //from the servo pin to the buzzer and never routes it back, leaving the
  //servo frozen while write() keeps silently updating currentServoAngle.
  //Channel 4 is unclaimed: the servo takes 0, analogWrite allocates from 15 down.
  setToneChannel(4);
  pinMode(BUZZERPIN, OUTPUT);

  //Occupancy calibration button
  pinMode(CALIB_BUTTON_PIN, INPUT_PULLUP);
  Serial.println("[occupancy] CALIBRATING: short-press toggles label, long-press (>1.5s) trains.");

  //MQTT setup
  SetupDht();
  SetupServo();
  ConnectToWiFi();
  SetupMqtt();
  ConnectToMqtt();

#if DEMO_MODE
  // Demo mode drives the clock from millis(), not the DS3231 - no RTC
  // hardware dependency, so a missing/faulty RTC can't hang the demo.
  demoClockBaseMillis = millis();
  Serial.print("DEMO MODE: 24h compressed into ");
  Serial.print(DEMO_CYCLE_MINUTES);
  Serial.print(" min (");
  Serial.print(DEMO_SPEED, 0);
  Serial.println("x speed), loops back to 00:00 automatically");
#else
  //RTC connection check
  if(!rtc.begin()){
    while(1);
  }
  //RTC powerloss
  if (rtc.lostPower()){
    Serial.println("RTC lost power");
    rtc.adjust(DateTime(__DATE__, __TIME__));
  }
  //rtc.adjust(DateTime(2026,8,26,1,0,0)); //Un comment for 1am (bed)
  rtc.adjust(DateTime(2026,8,26,7,0,0)); //Un comment for 7:00am (rising)
  //rtc.adjust(DateTime(2026,8,26,8,0,0)); //Un comment for 8:01am (wake)
  //rtc.adjust(DateTime(2026,8,26,9,0,0)); //Un comment for 9am (day)
  //rtc.adjust(DateTime(2026,8,26,20,0,0)); //Un comment for 8:01pm (winddown)
  Serial.println("RTC initialised");
#endif

  //PIR
  pinMode(PIRLEDPIN, OUTPUT);
  pinMode(PIRPIN, INPUT);

  //PWM PIN
  pinMode(PWMPIN, OUTPUT);

  //Network: WiFi + ThingSpeak (spawns the net task on core 0)
  netBegin();
}

void loop() {
  // Poll the button on every pass through loop() (not gated behind the
  // 2s cycle below) so short-press vs long-press timing is accurate.
  // Previously this only ran once every ~2s because of a blocking
  // delay(2000) at the top of loop(), which meant a normal quick press
  // almost always looked like it spanned the whole gap between polls
  // and got misread as a long press (button toggle got stuck) or
  // missed the hold window entirely (training never triggered).
  handleCalibrationButton();

  static unsigned long lastCycleMillis = 0;
  unsigned long nowMillis = millis();
  if (nowMillis - lastCycleMillis < 2000) {
    return; // re-enter loop() immediately instead of blocking with delay()
  }
  lastCycleMillis = nowMillis;

  // Sensor readings

  //Gets value from DHT22 sensor
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  //User values can be changed to read from ThingSpeak channel
  float mintemp = 18.0;
  float maxtemp = 24.0;
  float desiredtemp = 21.0;

  //Air con values
  float hysteresis = 1.0;

  //RTC (or the sped-up demo clock - see DEMO_MODE above)
#if DEMO_MODE
  unsigned long elapsedRealMs = millis() - demoClockBaseMillis;
  uint32_t simSeconds = (uint32_t)((elapsedRealMs / 1000.0f) * DEMO_SPEED);
  simSeconds %= 86400UL; // wraps back to 00:00 after one simulated day
  DateTime now = DEMO_START + TimeSpan((int32_t)simSeconds);
#else
  DateTime now = rtc.now();
#endif

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

  //RTC time state management
  if (!customMode){
    if(now.hour() == 7){
      timeState = rising;
    } else if (now.hour() == 8){
      timeState = wake;
    } else if (now.hour() == 20){
      timeState = winddown;
    } else if (now.hour() >= 21 || now.hour() < 7){
      timeState = bed;
    } else {
      timeState = day;
    }
  }

  updateDisplay(now, timeState);

  // Sensor error checks (may need to include all sensors in the future)

  //DHT22 sensor error check
  if (isnan(humidity) || isnan(temperature)){ //Prevents read failure
    Serial.println("Failed to read from DHT22 sensor");
    return;
  }

  //Air con state management
  if (!customMode){
    switch (state) {
      case cooling:
        if (temperature < maxtemp - hysteresis) { //Cooling state that switches off at 23
          state = normal;
        }
        break;
      case heating:
        if (temperature > mintemp + hysteresis) { //Heating state that switches off at 19
          state = normal;
        }
        break;
      case normal:
        if (temperature > maxtemp) {
          state = cooling;
        }                                          //Normal state checking for temperature breach
        else if (temperature < mintemp) {
          state = heating;
        }
        break;
    }
  } else {
    state = custom;
  }

  //Time Based Rules Using RTC
  // Lighting and Blinds control based on time of day

  //Rising: fade the light up across the hour, driven by the clock
  //itself rather than a per-loop increment (see fadeLevel() above).
  if(!customMode){
    if (timeState == rising) {
      alarmFiredThisCycle = false; //arm the wake alarm for this lap
      pwmval = fadeLevel(now, true);
      analogWrite(PWMPIN, pwmval);
      int servoAngle = floor(pwmval / 1.41);
      SetServoAngle(servoAngle);
    }

    //Wake: alarm fires once, right as the state is entered.
    if (timeState == wake && !alarmFiredThisCycle) {
      tone(BUZZERPIN, 500, 500);
      alarmFiredThisCycle = true;
    }

    //LDR Logic for wake/day time states
    if (!lightOn && ldrval > LDR_THRESHOLD) {
      lightOn = true;
      Serial.println("Light ON");
    } else if (lightOn && ldrval < LDR_THRESHOLD_OFF) {
      lightOn = false;
      Serial.println("Light OFF");
    }

    // LDR controlling light after rising
    if (timeState == day || timeState == wake) {
      SetServoAngle(180);
      if (lightOn) {
        analogWrite(PWMPIN, 255);
      } else {
        analogWrite(PWMPIN, 0);
      }
    }

    //Winddown: fade the light back down across the hour, same
    //clock-driven approach as the rising fade.
    if (timeState == winddown) {
      pwmval = fadeLevel(now, false);
      analogWrite(PWMPIN, pwmval);
      int servoAngle = floor(pwmval / 1.41);
      SetServoAngle(servoAngle);    }

    if (timeState == bed) {
      analogWrite(PWMPIN, 0);
      SetServoAngle(0);
      lightOn = false;
    }

    //PIR Motion Detector
    PIRval = digitalRead(PIRPIN);
    if(PIRval == HIGH && (timeState == bed || timeState == winddown)){
      digitalWrite(PIRLEDPIN, HIGH);
      if (PIRSTATE == LOW){
        Serial.println("Motion Detected"); //Motion sensor LED turns on
        PIRSTATE = HIGH;
      }
    } else {
      digitalWrite(PIRLEDPIN, LOW);
      if (PIRSTATE == HIGH){
        Serial.println("Motion Ended");
      }
      PIRSTATE = LOW;
    }
  }

  //Air con proxy lights signifying heating (red) and cooling (blue)
  if (!customMode){
    if (state == cooling) {
      digitalWrite(COLDLEDPIN, HIGH);
      digitalWrite(HOTLEDPIN, LOW);
      Serial.println("Cooling Activated!");
    } else if (state == heating) {
      digitalWrite(HOTLEDPIN, HIGH);
      digitalWrite(COLDLEDPIN, LOW);
      Serial.println("Heating Activated!");
    } else {
      digitalWrite(COLDLEDPIN, LOW);
      digitalWrite(HOTLEDPIN, LOW);
      Serial.println("Good Temperature!");
    }
  }

  //MQTT Communication
  if (WiFi.status() != WL_CONNECTED)
  {
    ConnectToWiFi();
  }

  if (!mqttClient.connected())
  {
    ConnectToMqtt();
  }

  // Must run frequently to maintain the MQTT connection and receive messages.
  mqttClient.loop();

  PublishServoState();
  PublishModeState();
  SetActuatorStates();

  const unsigned long nowMQTT = millis();
  if (nowMQTT - lastSensorPublishTime >= DHT_PUBLISH_INTERVAL_MS)
  {
    lastSensorPublishTime = nowMQTT;
    PublishDhtReadings(temperature, humidity);
    PublishPirReading(PIRval);
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
  tele.climateState = (int)state;
  tele.timeState    = tsTimeState(timeState);
  tele.motion       = (PIRval == HIGH);

  netUpdateTelemetry(tele);
  Command cmd = netGetCommand();

  //TODO(firmware team): apply `cmd` (mode / setpoints / blinds / lights
  //overrides) to the control logic above once task #1 lands. For now the
  //remote command is fetched and logged only.
  (void)cmd;

  //Edge AI: occupancy calibration/prediction (button on CALIB_BUTTON_PIN)
  updateOccupancy(temperature, humidity, tele.light, PIRval == HIGH);
  Serial.print(currentServoAngle);
}