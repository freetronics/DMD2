# ESP32 Notes

See the [README](../README.md#esp32-support) for a wiring example. These notes apply to `SPIDMD` on ESP32. Use output-capable GPIOs for all display signals, and avoid pins reserved for flash or PSRAM.

## Refresh and SPI

The display uses a worker task to shift the data, latch it and select the next row. A timer wakes the task every 2083 microseconds, giving about 120 complete frames per second when the task and SPI bus are available. Missed updates are skipped rather than queued. SPI runs at 4 MHz.

Other devices on the same SPI bus must use SPI transactions too. A DMD has no chip select, so data sent to another device also reaches the display's shift registers. `setOtherCS()` can skip a display scan while another device's chip select is active; it does not electrically isolate the display.

## Starting and stopping

* `begin()` starts automatic refresh. Calling it again while running leaves the display alone.
* `isRunning()` tells you whether refresh started. It returns false if PWM or refresh resources could not be set up.
* `end()` waits for the current scan to finish, stops refresh for that display and releases its PWM output. It is safe to call more than once.
* Destroying a `SPIDMD` stops it. If you derive your own display class, call `end()` before destroying any state used by its scan method.

ESP32 `SPIDMD` objects cannot be copied or assigned. Use a `DMDFrame` copy if you need a snapshot of the image. Keep drawing and start/stop calls in one application task; they are not safe to call concurrently from several tasks. The shared worker task and mutex stay allocated for reuse after the last display stops.

## Brightness and PWM

Brightness uses 5 kHz, 8-bit PWM. Don't reconfigure pins owned by an active display.

Arduino-ESP32 core 3 allocates PWM channels automatically. On core 2 the default is channel 0. Call `setPWMChannel(channel)` before `begin()` to choose another channel, and use a different channel for each display. DMD2 rejects duplicate channel assignments between its displays, but it cannot detect channels used by unrelated libraries. Reserve the channel and its paired LEDC timer for DMD2.

`setPWMChannel()` does not select a channel on core 3, and will not change it while PWM is attached.
