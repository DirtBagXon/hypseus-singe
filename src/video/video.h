/*
 * ____ DAPHNE COPYRIGHT NOTICE ____
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

#include "SDL_FontCache.h"
#include <SDL.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace video
{
// 16 is normal, the 17th is for the 'A' in SAE
static const uint8_t LED_RANGE          = 17;
static const uint8_t OVERLAY_LED_WIDTH  = 8;
static const uint8_t OVERLAY_LED_HEIGHT = 13;
// width of each LDP1450 overlay character
static const uint8_t OVERLAY_LDP1450_WIDTH = 16;
// height of each LDP1450 overlay character
static const uint8_t OVERLAY_LDP1450_HEIGHT = 16;
// spacing between LDP1450 overlay characters
static const uint8_t OVERLAY_LDP1450_CHARACTER_SPACING = 15;
// spacing between LDP1450 overlay lines
static const uint8_t OVERLAY_LDP1450_LINE_SPACING = 16;

// dimensions of small font
static const uint8_t FONT_SMALL_W = 6;
static const uint8_t FONT_SMALL_H = 13;

enum {
    B_DL_PLAYER1,
    B_DL_PLAYER2,
    B_DL_LIVES,
    B_DL_CREDITS,
    B_HYPSEUS_SAVEME,
    B_GAMENOWOOK,
    B_OVERLAY_LEDS,
    B_OVERLAY_LDP1450,
    B_EMPTY
}; // bitmaps
enum {
    FONT_SMALL,
    FONT_BIG
}; // font enumeration, dependent on which order font .bmp's are loaded in

bool init_display();

// MAC: YUV surface protection block: it needs protection because it's accessed from both
// the main "hypseus" thread and the vldp thread.

bool init_display();
bool deinit_display();

SDL_Texture *vid_create_yuv_texture (int width, int height);
void vid_setup_yuv_overlay (int width, int height);
// MAC : REMEMBER, vid_update_yuv_overlay() ONLY updates the YUV surface. The YUV texture is updated on vid_blit()
int vid_update_yuv_overlay (uint8_t *Yplane, uint8_t *Uplane, uint8_t *Vplane, int Ypitch, int Upitch, int Vpitch);
int vid_update_yuv_texture (uint8_t *Yplane, uint8_t *Uplane, uint8_t *Vplane, int Ypitch, int Upitch, int Vpitch);
void vid_blank_yuv_texture ();
void vid_free_yuv_overlay ();

void vid_update_overlay_surface(SDL_Surface *tx, int x, int y);
void vid_blit();
// MAC: sdl_video_run thread block ends here

#ifdef USE_OPENGL
bool init_opengl();
#endif // USE_OPENGL

void shutdown_display();

// flips the video buffers (if in double buffering mode)
void vid_flip();

// blanks the back video buffer (makes it black)
void vid_blank();

void display_repaint();
bool load_bmps();
bool draw_led(int, int, int);
void draw_overlay_leds(unsigned int led_values[], int num_values, int x, int y,
                       SDL_Surface *overlay);
void draw_singleline_LDP1450(char *LDP1450_String, int start_x, int y, SDL_Surface *overlay);
bool draw_othergfx(int which, int x, int y, bool bSendToScreenBlitter = true);
void free_bmps();
SDL_Surface *load_one_bmp(const char *);
void free_one_bmp(SDL_Surface *);
void draw_rectangle(short x, short y, unsigned short w, unsigned short h,
                    unsigned char red, unsigned char green, unsigned char blue);
SDL_Renderer *get_renderer();
SDL_Texture *get_screen();
SDL_Surface *get_screen_blitter();
FC_Font *get_font();
bool get_fullscreen();
void set_fullscreen(bool value);
int get_scalefactor();           // by RDG2010
void set_scalefactor(int value); // by RDG2010
void set_rotate_degrees(float fDegrees);
void set_sboverlay_characterset(int value);
Uint16 get_video_width();
void set_video_width(Uint16);
Uint16 get_video_height();
void set_video_height(Uint16);
void draw_string(const char *, int, int, SDL_Renderer *);
void vid_toggle_fullscreen();

void set_force_aspect_ratio(bool bEnabled);

bool get_force_aspect_ratio();

unsigned int get_draw_width();
unsigned int get_draw_height();

int get_yuv_overlay_width();
int get_yuv_overlay_height();

bool get_yuv_overlay_ready();

}
#endif
