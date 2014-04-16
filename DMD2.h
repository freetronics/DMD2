/*--------------------------------------------------------------------------------------

 DMD2.h   - Arduino library for the Freetronics DMD, a 512 LED matrix display
           panel arranged in a 32 x 16 layout.

 This is a non-compatible replacement for the original DMD library.
*/
#ifndef DMD2_H
#define DMD2_H

#include "Print.h"
#include "SPI.h"

enum DMDTestPattern {
  PATTERN_ALT_0,
  PATTERN_ALT_1,
  PATTERN_STRIPE_0,
  PATTERN_STRIPE_1
};

class BaseDMD
{
protected:
  BaseDMD(byte panelsWide, byte panelsHigh, byte pin_noe, byte pin_a, byte pin_b, byte pin_sck);

  virtual void writeSPIData(uint8_t *rows[4], const int rowsize) = 0;
public:
  virtual void initialize();

  /* Refresh the display by manually scanning out current array of
     pixels. Call often, or use begin()/end() to automatically scan
     the display (see below.)
  */
  virtual void scanDisplay();

  /* Automatically start/stop scanning of the display output at
     flicker-free speed.  Setting this option will use Timer2 on
     AVR-based Arduinos or Timer3 on Arduino Due.
  */
  void begin();
  void end();

  // Set a single LED on or off
  void setPixel(unsigned int x, unsigned int y, const bool on);

  // Fill the screen on or off
  void fillScreen(bool on);
  inline void clearScreen() { fillScreen(false); };

  // Drawing primitives
  void drawLine(int x1, int y1, int x2, int y2, bool on = true);
  void drawCircle(int xCenter, int yCenter, int radius, bool on = true);
  void drawBox(int x1, int y1, int x2, int y2, bool on = true);
  void drawFilledBox(int x1, int y1, int x2, int y2, bool on = true);
  void drawTestPattern(DMDTestPattern pattern);


protected:
  uint8_t *bitmap;
  byte scan_row;
  byte width; // in displays not pixels
  byte height; // in displays not pixels

  byte pin_noe;
  byte pin_a;
  byte pin_b;
  byte pin_sck;

  bool default_pins; // shortcut for default pin behaviour, can use macro writes
  byte pin_other_cs; // CS pin to check before SPI behaviour, only makes sense for SPIDMD
};

class SPIDMD : public BaseDMD
{
public:
  /* Create a single DMD display */
  SPIDMD();
  /* Create a DMD display using the default pinout, panelsWide x panelsHigh panels in size */
  SPIDMD(byte panelsWide, byte panelsHigh);
  /* Create a DMD display using a custom pinout for all the non-SPI pins (SPI pins set by hardware) */
  SPIDMD(byte panelsWide, byte panelsHigh, byte pin_noe, byte pin_a, byte pin_b, byte pin_sck);

  void initialize();

  /* Set the "other CS" pin that is checked for in use before scanning the DMD */
  void setOtherCS(byte pin_other_cs) { this->pin_other_cs = pin_other_cs; }

protected:
  void writeSPIData(uint8_t *rows[4], const int rowsize);
};

class SoftDMD : public BaseDMD
{
public:
  SoftDMD(byte panelsWide, byte panelsHigh);
  SoftDMD(byte panelsWide, byte panelsHigh, byte pin_noe, byte pin_a, byte pin_b, byte pin_sck,
          byte pin_clk, byte pin_r_data);

  void initialize();

protected:
  void writeSPIData(uint8_t *rows[4], const int rowsize);
private:
  inline void softSPITransfer(uint8_t byte) __attribute__((always_inline));
  byte pin_clk;
  byte pin_r_data;
};

#endif
