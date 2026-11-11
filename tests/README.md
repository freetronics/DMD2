# Regression checks

Run `tests/native/run.sh` with a C++11 compiler and Address/UndefinedBehavior
Sanitizer support. This runs the real framebuffer implementation with minimal
Arduino type stubs; it checks dimensions, pixel/copy operations, flipping and
scroll boundaries at 15 sizes including 255/256 pixels and 2048-pixel width.
It does not emulate an Arduino, GPIO, SPI or timers.

For real Arduino compilation, install PlatformIO 6.2.0 and run
`pio run -d tests/compile`. Targets are Uno, the original AVR Countdown example,
ESP8266 core 3, and ESP32 cores 2 and 3. These are build checks; successful
compilation does not establish physical display behaviour.
