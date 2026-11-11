DMD2 is an Arduino library designed as an updated replacement for the [original DMD library](http://github.com/freetronics/DMD). Both libraries are designed for use with the [Freetronics Dot Matrix Display](http://www.freetronics.com/collections/display/dmd).

**This library is currently in BETA release meaning the documentation is incomplete, and the implementation may contain bugs. If you want the stable version, please use the [original DMD library](http://github.com/freetronics/DMD) for now not DMD2.**

If you find any bugs, please use the Issues feature in github to report them or [post on the Freetronics Dot Matrix Display forum](http://forum.freetronics.com/viewforum.php?f=26).

# Changes from DMD library

The DMD2 library includes the following new features:

* Supports Arduino Due (IDE 1.5.6 or newer), as well as AVR-based Arduinos.
* Adds new "SoftDMD" support, which allows the standard DMDCon board connections to be used on all standard Arduino compatible models - including Arduino Mega/EtherMega, Arduino Due/EtherDue or Arduino Leonardo.
* Integrated timer management for simpler sketches.
* Improved performance, on an AVR-based Arduino a single DMD panel uses approximately 5-6% CPU overhead to update (including when using the SoftDMD mode rather than hardware SPI.)
* New drawString() methods accept flash strings, or the Arduino String type, directly. See the "AllDrawingOperations" example.
* New DMD_TextBox class supports automatic scrolling, and automatic `print()` interface for writing out numbers, variables, etc. See "Countdown" and "ScrollingAlphabet" examples.
* New dmd.setBrightness() call allows changing DMD brightness (no more blindingly bright displays!)
* New DMDFrame base class allows direct swapping of the DMD framebuffer, supporting double buffering operations and similar (see "GameOfLife" example.)

# Not Yet Implemented

* Test patterns

# Getting Started

The easiest way to install the DMD2 library is to use the Arduino IDE's Library Manager:

* Install Arduino IDE 1.6.5 or newer.
* Under the Sketch menu, choose Include Library -> Manage Libraries.
* After the list finishes updating, type "DMD2" in the search field at the top. The DMD2 library should appear.
* Select the latest DMD2 version from the dropdown and click "Install"

Otherwise, you can [manually install the Arduino library using the steps described here](http://www.freetronics.com/pages/how-to-install-arduino-libraries).

After the library is installed, you will find some example sketches under File -> Examples -> DMD2.

More documentation will be produced before the library reaches final release rather than Beta status. Many of the DMD2 concepts are borrowed from FTOLED library so the [FTOLED wiki](https://github.com/freetronics/FTOLED/wiki/) may be of use.

Feel free to ask [questions on the forum](http://forum.freetronics.com/viewforum.php?f=26) if things don't work.

# ESP8266 Support

Thanks to @h4rm0n1c there is support for DMD2 on ESP8266 using the Arduino environment. See [this comment](https://github.com/freetronics/DMD2/pull/7#issue-97282971) for the pin connections.

With ESP8266 Arduino core 3, display refresh runs outside the interrupt handler so it can work alongside Wi-Fi. Network traffic can still cause visible shimmer. Older cores use the original timer interrupt code.

# ESP32 Support

DMD2 supports `SPIDMD` on ESP32 with Arduino-ESP32 cores 2 and 3. SoftDMD is not supported on ESP32.

Set up SPI before starting the display if you use custom pins. For example, these connections were used on the ESP32/WROVER test board:

```cpp
#include <SPI.h>
#include <DMD2.h>

// One panel; output enable, row A, row B, latch
SPIDMD dmd(1, 1, 25, 26, 27, 33);

void setup() {
  // Clock, MISO (unused), MOSI, SS (unused)
  SPI.begin(14, -1, 23, -1);
  dmd.begin();
}

void loop() {
}
```

Choose pins to suit your board. GPIO16 and GPIO17 are used by PSRAM on WROVER modules. See [ESP32 notes](docs/ESP32.md) for shared SPI, PWM and start/stop behaviour.

# Panel Layout

Use `dmd.setFlipPanels(true)` if alternate rows of panels are mounted upside down to simplify the ribbon cable connections. This is optional; existing layouts are unchanged.

Frames wider than 255 pixels are supported. Make sure the framebuffer, and any temporary frames used when scrolling, fit in your board's RAM.

# About the Makefiles

You'll notice the examples directory contains some files named `Makefile`. You can ignore these if you are using the Arduino IDE.

However, if you want to use other development tools with the DMD library, the Makefiles work with with the [arduino-mk](http://www.mjoldfield.com/atelier/2009/02/arduino-cli.html) package, version 1.3.1. They may need updating to work with newer versions.

# Testing

Version 0.0.4 was tested on real ESP8266 and ESP32/WROVER hardware with P10 panels.

* ESP8266 tests used two panels and covered drawing, scrolling, flipped rows and Wi-Fi traffic.
* ESP32 tests used Arduino-ESP32 2.0.17 and 3.3.12 and covered refresh, brightness, Wi-Fi traffic, repeated start/stop and shared SPI use.
* Automated checks cover framebuffer operations and builds for Uno, Countdown, ESP8266 and both ESP32 core versions. See [tests](tests/README.md) to run them.

We haven't tested a physical chain of eight or more panels, a complete multirow wall, or other ESP32 variants. AVR and Due haven't had new hardware tests. The remaining ESP8266 OTA, MQTT and long-running watchdog reports are still open.
