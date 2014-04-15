/*--------------------------------------------------------------------------------------

 DMD2.h   - Arduino library for the Freetronics DMD, a 512 LED matrix display
           panel arranged in a 32 x 16 layout.

 This is a non-compatible replacement for the original DMD library.
*/
#ifndef DMD2_H
#define DMD2_H

#include "Print.h"
#include "SPI.h"

class BaseDMD
{
protected:
  BaseDMD(byte panelsWide, byte panelsHigh, byte pin_noe, byte pin_a, byte pin_b, byte pin_sck);

public:
  virtual void initialize();

  /* Refresh the display by manually scanning out current array of
     pixels. Call often, or use begin()/end() to automatically scan
     the display (see below.)
  */
  virtual void scanDisplay() = 0;

  /* Automatically start/stop scanning of the display output at
     flicker-free speed.  Setting this option will use Timer1 on
     AVR-based Arduinos or Timer3 on Arduino Due.
  */
  void begin();
  void end();

  // Set a single LED on or off
  void setPixel(unsigned int x, unsigned int y, const bool on);

  // Fill the screen on or off
  void fillScreen(bool on);
  void clearScreen() { fillScreen(false); };

protected:
  uint8_t *bitmap;
  byte scan_row;
  byte width; // in displays not pixels
  byte height; // in displays not pixels

  byte pin_noe;
  byte pin_a;
  byte pin_b;
  byte pin_sck;
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
  void scanDisplay();

  /* Set the "other CS" pin that is checked for in use before scanning the DMD */
  void setOtherCS(byte pin_other_cs) { this->pin_other_cs = pin_other_cs; }

private:
  bool default_pins; // shortcut for default pin behaviour, can use macro writes
  byte pin_other_cs;
};

class SoftDMD : public BaseDMD
{
public:
  SoftDMD(byte panelsWide, byte panelsHigh);
  SoftDMD(byte panelsWide, byte panelsHigh, byte pin_noe, byte pin_a, byte pin_b, byte pin_sck,
          byte pin_clk, byte pin_r_data);

  void initialize();
  void scanDisplay();

private:
  byte pin_clk;
  byte pin_r_data;
};

#endif
