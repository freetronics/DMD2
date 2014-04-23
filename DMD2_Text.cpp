/*
 DMD2 Text Handling Functions.

 Copyright (C) 2014 Freetronics, Inc. (info <at> freetronics <dot> com)

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

#include "DMD2.h"

void DMDFrame::selectFont(const uint8_t* font)
{
  this->font = (uint8_t *)font;
}

int DMDFrame::drawChar(const int x, const int y, const char letter, bool inverse, const uint8_t *font)
{
  if(!font)
    font = this->font;
  if(x >= (int)pixel_width() || y >= (int)pixel_height())
    return -1;

  struct FontHeader header;
  memcpy_P(&header, (void*)font, sizeof(FontHeader));

  char c = letter;
  if (c == ' ') {
    int charWide = charWidth(' ');
    this->drawFilledBox(x, y, x + charWide, y + header.height, inverse);
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
            setPixel(x + j, y + offset + k, !inverse);
          } else {
            setPixel(x + j, y + offset + k, inverse);
          }
        }
      }
    }
  }
  return width;
}

// Generic drawString implementation for various kinds of strings
template <class StrType> __attribute__((always_inline)) inline void _drawString(DMDFrame *dmd, int x, int y, StrType str, bool inverse, const uint8_t *font)
{
  struct FontHeader header;
  memcpy_P(&header, font, sizeof(FontHeader));

  if (y+header.height<0)
    return;

  int strWidth = 0;
  if(x > 0)
    dmd->drawLine(x-1 , y, x-1 , y + header.height - 1, inverse);

  char c;
  for(int idx = 0; c = str[idx], c != 0; idx++) {
    if(c == '\n') { // Newline
      strWidth = 0;
      y = y - header.height - 1;
    }
    else {
      int charWide = dmd->drawChar(x+strWidth, y, c, inverse);
      if (charWide > 0) {
        strWidth += charWide ;
        dmd->drawLine(x + strWidth , y, x + strWidth , y + header.height-1, inverse);
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


#ifdef __AVR__
// Small wrapper class to allow indexing of progmem strings via [] (should be inlined out of the actual implementation)
class _FlashStringWrapper {
  const char *str;
public:
  _FlashStringWrapper(const char * flstr) : str(flstr) { }
  inline char operator[](unsigned int index) {
    return pgm_read_byte(str + index);
  }
};

void DMDFrame::drawString_P(int x, int y, const char *flashStr, bool inverse, const uint8_t *font)
{
  if(!font)
    font = this->font;
  if(x >= (int)pixel_width() || y >= (int)pixel_height())
    return;
  _FlashStringWrapper wrapper(flashStr);
  _drawString(this, x, y, wrapper, inverse, font);
}

unsigned int DMDFrame::stringWidth_P(const char *flashStr, const uint8_t *font)
{
  if(!font)
    font = this->font;
  _FlashStringWrapper wrapper(flashStr);
  return _stringWidth(this, font, wrapper);
}

#endif

void DMDFrame::drawString(int x, int y, const char *bChars, bool inverse, const uint8_t *font)
{
  if(!font)
    font = this->font;
  if (x >= (int)pixel_width() || y >= (int)pixel_height())
    return;
  _drawString(this, x, y, bChars, inverse, font);
}

void DMDFrame::drawString(int x, int y, const String &str, bool inverse, const uint8_t *font)
{
  if(!font)
    font = this->font;
  if (x >= (int)pixel_width() || y >= (int)pixel_height())
    return;
  _drawString(this, x, y, str, inverse, font);
}

//Find the width of a character
int DMDFrame::charWidth(const char letter, const uint8_t *font)
{
  if(!font)
    font = this->font;

  char c = letter;
  uint8_t width = 0;

  struct FontHeader header;
  memcpy_P(&header, (void*)font, sizeof(FontHeader));

 trychar:

  if (c < header.firstChar || c >= (header.firstChar + header.charCount)) {
    if(c == ' ') {
      c = 'n'; // if ' ' not included, try using 'n'
      goto trychar;
    }
    return 0;
  }
  c -= header.firstChar;

  if (header.size == 0) {
    // zero length is flag indicating fixed width font (array does not contain width data entries)
    return header.fixedWidth;
  } else {
    // variable width font, read width data for character
    width = pgm_read_byte(font + sizeof(FontHeader) + c);
  }

  if(width == 0 && c == ' ') {
    c = 'n'; // if ' ' not included, try using 'n'
    goto trychar;
  }
  return width;
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

