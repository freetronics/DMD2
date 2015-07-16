/*
 DMD2 Text Handling Functions.

 Copyright (C) 2014 Freetronics, Inc. (info <at> freetronics <dot> com)

 Updated by Angus Gratton, based on DMD by Marc Alexander & FTOLED_Text.cpp
 from the FTOLED library.

---

 This program is free software: you can redistribute it and/or modify it under the terms
 of the version 3 GNU General Public License as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program.
 If not, see <http://www.gnu.org/licenses/>.
*/

#include "DMD2.h"

void DMDFrame::selectFont(const uint8_t* font)
{
  this->font = (uint8_t *)font;
}

int DMDFrame::drawChar(const int x, const int y, const char letter, DMDGraphicsMode mode, const uint8_t *font)
{
  if(!font)
    font = this->font;
  if(x >= (int)width || y >= height)
    return -1;

  struct FontHeader header;
  memcpy_P(&header, (void*)font, sizeof(FontHeader));

  DMDGraphicsMode invertedMode = inverseMode(mode);

  char c = letter;
  if (c == ' ') {
    int charWide = charWidth(' ');
    this->drawFilledBox(x, y, x + charWide, y + header.height, invertedMode);
    return charWide;
  }
  uint8_t width = 0;
  uint8_t bytes = (header.height + 7) / 8;
  uint16_t index = 0;

  if (c < header.firstChar || c >= (header.firstChar + header.charCount))
    return 0;
  c -= header.firstChar;

  if (header.size == 0) {
    // zero length is flag indicating fixed width font (array does not contain width data entries)
    width = header.fixedWidth;
    index = sizeof(FontHeader) + c * bytes * width;
  } else {
    // variable width font, read width data, to get the index
    for (uint8_t i = 0; i < c; i++) {
      index += pgm_read_byte(font + sizeof(FontHeader) + i);
    }
    index = index * bytes + header.charCount + sizeof(FontHeader);
    width = pgm_read_byte(font + sizeof(FontHeader) + c);
  }
  if (x < -width || y < -header.height)
    return width;
    
  bool inverse = false;
  if (mode == GRAPHICS_INVERSE) {
      inverse = true;
  }

  // last but not least, draw the character
  for (uint8_t j = 0; j < width; j++) { // Width
    for (uint8_t i = bytes - 1; i < 254; i--) { // Vertical Bytes
      uint8_t data = pgm_read_byte(font + index + j + (i * width));
      int offset = (i * 8);
      if ((i == bytes - 1) && bytes > 1) {
        offset = header.height - 8;
      }
      for (uint8_t k = 0; k < 8; k++) { // Vertical bits
        if ((offset+k >= i*8) && (offset+k <= header.height)) {
          if (data & (1 << k)) {
              if(inverse) {
                setPixel(x + j, y + offset + k, GRAPHICS_OFF);
              } else {
                setPixel(x + j, y + offset + k, GRAPHICS_ON);
              }
          } else {
              if(inverse) {
                  setPixel(x + j, y + offset + k, GRAPHICS_ON);
              } else {
                  setPixel(x + j, y + offset + k, GRAPHICS_OFF);
              }
          }
        }
      }
    }
  }
  return width;
}

// Generic drawString implementation for various kinds of strings
template <class StrType> __attribute__((always_inline)) inline void _drawString(DMDFrame *dmd, int x, int y, StrType str, DMDGraphicsMode mode, const uint8_t *font)
{
  struct FontHeader header;
  memcpy_P(&header, font, sizeof(FontHeader));

  if (y+header.height<0)
    return;

  DMDGraphicsMode invertedMode = inverseMode(mode);
  int strWidth = 0;
  if(x > 0)
    dmd->drawLine(x-1 , y, x-1 , y + header.height - 1, invertedMode);

  char c;
  for(int idx = 0; c = str[idx], c != 0; idx++) {
    if(c == '\n') { // Newline
      strWidth = 0;
      y = y - header.height - 1;
    }
    else {
      int charWide = dmd->drawChar(x+strWidth, y, c, mode);
      if (charWide > 0) {
        strWidth += charWide ;
        dmd->drawLine(x + strWidth , y, x + strWidth , y + header.height-1, invertedMode);
        strWidth++;
      } else if (charWide < 0) {
        return;
      }
    }
  }
}

// Generic stringWidth implementation for various kinds of strings
template <class StrType> __attribute__((always_inline)) inline unsigned int _stringWidth(DMDFrame *dmd, const uint8_t *font, StrType str)
{
  unsigned int width = 0;
  char c;
  int idx;
  for(idx = 0; c = str[idx], c != 0; idx++) {
    int cwidth = dmd->charWidth(c);
    if(cwidth > 0)
      width += cwidth + 1;
  }
  if(width) {
    width--;
  }
  return width;
}


#if defined(__AVR__) || defined (ESP8266)
// Small wrapper class to allow indexing of progmem strings via [] (should be inlined out of the actual implementation)
class _FlashStringWrapper {
  const char *str;
public:
  _FlashStringWrapper(const char * flstr) : str(flstr) { }
  inline char operator[](unsigned int index) {
    return pgm_read_byte(str + index);
  }
};

void DMDFrame::drawString_P(int x, int y, const char *flashStr, DMDGraphicsMode mode, const uint8_t *font)
{
  if(!font)
    font = this->font;
  if(x >= (int)width || y >= height)
    return;
  _FlashStringWrapper wrapper(flashStr);
  _drawString(this, x, y, wrapper, mode, font);
}

unsigned int DMDFrame::stringWidth_P(const char *flashStr, const uint8_t *font)
{
  if(!font)
    font = this->font;
  _FlashStringWrapper wrapper(flashStr);
  return _stringWidth(this, font, wrapper);
}

#endif

void DMDFrame::drawString(int x, int y, const char *bChars, DMDGraphicsMode mode, const uint8_t *font)
{
  if(!font)
    font = this->font;
  if (x >= (int)width || y >= height)
    return;
  _drawString(this, x, y, bChars, mode, font);
}

void DMDFrame::drawString(int x, int y, const String &str, DMDGraphicsMode mode, const uint8_t *font)
{
  if(!font)
    font = this->font;
  if (x >= (int)width || y >= height)
    return;
  _drawString(this, x, y, str, mode, font);
}

//Find the width of a character
int DMDFrame::charWidth(const char letter, const uint8_t *font)
{
  struct FontHeader header;
  memcpy_P(&header, (void*)this->font, sizeof(FontHeader));

  if(!font)
    font = this->font;

  if(letter == ' ') {
    // if the letter is a space then return the font's fixedWidth
    // (set as the 'width' field in New Font dialog in GLCDCreator.)
    return header.fixedWidth;
  }

  if((uint8_t)letter < header.firstChar || (uint8_t)letter >= (header.firstChar + header.charCount)) {
    return 0;
  }

  if(header.size == 0) {
    // zero length is flag indicating fixed width font (array does not contain width data entries)
    return header.fixedWidth;
  }

  // variable width font, read width data for character
  return pgm_read_byte(this->font + sizeof(FontHeader) + letter - header.firstChar);
}

unsigned int DMDFrame::stringWidth(const char *bChars, const uint8_t *font)
{
  if(!font)
    font = this->font;
  return _stringWidth(this, font, bChars);
}

unsigned int DMDFrame::stringWidth(const String &str, const uint8_t *font)
{
  if(!font)
    font = this->font;
  return _stringWidth(this, font, str);
}

