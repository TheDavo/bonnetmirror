# Introduction

This repo is an exercise as part of CU Boulder's Embedded Linux Development
course to learn and understand how to create a bootable Linux image using
Yocto or Buildroot as the build tool.

This project takes a Raspberry Pi 4 with Adafruit's OLED Bonnet, which features
an OLED screen driven by the SSD1306 chip and a couple of GPIO-based buttons,
to send the screen display to a client via sockets.

The client repo can be found [here](https://github.com/TheDavo/bonnetmirror_client).

This repository contains the source code for a UI/game made using my own
bonnet library.

# Dependencies

This program/project has been run and tested on a Raspberry Pi 4, with the
Adafruit OLED Bonnet, and as such no expectations are made that is can be
installed and ran on another setup.

The main dependency is on `adafruit-oled-bonnet`, and that library requires
`libgpiod` to be installed to easily get and setup the GPIO buttons.

# Build

Run `make` from the project repository to compile the program.

The executable file is in the `bin` directory

# Run

Once the `make` command is executed, run the resulting program from the `./bin/`
directory.
