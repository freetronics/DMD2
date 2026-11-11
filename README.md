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

Thanks to @h4rm0n1c there is support for DMD2 on ESP8266 using the Arduino environment. See [this comment](https://github.com/freetronics/DMD2/pull/7#issue-97282971) for an explanation of using DMD2 on ESP8266.

Freetronics is unable to guarantee support for DMD2 on ESP8266, but we will try and help if we can.

# ESP32 support

`SPIDMD` refresh runs in a worker task, with a timer notification every 2083 µs
(about 120 complete four-row frames/s when scheduling and SPI are available).
Missed notifications are coalesced rather than replayed. SPI runs at 4 MHz;
the transaction covers shifting, latching and row selection. Other SPI users
must use transactions too. A DMD has no chip-select input: other devices' bus
traffic still reaches its shift registers, and this is not electrical isolation.
`setOtherCS()` can suppress a scan while another device's CS is active.

Initialize custom SPI pins before `begin()`, for example
`SPI.begin(14, -1, 23, -1)`. The display constructor takes the non-SPI pins
in OE, A, B, latch order. Choose GPIOs appropriate for your module; flash,
PSRAM and input-only pins are not interchangeable with general output pins.
SoftDMD is not implemented for ESP32.

On ESP32, repeated `begin()` leaves an already-running display intact.
`isRunning()` reports whether automatic refresh started. PWM or refresh
resource failure leaves it false. `end()` waits for any worker scan, removes
the display and detaches its PWM output; repeated calls are safe. A stock
SPIDMD also stops on destruction. ESP32 SPIDMD instances cannot be copied
or assigned; copy a `DMDFrame` snapshot instead. Stop custom derived displays with `end()`
before destroying their own scan state. Keep framebuffer drawing and lifecycle
operations in one application task; this is not a general thread-safe drawing API.
The shared worker and mutex are retained for reuse after the last display ends.

Arduino-ESP32 core 3 allocates PWM channels through its LEDC API. Core 2 uses
channel 0 by default; call `setPWMChannel(channel)` before `begin()` to choose
a different channel for each display. Duplicate DMD2 channel claims are rejected.
Reserve that channel and its paired LEDC timer against unrelated libraries:
core 2 has no shared Arduino channel allocator for DMD2 to consult. The setter
has no channel-selection effect on core 3 and refuses changes while PWM is attached.
All displays use 5 kHz, 8-bit PWM. External code must not reconfigure DMD-owned pins.

# About the Makefiles

You'll notice the examples directory contains some files named `Makefile`. You can ignore these if you are using the Arduino IDE.

However, if you want to use other development tools with the DMD library, the Makefiles work with with the [arduino-mk](http://www.mjoldfield.com/atelier/2009/02/arduino-cli.html) package, version 1.3.1. They may need updating to work with newer versions.

# Maintenance validation

The 2026 maintenance integration incorporates #41, #43/#44, #56/#57 and #58
with corrective patches. Original contributor commits are preserved.

Framebuffer dimensions and byte strides now use 16-bit storage. This removes
the 255-pixel dimension limit; it does not guarantee that any requested frame
fits your board's RAM. Large frames and temporary frames used by scrolling
must fit available memory and the target's address space. Drawing coordinates
remain limited by the platform's integer range. Eight-panel physical output
has not been tested.

Local ESP8266 panel tests covered frame indexing, scrolling and flipped rows.
ESP32 tests on the original ESP32 module with Arduino 2.0.17 and 3.3.12 covered
refresh, Wi-Fi traffic, brightness, repeated stop/start, destruction, PWM
ownership and a competing SPI task. ESP32 logical frame tests also covered the
flipping corrections; physical multi-row ESP32 output and other ESP32 variants
remain unverified. Uno and the original SoftDMD Countdown example compile;
there is no new AVR hardware validation or reproduction of every issue #40
variation. The Countdown change clears its textbox between successive values.

The ESP8266 core 3 backend services display refresh cooperatively rather than
calling flash-resident drawing/SPI code inside an interrupt. Its requested
250-us interval is retained from panel tests: it is a scheduling request, not
a guarantee of a 1-kHz full-frame rate. Wi-Fi traffic can introduce visible
shimmer. AVR and Due refresh backends retain their existing timing.

See [tests/README.md](tests/README.md) for repeatable build and framebuffer
regression checks. No RTC, light-sensor or clock-application compatibility is
implied by the library tests.
