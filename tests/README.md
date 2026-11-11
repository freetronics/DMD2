# Tests

## Framebuffer

Run from the library directory:

```sh
tests/native/run.sh
```

This needs a C++11 compiler with AddressSanitizer and UndefinedBehaviorSanitizer. It compiles `DMDFrame.cpp` with small Arduino stubs and checks dimensions, pixels, copying, flipped rows and scrolling at 15 frame sizes. These include the 255/256-pixel boundary and a width of 2048 pixels.

This checks the framebuffer code, not GPIO, SPI or timers.

## Arduino builds

Install PlatformIO 6.2.0, then run:

```sh
pio run -d tests/compile
```

This builds for Uno, the original AVR Countdown example, ESP8266 core 3, and ESP32 cores 2 and 3. The same checks run in GitHub Actions.

## Hardware tests for 0.0.4

* ESP8266: two physical panels; drawing, scrolling, alternate-row flipping and Wi-Fi traffic.
* Original ESP32/WROVER: Arduino-ESP32 2.0.17 and 3.3.12; refresh, brightness, repeated start/stop, destruction, PWM channel use and another task sharing SPI. The final core 3 run completed 838 HTTP requests in 30 seconds without a failed request, and 100 seconds of serial monitoring without a panic or watchdog reset.
* ESP32 also ran framebuffer dimension and pixel checks. The larger frames were checked in memory, not on a matching number of physical panels.

There were no new AVR or Due hardware tests. Other ESP32 variants, a physical chain of eight or more panels and a complete multirow wall remain untested. Short HTTP tests don't settle the outstanding long-running watchdog, MQTT or OTA crash reports. RTC, light-sensor and clock-application compatibility were not part of these library tests.
