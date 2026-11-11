#pragma once
#include <stddef.h>
#include <stdint.h>
class Print { public: virtual ~Print() {} virtual size_t write(uint8_t) = 0; };
