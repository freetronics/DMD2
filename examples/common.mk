BOARD_TAG    = uno
ARDUINO_PORT ?= /dev/ttyUSB0
ARDUINO_LIBS = SPI DMD2 SD

ARDUINO_DIR ?= /usr/share/arduino
ARDMK_DIR   ?= /usr/share/arduino
AVR_TOOLS_DIR ?= /usr

include ${ARDUINO_DIR}/Arduino.mk
CPPFLAGS += -Wall
