#!/bin/sh
gcc -g main.c -o sdloverlay-linux `sdl-config --cflags --libs`
