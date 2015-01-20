/*
  Use the Marquee function to make an LED-sign type display

  This marquee function scrolls in all directions, random distances for each
  direction. If you edit the code in the "switch" statement below then you can
  make a simpler marquee that only scrolls in one direction.
*/
#include <SPI.h>
#include <DMD2.h>
#include <fonts/Arial14.h>

/* For "Hello World" as your message, leave the width at 4 even if you only have one display connected */
#define DISPLAYS_WIDE 3
#define DISPLAYS_HIGH 1

SoftDMD dmd(DISPLAYS_WIDE,DISPLAYS_HIGH);
DMD_TextBox box(dmd, 0, 0, 32, 16);

// the setup routine runs once when you press reset:
void setup() {
  dmd.setBrightness(255);
  dmd.selectFont(Arial14);
  dmd.begin();
  /* TIP: If you want a longer string here than fits on your display, just define the display DISPLAYS_WIDE value to be wider than the
    number of displays you actually have.
   */
  dmd.drawString(0, 0, F("Hello World!"));
}

int phase = 0; // 0-3, 'phase' value determines direction

// the loop routine runs over and over again forever:
void loop() {
  int steps = random(48); // Each time we scroll a random distance
  for(int i = 0; i < steps; i++) {
    // Do a different type of scroll, depending on which phase we are in
    switch(phase) {
      case 0:
       dmd.marqueeScrollX(1); break;
      case 1:
       dmd.marqueeScrollX(-1); break;
      case 2:
       dmd.marqueeScrollY(1); break;
      case 3:
       dmd.marqueeScrollY(-1); break;
    }
    delay(10);
  }

  // Move to the next phase
  phase = (phase + 1) % 4;
}
