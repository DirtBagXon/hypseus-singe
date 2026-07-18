/*
 * ____ HYPSEUS COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2001 Matt Ownby, 2024 DirtBagXon
 *
 * This file is part of HYPSEUS, a laserdisc arcade game emulator
 *
 * HYPSEUS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HYPSEUS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// by Matt Ownby, DirtBagXon
// Part of the HYPSEUS emulator

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
#define YUV_FLAG_BLEND     0x01
#define YUV_FLAG_GRAYSCALE 0x02
#define YUV_FLAG_LUMA      0x04

#define VIDEO_SET(flag) \
    (g_vflags |= (flag))

#define VIDEO_CLEAR(flag) \
    (g_vflags &= ~(flag))

#define VIDEO_HAS(flag) \
    ((g_vflags & (flag)) != 0)

#define VIDEO_ASSIGN(flag, value) \
    ((value) ? VIDEO_SET(flag) : VIDEO_CLEAR(flag))

#define VIDEO_TOGGLE(flag) \
    (g_vflags ^= (flag))

#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3/SDL.h>
#include <vector>

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

enum
{
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

enum
{
    YUV_BLANK = 0,
    YUV_VISIBLE,
    YUV_SHUTTER,
    YUV_FLASH
};

enum VideoState : uint64_t
{
    INDEX8               = 1ull << 0,
    LUA_GAME             = 1ull << 1,

    FULLSCREEN           = 1ull << 2,
    TAKE_SCREENSHOT      = 1ull << 3,

    SCALE_LINEAR         = 1ull << 4,
    SCANLINES            = 1ull << 5,

    OPENGL               = 1ull << 6,
    VULKAN               = 1ull << 7,
    SOFT_RENDER          = 1ull << 8,
    VSYNC                = 1ull << 9,

    TEARDOWN             = 1ull << 10,
    INTRO                = 1ull << 11,
    LOGO                 = 1ull << 12,

    FORCE_TOP            = 1ull << 13,
    GRAB_MOUSE           = 1ull << 14,

    YUV_BLUE             = 1ull << 15,

    VIDEO_RESIZED        = 1ull << 16,

    BLOCK_DRIVER_OVERLAY = 1ull << 17,

    ENHANCE_OVERLAY      = 1ull << 18,
    OVERLAY_DYNAMIC      = 1ull << 19,
    LEGACY_OVERLAY       = 1ull << 20,
    OVERLAY_WHITE        = 1ull << 21,

    FORCE_ASPECT         = 1ull << 22,
    IGNORE_ASPECT        = 1ull << 23,

    BEZEL_LOAD           = 1ull << 24,
    BEZEL_TOGGLE         = 1ull << 25,
    BEZEL_REVERSE        = 1ull << 26,
    KEYBOARD_BEZEL       = 1ull << 27,
    SCOREBOARD_BEZEL     = 1ull << 28,

    AUX_BEZEL            = 1ull << 29,
    ANNUN_LAMPS          = 1ull << 30,
    DED_ANNUN_BEZEL      = 1ull << 31,

    SCALED               = 1ull << 32,
    VERTICAL_ORIENTATION = 1ull << 33,
};

bool init_display();
bool deinit_display();

void vid_setup_yuv_overlay (int width, int height);
// MAC : REMEMBER, vid_update_yuv_overlay() ONLY updates the YUV surface. The YUV texture is updated on vid_blit()
int vid_update_yuv_overlay (uint8_t *Yplane, uint8_t *Uplane, uint8_t *Vplane, int Ypitch, int Upitch, int Vpitch);
int vid_update_yuv_texture (uint8_t *Yplane, uint8_t *Uplane, uint8_t *Vplane, int Ypitch, int Upitch, int Vpitch);
void vid_free_yuv_overlay ();

void vid_update_overlay_surface(SDL_Surface *tx);
void vid_blit();
// MAC: sdl_video_run thread block ends here

// blanks the back video buffer (makes it black)
void vid_blank();

bool load_bmps();
bool draw_led(int, int, int, unsigned char);
void draw_overlay_leds(unsigned int led_values[], int num_values, int x, int y);
void draw_string(const char *, int, int, SDL_Surface *, SDL_Color, bool);
void draw_singleline_LDP1450(char *LDP1450_String, int start_x, int y);
bool draw_othergfx(int which, int x, int y);
void free_bmps();
SDL_Window *get_window();
SDL_Renderer *get_renderer();
SDL_Texture *get_overlay_texture();
SDL_Surface *get_screen_leds();
SDL_Rect get_aux_rect();
SDL_Rect get_scoreboard_rect();
TTF_Font *get_font();
TTF_TextEngine *get_font_engine();
bool use_legacy_font();
bool get_opengl();
bool get_vulkan();
bool get_fullscreen();
void set_teardown();
bool get_aux_bezel();
bool get_video_resized();
void set_opengl(bool value);
void set_vulkan(bool value);
void set_luma(bool, uint8_t);
void set_grayscale(bool value);
void set_blendfilter(bool value);
void set_forcetop(bool value);
void set_software_render(bool value);
SDL_TextureAccess get_textureaccess();
void set_textureaccess(SDL_TextureAccess value);
void set_grabmouse(bool value);
void toggle_grabmouse();
void set_vsync(bool value);
void set_intro(bool value);
void set_logo(bool value);
void set_yuv_blue(bool value);
void set_fullscreen(bool value);
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
void set_overlay_alpha(uint8_t value);
void set_game_window(const char* value);
Uint16 get_video_width();
Uint16 get_viewport_width();
Uint16 get_viewport_height();
Uint16 get_video_height();
void set_video_width(Uint16);
void set_video_height(Uint16);
void draw_srt(const char*, uint8_t, int);
void draw_subtitle(const char *, uint8_t, bool);
void vid_toggle_fullscreen();
void vid_toggle_scanlines();
void vid_toggle_bezel();
void vid_scoreboard_switch();
void set_aspect_ratio(int fRatio);
void set_detected_height(int pHeight);
void set_detected_width(int pWidth);
void set_LDP1450_enabled(bool bEnabled);
void set_bezel_file(const char *);
void set_bezel_path(const char *);
void set_bezel_reverse(bool display);
void set_aspect_change(int aspectWidth, int aspectHeight);
void set_scoreboard_window_position(int, int);
void set_aux_bezel_position(int, int);
void set_scoreboard_bezel(bool bEnabled);
void set_scoreboard_bezel_scale(int value);
void set_aux_bezel_scale(int value);
void set_tq_keyboard(bool bEnabled);
void set_annun_lamponly(bool bEnabled);
void set_aux_bezel(bool bEnabled);
void set_ded_annun_bezel(bool bEnabled);
void set_scale_h_shift(int value);
void set_scale_v_shift(int value);
void set_display_screen(int value);
void set_scoreboard_screen(int value);
void set_fRotateDegrees(float fDegrees, bool);
void set_yuv_scale(int value, uint8_t axis);
void set_yuv_rect(float, float, float, float);
void reset_yuv_rect();

void set_legacy_overlay(bool);

void set_vertical_orientation(bool);

bool draw_annunciator(int which);

bool get_bezelstatus();

void set_queue_screenshot(bool bEnabled);

unsigned int get_logical_width();
unsigned int get_logical_height();
float get_fRotateDegrees();

std::vector<SDL_Rect> get_displays();

void reset_shiftvalue(int, bool, uint8_t);
int get_scale_h_shift();
int get_scale_v_shift();
int get_display_no();

int get_scoreboard_bezel_scale();
int get_aux_bezel_scale();

void set_overlay_offset(int, int);
int get_yuv_overlay_width();
int get_yuv_overlay_height();
void reset_yuv_overlay();

void notify_stats(int width, int height, const char*);
void notify_positions();

bool get_yuv_overlay_ready();

}
#endif
