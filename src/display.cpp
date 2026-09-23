#include "display.h"
#include "demo_clock.h"

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
