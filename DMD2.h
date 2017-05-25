/*--------------------------------------------------------------------------------------

 DMD2.h   - Arduino library for the Freetronics DMD, a 512 LED matrix display
           panel arranged in a 32 x 16 layout.

 Copyright (C) 2014 Freetronics, Inc. (info <at> freetronics <dot> com)

 This is a non-backwards-compatible replacement for the original DMD library.

 Updated by Angus Gratton, based on DMD by Marc Alexander.

---

 This program is free software: you can redistribute it and/or modify it under the terms
 of the version 3 GNU General Public License as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program.
 If not, see <http://www.gnu.org/licenses/>.

*/
#ifndef DMD2_H
#define DMD2_H

#include "Print.h"
#include "SPI.h"

// Dimensions of a single display
const unsigned int PANEL_WIDTH = 32;
const unsigned int PANEL_HEIGHT = 16;

// Clamp a value between two limits
template<typename T> static inline void clamp(T &value, T lower, T upper) {
  if(value < lower)
    value = lower;
  else if(value > upper)
    value = upper;
}

// Swap A & B "in place" (well, with a temp variable!)
template<typename T> static inline void swap(T &a, T &b)
{
  T tmp(a); a=b; b=tmp;
}

// Check a<=b, and swap them otherwise
template<typename T> static inline void ensureOrder(T &a, T &b)
{
  if(b<a) swap(a,b);
}

extern const uint8_t DMD_Pixel_Lut[]; /* Lookup table for the DMD pixel locations */

enum DMDTestPattern {
  PATTERN_ALT_0,
  PATTERN_ALT_1,
  PATTERN_STRIPE_0,
  PATTERN_STRIPE_1
};

//Pixel/graphics writing modes
enum DMDGraphicsMode {
    GRAPHICS_OFF, // unconditionally off (pixel turns off)
    GRAPHICS_ON, //unconditionally on (pixel turns on, the usual default for drawing)
    GRAPHICS_INVERSE, // on if was going to set to off
    GRAPHICS_OR, // add to pixels already on
    GRAPHICS_NOR, // subtract from pixels already on, don't turn any new ones on
    GRAPHICS_XOR, // swap on/off state of pixels
    GRAPHICS_NOOP // No-Op, ie don't actually change anything
};

// Return the inverse/"clear" version of the given mode
// ie for normal pixel-on modes, the "clear" is to turn off.
// for inverse mode, it's to turn on.
// for all other modes, this is kind of meaningless so we return a no-op
inline static DMDGraphicsMode inverseMode(DMDGraphicsMode mode) {
  switch(mode) {
    case GRAPHICS_ON:
      return GRAPHICS_OFF;
    case GRAPHICS_INVERSE:
      return GRAPHICS_ON;
    default:
      return GRAPHICS_NOOP;
  }
};

class DMD_TextBox;

/* DMDFrame is a class encapsulating a framebuffer for the DMD, and all the graphical
   operations associated with it.

   This allows you to implement double buffered/frame flipping, etc.
*/
class DMDFrame
{
  friend class DMD_TextBox;
 public:
  DMDFrame(byte pixelsWide, byte pixelsHigh);
  DMDFrame(const DMDFrame &source);
  virtual ~DMDFrame();

  // Set a single LED on or off
  void setPixel(unsigned int x, unsigned int y, DMDGraphicsMode mode=GRAPHICS_ON);

  // Get status of a single LED
  bool getPixel(unsigned int x, unsigned int y);

  // Move a region of pixels from one area to another
  void movePixels(unsigned int from_x, unsigned int from_y,
                  unsigned int to_x, unsigned int to_y,
                  unsigned int width, unsigned int height);

  // Extract a sub-region of the frame as a new frame
  DMDFrame subFrame(unsigned int left, unsigned int top, unsigned int width, unsigned int height);

  // Copy the contents of another frame back into this one at the given location
  void copyFrame(DMDFrame &from, unsigned int left, unsigned int top);

  // Fill the screen on or off
  void fillScreen(bool on);
  inline void clearScreen() { fillScreen(false); };
  
  // the buffer passed must be large enough for all pixels plus a line ending
  void debugPixelLine(unsigned int y, char *buf);

  // Drawing primitives
  void drawLine(int x1, int y1, int x2, int y2, DMDGraphicsMode mode=GRAPHICS_ON);
  void drawCircle(unsigned int xCenter, unsigned int yCenter, int radius, DMDGraphicsMode mode=GRAPHICS_ON);
  void drawBox(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, DMDGraphicsMode mode=GRAPHICS_ON);
  void drawFilledBox(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, DMDGraphicsMode mode=GRAPHICS_ON);
  void drawTestPattern(DMDTestPattern pattern);

  // Text primitives
  void selectFont(const uint8_t* font);
  const inline uint8_t *getFont(void) { return font; }
  int drawChar(const int x, const int y, const char letter, DMDGraphicsMode mode=GRAPHICS_ON, const uint8_t *font = NULL);

  void drawString(int x, int y, const char *bChars, DMDGraphicsMode mode=GRAPHICS_ON, const uint8_t *font = NULL);
  void drawString(int x, int y, const String &str, DMDGraphicsMode mode=GRAPHICS_ON, const uint8_t *font = NULL);
#if defined(__AVR__) || defined(ESP8266)
  void drawString_P(int x, int y, const char *flashStr, DMDGraphicsMode mode=GRAPHICS_ON, const uint8_t *font = NULL);
  inline void drawString(int x, int y, const __FlashStringHelper *flashStr, DMDGraphicsMode mode=GRAPHICS_ON, const uint8_t *font = NULL) {
    return drawString_P(x,y,(const char*)flashStr, mode, font);
  }
#endif

  //Find the width of a character
  int charWidth(const char letter, const uint8_t *font = NULL);

  //Find the width of a string (width of all characters plus 1 pixel "kerning" between each character)
#if defined(__AVR__) || defined(ESP8266)
  unsigned int stringWidth_P(const char *flashStr, const uint8_t *font = NULL);
  inline unsigned int stringWidth(const __FlashStringHelper *flashStr, const uint8_t *font = NULL) {
    return stringWidth_P((const char*)flashStr, font);
  }
#endif
  unsigned int stringWidth(const char *bChars, const uint8_t *font = NULL);
  unsigned int stringWidth(const String &str, const uint8_t *font = NULL);

  // Scrolling & marquee support
  void scrollY(int scrollBy);
  void scrollX(int scrollBy);
  void marqueeScrollX(int scrollBy);
  void marqueeScrollY(int scrollBy);

  void swapBuffers(DMDFrame &other);

  const byte width; // in pixels
  const byte height; // in pixels
 protected:
  volatile uint8_t *bitmap;
  byte row_width_bytes; // width in bitmap, bit-per-pixel rounded up to nearest byte
  byte height_in_panels; // in panels

  uint8_t *font;

  inline size_t bitmap_bytes() {
    // total bytes in the bitmap
    return row_width_bytes * height;
  }
  inline size_t unified_width_bytes() {
    // controller sees all panels as end-to-end, so bitmap arranges it that way
    return row_width_bytes * height_in_panels;
  }
  inline int pixelToBitmapIndex(unsigned int x, unsigned int y) {
    // Panels seen as stretched out in a row for purposes of finding index
    uint8_t panel = (x/PANEL_WIDTH) + ((width/PANEL_WIDTH) * (y/PANEL_HEIGHT));
    x = (x % PANEL_WIDTH)  + (panel * PANEL_WIDTH);
    y = y % PANEL_HEIGHT;
    int res = x / 8 + (y * unified_width_bytes());
    return res;
  }
  inline uint8_t pixelToBitmask(unsigned int x) {
    int res = pgm_read_byte(DMD_Pixel_Lut + (x & 0x07));
    return res;
  }

  template<typename T> inline void clamp_xy(T &x, T&y) {
    clamp(x, (T)0, (T)width-1);
    clamp(y, (T)0, (T)width-1);
  }
};

class BaseDMD : public DMDFrame
{
protected:
  BaseDMD(byte panelsWide, byte panelsHigh, byte pin_noe, byte pin_a, byte pin_b, byte pin_sck);

  virtual void writeSPIData(volatile uint8_t *rows[4], const int rowsize) = 0;
public:
  /* Refresh the display by manually scanning out current array of
     pixels. Call often, or use begin()/end() to automatically scan
     the display (see below.)
  */
  virtual void scanDisplay();

  /* Automatically start/stop scanning of the display output at
     flicker-free speed.  Setting this option will use Timer1 on
     AVR-based Arduinos or Timer7 on Arduino Due.
  */
  void begin();
  void end();

  /* Start display, but use manual scanning */
  virtual void beginNoTimer();

  inline void setBrightness(byte level) { this->brightness = level; };
protected:
  volatile byte scan_row;
  byte pin_noe;
  byte pin_a;
  byte pin_b;
  byte pin_sck;

  bool default_pins; // shortcut for default pin behaviour, can use macro writes
  int8_t pin_other_cs; // CS pin to check before SPI behaviour, only makes sense for SPIDMD

  uint8_t brightness;

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

  void beginNoTimer();

  /* Set the "other CS" pin that is checked for in use before scanning the DMD */
  void setOtherCS(byte pin_other_cs) { this->pin_other_cs = pin_other_cs; }

protected:
  void writeSPIData(volatile uint8_t *rows[4], const int rowsize);
};

#ifdef ESP8266
// No SoftDMD for ESP8266 for now
#else
class SoftDMD : public BaseDMD
{
public:
  SoftDMD(byte panelsWide, byte panelsHigh);
  SoftDMD(byte panelsWide, byte panelsHigh, byte pin_noe, byte pin_a, byte pin_b, byte pin_sck,
          byte pin_clk, byte pin_r_data);

  void beginNoTimer();

protected:
  void writeSPIData(volatile uint8_t *rows[4], const int rowsize);
private:
  byte pin_clk;
  byte pin_r_data;
};
#endif

class DMD_TextBox : public Print {
public:
  DMD_TextBox(DMDFrame &dmd, int left = 0, int top = 0, int width = 0, int height = 0);
  virtual size_t write(uint8_t);
  void clear();
  void reset();
  void invertDisplay() { inverted = !inverted; }

  void scrollY(int scrollBy);
  void scrollX(int scrollBy);
private:
  DMDFrame &dmd;
  bool inverted;
  int left;
  int top;
  int width;
  int height;
  int16_t cur_x;
  int16_t cur_y;
  bool pending_newline;
};

// Six byte header at beginning of FontCreator font structure, stored in PROGMEM
struct FontHeader {
  uint16_t size;
  uint8_t fixedWidth;
  uint8_t height;
  uint8_t firstChar;
  uint8_t charCount;
};

#endif
