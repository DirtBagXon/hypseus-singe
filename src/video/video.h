/*
 * ____ DAPHNE COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2001 Matt Ownby, 2024 DirtBagXon
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

#if defined(WIN32) || defined(_WIN32)
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

#define ASPECTPD 75
#define ASPECTSD 133
#define ASPECTWS 178 // Round up
#define NOSQUARE 0x2D0
#define TITLE_LENGTH 42

#define YUV_H 0x00
#define YUV_V 0x01
#define YUV_FLAG_BLEND  0x01
#define YUV_FLAG_GRAYSCALE 0x02

#include "SDL_FontCache.h"
#include <SDL.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace video
{
// 16 is normal, the 17th is for the 'A' in SAE
static const uint8_t LED_RANGE          = 18;
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
static const uint8_t OVERLAY_LDP1450_LINES = 16;
static const uint8_t ANUN_CHAR_HEIGHT = 15;
static const uint8_t ANUN_RANK_HEIGHT = 46;
static const uint8_t ANUN_LEVELS = 3;

enum {
    B_DL_PLAYER1,
    B_DL_PLAYER2,
    B_DL_LIVES,
    B_DL_CREDITS,
    B_TQ_TIME,
    B_OVERLAY_LEDS,
    B_OVERLAY_LDP1450,
    B_ANUN_OFF,
    B_ANUN_ON,
    B_ACE_SPACE,
    B_ACE_CAPTAIN,
    B_ACE_CADET,
    B_MIA,
    B_ACE_ON,
    B_CAPTAIN_ON,
    B_CADET_ON,
    B_ACE_OFF,
    B_CAPTAIN_OFF,
    B_CADET_OFF,
    B_EMPTY
}; // bitmaps

enum {
    YUV_BLANK = 0,
    YUV_VISIBLE,
    YUV_SHUTTER,
    YUV_FLASH
};

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
void vid_free_yuv_overlay ();

void vid_update_overlay_surface(SDL_Surface *tx);
void vid_blit();
// MAC: sdl_video_run thread block ends here

void shutdown_display();
void resize_cleanup();

// flips the video buffers (if in double buffering mode)
void vid_flip();

// blanks the back video buffer (makes it black)
void vid_blank();

bool load_bmps();
bool draw_led(int, int, int, unsigned char);
void draw_overlay_leds(unsigned int led_values[], int num_values, int x, int y);
void draw_singleline_LDP1450(char *LDP1450_String, int start_x, int y);
bool draw_othergfx(int which, int x, int y);
void free_bmps();
SDL_Surface *load_one_bmp(const char *, bool);
SDL_Surface *load_one_png(const char *);
void free_one_bmp(SDL_Surface *);
SDL_Window *get_window();
SDL_Renderer *get_renderer();
SDL_Texture *get_screen();
SDL_Texture *get_yuv_screen();
SDL_Surface *get_screen_blitter();
SDL_Surface *get_screen_leds();
SDL_Rect get_aux_rect();
SDL_Rect get_sb_rect();
FC_Font *get_font();
bool use_old_font();
bool get_opengl();
bool get_vulkan();
bool get_fullscreen();
void set_teardown();
bool get_aux_bezel();
bool get_fullwindow();
bool get_singe_blend_sprite();
bool get_video_resized();
void set_opengl(bool value);
void set_vulkan(bool value);
void set_grayscale(bool value);
void set_blendfilter(bool value);
void set_forcetop(bool value);
int get_textureaccess();
void set_textureaccess(int value);
void set_grabmouse(bool value);
void toggle_grabmouse();
void set_vsync(bool value);
void set_intro(bool value);
void set_yuv_blue(bool value);
void set_fullscreen(bool value);
void set_fakefullscreen(bool value);
void set_scale_linear(bool value);
void set_force_aspect_ratio(bool bEnabled);
void set_ignore_aspect_ratio(bool bEnabled);
void set_scanlines(bool value);
void set_shunt(uint8_t value);
void set_alpha(uint8_t value);
void set_yuv_blank(int value);
void set_yuv_flash();
int get_scalefactor();           // by RDG2010
void set_scalefactor(int value); // by RDG2010
void scalekeyboard(int value);
void reset_scalefactor(int, uint8_t, bool);
void set_rotate_degrees(float fDegrees);
void set_sboverlay_characterset(int value);
void set_sboverlay_white(bool value);
void set_window_title(char* value);
void set_game_window(const char* value);
Uint16 get_video_width();
Uint16 get_viewport_width();
Uint16 get_viewport_height();
Uint16 get_video_height();
void set_video_width(Uint16);
void set_video_height(Uint16);
void draw_scanlines(int);
void draw_border(int, int);
void draw_string(const char *, int, int, SDL_Surface *);
void draw_subtitle(char *, bool, bool);
void vid_toggle_fullscreen();
void vid_toggle_scanlines();
void vid_toggle_bezel();
void vid_scoreboard_switch();
void set_aspect_ratio(int fRatio);
void set_detected_height(int pHeight);
void set_detected_width(int pWidth);
void set_subtitle_display(char *);
void set_LDP1450_enabled(bool bEnabled);
void set_singe_blend_sprite(bool bEnabled);
void set_bezel_file(const char *);
void set_bezel_path(const char *);
void set_bezel_reverse(bool display);
void set_aspect_change(int aspectWidth, int aspectHeight);
void set_sb_window_position(int, int);
void set_aux_bezel_position(int, int);
void set_score_bezel(bool bEnabled);
void set_score_bezel_alpha(int8_t value);
void set_score_bezel_scale(int value);
void set_aux_bezel_scale(int value);
void set_tq_keyboard(bool bEnabled);
void set_annun_lamponly(bool bEnabled);
void set_aux_bezel(bool bEnabled);
void set_ded_annun_bezel(bool bEnabled);
void set_aux_bezel_alpha(int8_t value);
void set_scale_h_shift(int value);
void set_scale_v_shift(int value);
void set_display_screen(int value);
void set_score_screen(int value);
void set_fRotateDegrees(float fDegrees, bool);
void set_yuv_scale(int value, uint8_t axis);
void set_yuv_rect(int, int, int, int);
void reset_yuv_rect();

void set_vertical_orientation(bool);
void format_fullscreen_render();
void format_window_render();

bool draw_ranks();
bool draw_annunciator(int which);
bool draw_annunciator1(int which);
bool draw_annunciator2(int which);
void calc_annun_rect();

void take_screenshot();
void set_queue_screenshot(bool bEnabled);

unsigned int get_logical_width();
unsigned int get_logical_height();
float get_fRotateDegrees();

void reset_shiftvalue(int, bool, uint8_t);
int get_scale_h_shift();
int get_scale_v_shift();

int get_score_bezel_scale();
int get_aux_bezel_scale();

void set_overlay_offset(int offset);
int get_yuv_overlay_width();
int get_yuv_overlay_height();
void reset_yuv_overlay();

void notify_stats(int width, int height, const char*);
void notify_positions();

bool get_yuv_overlay_ready();

}
#endif
