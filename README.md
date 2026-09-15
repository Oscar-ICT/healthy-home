# Healthy Home

Smart-home automation on an ESP32, simulated in [Wokwi](https://wokwi.com).
Combines local sensor-driven automation with cloud telemetry/remote
control (ThingSpeak) and an MQTT link (Node-RED), following the four-layer
IoT architecture (Perception → Network → Processing → Application).

> Originally built at [wokwi.com/projects/473567080028025857](https://wokwi.com/projects/473567080028025857).

## Contents

- [Features](#features)
- [Hardware](#hardware)
- [Getting started](#getting-started)
- [Configuration](#configuration)
- [Running the simulation](#running-the-simulation)
- [Demo mode](#demo-mode)
- [Project structure](#project-structure)
- [Known limitations](#known-limitations)

## Features

- **Closed-loop climate control** — DHT22-driven heating/cooling state
  machine with hysteresis (red/blue LEDs as relay proxies).
- **Time-of-day automation** — light fade-up/down, a wake alarm, PIR-triggered
  night lighting, and blinds (servo), gated by an RTC-driven schedule.
- **Cloud telemetry + remote control** — sensor readings pushed to a
  ThingSpeak channel over HTTPS; a second ThingSpeak channel polled for
  remote climate/lighting commands.
- **MQTT bridge** — temperature/humidity published to a public broker for
  Node-RED integration.
- **On-device status display** — an SSD1306 OLED shows the current time
  and time-of-day phase at a glance.
- **Demo mode** — optionally compresses the full 24h cycle into a few
  minutes of real time, so every time-based behaviour can be shown live
  in a short demo. See [Demo mode](#demo-mode).

## Hardware

| Component | Role | Pin(s) |
|---|---|---|
| DHT22 | Temperature + humidity | GPIO15 |
| Photoresistor (LDR) | Ambient light | GPIO32 (ADC1) |
| PIR motion sensor | Occupancy / night lighting | GPIO18 |
| DS1307/DS3231 RTC | Real-time clock (I2C) | SDA 21, SCL 22 |
| SSD1306 OLED | Status display (I2C, shares the RTC bus) | SDA 21, SCL 22 |
| Servo | Blinds proxy | GPIO23 |
| Buzzer | Wake alarm | GPIO25 |
| LED (red) | Heating indicator | GPIO13 |
| LED (blue) | Cooling indicator | GPIO14 |
| LED (yellow) | PIR motion indicator | GPIO26 |
| LED (white) | Overhead light (PWM) | GPIO27 |

Full wiring is defined in [diagram.json](diagram.json).

> **Note:** the RTC part in the diagram is a `wokwi-ds1307`, while the
> firmware talks to it via the `RTC_DS3231` driver class. This works for
> basic timekeeping but is a known mismatch — see [Known limitations](#known-limitations).

## Getting started

### Prerequisites

- [VS Code](https://code.visualstudio.com/)
- **PlatformIO IDE** and **Wokwi for VS Code** extensions (VS Code will
  offer to install both from [.vscode/extensions.json](.vscode/extensions.json) on first open)
- A free [Wokwi](https://wokwi.com/) account/license (prompted on first simulator run)
- A [ThingSpeak](https://thingspeak.com/) account, if you want telemetry/remote control working

### Setup

1. Clone the repo and open it in VS Code.
2. Copy the config template and fill in your own credentials:
   ```
   cp src/config.h.example src/config.h
   ```
   See [Configuration](#configuration) — **the firmware will not build without this file.**
3. Build: PlatformIO toolbar → **Build** (or `pio run`).

## Configuration

[`src/config.h`](src/config.h.example) is **gitignored** — it holds
per-developer secrets and is never committed. Each teammate creates their
own copy from `config.h.example`.

| Setting | Purpose |
|---|---|
| `WIFI_SSID` / `WIFI_PASSWORD` | WiFi credentials (`Wokwi-GUEST` / empty for the simulator) |
| `TS_TELEMETRY_CHANNEL_ID` / `TS_TELEMETRY_WRITE_KEY` | ThingSpeak channel the device **writes** sensor data to |
| `TS_CONTROL_CHANNEL_ID` / `TS_CONTROL_READ_KEY` | ThingSpeak channel the device **reads** remote commands from |
| `TS_UPLOAD_INTERVAL_MS` / `TS_POLL_INTERVAL_MS` | Upload/poll cadence (ThingSpeak's free tier limits writes to one per 15s) |

Ask a teammate for the current keys rather than generating new ones, so
everyone reads/writes the same channels.

## Running the simulation

1. Build the firmware (PlatformIO toolbar → **Build**, or `pio run`).
2. Start the simulator: **Wokwi: Start Simulator** (command palette), or
   open [diagram.json](diagram.json) and press ▶.
3. Rebuild after any code change, then restart the simulator to pick it up.

Watch the serial monitor for `[net] ...` lines confirming WiFi/ThingSpeak
activity, and the OLED for the current time/phase.

## Demo mode

For presentations/interviews, the firmware can compress the entire 24h
time-of-day cycle into a few minutes so every time-based behaviour
(rising fade, wake alarm, winddown fade, night lighting) plays out live
instead of over a real day.

Controlled by a single flag in [src/main.cpp](src/main.cpp):

```cpp
#define DEMO_MODE 1          // 0 = use the real RTC at real speed
const float DEMO_CYCLE_MINUTES = 4.0f;  // real minutes per simulated 24h day
```

With `DEMO_MODE` on, the clock loops back to `00:00` automatically and the
OLED shows a "DEMO MODE" tag. Set it to `0` to run on the real DS3231 RTC
at real speed for normal operation — no other changes needed.

## Project structure

| File | Purpose |
|---|---|
| `src/main.cpp` | Main firmware: sensors, state machines, time-of-day rules, OLED, MQTT |
| `src/net.h` / `src/net.cpp` | WiFi + ThingSpeak module (telemetry upload, command poll) |
| `src/config.h.example` | Template for `src/config.h` (WiFi + ThingSpeak keys) |
| `diagram.json` | Wokwi circuit definition |
| `wokwi.toml` | Points the Wokwi extension at the built firmware |
| `platformio.ini` | Board (`esp32dev`), build flags, and library dependencies |

## Known limitations

- **RTC part mismatch** — diagram uses `wokwi-ds1307`, firmware uses the
  `RTC_DS3231` driver. Fine for basic timekeeping; matters if DS3231-only
  features (e.g. `lostPower()`) are relied on.
- **Remote commands not yet applied** — the `Command` struct polled from
  the ThingSpeak control channel is fetched and logged but not yet wired
  into the heating/lighting control logic.
- **Blocking WiFi/MQTT reconnects** — `ConnectToWiFi()`/`ConnectToMqtt()`
  in `main.cpp` retry with no timeout; a prolonged outage on either
  service will stall the whole loop.
- **HTTPS uses `setInsecure()`** — encrypts traffic but doesn't verify
  ThingSpeak's certificate. Pinning the root CA is a follow-up for the
  security section of the project.
