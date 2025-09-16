CC=gcc
errors=-Wall -Werror
libs=-lgpiod -lm
src_dir=src
tests_gl_dir=tests_gl
srcs=$(wildcard ./src/*.c)

adafruit_bonnet_srcdir=./src/lib/adafruit-oled-bonnet
adafruit_bonnet_src=$(wildcard ./src/lib/adafruit-oled-bonnet/src/*.c)
adafruit_bonnet_uic_src=$(wildcard ./src/lib/adafruit-oled-bonnet/src/uic/*.c)
adafruit_bonnet_fonts_src=$(wildcard ./src/lib/adafruit-oled-bonnet/fonts/*.h)

bonnetmirror_src=./src/main.c

.PHONY = clean

bonnetmirror: $(bonnetmirror_src) $(adafruit_bonnet_font_src) $(adafruit_bonnet_src)
	mkdir -p ./bin/
	$(CC) -o ./bin/bonnetmirror -I$(adafruit_bonnet_srcdir) \
	$(bonnetmirror_src) $(adafruit_bonnet_src) $(adafruit_bonnet_uic_src) \
	$(adafruit_bonnet_font_src) $(errors) $(libs) -g

clean:
	rm -f ./bin/*
