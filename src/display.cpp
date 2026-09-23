#include "display.h"

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "pins.h"

namespace {

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
bool oledReady = false;

}  // namespace

void SetupDisplay() {
  Wire.begin();
  oledReady = display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (!oledReady) {
    Serial.println("SSD1306 init failed - continuing without display");
  } else {
    display.clearDisplay();
    display.display();
  }
}

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
