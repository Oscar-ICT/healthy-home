#include <Arduino.h>
#include <DHT.h>
#include <RTClib.h>
#include <ESP32Servo.h>
#include <math.h> // Used for floor() function
#include "net.h"  // WiFi + ThingSpeak (telemetry upload + command poll)
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>

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

//Alarm
int BUZZERPIN = 25;

//Overhead light PWM pin and settings
int PWMPIN = 27;
int pwmval = 0;
bool fadeUp = false;
bool fadeDown = false;
bool faded = false;
const int fadeSpeed = 10;
unsigned long previousPWMTime = 0;

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
void PublishDhtReadings()
{
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity))
  {
    Serial.println("Failed to read from DHT22 sensor.");
    return;
  }

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

  dht.begin();

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

  // Sensor error checks (may need to include all sensors in the future)

  //DHT22 sensor error check
  if (isnan(humidity) || isnan(temperature)){ //Prevents read failure
    Serial.println("Failed to read from DHT22 sensor");
    return;
  }

  // LDR sensor error check
  if (isnan(ldrval)){ //Prevents read failure
    Serial.println("Failed to read from LDR sensor");
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

  //Rising time state triggers fade up of lights
  //State control
  if (timeState == rising && faded == false) {
    fadeUp = true;
    fadeDown = false;
  }
  //Lights fade on logic
  if (fadeUp && millis() - previousPWMTime >= fadeSpeed) {
    previousPWMTime = millis();
    analogWrite(PWMPIN, pwmval);
    myServo.write(floor(pwmval/1.41));
    Serial.println(pwmval);
    Serial.println(floor(pwmval/1.41));
    pwmval++;
    if (pwmval == 254) {
      tone(BUZZERPIN, 500, 500);
    }
    if (pwmval >= 255) {
      pwmval = 255;
      fadeUp = false;
      faded = true;
    }
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

  if (timeState == winddown && !fadeDown && pwmval > 0) {
    fadeDown = true;
    fadeUp = false;
    faded = false;
  }

  //Lights fade off winddown logic
  if (fadeDown && millis() - previousPWMTime >= fadeSpeed) {
    previousPWMTime = millis();
    analogWrite(PWMPIN, pwmval);
    Serial.println(pwmval);
    myServo.write(floor(pwmval/1.41));
    pwmval--;
    if (pwmval < 0) {
      pwmval = 0;
      fadeDown = false;
    }
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
    PublishDhtReadings();
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

  // Remember start time
  unsigned long uploadStart = millis();

  Command cmd = netTick(tele);

  // Blocking HTTPS calls can take a couple of seconds; without this the
  // demo clock would silently "lose" that time on every upload/poll and
  // drift out of sync with the elapsed real time. Shifting the base
  // forward excludes it, so the demo clock only counts time spent
  // actually running the local simulation.
  unsigned long uploadDuration = millis() - uploadStart;
  demoClockBaseMillis += uploadDuration;

  //TODO(firmware team): apply `cmd` (mode / setpoints / blinds / lights
  //overrides) to the control logic above once task #1 lands. For now the
  //remote command is fetched and logged only.
  (void)cmd;
}