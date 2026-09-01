#include <Arduino.h>

#include <DHT.h>
#include <RTClib.h>
#include <ESP32Servo.h>
#include <math.h>

#include "net.h"  // WiFi + ThingSpeak (telemetry upload + command poll)

//Servo set up
Servo myServo;
#define SERVOPIN 23

//DHT22 Sensor SetUp
#define DHTPIN 15
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

//Air Con Pin SetUp
int HOTLEDPIN = 13;
int COLDLEDPIN = 14;

//Air Con States
enum systemState {normal, cooling, heating};
systemState state = normal;

//LDR
#define LDRPIN 4

//RTC
RTC_DS3231 rtc;

//PIR
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

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Initialsing DHT22 sensor...");

  dht.begin();
  Serial.println("DHT22 Sensor Initialised");

  //Air Conditioner Pins
  pinMode(HOTLEDPIN, OUTPUT);
  pinMode(COLDLEDPIN, OUTPUT);

  //LDR pins
  pinMode(LDRPIN, INPUT);

  //Buzzer pin
  pinMode(BUZZERPIN, OUTPUT);

  //RTC connection check
  if(!rtc.begin()){
    Serial.println("Couldn't find rtc");
    while(1);
  }
  //RTC powerloss
  if (rtc.lostPower()){
    Serial.println("RTC lost power");
    rtc.adjust(DateTime(__DATE__, __TIME__));
  }
  //rtc.adjust(DateTime(2026,8,26,1,0,0)); //Un comment for 1am (bed)
  rtc.adjust(DateTime(2026,8,26,7,0,0)); //Un comment for 7:00am (rising)
  //rtc.adjust(DateTime(2026,8,26,8,1,0)); //Un comment for 8:01am (wake)
  //rtc.adjust(DateTime(2026,8,26,20,1,0)); //Un comment for 8:01pm (winddown)
  Serial.println("RTC initialised");

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
  //Gets value from DHT22 sensor
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  //User values
  float mintemp = 18.0;
  float maxtemp = 24.0;
  float desiredtemp = 21.0;

  //Air con values
  float hysteresis = 1.0;

  //LDR
  int ldrval = analogRead(LDRPIN);

  //RTC
  DateTime now = rtc.now();
  
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

  //PIR
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

  //DHT22
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

  //Network layer: push telemetry, pull remote commands (both rate-limited
  //inside netTick). NOTE: LDRPIN (GPIO4) is on ADC2, which the ESP32
  //cannot sample while WiFi is active - ldrval will read 0 here until the
  //LDR is moved to an ADC1 pin (32-39). Tracked as firmware task #1.
  Telemetry tele;
  tele.temperature  = temperature;
  tele.humidity     = humidity;
  tele.light        = 100.0f * ldrval / 4095.0f;
  tele.climateState = (int)state;
  tele.timeState    = tsTimeState(timeState);
  tele.motion       = (PIRval == HIGH);

  Command cmd = netTick(tele);
  //TODO(firmware team): apply `cmd` (mode / setpoints / blinds / lights
  //overrides) to the control logic above once task #1 lands. For now the
  //remote command is fetched and logged only.
  (void)cmd;

  //Time Based Rules Using RTC
  if (timeState == rising && faded == false) {
    fadeUp = true;
    fadeDown = false;
  }
  //Lights fade on
  if (fadeUp && millis() - previousPWMTime >= fadeSpeed) {
    previousPWMTime = millis();
    analogWrite(PWMPIN, pwmval);
    myServo.write(floor(pwmval/1.41));
    Serial.println(pwmval);
    Serial.println(floor(pwmval/1.41));
    pwmval++;
    if (pwmval == 254) {
      tone(BUZZERPIN, 500);
    }
    if (pwmval > 255) {
      pwmval = 255;
      fadeUp = false;
      faded = true;
    }
  }
  if (timeState == winddown) {
    fadeDown = true;
    fadeUp = false;
    faded = false;
  }
  //Lights fade off
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

  if (timeState == wake) {
    digitalWrite(PWMPIN, LOW);
  }
}