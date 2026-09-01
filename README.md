# Healthy Home

ESP32 smart-home controller simulated in [Wokwi](https://wokwi.com).
Originally built at https://wokwi.com/projects/473567080028025857

Sensors/actuators: DHT22 (temp/humidity), photoresistor (LDR), PIR motion
sensor, DS3231 RTC, servo, buzzer, and status LEDs — driven by time-of-day
rules and a hysteresis-based heating/cooling state machine.

## Develop in VS Code

Requires the **PlatformIO IDE** and **Wokwi for VS Code** extensions
(VS Code will prompt to install the recommended ones on first open).

1. Build the firmware: PlatformIO toolbar → **Build** (or `pio run`).
2. Start the simulation: open `diagram.json` (or run the command
   **Wokwi: Start Simulator**). Wokwi loads the firmware built at
   `.pio/build/esp32dev/firmware.bin`.
3. Rebuild after code changes, then restart the simulator.

A Wokwi license (free for personal use) is needed the first time the
extension runs: command **Wokwi: Request a new License**.

## Layout

| File | Purpose |
|------|---------|
| `src/main.cpp` | Firmware (converted from the original `sketch.ino`) |
| `diagram.json` | Wokwi circuit definition |
| `wokwi.toml` | Points the Wokwi extension at the built firmware |
| `platformio.ini` | Board (`esp32dev`) and library dependencies |
