#include "net.h"
#include "config.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

namespace {

const char* TS_BASE = "https://api.thingspeak.com";

WiFiClientSecure secure;

// Sensible defaults so the firmware has something to run with before the
// first successful poll (matches the constants in main.cpp).
Command lastCmd = {0, 21.0f, 18.0f, 24.0f, 0, 0, false};

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

  // ThingSpeak returns each field as a string, or null when never set.
  auto num = [&](const char* key, float fallback) -> float {
    JsonVariant v = doc[key];
    if (v.isNull()) return fallback;
    return v.as<String>().toFloat();
  };

  lastCmd.mode        = (int)num("field1", lastCmd.mode);
  lastCmd.desiredTemp =      num("field2", lastCmd.desiredTemp);
  lastCmd.minTemp     =      num("field3", lastCmd.minTemp);
  lastCmd.maxTemp     =      num("field4", lastCmd.maxTemp);
  lastCmd.blinds      = (int)num("field5", lastCmd.blinds);
  lastCmd.lights      = (int)num("field6", lastCmd.lights);
  lastCmd.valid = true;

  Serial.printf("[net] command: mode=%d desired=%.1f min=%.1f max=%.1f blinds=%d lights=%d\n",
                lastCmd.mode, lastCmd.desiredTemp, lastCmd.minTemp,
                lastCmd.maxTemp, lastCmd.blinds, lastCmd.lights);
}

}  // namespace

void netBegin() {
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

  Serial.println("[net] started, connecting to WiFi");
}

Command netTick(const Telemetry& t) {
  ensureWifi();

  if (WiFi.status() == WL_CONNECTED) {
    unsigned long now = millis();
    if (lastUpload == 0 || now - lastUpload >= TS_UPLOAD_INTERVAL_MS) {
      lastUpload = now;
      uploadTelemetry(t);
    }
    if (lastPoll == 0 || now - lastPoll >= TS_POLL_INTERVAL_MS) {
      lastPoll = now;
      pollCommands();
    }
  }

  return lastCmd;
}

Command netLastCommand() {
  return lastCmd;
}
