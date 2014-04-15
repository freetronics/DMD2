#include "DMD2.h"

const int PIX_WIDTH = 32; // per display
const int PIX_HEIGHT = 16;

#define TOTAL_PANELS (width*height)
#define BITMAP_SZ (TOTAL_PANELS * (PIX_WIDTH * PIX_HEIGHT / 8))
#define TOTAL_WIDTH (width * PIX_WIDTH)
#define TOTAL_HEIGHT (height * PIX_HEIGHT)

/* pixel width across all panels when laid end to end for scanning */
#define UNIFIED_WIDTH (TOTAL_PANELS * PIX_WIDTH)


SPIDMD::SPIDMD(byte panelsWide, byte panelsHigh)
  : BaseDMD(panelsWide, panelsHigh, 9, 6, 7, 8),
    default_pins(true),
    pin_other_cs(-1)
{
}

/* Create a DMD display using a custom pinout for all the non-SPI pins (SPI pins set by hardware) */
SPIDMD::SPIDMD(byte panelsWide, byte panelsHigh, byte pin_noe, byte pin_a, byte pin_b, byte pin_sck)
  : BaseDMD(panelsWide, panelsHigh, pin_noe, pin_a, pin_b, pin_sck),
    default_pins(false),
    pin_other_cs(-1)
{
}

void SPIDMD::initialize()
{
  BaseDMD::initialize();
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);	// CPOL=0, CPHA=0
  SPI.setClockDivider(SPI_CLOCK_DIV16); // Try DIV4 soon
}

void SPIDMD::scanDisplay()
{
  if(pin_other_cs >= 0 && digitalRead(pin_other_cs) != HIGH)
    return;

  // Rows are send out in 4 blocks of 4 (interleaved), across all panels

  int rowsize = UNIFIED_WIDTH / 8; // in bytes

  uint8_t *rows[4] = { // Scanning out 4 interleaved rows
    bitmap + (scan_row + 0) * rowsize,
    bitmap + (scan_row + 4) * rowsize,
    bitmap + (scan_row + 8) * rowsize,
    bitmap + (scan_row + 12) * rowsize,
  };

  for(int i = 0; i < rowsize; i++) {
    SPI.transfer(*(rows[3]++));
    SPI.transfer(*(rows[2]++));
    SPI.transfer(*(rows[1]++));
    SPI.transfer(*(rows[0]++));
  }

  // TODO: Do version for default_pins
  digitalWrite(pin_noe, LOW);
  digitalWrite(pin_sck, HIGH); // Latch DMD shift register output
  digitalWrite(pin_sck, LOW);
  switch(scan_row) {
  case 0: // rows 1,5,9,13
    digitalWrite(pin_a, LOW);
    digitalWrite(pin_b, LOW);
    scan_row = 1;
    break;
  case 1: // rows 2,6,10,14
    digitalWrite(pin_a, HIGH);
    digitalWrite(pin_b, LOW);
    scan_row = 2;
    break;
  case 2: // rows 3,7,11,15
    digitalWrite(pin_a, LOW);
    digitalWrite(pin_b, HIGH);
    scan_row = 3;
    break;
  case 3: // rows 4,8,12,16
    digitalWrite(pin_a, HIGH);
    digitalWrite(pin_b, HIGH);
    scan_row = 0;
    break;
  }
  digitalWrite(pin_noe, HIGH);
}

SoftDMD::SoftDMD(byte panelsWide, byte panelsHigh)
  : BaseDMD(panelsWide, panelsHigh, 9, 6, 7, 8),
    pin_clk(pin_clk),
    pin_r_data(pin_r_data)
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
  BaseDMD::initialize();

  digitalWrite(pin_clk, LOW);
  pinMode(pin_clk, OUTPUT);

  digitalWrite(pin_r_data, LOW);
  pinMode(pin_r_data, OUTPUT);
}

BaseDMD::BaseDMD(byte panelsWide, byte panelsHigh, byte pin_noe, byte pin_a, byte pin_b, byte pin_sck)
  : scan_row(0),
    width(panelsWide),
    height(panelsHigh),
    pin_noe(pin_noe),
    pin_a(pin_a),
    pin_b(pin_b),
    pin_sck(pin_sck)
{
  bitmap = (uint8_t *)malloc(BITMAP_SZ);
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
}

// Set the colour of a single pixel
void BaseDMD::setPixel(unsigned int x, unsigned int y, const bool on)
{
  if(x >= TOTAL_WIDTH || y >= TOTAL_HEIGHT)
     return;

  // Panels seen as stretched out in a row for purposes of finding index
  uint8_t panel = (x/PIX_WIDTH) + (width * (y/PIX_HEIGHT));
  x = (x % PIX_WIDTH)  + (panel * PIX_WIDTH);
  y = y % PIX_HEIGHT;

  int byte_idx = x / 8 + (y * UNIFIED_WIDTH / 8);
  // TODO: investigate the lookup table optimisation from original DMD
  if(on)
    bitmap[byte_idx] &= ~(1 << (7 - (x & 0x07)));
  else
    bitmap[byte_idx] |= (1 << (7 - (x & 0x07)));
}

// Set the entire screen
void BaseDMD::fillScreen(bool on)
{
  memset(bitmap, on ? 0 : 0xFF, BITMAP_SZ);
}

