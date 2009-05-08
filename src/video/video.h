/*
 * video.h
 *
 * Copyright (C) 2001 Matt Ownby
 *
 * This file is part of DAPHNE, a laserdisc arcade game emulator
 *
 * DAPHNE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * DAPHNE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// by Matt Ownby
// Part of the DAPHNE emulator

#ifndef BITMAP_H
#define BITMAP_H

#include <SDL.h>

#define LED_RANGE 17 //16 is normal, the 17th is for the 'A' in SAE

#define OVERLAY_LED_WIDTH 8
#define OVERLAY_LED_HEIGHT 13

#define OVERLAY_LDP1450_WIDTH 16                //width of each LDP1450 overlay character
#define OVERLAY_LDP1450_HEIGHT 16               //height of each LDP1450 overlay character
#define OVERLAY_LDP1450_CHARACTER_SPACING 15    //spacing between LDP1450 overlay characters
#define OVERLAY_LDP1450_LINE_SPACING 16         //spacing between LDP1450 overlay lines

enum { B_DL_PLAYER1, B_DL_PLAYER2, B_DL_LIVES, B_DL_CREDITS, B_DAPHNE_SAVEME, B_GAMENOWOOK, B_OVERLAY_LEDS, B_OVERLAY_LDP1450, B_EMPTY }; // bitmaps
enum { FONT_SMALL, FONT_BIG };	// font enumeration, dependent on which order font .bmp's are loaded in

// dimensions of small font
#define FONT_SMALL_W 6
#define FONT_SMALL_H 13

////////////////////////////////////////////////////////////////////////////////////////////////////////

bool init_display();

#ifdef USE_OPENGL
bool init_opengl();
void take_screenshot_GL();
#endif // USE_OPENGL

void shutdown_display();

// flips the video buffers (if in double buffering mode)
void vid_flip();

// blanks the back video buffer (makes it black)
void vid_blank();

// blits an SDL Surface to the back buffer
void vid_blit(SDL_Surface *srf, int x, int y);

void display_repaint();
bool load_bmps();
bool draw_led(int, int, int);
void draw_overlay_leds(unsigned int led_values[], int num_values, int x, int y, SDL_Surface *overlay);
void draw_singleline_LDP1450(char *LDP1450_String, int start_x, int y, SDL_Surface *overlay);
bool draw_othergfx(int which, int x, int y, bool bSendToScreenBlitter = true);
void free_bmps();
SDL_Surface *load_one_bmp(const char *);
void free_one_bmp(SDL_Surface *);
void draw_rectangle(short x, short y, unsigned short w, unsigned short h, unsigned char red, unsigned char green, unsigned char blue);
SDL_Surface *get_screen();
SDL_Surface *get_screen_blitter();
int get_console_initialized();
bool get_fullscreen();
void set_fullscreen(bool value);
void set_rotate_degrees(float fDegrees);
void set_sboverlay_characterset(int value);
Uint16 get_video_width();
void set_video_width(Uint16);
Uint16 get_video_height();
void set_video_height(Uint16);
void take_screenshot(SDL_Overlay *yuvimage);
void save_screenshot(SDL_Surface *shot);
void yuv2rgb(SDL_Color *result, int y, int u, int v);
void draw_string(const char*, int, int, SDL_Surface*);
void vid_toggle_fullscreen();

// used to enable/disable the HWACCEL environment variable
// (the YUV overlay must be created after this has been called for it to take effect)
// if 'enabled' is true, the YUV hw accel is enabled.
void set_yuv_hwaccel(bool enabled);

// returns true if acceleration is enabled or false if not
bool get_yuv_hwaccel();

void set_force_aspect_ratio(bool bEnabled);

bool get_force_aspect_ratio();

#ifdef USE_OPENGL
// sets the value of g_bUseOpenGL
void set_use_opengl(bool enabled);

// returns value of g_bUseOpenGL
bool get_use_opengl();
#endif // USE_OPENGL

#endif
