#!/bin/sh
arm-open2x-linux-gcc -g main.c -o sdloverlay `arm-open2x-linux-sdl-config --cflags --libs` -static
