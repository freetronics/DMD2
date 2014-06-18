/*
 DMD TextBox implementation

 Allows a simple scrolling text box that implements the Arduino Print
 interface, so you can write to it like a serial port or character
 LCD display.

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

DMD_TextBox::DMD_TextBox(DMDFrame &dmd, int left, int top, int width, int height) :
  dmd(dmd),
  inverted(false),
  left(left),
  top(top),
  width(width),
  height(height),
  cur_x(0),
  cur_y(0),
  pending_newline(false)
{
}

size_t DMD_TextBox::write(uint8_t character) {
  struct FontHeader header;
  memcpy_P(&header, (void *)this->dmd.font, sizeof(FontHeader));
  uint8_t rowHeight = header.height+1;

  if(width == 0)
    width = dmd.width - left;
  if(height == 0)
    height = dmd.height - top;

  uint8_t char_width = dmd.charWidth(character) + 1;
  while((cur_x > 0 && cur_x + char_width >= this->width) || pending_newline) { // Need to wrap to new line
    if(cur_y + rowHeight * 2 <= height) { // No need to scroll
      cur_y += rowHeight;
      cur_x = 0;
    } else if (height >= rowHeight * 2) { // Scroll vertically, there's enough room for 2 rows of chars
      scrollY(rowHeight);
    } else if(pending_newline) { // No room, so just clear display
      clear();
    } else { // Scroll characters horizontally
      scrollX(char_width - (this->width - cur_x) + 1);
    }
    pending_newline = false;
  }

  if(character == '\n') {
    pending_newline = true;
    // clear the rest of the line after the current cursor position,
    // this allows you to then use reset() and do a flicker-free redraw
    dmd.drawFilledBox(cur_x+left,cur_y+top,left+width,cur_y+top+rowHeight, inverted);
  }

  dmd.drawChar(cur_x+left,cur_y+top,character, inverted);
  cur_x += char_width;
  return 1;
}

void DMD_TextBox::scrollY(uint8_t rowHeight) {
  dmd.movePixels(left, top + rowHeight, left, top, width,
                 (rowHeight > height) ? 0 : (height - rowHeight));
  cur_x = 0;
}

void DMD_TextBox::scrollX(uint8_t charWidth) {
  dmd.movePixels(left+charWidth, top, left, top, width - charWidth, height);
  cur_x -= charWidth;
}


void DMD_TextBox::clear() {
  this->reset();
  dmd.drawFilledBox(left,top,left+width,top+height,inverted);
}

void DMD_TextBox::reset() {
  cur_x = 0;
  cur_y = 0;
  pending_newline = false;
}
