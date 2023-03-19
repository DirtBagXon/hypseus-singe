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

#include "config.h"

// video.cpp
// Part of the DAPHNE emulator
// This code started by Matt Ownby, May 2000

#include "../hypseus.h"
#include "../game/game.h"
#include "../io/conout.h"
#include "../io/error.h"
#include "../io/mpo_fileio.h"
#include "../io/mpo_mem.h"
#include "../ldp-out/ldp.h"
#include "palette.h"
#include "video.h"
#include <SDL_syswm.h> // rdg2010
#include <SDL_image.h> // screenshot
#include <plog/Log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string> // for some error messages

using namespace std;

namespace video
{
int g_vid_width = 640, g_vid_height = 480; // default video dimensions
unsigned int g_draw_width = g_vid_width, g_probe_width = g_vid_width;
unsigned int g_draw_height = g_vid_height, g_probe_height = g_vid_height;
int g_sb_w = 340, g_sb_h = 480;
int s_alpha = 255;
int s_shunt = 2;

int g_viewport_width = g_vid_width, g_viewport_height = g_vid_height;
int g_aspect_width = 0, g_aspect_height = 0;

int sb_window_pos_x = 0, sb_window_pos_y = 0;

// the current game overlay dimensions
unsigned int g_overlay_width = 0, g_overlay_height = 0;

FC_Font *g_font                    = NULL;
FC_Font *g_fixfont                 = NULL;
TTF_Font *g_ttfont                 = NULL;
SDL_Surface *g_led_bmps[LED_RANGE] = {0};
SDL_Surface *g_other_bmps[B_EMPTY] = {0};
SDL_Window *g_window               = NULL;
SDL_Window *g_sb_window            = NULL;
SDL_Renderer *g_renderer           = NULL;
SDL_Renderer *g_sb_renderer        = NULL;
SDL_Texture *g_overlay_texture     = NULL; // The OVERLAY texture, excluding LEDs wich are a special case
SDL_Texture *g_yuv_texture         = NULL; // The YUV video texture, registered from ldp-vldp.cpp
SDL_Surface *g_screen_blitter      = NULL; // The main blitter surface
SDL_Surface *g_leds_surface        = NULL;
SDL_Surface *g_sb_surface          = NULL;
SDL_Texture *g_sb_texture          = NULL;
SDL_Surface *g_sb_blit_surface     = NULL;
SDL_Texture *g_bezel_texture       = NULL;

SDL_Rect g_overlay_size_rect; 
SDL_Rect g_scaling_rect = {0, 0, 0, 0};
SDL_Rect g_sb_bezel_rect = {0, 0, 0, 0};
SDL_Rect g_leds_size_rect = {0, 0, 320, 240}; 
SDL_Rect g_render_size_rect = g_leds_size_rect;

LDP1450_CharStruct LDP1450_CharSet[OVERLAY_LDP1450_LINES];

bool queue_take_screenshot = false;
bool g_fs_scale_nearest = false;
bool g_singe_blend_sprite = false;
bool g_scanlines = false;
bool g_fakefullscreen = false;
bool g_opengl = false;
bool g_vulkan = false;
bool g_grabmouse = false;
bool g_vsync = true;
bool g_yuv_blue = false;
bool g_vid_resized = false;
bool g_enhance_overlay = false;
bool g_overlay_resize = false;
bool g_bForceAspectRatio = false;
bool g_bIgnoreAspectRatio = false;
bool g_LDP1450_overlay = false;
bool g_fullscreen = false; // initialize video in fullscreen
bool g_bezel_toggle = false;
bool g_scale_view = false;
bool g_sb_bezel_alpha = false;
bool g_sb_bezel = false;

int g_scalefactor = 100;   // by RDG2010 -- scales the image to this percentage
int g_aspect_ratio = 0;
int sboverlay_characterset = 2;
int g_texture_access = SDL_TEXTUREACCESS_TARGET;
int g_sb_bezel_scale = 14;

// Move subtitle rendering to SDL_RenderPresent(g_renderer);
bool g_bSubtitleShown = false;
char *subchar = NULL;

string g_bezel_file = "";

// degrees in clockwise rotation
SDL_RendererFlip g_flipState = SDL_FLIP_NONE;
float g_fRotateDegrees = 0.0;

// YUV structure
typedef struct {
    uint8_t *Yplane;
    uint8_t *Uplane;
    uint8_t *Vplane;
    int width, height;
    int Ysize, Usize, Vsize; // The size of each plane in bytes.
    int Ypitch, Upitch, Vpitch; // The pitch of each plane in bytes.
    SDL_mutex *mutex;
} g_yuv_surface_t;

g_yuv_surface_t *g_yuv_surface;

// Blitting parameters. What textures need updating from their surfaces during a blitting strike?
bool g_scoreboard_needs_update = false;
bool g_softsboard_needs_update = false;
bool g_overlay_needs_update    = false;
bool g_yuv_video_needs_update  = false;
bool g_yuv_video_needs_blank   = false;
bool g_yuv_video_timer_blank   = false;

////////////////////////////////////////////////////////////////////////////////////////////////////

// initializes the window in which we will draw our BMP's
// returns true if successful, false if failure
bool init_display()
{
    bool result = false; // whether video initialization is successful or not
    Uint32 sdl_flags = 0;
    Uint32 sdl_sb_flags = 0;
    Uint8  sdl_render_flags = 0;
    Uint8  sdl_sb_render_flags = 0;
    static bool notify = false;
    char bezelpath[96] = {};
    char title[50] = "HYPSEUS Singe: Multiple Arcade Laserdisc Emulator";

    SDL_SysWMinfo info;
    sdl_flags = SDL_WINDOW_SHOWN;
    sdl_sb_flags = SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_BORDERLESS;
    sdl_render_flags = SDL_RENDERER_TARGETTEXTURE;

    // if we were able to initialize the video properly
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) >= 0) {

        if (g_fullscreen) sdl_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        else if (g_fakefullscreen) sdl_flags |= SDL_WINDOW_MAXIMIZED | SDL_WINDOW_BORDERLESS;

        if (g_opengl) {
            sdl_flags |= SDL_WINDOW_OPENGL;
            sdl_sb_flags |= SDL_WINDOW_OPENGL;
        } else if (g_vulkan) {
            sdl_flags |= SDL_WINDOW_VULKAN;
            sdl_sb_flags |= SDL_WINDOW_VULKAN;
        }

        g_overlay_width = g_game->get_video_overlay_width();
        g_overlay_height = g_game->get_video_overlay_height();

        // Enforce a minimum window size
        if ((int)g_probe_width < g_vid_width) g_probe_width = g_vid_width;
        if ((int)g_probe_height < g_vid_height) g_probe_height = g_vid_height;

        if (g_vid_resized) {
            g_draw_width  = g_viewport_width  = g_vid_width;
            g_draw_height = g_viewport_height = g_vid_height;

        } else {

            g_draw_width  = g_probe_width;
            g_draw_height = g_probe_height;

            if (!g_bIgnoreAspectRatio && g_aspect_width > 0) {
                g_viewport_width  = g_aspect_width;
                g_viewport_height = g_aspect_height;
            } else {
                g_viewport_width  = g_draw_width;
                g_viewport_height = g_draw_height;
            }
        }

        // Enforce 4:3 aspect ratio
        if (g_bForceAspectRatio) {
            double dCurAspectRatio = (double)g_draw_width / g_draw_height;
            const double dTARGET_ASPECT_RATIO = 4.0 / 3.0;

            if (dCurAspectRatio < dTARGET_ASPECT_RATIO) {
                g_draw_height = (g_draw_width * 3) / 4;
                g_viewport_height = (g_viewport_width * 3) / 4;
            }
            else if (dCurAspectRatio > dTARGET_ASPECT_RATIO) {
                g_draw_width = (g_draw_height * 4) / 3;
                g_viewport_width = (g_viewport_height * 4) / 3;
            }
        }

        // if we're supposed to scale the image...
        if (g_scalefactor < 100) {
            g_scaling_rect.w = (g_viewport_width * g_scalefactor) / 100;
            g_scaling_rect.h = (g_viewport_height * g_scalefactor) / 100;
            g_scaling_rect.x = ((g_viewport_width - g_scaling_rect.w) >> 1);
            g_scaling_rect.y = ((g_viewport_height - g_scaling_rect.h) >> 1);
        }

        if (!SDL_RectEmpty(&g_scaling_rect)) g_scale_view = true;

        if (notify) {
            LOGI << fmt("Viewport resolution: %dx%d", g_viewport_width, g_viewport_height);
        }

        if (g_window) resize_cleanup();

        if (g_fRotateDegrees != 0) {
            if (g_fRotateDegrees != 180.0) {
                LOGW << "Screen rotation enabled, aspect ratios will be ignored";
                g_viewport_height = g_viewport_width;
            }
	}

	g_window =
            SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                         g_viewport_width, g_viewport_height, sdl_flags);

        if (!g_window) {
            LOGE << fmt("Could not initialize window: %s", SDL_GetError());
            exit(SDL_ERROR_INIT);
        } else {
            if (g_game->m_sdl_software_rendering) {
                sdl_render_flags |= SDL_RENDERER_SOFTWARE;
            } else {
                sdl_render_flags |= SDL_RENDERER_ACCELERATED;
            }

            sdl_sb_render_flags = sdl_render_flags;

            if (g_vsync && (sdl_render_flags & SDL_RENDERER_ACCELERATED))
                sdl_render_flags |= SDL_RENDERER_PRESENTVSYNC;

            g_renderer = SDL_CreateRenderer(g_window, -1, sdl_render_flags);

            if (!g_renderer) {
                LOGE << fmt("Could not initialize renderer: %s", SDL_GetError());
                exit(SDL_ERROR_MAINRENDERER);
            } else {

                if (g_bezel_file.length() > 0) {
                    snprintf(bezelpath, sizeof(bezelpath), "bezels/%s", g_bezel_file.c_str());
                    g_bezel_texture = IMG_LoadTexture(g_renderer, bezelpath);

                    if (!notify) {
                        if (g_bezel_texture) {
                            LOGI << fmt("Loaded bezel file: %s", bezelpath);
                        } else {
                            LOGW << fmt("Failed to load bezel: %s", bezelpath);
                        }
                    }
                }

                SDL_VERSION(&info.version);

                // Set bilinear filtering by default
                if (!g_fs_scale_nearest)
                    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

                // Create a 32-bit surface with alpha component.
                int surfacebpp;
                Uint32 Rmask, Gmask, Bmask, Amask;
                SDL_PixelFormatEnumToMasks(SDL_PIXELFORMAT_RGBA8888, &surfacebpp,
                                &Rmask, &Gmask, &Bmask, &Amask);

                if (g_game->m_sdl_software_scoreboard) {

                    g_sb_blit_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, g_sb_w, g_sb_h,
                                            surfacebpp, Rmask, Gmask, Bmask, Amask);

                    if (g_sb_bezel) {

                        double scale = 9.0f - double((g_sb_bezel_scale << 1) / 10.0f);

                        g_sb_bezel_rect.x = sb_window_pos_x;
                        g_sb_bezel_rect.y = sb_window_pos_y;
                        g_sb_bezel_rect.w = (g_viewport_width / scale);
                        g_sb_bezel_rect.h = (g_sb_bezel_rect.w * 1.411f); // 17:24

                        if (!g_sb_bezel_alpha)
                            SDL_FillRect(g_sb_blit_surface, NULL, 0x000000ff);

                    } else if (SDL_GetWindowWMInfo(g_window, &info) && !g_sb_window) {

                        g_sb_window = SDL_CreateWindow(NULL, sb_window_pos_x,
                                          sb_window_pos_y, g_sb_w, g_sb_h, sdl_sb_flags);

                        if (!g_sb_window) {
                            LOGE << fmt("Could not initialize scoreboard window: %s", SDL_GetError());
                            g_game->set_game_errors(SDL_ERROR_SCOREWINDOW);
                            set_quitflag();
                        }

                        g_sb_renderer = SDL_CreateRenderer(g_sb_window, -1, sdl_sb_render_flags);

                        if (!g_sb_renderer) {
                            LOGE << fmt("Could not initialize scoreboard renderer: %s", SDL_GetError());
                            g_game->set_game_errors(SDL_ERROR_SCORERENDERER);
                            set_quitflag();
                        }
                        SDL_SetRenderDrawColor(g_sb_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
                        SDL_RenderClear(g_sb_renderer);
                        SDL_RenderPresent(g_sb_renderer);
                    } else if (!notify) { LOGE << "Cannot create a Scoreboard entity..."; }
                }

                // MAC: If we start in fullscreen mode, we have to set the logical
                // render size to get the desired aspect ratio.
                if ((sdl_flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0 ||
                               (sdl_flags & SDL_WINDOW_MAXIMIZED) != 0) {

                    SDL_RenderSetLogicalSize(g_renderer, g_viewport_width, g_viewport_height);

                    if (g_bezel_texture || !SDL_RectEmpty(&g_sb_bezel_rect))
                        g_bezel_toggle = true;
                }


		// Always hide the mouse cursor
                SDL_ShowCursor(SDL_DISABLE);

                if (g_grabmouse)
                    SDL_SetWindowGrab(g_window, SDL_TRUE);

                if (g_scanlines)
                    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);

                int imgflags = IMG_INIT_PNG | IMG_INIT_JPG;
                if (IMG_Init(imgflags) != imgflags) {
                    LOGE << fmt("Failed to initialize IMG: %s", SDL_GetError());
                    set_quitflag();
                }

                // Calculate font sizes
                int ffs;
                int fs = get_draw_width() / 36;
                if (g_aspect_ratio == ASPECTWS) ffs = get_draw_width() / 24;
                else ffs = get_draw_width() / 18;

                char font[32]="fonts/default.ttf";
                char fixfont[32] = "fonts/timewarp.ttf";
                char ttfont[32];

                if (g_game->get_use_old_overlay()) strncpy(ttfont, "fonts/daphne.ttf", sizeof(ttfont));
                else strncpy(ttfont, "fonts/digital.ttf", sizeof(ttfont));

                g_font = FC_CreateFont();
                FC_LoadFont(g_font, g_renderer, font, fs,
                            FC_MakeColor(0xff, 0xff, 0xff, 0xff), TTF_STYLE_NORMAL);

                g_fixfont = FC_CreateFont();
                FC_LoadFont(g_fixfont, g_renderer, fixfont, ffs,
                            FC_MakeColor(0xff, 0xff, 0xff, 0xff), TTF_STYLE_NORMAL);

                TTF_Init();
                if (g_game->get_use_old_overlay())
                    g_ttfont = TTF_OpenFont(ttfont, 12);
                else
                    g_ttfont = TTF_OpenFont(ttfont, 14);

                if (g_ttfont == NULL) {
                    LOG_ERROR << fmt("Cannot load TTF font: '%s'", (char*)ttfont);
                    deinit_display();
                    shutdown_display();
                    exit(SDL_ERROR_FONT);
                }

		g_screen_blitter =
		    SDL_CreateRGBSurface(SDL_SWSURFACE, g_overlay_width, g_overlay_height,
					surfacebpp, Rmask, Gmask, Bmask, Amask);

		// Check for game overlay enhancements (depth and size)
		g_enhance_overlay = g_game->get_overlay_upgrade();
		g_overlay_resize = g_game->get_fullsize_overlay();

		g_leds_surface =
		    SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 240,
					surfacebpp, Rmask, Gmask, Bmask, Amask);

                // Convert the LEDs surface to the destination surface format for faster blitting,
                // and set it's color key to NOT copy 0x000000ff pixels.
                // We couldn't do it earlier in load_bmps() because we need the g_screen_blitter format.
                g_other_bmps[B_OVERLAY_LEDS] = SDL_ConvertSurface(g_other_bmps[B_OVERLAY_LEDS],
                                    g_screen_blitter->format, 0);
                SDL_SetColorKey (g_other_bmps[B_OVERLAY_LEDS], SDL_TRUE, 0x000000ff);

                if (g_game->get_use_old_overlay()) {
                    g_other_bmps[B_OVERLAY_LDP1450] = SDL_ConvertSurface(g_other_bmps[B_OVERLAY_LDP1450],
                                    g_screen_blitter->format, 0);
                    SDL_SetColorKey (g_other_bmps[B_OVERLAY_LDP1450], SDL_TRUE, 0x000000ff);

                }

                // MAC: If the game uses an overlay, create a texture for it.
                // The g_screen_blitter surface is used from game.cpp anyway, so we always create it, used or not.
		if (g_overlay_width && g_overlay_height) {
		    g_overlay_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGBA8888,
						 g_texture_access,
						 g_overlay_width, g_overlay_height);

		    SDL_SetTextureBlendMode(g_overlay_texture, SDL_BLENDMODE_BLEND);
		    SDL_SetTextureAlphaMod(g_overlay_texture, 0xff);
                }

                SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);

                SDL_RenderClear(g_renderer);
                SDL_RenderPresent(g_renderer);
                // NOTE: SDL Console was initialized here.
                result = true;
            }
        }
    } else {
        LOGE << fmt("Could not initialize SDL: %s", SDL_GetError());
        exit(SDL_ERROR_INIT);
    }

    notify = true;
    return (result);
}

void vid_free_yuv_overlay () {
    // Here we free both the YUV surface and YUV texture.
    SDL_DestroyMutex (g_yuv_surface->mutex);
   
    free(g_yuv_surface->Yplane);
    free(g_yuv_surface->Uplane);
    free(g_yuv_surface->Vplane);
    free(g_yuv_surface);

    SDL_DestroyTexture(g_yuv_texture);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// deinitializes the window and renderer we have used.
// returns true if successful, false if failure
bool deinit_display()
{
    SDL_SetWindowGrab(g_window, SDL_FALSE);

    if (g_sb_texture)
        SDL_DestroyTexture(g_sb_texture);

    if (g_sb_window) {
        if (g_sb_renderer) SDL_DestroyRenderer(g_sb_renderer);
        SDL_DestroyWindow(g_sb_window);
    }

    if (g_sb_blit_surface) SDL_FreeSurface(g_sb_blit_surface);

    SDL_FreeSurface(g_screen_blitter);
    SDL_FreeSurface(g_leds_surface);

    IMG_Quit();
    TTF_Quit();
    FC_FreeFont(g_font);
    FC_FreeFont(g_fixfont);

    if (g_bezel_texture)
        SDL_DestroyTexture(g_bezel_texture);

    SDL_DestroyTexture(g_overlay_texture);
    SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_window);

    return (true);
}

void resize_cleanup()
{
    SDL_SetWindowGrab(g_window, SDL_FALSE);

    if (g_sb_blit_surface) SDL_FreeSurface(g_sb_blit_surface);
    if (g_screen_blitter) SDL_FreeSurface(g_screen_blitter);
    if (g_leds_surface) SDL_FreeSurface(g_leds_surface);

    if (g_bezel_texture) SDL_DestroyTexture(g_bezel_texture);

    if (g_overlay_texture) SDL_DestroyTexture(g_overlay_texture);
    if (g_renderer) SDL_DestroyRenderer(g_renderer);

    SDL_DestroyWindow(g_window);

}

// shuts down video display
void shutdown_display()
{
    LOGD << "Shutting down video display...";

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

// Clear the renderer. Good for avoiding texture mess (YUV, LEDs, Overlay...)
void vid_blank()
{
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(g_renderer);
}

// redraws the proper display (Scoreboard, etc) on the screen, after first
// clearing the screen
// call this every time you want the display to return to normal
void display_repaint()
{
    vid_blank();
    //vid_flip();
    g_game->force_blit();
}

// loads all the .bmp's
// returns true if they were all successfully loaded, or a false if they weren't
bool load_bmps()
{

    bool result = true; // assume success unless we hear otherwise
    int index   = 0;
    char filename[81];

    for (; index < LED_RANGE; index++) {
        snprintf(filename, sizeof(filename), "pics/led%d.bmp", index);

        g_led_bmps[index] = load_one_bmp(filename);

        // If the bit map did not successfully load
        if (g_led_bmps[index] == 0) {
            result = false;
        }
    }

    g_other_bmps[B_DL_PLAYER1]     = load_one_bmp("pics/player1.bmp");
    g_other_bmps[B_DL_PLAYER2]     = load_one_bmp("pics/player2.bmp");
    g_other_bmps[B_DL_LIVES]       = load_one_bmp("pics/lives.bmp");
    g_other_bmps[B_DL_CREDITS]     = load_one_bmp("pics/credits.bmp");
    g_other_bmps[B_HYPSEUS_SAVEME] = load_one_bmp("pics/saveme.bmp");
    g_other_bmps[B_GAMENOWOOK]     = load_one_bmp("pics/gamenowook.bmp");

    if (sboverlay_characterset != 2)
	g_other_bmps[B_OVERLAY_LEDS] = load_one_bmp("pics/overlayleds1.bmp");
    else
	g_other_bmps[B_OVERLAY_LEDS] = load_one_bmp("pics/overlayleds2.bmp");
   
    g_other_bmps[B_OVERLAY_LDP1450] = load_one_bmp("pics/ldp1450font.bmp");

    // check to make sure they all loaded
    for (index = 0; index < B_EMPTY; index++) {
        if (g_other_bmps[index] == NULL) {
            result = false;
        }
    }

    return (result);
}

SDL_Surface *load_one_bmp(const char *filename)
{
    SDL_Surface *result  = SDL_LoadBMP(filename);

    if (!result) {
        LOGW << fmt("Could not load bitmap: %s", SDL_GetError());
    }
 
    return (result);
}

// Draw's one of our LED's to the screen
// value contains the bitmap to draw (0-9 is valid)
// x and y contain the coordinates on the screen
// This function is called from img-scoreboard.cpp
// 1 is returned on success, 0 on failure
bool draw_led(int value, int x, int y)
{
    g_sb_surface = g_led_bmps[value];
    static char led = 0;

    SDL_Rect dest;
    dest.x = (short) x;
    dest.y = (short) y;
    dest.w = (unsigned short) g_sb_surface->w;
    dest.h = (unsigned short) g_sb_surface->h;

    SDL_BlitSurface(g_sb_surface, NULL, g_sb_blit_surface, &dest);

    if (led == 0xf) {
        if (g_sb_bezel)
            g_sb_texture = SDL_CreateTextureFromSurface(g_renderer,
                               g_sb_blit_surface);
        else {
            g_sb_texture = SDL_CreateTextureFromSurface(g_sb_renderer,
                               g_sb_blit_surface);
            SDL_RenderCopy(g_sb_renderer, g_sb_texture, NULL,  NULL);
            g_softsboard_needs_update = true;
        }
        led = -1;
    }
    led++;
    return true;
}


// Update scoreboard surface
void draw_overlay_leds(unsigned int values[], int num_digits,
                       int start_x, int y)
{
    SDL_Rect src, dest;

    dest.x = start_x;
    dest.y = y;
    dest.w = OVERLAY_LED_WIDTH;
    dest.h = OVERLAY_LED_HEIGHT;

    src.y = 0;
    src.w = OVERLAY_LED_WIDTH;
    src.h = OVERLAY_LED_HEIGHT;
    
    // Draw the digit(s) to the overlay surface
    for (int i = 0; i < num_digits; i++) {
        src.x = values[i] * OVERLAY_LED_WIDTH;

        // MAC : We need to call SDL_FillRect() here if we don't want our LED characters to "overlap", because
        // we set the g_other_bmps[B_OVERLAY_LEDS] color key in such a way black is not being copied
        // so segments are not clean when we go from 0 to 1, for example.
        // Also, note that SDL_BlitSurface() won't blit the 0x000000ff pixels because we set up a color key
        // using SDL_SetColorKey() in init_display(). See notes there on why we don't do it in load_bmps().
        // If scoreboard transparency problems appear, look there.
        SDL_FillRect(g_leds_surface, &dest, 0x00000000); 
        SDL_BlitSurface(g_other_bmps[B_OVERLAY_LEDS], &src, g_leds_surface, &dest);

        dest.x += OVERLAY_LED_WIDTH;
    }

    g_scoreboard_needs_update = true;

    // MAC: Even if we updated the overlay surface here, there's no need to do not-thread-safe stuff
    // like SDL_UpdateTexture(), SDL_RenderCopy(), etc... until we are going to compose a final frame
    // with the YUV texture and the overlay on top (which is issued from vldp for now) in VIDEO_RUN_BLIT.
}

// Draw LDP1450 overlay characters to the screen - rewrite for SDL2 (DBX)
void draw_charline_LDP1450(char *LDP1450_String, int start_x, int y)
{
    float x;
    int i, j = 0;
    int LDP1450_strlen;
    int index = (int)((y / OVERLAY_LDP1450_HEIGHT) + 0.5f);

    LDP1450_CharSet[index].enable = false;
    LDP1450_strlen = strlen(LDP1450_String);

    if (!LDP1450_strlen)
    {
        return;
    }
    else {
	if (LDP1450_strlen <= 11)
        {
            for (i = LDP1450_strlen; i <= 11; i++)
                 LDP1450_String[i] = 32;

            LDP1450_strlen = strlen(LDP1450_String);
        }
    }

    g_LDP1450_overlay = true;

    switch(g_aspect_ratio)
    {
       case ASPECTWS:
          x = (((double)g_draw_width / 225) * start_x);
          break;
       default:
          if (g_draw_width == NOSQUARE)
              x = (((double)g_draw_width / 384) * start_x);
          else
              x = (((double)g_draw_width / 256) * start_x);
          break;
    }

    for (i = 0; i < LDP1450_strlen; i++)
    {
         if (LDP1450_String[i] == 0x13)
             LDP1450_String[i] = 0x5f;

         if (LDP1450_String[i] != 32) j++;
    }

    if (j > 0) {
        LDP1450_CharSet[index].enable = true;
        LDP1450_CharSet[index].x = x;
        LDP1450_CharSet[index].y = (y * (double)(get_draw_height() * 0.004f));
        LDP1450_CharSet[index].OVERLAY_LDP1450_String = LDP1450_String;
    }
}

void draw_singleline_LDP1450(char *LDP1450_String, int start_x, int y)
{
    SDL_Rect src, dest;

    int i = 0;
    int value = 0;
    int LDP1450_strlen;
    g_scoreboard_needs_update = true;

    if (g_aspect_ratio == ASPECTSD &&
             g_draw_width == NOSQUARE)
        start_x = (start_x - (start_x / 4));

    dest.x = start_x;
    dest.y = y;
    dest.w = OVERLAY_LDP1450_WIDTH;
    dest.h = OVERLAY_LDP1450_HEIGHT;

    src.y = 0;
    src.w = OVERLAY_LDP1450_WIDTH;
    src.h = OVERLAY_LDP1450_HEIGHT;

    LDP1450_strlen = strlen(LDP1450_String);

    if (!LDP1450_strlen)
    {
        strcpy(LDP1450_String,"           ");
        LDP1450_strlen = strlen(LDP1450_String);
    }
    else
    {
        if (LDP1450_strlen <= 11)
        {
            for (i=LDP1450_strlen; i<=11; i++)
                 LDP1450_String[i] = 32;

            LDP1450_strlen = strlen(LDP1450_String);
        }
    }

    for (i=0; i<LDP1450_strlen; i++)
    {
         value = LDP1450_String[i];

         if (value >= 0x26 && value <= 0x39) value -= 0x25;
         else if (value >= 0x41 && value <= 0x5a) value -= 0x2a;
         else if (value == 0x13) value = 0x32;
         else value = 0x31;

         src.x = value * OVERLAY_LDP1450_WIDTH;
         SDL_FillRect(g_leds_surface, &dest, 0x00000000);
         SDL_BlitSurface(g_other_bmps[B_OVERLAY_LDP1450], &src, g_leds_surface, &dest);
         dest.x += OVERLAY_LDP1450_CHARACTER_SPACING;
    }
}

//  used to draw non LED stuff like scoreboard text
//  'which' corresponds to enumerated values
bool draw_othergfx(int which, int x, int y)
{
    g_sb_surface = g_other_bmps[which];

    if (g_sb_renderer && which == B_DL_PLAYER1)
        SDL_RenderClear(g_sb_renderer);

    SDL_Rect dest;
    dest.x = (short)x;
    dest.y = (short)y;
    dest.w = (unsigned short) g_sb_surface->w;
    dest.h = (unsigned short) g_sb_surface->h;

    SDL_BlitSurface(g_sb_surface, NULL, g_sb_blit_surface, &dest);

    return true;
}

// de-allocates all of the .bmps that we have allocated
void free_bmps()
{

    int nuke_index = 0;

    // get rid of all the LED's
    for (; nuke_index < LED_RANGE; nuke_index++) {
        free_one_bmp(g_led_bmps[nuke_index]);
    }
    for (nuke_index = 0; nuke_index < B_EMPTY; nuke_index++) {
        // check to make sure it exists before we try to free
        if (g_other_bmps[nuke_index]) {
            free_one_bmp(g_other_bmps[nuke_index]);
        }
    }
}

void free_one_bmp(SDL_Surface *candidate) { 
	SDL_FreeSurface(candidate); 
}

//////////////////////////////////////////////////////////////////////////////////////////

SDL_Window *get_window() { return g_window; }
SDL_Renderer *get_renderer() { return g_renderer; }
SDL_Texture *get_screen() { return g_overlay_texture; }
SDL_Surface *get_screen_blitter() { return g_screen_blitter; }
SDL_Texture *get_yuv_screen() { return g_yuv_texture; }
SDL_Surface *get_screen_leds() { return g_leds_surface; }

FC_Font *get_font() { return g_font; }

Uint16 get_video_width() { return g_vid_width; }
Uint16 get_video_height() { return g_vid_height; }

int get_textureaccess() { return g_texture_access; }
int get_scalefactor() { return g_scalefactor; }
unsigned int get_draw_width() { return g_draw_width; }
unsigned int get_draw_height() { return g_draw_height; }

bool get_opengl() { return g_opengl; }
bool get_vulkan() { return g_vulkan; }
bool get_fullscreen() { return g_fullscreen; }
bool get_singe_blend_sprite() { return g_singe_blend_sprite; }
bool get_use_old_osd() { return g_game->get_use_old_overlay(); }
bool get_video_timer_blank() { return g_yuv_video_timer_blank; }

void set_fullscreen(bool value) { g_fullscreen = value; }
void set_fakefullscreen(bool value) { g_fakefullscreen = value; }
void set_opengl(bool value) { g_opengl = value; }
void set_vulkan(bool value) { g_vulkan = value; }
void set_textureaccess(int value) { g_texture_access = value; }
void set_grabmouse(bool value) { g_grabmouse = value; }
void set_vsync(bool value) { g_vsync = value; }
void set_yuv_blue(bool value) { g_yuv_blue = value; }
void set_scanlines(bool value) { g_scanlines = value; }
void set_shunt(int value) { s_shunt = value; }
void set_alpha(int value) { s_alpha = value; }
void set_queue_screenshot(bool value) { queue_take_screenshot = value; }
void set_fullscreen_scale_nearest(bool value) { g_fs_scale_nearest = value; }
void set_singe_blend_sprite(bool value) { g_singe_blend_sprite = value; }
void set_yuv_video_blank(bool value) { g_yuv_video_needs_blank = value; }
void set_video_timer_blank(bool value) { g_yuv_video_timer_blank = value; }
void set_rotate_degrees(float fDegrees) { g_fRotateDegrees = fDegrees; }
void set_sboverlay_characterset(int value) { sboverlay_characterset = value; }
void set_subtitle_enabled(bool bEnabled) { g_bSubtitleShown = bEnabled; }
void set_subtitle_display(char *s) { subchar = strdup(s); }
void set_force_aspect_ratio(bool bEnabled) { g_bForceAspectRatio = bEnabled; }
void set_ignore_aspect_ratio(bool bEnabled) { g_bIgnoreAspectRatio = bEnabled; }
void set_aspect_ratio(int fRatio) { g_aspect_ratio = fRatio; }
void set_detected_height(int pHeight) { g_probe_height = pHeight; }
void set_detected_width(int pWidth) { g_probe_width = pWidth; }
void set_bezel_file(const char *bezelFile) { g_bezel_file = bezelFile; }
void set_score_bezel(bool bEnabled) { g_sb_bezel = bEnabled; }
void set_score_bezel_alpha(bool bEnabled) { g_sb_bezel_alpha = bEnabled; }
void set_score_bezel_scale(int value) { g_sb_bezel_scale = value; }

void set_scalefactor(int value)
{
    if (value > 100 || value < 50) // Validating in case user inputs crazy
                                   // values.
    {
        printline("Invalid scale value. Ignoring -scalefactor parameter.");
        g_scalefactor = 100;

    } else {
        g_scalefactor = value;
    }
}

// position sb_scoreboard
void set_sb_window(int xValue, int yValue)
{
    sb_window_pos_x = xValue;
    sb_window_pos_y = yValue;
}

// deal with MPEG headers aspect ratio
void set_aspect_change(int aspectWidth, int aspectHeight)
{
    g_aspect_width  = aspectWidth;
    g_aspect_height = aspectHeight;
}

// sets g_vid_width
void set_video_width(Uint16 width)
{
    // Let the user specify whatever width s/he wants (and suffer the
    // consequences)
    // We need to support arbitrary resolution to accomodate stuff like screen
    // rotation
    g_vid_width = width;
    g_vid_resized = true;
}

// sets g_vid_height
void set_video_height(Uint16 height)
{
    // Let the user specify whatever height s/he wants (and suffer the
    // consequences)
    // We need to support arbitrary resolution to accomodate stuff like screen
    // rotation
    g_vid_height = height;
    g_vid_resized = true;
}

void draw_string(const char *t, int col, int row, SDL_Surface *surface)
{
    SDL_Rect dest;
    dest.y = (short)(row);
    dest.w = (unsigned short)(6 * strlen(t));
    dest.h = 14;

    SDL_Surface *text_surface;

    if (g_game->get_use_old_overlay()) dest.x = (short)((col * 6));
    else dest.x = (short)((col * 5));

    SDL_FillRect(surface, &dest, 0x00000000);
    SDL_Color color={0xe1, 0xe1, 0xe1};
    text_surface=TTF_RenderText_Solid(g_ttfont, t, color);

    SDL_BlitSurface(text_surface, NULL, surface, &dest);
    SDL_FreeSurface(text_surface);
}

void draw_subtitle(char *s, bool insert)
{
    int x = (int)(g_draw_width - (g_draw_width * 0.97));
    int y = (int)(g_draw_height * 0.92);
    static int m_message_timer;
    const int timeout = 200;

    if (insert) {
       m_message_timer = 0;
       set_subtitle_enabled(true);
       set_subtitle_display(s);
    } else if (m_message_timer > timeout) {
       set_subtitle_enabled(false);
    }

    FC_Draw(g_font, g_renderer, x, y, s);
    m_message_timer++;
}

void draw_LDP1450_overlay()
{
    for (int i = 0; i < OVERLAY_LDP1450_LINES; i++) {
        if (LDP1450_CharSet[i].enable) {
		FC_Draw(g_fixfont, g_renderer,
		   LDP1450_CharSet[i].x, LDP1450_CharSet[i].y,
		   LDP1450_CharSet[i].OVERLAY_LDP1450_String);
	}
    }
}

// toggles fullscreen mode
void vid_toggle_fullscreen()
{
    if (!g_fakefullscreen) g_bezel_toggle = false;
    Uint32 flags = (SDL_GetWindowFlags(g_window) ^ SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (SDL_SetWindowFullscreen(g_window, flags) < 0) {
        LOGW << fmt("Toggle fullscreen failed: %s", SDL_GetError());
        return;
    }
    if ((flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) {
        SDL_RenderSetLogicalSize(g_renderer, g_viewport_width, g_viewport_height);
        if (g_bezel_texture || !SDL_RectEmpty(&g_sb_bezel_rect))
            g_bezel_toggle = true;
        return;
    }
    SDL_SetWindowSize(g_window, g_viewport_width, g_viewport_height);
    SDL_SetWindowPosition(g_window, SDL_WINDOWPOS_CENTERED,
                          SDL_WINDOWPOS_CENTERED);
}

void vid_toggle_scanlines()
{
    SDL_BlendMode mode;
    SDL_GetRenderDrawBlendMode(g_renderer, &mode);
    if (mode != SDL_BLENDMODE_BLEND && !g_scanlines)
        SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);

    if (g_scanlines) {
        g_scanlines = false;
        SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);
    } else g_scanlines = true;
}

void vid_setup_yuv_overlay (int width, int height) {
    // Prepare the YUV overlay, wich means setting up both the YUV surface and YUV texture.

    // If we have already been here, free things first.
    if (g_yuv_surface) {
        // Free both the YUV surface and YUV texture.
        vid_free_yuv_overlay();
    }

    g_yuv_surface = (g_yuv_surface_t*) malloc (sizeof(g_yuv_surface_t));

    // 12 bits (1 + 0.5 bytes) per pixel, and each plane has different size. Crazy stuff.
    g_yuv_surface->Ysize = width * height;
    g_yuv_surface->Usize = g_yuv_surface->Ysize / 4;
    g_yuv_surface->Vsize = g_yuv_surface->Ysize / 4;
   
    g_yuv_surface->Yplane = (uint8_t*) malloc (g_yuv_surface->Ysize);
    g_yuv_surface->Uplane = (uint8_t*) malloc (g_yuv_surface->Usize);
    g_yuv_surface->Vplane = (uint8_t*) malloc (g_yuv_surface->Vsize);

    g_yuv_surface->width  = width;
    g_yuv_surface->height = height;

    // Setup the threaded access stuff, since this surface is accessed from the vldp thread, too.
    g_yuv_surface->mutex = SDL_CreateMutex();
}

SDL_Texture *vid_create_yuv_texture (int width, int height) {
    g_yuv_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_YV12,
        g_texture_access, width, height);
    vid_blank_yuv_texture(true);
    return g_yuv_texture;
}

void vid_blank_yuv_texture (bool s) {

    if (g_yuv_blue) {
        // Blue: YUV#1DEB6B
        memset(g_yuv_surface->Yplane, 0x1d, g_yuv_surface->Ysize);
        memset(g_yuv_surface->Uplane, 0xeb, g_yuv_surface->Usize);
        memset(g_yuv_surface->Vplane, 0x6b, g_yuv_surface->Vsize);
    } else {
        // Black: YUV#108080, YUV(16,0,0)
        memset(g_yuv_surface->Yplane, 0x10, g_yuv_surface->Ysize);
        memset(g_yuv_surface->Uplane, 0x80, g_yuv_surface->Usize);
        memset(g_yuv_surface->Vplane, 0x80, g_yuv_surface->Vsize);
    }

    if (s) SDL_UpdateYUVTexture(g_yuv_texture, NULL,
	    g_yuv_surface->Yplane, g_yuv_surface->width,
            g_yuv_surface->Uplane, g_yuv_surface->width/2,
            g_yuv_surface->Vplane, g_yuv_surface->width/2);
}

// REMEMBER it updates the YUV surface ONLY: the YUV texture is updated on vid_blit().
int vid_update_yuv_overlay ( uint8_t *Yplane, uint8_t *Uplane, uint8_t *Vplane,
	int Ypitch, int Upitch, int Vpitch)
{
    // This function is called from the vldp thread, so access to the
    // yuv surface is protected (mutexed).
    // As a reminder, mutexes are very simple: this fn tries to lock(=get)
    // the mutex, but if it's already taken, LockMutex doesn't return
    // until the mutex is free and we can lock(=get) it here.
    SDL_LockMutex(g_yuv_surface->mutex);

    if (g_yuv_video_timer_blank) {

        vid_blank_yuv_texture(false);

    } else if (g_yuv_video_needs_blank) {

        vid_blank_yuv_texture(false);
        set_yuv_video_blank(false);

    } else {

        memcpy (g_yuv_surface->Yplane, Yplane, g_yuv_surface->Ysize);
        memcpy (g_yuv_surface->Uplane, Uplane, g_yuv_surface->Usize);
        memcpy (g_yuv_surface->Vplane, Vplane, g_yuv_surface->Vsize);

        g_yuv_surface->Ypitch = Ypitch;
        g_yuv_surface->Upitch = Upitch;
        g_yuv_surface->Vpitch = Vpitch;
    }

    g_yuv_video_needs_update = true;

    SDL_UnlockMutex(g_yuv_surface->mutex);

    return 0;
}

void vid_update_overlay_surface (SDL_Surface *tx, int x, int y) {
    // We have got here from game::blit(), which is also called when scoreboard is updated,
    // so in that case we simply return and don't do any overlay surface update. 
    if (g_scoreboard_needs_update) {
        return;
    }
    
    // Remember: tx is m_video_overlay[] passed from game::blit() 
    // Careful not comment this part on testing, because this rect is used in vid_blit!
    g_overlay_size_rect.x = (short)x;
    g_overlay_size_rect.y = (short)y;
    g_overlay_size_rect.w = tx->w;
    g_overlay_size_rect.h = tx->h;

    if (g_overlay_resize)
        g_render_size_rect = g_overlay_size_rect;

    if (g_enhance_overlay) {

        // DBX: 32bit overlay from Singe
        SDL_SetColorKey (tx, SDL_TRUE, 0x00000000);
        SDL_FillRect(g_screen_blitter, NULL, 0x00000000);
        SDL_BlitSurface(tx, NULL, g_screen_blitter, NULL);

    } else {

        // MAC: 8bpp to RGBA8888 conversion. Black pixels are considered totally transparent so they become 0x00000000;
        for (int i = 0; i < (tx->w * tx->h); i++) {
	    *((uint32_t*)(g_screen_blitter->pixels)+i) =
	    (0x00000000 | tx->format->palette->colors[*((uint8_t*)(tx->pixels)+i)].r) << 24 |
	    (0x00000000 | tx->format->palette->colors[*((uint8_t*)(tx->pixels)+i)].g) << 16 |
	    (0x00000000 | tx->format->palette->colors[*((uint8_t*)(tx->pixels)+i)].b) << 8  |
	    (0x00000000 | tx->format->palette->colors[*((uint8_t*)(tx->pixels)+i)].a);
        }
    }

    g_overlay_needs_update = true;
}

void vid_blit () {
    // *IF* we get to SDL_VIDEO_BLIT from game::blit(), then the access to the
    // overlay and scoreboard textures is done from the "hypseus" thread, that blocks
    // until all blitting operations are completed and only then loops again, so NO
    // need to protect the access to these surfaces or their needs_update booleans.
    // However, since we get here from game::blit(), the yuv "surface" is accessed
    // simultaneously from the vldp thread and from here, the main thread (to update
    // the YUV texture from the YUV surface), so access to that surface and it's
    // boolean DO need to be protected with a mutex.


    // First clear the renderer before the SDL_RenderCopy() calls for this frame.
    // Prevents stroboscopic effects on the background in fullscreen mode,
    // and is recommended by SDL_Rendercopy() documentation.
    SDL_RenderClear(g_renderer);

    // Does YUV texture need update from the YUV "surface"?
    // Don't try if the vldp object didn't call setup_yuv_surface (in noldp mode)
    if (g_yuv_surface) {
	SDL_LockMutex(g_yuv_surface->mutex);
	if (g_yuv_video_needs_update) {
	    // If we don't have a YUV texture yet (we may be here for the first time or the vldp could have
	    // ordered it's destruction in the mpeg_callback function because video dimensions have changed),
	    // create it now. Dimensions were passed to the video object (this) by the vldp object earlier,
	    // using vid_setup_yuv_texture()
	    if (!g_yuv_texture) {
		g_yuv_texture = vid_create_yuv_texture(g_yuv_surface->width, g_yuv_surface->height);
	    }

	    SDL_UpdateYUVTexture(g_yuv_texture, NULL,
		g_yuv_surface->Yplane, g_yuv_surface->Ypitch,
		g_yuv_surface->Uplane, g_yuv_surface->Vpitch,
		g_yuv_surface->Vplane, g_yuv_surface->Vpitch);
	    g_yuv_video_needs_update = false;
	}
	SDL_UnlockMutex(g_yuv_surface->mutex);
    }

    // Does OVERLAY texture need update from the scoreboard surface?
    if(g_scoreboard_needs_update) {
	SDL_UpdateTexture(g_overlay_texture, &g_leds_size_rect,
	    (void *)g_leds_surface->pixels, g_leds_surface->pitch);
    }

    // Does OVERLAY texture need update from the overlay surface?
    if(g_overlay_needs_update) {
	SDL_UpdateTexture(g_overlay_texture, &g_overlay_size_rect,
	    (void *)g_screen_blitter->pixels, g_screen_blitter->pitch);

	g_overlay_needs_update = false;
    }

    // Sadly, we have to RenderCopy the YUV texture on every blitting strike, because
    // the image on the renderer gets "dirty" with previous overlay frames on top of the yuv.
    if (g_yuv_texture) {
        if (!g_scale_view)
            SDL_RenderCopy(g_renderer, g_yuv_texture, NULL, NULL);
        else
            SDL_RenderCopy(g_renderer, g_yuv_texture, NULL, &g_scaling_rect);
    }

    // If there's an overlay texture, it means we are using some kind of overlay,
    // be it LEDs or any other thing, so RenderCopy it to the renderer ON TOP of the YUV video.
    if (g_overlay_texture) {
        if (!g_scale_view)
            SDL_RenderCopy(g_renderer, g_overlay_texture, &g_render_size_rect, NULL);
        else
            SDL_RenderCopy(g_renderer, g_overlay_texture, &g_render_size_rect, &g_scaling_rect);
    }

    // If there's a subtitle overlay
    if (g_bSubtitleShown) draw_subtitle(subchar, false);

    // LDP1450 overlay
    if (g_LDP1450_overlay) draw_LDP1450_overlay();

    if (g_scanlines)
        draw_scanlines(g_viewport_width, g_viewport_height, s_shunt);

    if (g_fRotateDegrees != 0) {
        if (g_yuv_texture)
            SDL_RenderCopyEx(g_renderer, g_yuv_texture, NULL, NULL,
                      g_fRotateDegrees, NULL, g_flipState);
        if (g_overlay_texture) {
            char ar = 2;
            SDL_Rect rotate_rect;
            if (g_aspect_ratio == ASPECTWS && g_overlay_resize) ar--;
            rotate_rect.w = g_render_size_rect.h + (g_render_size_rect.w >> ar);
            rotate_rect.h = g_render_size_rect.h;
            rotate_rect.x = rotate_rect.y = 0;
            SDL_RenderCopyEx(g_renderer, g_overlay_texture, &rotate_rect, NULL,
                      g_fRotateDegrees, NULL, g_flipState);
        }
    } else if (g_game->get_sinden_border())
	    if (!g_bezel_texture)
                draw_border(g_game->get_sinden_border(),
                      g_game->get_sinden_border_color());

    if (g_bezel_toggle) {
        SDL_RenderSetViewport(g_renderer, NULL);

        if (g_bezel_texture)
            SDL_RenderCopy(g_renderer, g_bezel_texture, NULL, NULL);

        if (g_sb_bezel) {
            SDL_RenderCopy(g_renderer, g_sb_texture, NULL, &g_sb_bezel_rect);
        }
        SDL_RenderSetLogicalSize(g_renderer, g_viewport_width, g_viewport_height);
    }

    SDL_RenderPresent(g_renderer);

    if (g_softsboard_needs_update) {
        SDL_RenderPresent(g_sb_renderer);
        g_softsboard_needs_update = false;
    }

    if (queue_take_screenshot) {
        set_queue_screenshot(false);
        take_screenshot();
    }
}

int get_yuv_overlay_width() {
    if (g_yuv_surface) {
        return g_yuv_surface->width;
    }
    else return 0; 
}

int get_yuv_overlay_height() {
    if (g_yuv_surface) {
        return g_yuv_surface->height;
    }
    else return 0; 
}

bool get_yuv_overlay_ready() {
    if (g_yuv_surface && g_yuv_texture) return true;
    else return false;
}

void take_screenshot()
{
    struct       stat info;
    char         filename[64];
    int32_t      screenshot_num = 0;
    bool         fullscreen = false;
    const char   dir[12] = "screenshots";

    if (stat(dir, &info ) != 0 )
        { LOGW << fmt("'%s' directory does not exist.", dir); return; }
    else if (!(info.st_mode & S_IFDIR))
        { LOGW << fmt("'%s' is not a directory.", dir); return; }

    int flags = SDL_GetWindowFlags(g_window);
    SDL_Rect     screenshot;
    SDL_Surface  *surface      = NULL;
    SDL_Surface  *scoreboard   = NULL;

    if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP || flags & SDL_WINDOW_MAXIMIZED)
        fullscreen = true;

    if (g_renderer) {

        if (fullscreen)
            SDL_RenderSetViewport(g_renderer, NULL);

        SDL_RenderGetViewport(g_renderer, &screenshot);

        if (fullscreen)
            SDL_GetRendererOutputSize(g_renderer, &screenshot.w, &screenshot.h);

        surface = SDL_CreateRGBSurface(0, screenshot.w, screenshot.h, 32, 0, 0, 0, 0);

        if (!surface) { LOGE << "Cannot allocate surface"; return; }

        if (SDL_RenderReadPixels(g_renderer, &screenshot, surface->format->format,
            surface->pixels, surface->pitch) != 0)
            { LOGE << fmt("Cannot ReadPixels - Something bad happened: %s", SDL_GetError());
                 g_game->set_game_errors(SDL_ERROR_SCREENSHOT);
                 set_quitflag(); }

        if (g_sb_window) {

            SDL_DisplayMode mode;
            if (SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(g_window), &mode) != 0)
                { LOGE << fmt("Cannot GetDisplayMode: %s", SDL_GetError());
                 g_game->set_game_errors(SDL_ERROR_SCREENSHOT);
                 set_quitflag(); }

            SDL_Rect     boardrect;
            SDL_RenderGetViewport(g_sb_renderer, &boardrect);
            scoreboard = SDL_CreateRGBSurface(0, boardrect.w, boardrect.h, 32, 0, 0, 0, 0);

            if (scoreboard) {
                SDL_RenderReadPixels(g_sb_renderer, &boardrect, scoreboard->format->format,
                       scoreboard->pixels, scoreboard->pitch);

                boardrect.x = (mode.w / screenshot.w) * sb_window_pos_x;
                boardrect.y = (mode.h / screenshot.h) * sb_window_pos_y;
                SDL_BlitSurface(scoreboard, NULL, surface, &boardrect);
            }
        }

    } else {
        LOGE << "Could not allocate renderer";
        return;
    }

    if (fullscreen)
        SDL_RenderSetLogicalSize(g_renderer, g_viewport_width, g_viewport_height);

    for (;;) {
        screenshot_num++;
        snprintf(filename, sizeof(filename), "%s%shypseus-%d.png",
          dir, PATH_SEPARATOR, screenshot_num);

        if (!mpo_file_exists(filename))
            break;
    }

    if (IMG_SavePNG(surface, filename) == 0) {
        LOGI << fmt("Wrote screenshot: %s", filename);
    } else {
        LOGE <<  fmt("Could not write screenshot: %s !!", filename);
    }

    if (scoreboard)
        SDL_FreeSurface(scoreboard);

    SDL_FreeSurface(surface);
}

void draw_scanlines(int w, int h, int l) {

    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, s_alpha);

    for (int i = 0; i < h; i+=l) {
       SDL_RenderDrawLine(g_renderer, 0, i, w, i);
    }

    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
}

void draw_border(int s, int c) {

    SDL_Rect tb, lb, rb, bb;
    unsigned char r = 0xff, g = 0xff, b = 0xff;

    switch(c)
    {
       case 0x62:
          r = 0x00; g = 0x66; b = 0xff;
          break;
       case 0x67:
          r = 0x66; g = 0xff; b = 0x40;
          break;
       case 0x72:
          r = 0xff; g = 0x1a; b = 0x1a;
          break;
       case 0x78:
          r = 0x00; g = 0x00; b = 0x00;
          break;
    }

    SDL_SetRenderDrawColor(g_renderer, r, g, b, SDL_ALPHA_OPAQUE);

    tb.x = lb.x = bb.x = 0;
    tb.y = lb.y = rb.y = 0;
    rb.x = g_viewport_width - s;
    bb.y = g_viewport_height - s;
    tb.w = bb.w = g_viewport_width;
    tb.h = bb.h = lb.w = rb.w = s;
    lb.h = rb.h = g_viewport_height;

    SDL_RenderFillRect(g_renderer, &tb);
    SDL_RenderFillRect(g_renderer, &lb);
    SDL_RenderFillRect(g_renderer, &rb);
    SDL_RenderFillRect(g_renderer, &bb);

    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);

    if (s < 0x0f) {

        unsigned char i = 0x01;
        if (s <= 0x02) i = 0x04;
        else if (s <= 0x08) i = 0x02;

        SDL_Rect tib, lib, rib, bib;
        tib.x = lib.x = bib.x = s;
        tib.y = lib.y = rib.y = s;
        rib.x = ((g_viewport_width - s) - i);
        bib.y = ((g_viewport_height - s) - i);
        tib.h = bib.h = lib.w = rib.w = i;
        tib.w = bib.w = g_viewport_width - (s<<1);
        lib.h = rib.h = g_viewport_height - (s<<1);

        SDL_RenderFillRect(g_renderer, &tib);
        SDL_RenderFillRect(g_renderer, &lib);
        SDL_RenderFillRect(g_renderer, &rib);
        SDL_RenderFillRect(g_renderer, &bib);
    }
}

}
