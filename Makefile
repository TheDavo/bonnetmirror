CC ?= gcc
CFLAGS ?= -Wall -Werror
LDLIBS ?=-lgpiod -lm
LDFLAGS ?= 
src_dir=src
srcs=$(wildcard ./src/*.c)

adafruit_bonnet_srcdir=./src/lib/adafruit-oled-bonnet
adafruit_bonnet_src=$(wildcard ./src/lib/adafruit-oled-bonnet/src/*.c)
adafruit_bonnet_uic_src=$(wildcard ./src/lib/adafruit-oled-bonnet/src/uic/*.c)
adafruit_bonnet_fonts_src=$(wildcard ./src/lib/adafruit-oled-bonnet/fonts/*.h)

bonnetmirror_src=./src/main.c

.PHONY = all clean

all: bonnetmirror

bonnetmirror: $(bonnetmirror_src) $(adafruit_bonnet_font_src) \
			  $(adafruit_bonnet_src) $(adafruit_bonnet_uic_src)
	mkdir -p ./bin/
	$(CC) $(CFLAGS) $(LDFLAGS) -o ./bin/bonnetmirror \
	$(bonnetmirror_src) $(adafruit_bonnet_src) $(adafruit_bonnet_uic_src) \
	$(adafruit_bonnet_font_src) $(LDLIBS) -g

clean:
	rm -f ./bin/*
