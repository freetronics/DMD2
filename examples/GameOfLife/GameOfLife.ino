/*
  Game of Life display

  Simulates Conway's Game of Life
  https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life
 */

#include <SPI.h>
#include <DMD2.h>

// How many displays do you have?
const int WIDTH = 2;
const int HEIGHT = 1;

SoftDMD dmd(WIDTH,HEIGHT);

void populate_random_cells() {
  // Populate the initial display randomly
  for(int x = 0; x < dmd.width; x++) {
    for(int y = 0; y < dmd.height; y++) {
      if(random(100) < 30) // Increase 30 to a higher number to set more initial pixels
        dmd.setPixel(x,y,GRAPHICS_ON);
    }
  }
}

// the setup routine runs once when you press reset:
void setup() {
  Serial.begin(9600);
  dmd.setBrightness(255);
  dmd.begin();

  randomSeed(analogRead(0));
  populate_random_cells();
}

// the loop routine runs over and over again forever:
void loop() {
  // Store the current generation by copying the current DMD frame contents
  DMDFrame current_generation(dmd);

  long start = millis();

  // Update next generation of every pixel
  bool change = false;
  for(int x = 0; x < dmd.width; x++) {
    for(int y = 0; y < dmd.height; y++) {
      bool state = current_generation.getPixel(x,y);
      int live_neighbours = 0;

      // Count how many live neighbours we have in the current generation
      for(int nx = x - 1; nx < x + 2; nx++) {
        for(int ny = y - 1; ny < y + 2; ny++) {
          if(nx == x && ny == y)
            continue;
          if(current_generation.getPixel(nx,ny))
            live_neighbours++;
        }
      }

      // Update pixel count for the next generation
      if(state && (live_neighbours < 2 || live_neighbours > 3)) {
        state = false;
        change = true;
      }
      else if(!state && (live_neighbours == 3)) {
        state = true;
        change = true;
      }
      dmd.setPixel(x,y,state ? GRAPHICS_ON : GRAPHICS_OFF);
    }
  }

  Serial.println(String("Generation time: ") + (millis() - start) + " ms");

  if(!change) {
    // We've made it to an unchanging state
    delay(500);
    populate_random_cells();
    // (We can't detect steady states where things change forward
    // and back, for these you need to press reset!)
  }
}
