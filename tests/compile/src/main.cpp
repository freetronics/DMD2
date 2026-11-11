#include <Arduino.h>
#include <SPI.h>
#include <DMD2.h>
#include <fonts/SystemFont5x7.h>

SPIDMD display(1, 1);

void setup() {
  display.setBrightness(128);
  display.selectFont(SystemFont5x7);
  display.begin();
  display.drawString(0, 0, "DMD2");
}

void loop() {}
