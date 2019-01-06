# Arduino Make file. Refer to https://github.com/sudar/Arduino-Makefile

BOARD_TAG = nano328
ARDUINO_LIBS = EtherCard
USER_LIB_PATH = libraries

CFLAGS+=-Wall -Wextra
CXXFLAGS+=-Wall -Wextra

ifeq ($(DEV),Y)
	CFLAGS+=-DTANK_DEV
	CXXFLAGS+=-DTANK_DEV
	BOARD_SUB+=dev
endif

include /usr/share/arduino/Arduino.mk
