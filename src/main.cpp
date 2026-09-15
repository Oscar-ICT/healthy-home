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

// Node Red Connection
const char *WIFI_SSID = "Wokwi-GUEST";
const char *WIFI_PASSWORD = "";

const char *MQTT_SERVER = "broker.hivemq.com";
const uint16_t MQTT_PORT = 1883;
const char *MQTT_COMMAND_TOPIC = "week6/HealthyHome/status/esp32";
const char *MQTT_STATUS_TOPIC = "week6/HealthyHome/status/esp32";
const char *MQTT_TEMPERATURE_TOPIC = "week6/HealthyHome/sensor/temperature";
const char *MQTT_HUMIDITY_TOPIC = "week6/HealthyHome/sensor/humidity";

const unsigned long DHT_PUBLISH_INTERVAL_MS = 5000;
const unsigned long MQTT_RETRY_INTERVAL_MS = 5000;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

unsigned long lastDhtPublishTime = 0;


// Servo set up
Servo myServo;
#define SERVOPIN 23

//DHT22 Sensor SetUp
#define DHTPIN 15
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

//Air Con SetUp
int HOTLEDPIN = 13;
int COLDLEDPIN = 14;

enum systemState {normal, cooling, heating};
systemState state = normal;

//LDR setup
const int LDRPIN = 32;

bool lightOn = false;
const int LDR_THRESHOLD = 1300; // Threshold for light ON
const int LDR_THRESHOLD_OFF = 1200; // Threshold for light OFF

//RTC set up
RTC_DS3231 rtc;

//PIR set up
int PIRLEDPIN = 26;
int PIRPIN = 18;
int PIRSTATE = LOW;
int PIRval = 0;

//Time states
enum TimeState {rising, wake, winddown, bed, day};
TimeState timeState = day;

const char* timeStateName(TimeState s) {
  switch (s) {
    case rising:   return "RISING";
    case wake:     return "WAKE";
    case winddown: return "WINDDOWN";
    case bed:      return "BED";
    case day:      return "DAY";
  }
  return "?";
}

//Computes an 8-bit brightness that ramps across one hour: 0 at the top
//of the hour up to 255 by the end for a rising fade, or the reverse for
//a winddown fade. Driven by the clock's own minute/second instead of
//counting +-1 per loop() call, so it completes correctly whether the
//hour is 3600 real seconds (normal RTC) or ~10 real seconds
//(DEMO_MODE) - a fixed per-loop step could only ever manage the
//former.
int fadeLevel(const DateTime& now, bool rampUp) {
  float fraction = (now.minute() * 60 + now.second()) / 3600.0f;
  if (fraction > 1.0f) fraction = 1.0f;
  int level = (int)(fraction * 255.0f);
  return rampUp ? level : (255 - level);
}

//Alarm
int BUZZERPIN = 25;
bool alarmFiredThisCycle = false; //ensures the wake alarm fires once per lap, not once per loop()

//Overhead light PWM pin and settings
int PWMPIN = 27;
int pwmval = 0;

// ---- Sped-up demo clock ------------------------------------------------
// Compresses a full 24h day into DEMO_CYCLE_MINUTES of real time so every
// time-based behaviour (rising/wake/day/winddown/bed - fades, buzzer,
// servo, PIR night lighting) plays out inside a short demo/interview
// slot instead of waiting for a real day to pass. Loops back to 00:00
// automatically. Set DEMO_MODE to 0 to run on the real DS3231 RTC.
#define DEMO_MODE 1
const float DEMO_CYCLE_MINUTES = 4.0f;
const float DEMO_SPEED = (24.0f * 60.0f * 60.0f) / (DEMO_CYCLE_MINUTES * 60.0f); // simulated seconds per real second
const DateTime DEMO_START(2026, 8, 26, 0, 0, 0);
unsigned long demoClockBaseMillis = 0; // shifted forward to "pause" the demo clock during blocking network calls

//OLED display: shows the (demo) clock + current time-of-day phase so the
//cycle can be followed at a glance during a demo instead of the serial
//monitor. Shares the I2C bus with the RTC (SDA=21, SCL=22).
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
bool oledReady = false;

void updateDisplay(const DateTime& now, TimeState ts) {
  if (!oledReady) return;

  char clockText[9];
  snprintf(clockText, sizeof(clockText), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(clockText);

  display.setTextSize(2);
  display.setCursor(0, 28);
  display.println(timeStateName(ts));

#if DEMO_MODE
  display.setTextSize(1);
  display.setCursor(0, 54);
  display.println("DEMO MODE");
#endif

  display.display();
}

//MQTT Communication
String CreateMqttClientId()
{
  const uint64_t chipId = ESP.getEfuseMac();
  const unsigned long high = static_cast<unsigned long>(chipId >> 32);
  const unsigned long low = static_cast<unsigned long>(chipId);

  char clientId[40];
  snprintf(clientId, sizeof(clientId), "ESP32Client-%08lX%08lX", high, low);
  return String(clientId);
}

void CallbackMqtt(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message received on ");
  Serial.print(topic);
  Serial.print(": ");

  for (unsigned int i = 0; i < length; i++)
  {
    Serial.print(static_cast<char>(payload[i]));
  }

  Serial.println();
}

void SetupDht()
{
dht.begin();
}

void SetupMqtt()
{
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(CallbackMqtt);
}

void ConnectToWiFi()
{
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\nWi-Fi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void ConnectToMqtt()
{
  while (!mqttClient.connected())
  {
    const String clientId = CreateMqttClientId();

    Serial.print("Connecting to MQTT as ");
    Serial.print(clientId);
    Serial.print("... ");

    if (mqttClient.connect(clientId.c_str()))
    {
      Serial.println("connected.");

      if (mqttClient.subscribe(MQTT_COMMAND_TOPIC))
      {
        Serial.print("Subscribed to: ");
        Serial.println(MQTT_COMMAND_TOPIC);
      }
      else
      {
        Serial.println("MQTT subscription failed.");
      }
    }
    else
    {
      Serial.print("failed, state=");
      Serial.println(mqttClient.state());
      Serial.println("Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

//MQTT Publish function
//Takes the readings already taken this loop() iteration instead of
//re-reading the DHT22 (loop() already validated they're not NaN).
void PublishDhtReadings(float temperature, float humidity)
{
  char temperatureText[10];
  char humidityText[10];

  dtostrf(temperature, 1, 1, temperatureText);
  dtostrf(humidity, 1, 1, humidityText);

  const bool temperaturePublished = mqttClient.publish(
    MQTT_TEMPERATURE_TOPIC,
    temperatureText
  );
  const bool humidityPublished = mqttClient.publish(
    MQTT_HUMIDITY_TOPIC,
    humidityText
  );

  Serial.print("Temperature: ");
  Serial.print(temperatureText);
  Serial.print(" C, Humidity: ");
  Serial.print(humidityText);
  Serial.print(" %, MQTT publish: ");
  Serial.println((temperaturePublished && humidityPublished) ? "success" : "failed");
}

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

  //Buzzer pin
  pinMode(BUZZERPIN, OUTPUT);

  //MQTT setup
  SetupDht();
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

  //Servo
  myServo.attach(SERVOPIN);

  //Network: WiFi + ThingSpeak
  netBegin();
}

//Maps the local TimeState enum to the ThingSpeak field5 encoding
//(0 day, 1 rising, 2 wake, 3 winddown, 4 bed).
int tsTimeState(TimeState s) {
  switch (s) {
    case day:      return 0;
    case rising:   return 1;
    case wake:     return 2;
    case winddown: return 3;
    case bed:      return 4;
  }
  return 0;
}

void loop() {
  delay(2000); // this speeds up the simulation

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

  updateDisplay(now, timeState);

  // Sensor error checks (may need to include all sensors in the future)

  //DHT22 sensor error check
  if (isnan(humidity) || isnan(temperature)){ //Prevents read failure
    Serial.println("Failed to read from DHT22 sensor");
    return;
  }

  //Air con state management
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

  //Time Based Rules Using RTC
  // Lighting and Blinds control based on time of day

  //Rising: fade the light up across the hour, driven by the clock
  //itself rather than a per-loop increment (see fadeLevel() above).
  if (timeState == rising) {
    alarmFiredThisCycle = false; //arm the wake alarm for this lap
    pwmval = fadeLevel(now, true);
    analogWrite(PWMPIN, pwmval);
    myServo.write(floor(pwmval / 1.41));
  }

  //Wake: alarm fires once, right as the state is entered.
  if (timeState == wake && !alarmFiredThisCycle) {
    tone(BUZZERPIN, 500, 500);
    alarmFiredThisCycle = true;
  }

  //LDR Logic for wake/day time states
  Serial.print("LDR Value: ");
  Serial.println(ldrval);

  if (!lightOn && ldrval > LDR_THRESHOLD) {
    lightOn = true;
    Serial.println("Light ON");
  } else if (lightOn && ldrval < LDR_THRESHOLD_OFF) {
    lightOn = false;
    Serial.println("Light OFF");
  }

  // LDR controlling light after rising
  if (timeState == day || timeState == wake) {
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
    myServo.write(floor(pwmval / 1.41));
  }

  if (timeState == bed) {
    analogWrite(PWMPIN, 0);
    myServo.write(0);
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

  //Air con proxy lights signifying heating (red) and cooling (blue)
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

  const unsigned long nowMQTT = millis();
  if (nowMQTT - lastDhtPublishTime >= DHT_PUBLISH_INTERVAL_MS)
  {
    lastDhtPublishTime = nowMQTT;
    PublishDhtReadings(temperature, humidity);
  }

  //Network layer: push telemetry, pull remote commands (both rate-limited
  //inside netTick). LDRPIN is now GPIO32 (ADC1), which is unaffected by
  //WiFi, so ldrval reads correctly here.
  Telemetry tele;
  tele.temperature  = temperature;
  tele.humidity     = humidity;
  tele.light        = 100.0f * ldrval / 4095.0f;
  tele.climateState = (int)state;
  tele.timeState    = tsTimeState(timeState);
  tele.motion       = (PIRval == HIGH);

  // Blocking HTTPS calls to ThingSpeak (TLS handshake included) can take
  // several real seconds. Three approaches so far:
  //  - hiding ALL of it from the demo clock made a lap take ~3x longer
  //    in real time than DEMO_CYCLE_MINUTES;
  //  - hiding NONE of it caused single-tick jumps of over an hour of
  //    sim-time - large enough to leap clean over the 1-hour-wide
  //    RISING/WINDDOWN windows and skip them entirely;
  //  - hiding only a small FIXED amount (e.g. 2000ms) barely helped: a
  //    ~10s stall still left ~8s uncompensated, which is what actually
  //    becomes the jump - the cap needs to bound the leftover, not the
  //    hidden part.
  // So: hide everything past MAX_UNCOMPENSATED_MS, letting only that
  // small remainder count. A jump is now bounded to ~MAX_UNCOMPENSATED_MS
  // worth of sim-time (a few sim-minutes) - far too small to skip an
  // hour-wide state - while total lap duration only grows by
  // MAX_UNCOMPENSATED_MS per network call instead of ballooning.
  const unsigned long MAX_UNCOMPENSATED_MS = 500;
  unsigned long uploadStart = millis();
  Command cmd = netTick(tele);
  unsigned long uploadDuration = millis() - uploadStart;
  if (uploadDuration > MAX_UNCOMPENSATED_MS) {
    demoClockBaseMillis += uploadDuration - MAX_UNCOMPENSATED_MS;
  }

  //TODO(firmware team): apply `cmd` (mode / setpoints / blinds / lights
  //overrides) to the control logic above once task #1 lands. For now the
  //remote command is fetched and logged only.
  (void)cmd;
}