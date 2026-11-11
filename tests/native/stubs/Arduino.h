#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#define PROGMEM
#define pgm_read_byte(p) (*(const uint8_t *)(p))
class String;
using std::min;
using std::max;
