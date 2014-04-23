#include "DMD2.h"
/*
 * DMDFrame class implementation.
 *
 * Non-hardware specific functions for updating the framebuffer
 */

DMDFrame::DMDFrame(byte panelsWide, byte panelsHigh)
  :
  width(panelsWide),
  height(panelsHigh),
  font(0)
{
  bitmap = (uint8_t *)malloc(bitmap_bytes());
}

DMDFrame::DMDFrame(const DMDFrame &source) :
  width(source.width),
  height(source.height),
  font(source.font)
{
  bitmap = (uint8_t *)malloc(bitmap_bytes());
  memcpy((void *)bitmap, (void *)source.bitmap, bitmap_bytes());
}

DMDFrame::~DMDFrame()
{
  free((void *)bitmap);
}

void DMDFrame::swapBuffers(DMDFrame &other)
{
#ifdef __AVR__
  // AVR can't write pointers atomically, so need to disable interrupts
  char oldSREG = SREG;
  cli();
#endif
  volatile uint8_t *temp = other.bitmap;
  other.bitmap = this->bitmap;
  this->bitmap = temp;
#ifdef __AVR__
  SREG = oldSREG;
#endif
}

inline int DMDFrame::pixelToBitmapIndex(unsigned int x, unsigned int y)
{
  // Panels seen as stretched out in a row for purposes of finding index
  uint8_t panel = (x/WIDTH_PIXELS) + (width * (y/HEIGHT_PIXELS));
  x = (x % WIDTH_PIXELS)  + (panel * WIDTH_PIXELS);
  y = y % HEIGHT_PIXELS;
  return x / 8 + (y * unified_width() / 8);
}

inline uint8_t DMDFrame::pixelToBitmask(unsigned int x)
{
  // Pixel lookup, marginally faster than bit shifting the argument
  static const PROGMEM uint8_t lookup_table[] = {
    0x80,   //0, bit 7
    0x40,   //1, bit 6
    0x20,   //2. bit 5
    0x10,   //3, bit 4
    0x08,   //4, bit 3
    0x04,   //5, bit 2
    0x02,   //6, bit 1
    0x01    //7, bit 0
  };
  return pgm_read_byte(lookup_table + (x & 0x07));
}


// Set a single LED on or off
void DMDFrame::setPixel(unsigned int x, unsigned int y, const bool on)
{
  if(x >= pixel_width() || y >= pixel_height())
     return;

  int byte_idx = pixelToBitmapIndex(x,y);
  uint8_t bit = pixelToBitmask(x);
  if(on)
    bitmap[byte_idx] &= ~bit;
  else
    bitmap[byte_idx] |= bit;
}


bool DMDFrame::getPixel(unsigned int x, unsigned int y)
{
  if(x >= pixel_width() || y >= pixel_height())
     return false;
  int byte_idx = pixelToBitmapIndex(x,y);
  uint8_t bit = pixelToBitmask(x);
  bool res = !(bitmap[byte_idx] & bit);
  return res;
}

void DMDFrame::movePixels(unsigned int from_x, unsigned int from_y,
                         unsigned int to_x, unsigned int to_y,
                         unsigned int width, unsigned int height)
{
  // NB: This implementation is pretty slow and uses too much RAM when non-overlapping
  // regions are moved. Would benefit from a rewrite.

  if(from_x >= pixel_width() || from_y >= pixel_height()
     || to_x >= pixel_width() || to_y >= pixel_height())
     return;

  uint8_t pixels[(width + 7) / 8][height];
  memset(pixels, 0, sizeof(pixels));

  for(unsigned int y = 0; y < height; y++) {
    for(unsigned int x = 0; x < width; x++) {
      unsigned int bit = x % 8;
      if(getPixel(from_x+x, from_y+y)) {
        pixels[x/8][y] |= (1<<bit);
        setPixel(from_x+x,from_y+y,false);
      }
    }
  }

  for(unsigned int y = 0; y < height; y++) {
    for(unsigned int x = 0; x < width; x++) {
      unsigned int bit = x % 8;
      setPixel(x+to_x,y+to_y, pixels[x/8][y] & (1<<bit));
    }
  }
}


// Set the entire screen
void DMDFrame::fillScreen(bool on)
{
  memset((void *)bitmap, on ? 0 : 0xFF, bitmap_bytes());
}

void DMDFrame::drawLine(int x1, int y1, int x2, int y2, bool on)
{
  int dy = y2 - y1;
  int dx = x2 - x1;
  int stepx, stepy;

  if (dy < 0) {
    dy = -dy;
    stepy = -1;
  } else {
    stepy = 1;
  }
  if (dx < 0) {
    dx = -dx;
    stepx = -1;
  } else {
    stepx = 1;
  }
  dy = dy * 2;
  dx = dx * 2;


  setPixel(x1, y1, on);
  if (dx > dy) {
    int fraction = dy - (dx / 2);	// same as 2*dy - dx
    while (x1 != x2) {
      if (fraction >= 0) {
        y1 += stepy;
        fraction -= dx;	// same as fraction -= 2*dx
      }
      x1 += stepx;
      fraction += dy;	// same as fraction -= 2*dy
      setPixel(x1, y1, on);
    }
  } else {
    int fraction = dx - (dy / 2);
    while (y1 != y2) {
      if (fraction >= 0) {
        x1 += stepx;
        fraction -= dy;
      }
      y1 += stepy;
      fraction += dx;
      setPixel(x1, y1, on);
    }
  }
}

void DMDFrame::drawCircle(unsigned int xCenter, unsigned int yCenter, int radius, bool on)
{
  // Bresenham's circle drawing algorithm
  int x = -radius;
  int y = 0;
  int error = 2-2*radius;
  while(x < 0) {
    setPixel(xCenter-x, yCenter+y, on);
    setPixel(xCenter-y, yCenter-x, on);
    setPixel(xCenter+x, yCenter-y, on);
    setPixel(xCenter+y, yCenter+x, on);
    radius = error;
    if (radius <= y) error += ++y*2+1;
    if (radius > x || error > y) error += ++x*2+1;
  }
}

void DMDFrame::drawBox(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, bool on)
{
  drawLine(x1, y1, x2, y1, on);
  drawLine(x2, y1, x2, y2, on);
  drawLine(x2, y2, x1, y2, on);
  drawLine(x1, y2, x1, y1, on);
}

void DMDFrame::drawFilledBox(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, bool on)
{
  for (unsigned int b = x1; b <= x2; b++) {
    drawLine(b, y1, b, y2, on);
  }
}
