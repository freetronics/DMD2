#include "DMD2.h"

SPIDMD::SPIDMD(byte panelsWide, byte panelsHigh)
  : BaseDMD(panelsWide, panelsHigh, 9, 6, 7, 8)
{
}

/* Create a DMD display using a custom pinout for all the non-SPI pins (SPI pins set by hardware) */
SPIDMD::SPIDMD(byte panelsWide, byte panelsHigh, byte pin_noe, byte pin_a, byte pin_b, byte pin_sck)
  : BaseDMD(panelsWide, panelsHigh, pin_noe, pin_a, pin_b, pin_sck)
{
}

void SPIDMD::initialize()
{
  // Configure SPI before initialising the base DMD
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);	// CPOL=0, CPHA=0
  SPI.setClockDivider(SPI_CLOCK_DIV16); // Try DIV4 soon
  BaseDMD::initialize();
}

void SPIDMD::writeSPIData(volatile uint8_t *rows[4], const int rowsize)
{
  for(int i = 0; i < rowsize; i++) {
    SPI.transfer(*(rows[3]++));
    SPI.transfer(*(rows[2]++));
    SPI.transfer(*(rows[1]++));
    SPI.transfer(*(rows[0]++));
  }
}

void BaseDMD::scanDisplay()
{
  if(pin_other_cs >= 0 && digitalRead(pin_other_cs) != HIGH)
    return;

  // Rows are send out in 4 blocks of 4 (interleaved), across all panels

  int rowsize = unified_width() / 8; // in bytes

  volatile uint8_t *rows[4] = { // Scanning out 4 interleaved rows
    bitmap + (scan_row + 0) * rowsize,
    bitmap + (scan_row + 4) * rowsize,
    bitmap + (scan_row + 8) * rowsize,
    bitmap + (scan_row + 12) * rowsize,
  };

  writeSPIData(rows, rowsize);

  // TODO: Do version for default_pins
  digitalWrite(pin_noe, LOW);
  digitalWrite(pin_sck, HIGH); // Latch DMD shift register output
  digitalWrite(pin_sck, LOW);

  // A, B determine which set of interleaved rows we are multiplexing on
  // 0 = 1,5,9,13
  // 1 = 2,6,10,14
  // 2 = 3,7,11,15
  // 3 = 4,8,12,16
  digitalWrite(pin_a, scan_row & 0x01);
  digitalWrite(pin_b, scan_row & 0x02);
  scan_row = (scan_row + 1) % 4;
  digitalWrite(pin_noe, HIGH);
}

SoftDMD::SoftDMD(byte panelsWide, byte panelsHigh)
  : BaseDMD(panelsWide, panelsHigh, 9, 6, 7, 8),
    pin_clk(13),
    pin_r_data(11)
{
}

SoftDMD::SoftDMD(byte panelsWide, byte panelsHigh, byte pin_noe, byte pin_a, byte pin_b, byte pin_sck,
          byte pin_clk, byte pin_r_data)
  : BaseDMD(panelsWide, panelsHigh, pin_noe, pin_a, pin_b, pin_sck),
    pin_clk(pin_clk),
    pin_r_data(pin_r_data)
{
}

void SoftDMD::initialize()
{
  digitalWrite(pin_clk, LOW);
  pinMode(pin_clk, OUTPUT);

  digitalWrite(pin_r_data, LOW);
  pinMode(pin_r_data, OUTPUT);
  BaseDMD::initialize();
}


inline void SoftDMD::softSPITransfer(uint8_t data) {
  // MSB first, data captured on rising edge
  for(uint8_t bit = 0; bit < 8; bit++) {
    digitalWrite(pin_r_data, (data & (1<<7))  ? HIGH : LOW);
    digitalWrite(pin_clk, HIGH);
    data <<= 1;
    digitalWrite(pin_clk, LOW);
  }
}

void SoftDMD::writeSPIData(volatile uint8_t *rows[4], const int rowsize)
{
  for(int i = 0; i < rowsize; i++) {
    softSPITransfer(*(rows[3]++));
    softSPITransfer(*(rows[2]++));
    softSPITransfer(*(rows[1]++));
    softSPITransfer(*(rows[0]++));
  }
}

BaseDMD::BaseDMD(byte panelsWide, byte panelsHigh, byte pin_noe, byte pin_a, byte pin_b, byte pin_sck)
  : scan_row(0),
    width(panelsWide),
    height(panelsHigh),
    pin_noe(pin_noe),
    pin_a(pin_a),
    pin_b(pin_b),
    pin_sck(pin_sck),
    default_pins(pin_noe == 9 && pin_a == 6 && pin_b == 7 && pin_sck == 8),
    pin_other_cs(-1)
{
  bitmap = (uint8_t *)malloc(bitmap_bytes());
}

void BaseDMD::initialize()
{
  digitalWrite(pin_noe, HIGH);
  pinMode(pin_noe, OUTPUT);

  digitalWrite(pin_a, LOW);
  pinMode(pin_a, OUTPUT);

  digitalWrite(pin_b, LOW);
  pinMode(pin_b, OUTPUT);

  digitalWrite(pin_sck, LOW);
  pinMode(pin_sck, OUTPUT);

  clearScreen();
  scanDisplay();
}

// Set a single LED on or off
void BaseDMD::setPixel(unsigned int x, unsigned int y, const bool on)
{
  if(x >= total_width() || y >= total_height())
     return;

  // Panels seen as stretched out in a row for purposes of finding index
  uint8_t panel = (x/WIDTH_PIXELS) + (width * (y/HEIGHT_PIXELS));
  x = (x % WIDTH_PIXELS)  + (panel * WIDTH_PIXELS);
  y = y % HEIGHT_PIXELS;

  int byte_idx = x / 8 + (y * unified_width() / 8);
  // TODO: investigate the lookup table optimisation from original DMD
  uint8_t bit = 1 << (7 - (x & 0x07));
  if(on)
    bitmap[byte_idx] &= ~bit;
  else
    bitmap[byte_idx] |= bit;
}

// Set the entire screen
void BaseDMD::fillScreen(bool on)
{
  memset((void *)bitmap, on ? 0 : 0xFF, bitmap_bytes());
}

void BaseDMD::drawLine(int x1, int y1, int x2, int y2, bool on)
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

void BaseDMD::drawCircle(unsigned int xCenter, unsigned int yCenter, int radius, bool on)
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

void BaseDMD::drawBox(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, bool on)
{
  drawLine(x1, y1, x2, y1, on);
  drawLine(x2, y1, x2, y2, on);
  drawLine(x2, y2, x1, y2, on);
  drawLine(x1, y2, x1, y1, on);
}

void BaseDMD::drawFilledBox(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, bool on)
{
  for (unsigned int b = x1; b <= x2; b++) {
    drawLine(b, y1, b, y2, on);
  }
}

