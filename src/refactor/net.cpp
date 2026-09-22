#include "net.h"
#include "../config.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

namespace {

const char* TS_BASE = "https://api.thingspeak.com";

WiFiClientSecure secure;

// Shared state - written by the net task, read by loop() (or vice
// versa for telemetry). Each has its own mutex so a read/write on one
// never has to wait on the other.
Telemetry sharedTelemetry = {0, 0, 0, 0, 0, false};
bool telemetryReady = false;
SemaphoreHandle_t telemetryMutex = nullptr;

Command sharedCommand = {0, 21.0f, 18.0f, 24.0f, 0, 0, false};
SemaphoreHandle_t commandMutex = nullptr;

TaskHandle_t netTaskHandle = nullptr;

unsigned long lastUpload  = 0;
unsigned long lastPoll    = 0;
unsigned long lastWifiTry = 0;
bool wifiUp = false;

void ensureWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiUp) {
      Serial.print("[net] WiFi connected, IP ");
      Serial.println(WiFi.localIP());
      wifiUp = true;
    }
    return;
  }

  if (wifiUp) {
    Serial.println("[net] WiFi lost, reconnecting");
    wifiUp = false;
  }
  if (lastWifiTry == 0 || millis() - lastWifiTry > 5000) {
    lastWifiTry = millis();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
}

void uploadTelemetry(const Telemetry& t) {
  String url = String(TS_BASE) + "/update?api_key=" + TS_TELEMETRY_WRITE_KEY +
               "&field1=" + String(t.temperature, 2) +
               "&field2=" + String(t.humidity, 2) +
               "&field3=" + String(t.light, 1) +
               "&field4=" + String(t.climateState) +
               "&field5=" + String(t.timeState) +
               "&field6=" + String(t.motion ? 1 : 0);

  HTTPClient https;
  https.setConnectTimeout(5000);
  https.setTimeout(5000);
  if (!https.begin(secure, url)) {
    Serial.println("[net] upload: begin() failed");
    return;
  }
  int code = https.GET();
  if (code == 200) {
    Serial.print("[net] upload ok, entry #");
    Serial.println(https.getString());
  } else {
    Serial.print("[net] upload failed, HTTP ");
    Serial.println(code);
  }
  https.end();
}

// Polls the control channel and, on success, writes straight into
// sharedCommand under commandMutex (fields that come back empty keep
// their previous value, same as before).
void pollCommands() {
  String url = String(TS_BASE) + "/channels/" + String(TS_CONTROL_CHANNEL_ID) +
               "/feeds/last.json?api_key=" + TS_CONTROL_READ_KEY;

  HTTPClient https;
  https.setConnectTimeout(5000);
  https.setTimeout(5000);
  if (!https.begin(secure, url)) {
    Serial.println("[net] poll: begin() failed");
    return;
  }
  int code = https.GET();
  if (code == 400) {
    // ThingSpeak returns 400 for last.json on a channel with no entries yet.
    Serial.println("[net] poll: control channel empty, keeping defaults");
    https.end();
    return;
  }
  if (code != 200) {
    Serial.print("[net] poll failed, HTTP ");
    Serial.println(code);
    https.end();
    return;
  }
  String body = https.getString();
  https.end();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    Serial.print("[net] poll parse error: ");
    Serial.println(err.c_str());
    return;
  }

  xSemaphoreTake(commandMutex, portMAX_DELAY);

  // ThingSpeak returns each field as a string, or null when never set.
  auto num = [&](const char* key, float fallback) -> float {
    JsonVariant v = doc[key];
    if (v.isNull()) return fallback;
    return v.as<String>().toFloat();
  };

  sharedCommand.mode        = (int)num("field1", sharedCommand.mode);
  sharedCommand.desiredTemp =      num("field2", sharedCommand.desiredTemp);
  sharedCommand.minTemp     =      num("field3", sharedCommand.minTemp);
  sharedCommand.maxTemp     =      num("field4", sharedCommand.maxTemp);
  sharedCommand.blinds      = (int)num("field5", sharedCommand.blinds);
  sharedCommand.lights      = (int)num("field6", sharedCommand.lights);
  sharedCommand.valid = true;

  Command logCopy = sharedCommand;
  xSemaphoreGive(commandMutex);

  Serial.printf("[net] command: mode=%d desired=%.1f min=%.1f max=%.1f blinds=%d lights=%d\n",
                logCopy.mode, logCopy.desiredTemp, logCopy.minTemp,
                logCopy.maxTemp, logCopy.blinds, logCopy.lights);
}

// Runs on core 0 for the life of the program. Everything blocking
// (WiFi reconnects, both HTTPS calls) happens only in here, so loop()
// on core 1 never stalls on the network.
void netTaskFn(void* /*pvParameters*/) {
  for (;;) {
    ensureWifi();

    if (WiFi.status() == WL_CONNECTED) {
      unsigned long now = millis();

      if (lastUpload == 0 || now - lastUpload >= TS_UPLOAD_INTERVAL_MS) {
        lastUpload = now;
        Telemetry snapshot;
        bool haveSample;
        xSemaphoreTake(telemetryMutex, portMAX_DELAY);
        snapshot = sharedTelemetry;
        haveSample = telemetryReady;
        xSemaphoreGive(telemetryMutex);
        if (haveSample) uploadTelemetry(snapshot);
      }

      if (lastPoll == 0 || now - lastPoll >= TS_POLL_INTERVAL_MS) {
        lastPoll = now;
        pollCommands();
      }
    }

    // Small yield so this task doesn't starve WiFi/lower-priority
    // tasks on the same core between its rate-limited HTTPS calls.
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

}  // namespace

void netBegin() {
  telemetryMutex = xSemaphoreCreateMutex();
  commandMutex   = xSemaphoreCreateMutex();

  WiFi.mode(WIFI_STA);
  // main.cpp's own ConnectToWiFi() (for MQTT) usually already connects
  // before this runs. Re-calling WiFi.begin() on an already-connected
  // station forces a disconnect/reconnect, which drops the MQTT socket
  // right as it's set up - only (re)connect here if actually needed.
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
  lastWifiTry = millis();

  // TODO(security): pin the ThingSpeak root CA instead of setInsecure()
  // to get real server authentication for the Security section.
  secure.setInsecure();

  // Pinned to core 0 (the network/protocol core) so it never contends
  // with loop() on core 1 for CPU time.
  xTaskCreatePinnedToCore(netTaskFn, "netTask", 8192, nullptr, 1, &netTaskHandle, 0);

  Serial.println("[net] started, net task running on core 0");
}

void netUpdateTelemetry(const Telemetry& t) {
  xSemaphoreTake(telemetryMutex, portMAX_DELAY);
  sharedTelemetry = t;
  telemetryReady = true;
  xSemaphoreGive(telemetryMutex);
}

Command netGetCommand() {
  Command c;
  xSemaphoreTake(commandMutex, portMAX_DELAY);
  c = sharedCommand;
  xSemaphoreGive(commandMutex);
  return c;
}