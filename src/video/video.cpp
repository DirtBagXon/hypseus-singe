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
#include "video.h"
#include "splash.h"
#include <SDL_syswm.h> // rdg2010
#include <SDL_image.h> // screenshot
#include <plog/Log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string> // for some error messages

using namespace std;

namespace {

// Convert surface to specified pixel format and free the original surface.
void ConvertSurface(SDL_Surface **surface, const SDL_PixelFormat *fmt)
{
    SDL_Surface *tmpSurface = SDL_ConvertSurface(*surface, fmt, 0);
    SDL_FreeSurface(*surface);
    *surface = tmpSurface;
}

void LogicalPosition(SDL_Rect *port, SDL_Rect *dst, int x, int y)
{
    SDL_Rect tmpRect = {0, 0, dst->w, dst->h};
    tmpRect.x = (((port->w - dst->w) * x) / 100);
    tmpRect.y = (((port->h - dst->h) * y) / 100);

    *dst = tmpRect;
}
}

namespace video {

int g_vid_width = 640, g_vid_height = 480; // default video dimensions

unsigned int g_probe_width = g_vid_width;
unsigned int g_probe_height = g_vid_height;

const int g_an_w = 220, g_an_h = 128;
const int g_sb_w = 340, g_sb_h = 480;

int g_viewport_width = g_vid_width, g_viewport_height = g_vid_height;
int g_aspect_width = 0, g_aspect_height = 0;
int g_alloc_screen = 0;
int g_score_screen = 0;
int g_displays = 0;
int g_display = 0;

int sb_window_pos_x = 0, sb_window_pos_y = 0;
int aux_bezel_pos_x = 0, aux_bezel_pos_y = 0;
int g_scale_h_shift = 100, g_scale_v_shift = 100;

// the current game overlay dimensions
unsigned int g_overlay_width = 0, g_overlay_height = 0;

FC_Font *g_font                    = NULL;
TTF_Font *g_ttfont                 = NULL;
SDL_Surface *g_led_bmps[LED_RANGE] = {0};
SDL_Surface *g_other_bmps[B_EMPTY] = {0};
SDL_Window *g_window               = NULL;
SDL_Window *g_sb_window            = NULL;
SDL_Renderer *g_renderer           = NULL;
SDL_Renderer *g_sb_renderer        = NULL;
SDL_Texture *g_overlay_texture     = NULL; // The OVERLAY texture, excluding LEDs wich are a special case
SDL_Texture *g_yuv_texture         = NULL; // The YUV video texture, registered from ldp-vldp.cpp
SDL_Surface *g_overlay_blitter     = NULL; // The main blitter surface
SDL_Surface *g_blit_surface        = NULL;
SDL_Surface *g_sb_surface          = NULL;
SDL_Texture *g_sb_texture          = NULL;
SDL_Surface *g_sb_blit_surface     = NULL;
SDL_Surface *g_aux_blit_surface    = NULL;
SDL_Texture *g_bezel_texture       = NULL;
SDL_Texture *g_aux_texture         = NULL;
SDL_Texture *g_rtr_texture         = NULL;

SDL_Rect g_aux_rect;
SDL_Rect g_border_rect;
SDL_Rect g_overlay_size_rect;
SDL_Rect g_scaling_rect = {0, 0, 0, 0};
SDL_Rect g_logical_rect = {0, 0, 0, 0};
SDL_Rect g_sb_bezel_rect = {0, 0, 0, 0};

SDL_Rect g_blit_size_rect = {0, 0, g_vid_width >> 1, g_vid_height >> 1};
SDL_Rect g_render_size_rect = g_blit_size_rect;
SDL_Rect g_annu_rect = g_blit_size_rect;

SDL_Rect *g_yuv_rect[2] = { NULL };

SDL_DisplayMode g_displaymode;

bool queue_take_screenshot = false;
bool g_scale_linear = false;
bool g_singe_blend_sprite = false;
bool g_scanlines = false;
bool g_yuv_grayscale = false;
bool g_fakefullscreen = false;
bool g_opengl = false;
bool g_vulkan = false;
bool g_teardown = false;
bool g_intro = true;
bool g_forcetop = false;
bool g_grabmouse = false;
bool g_vsync = true;
bool g_yuv_blue = false;
bool g_vid_resized = false;
bool g_enhance_overlay = false;
bool g_overlay_resize = false;
bool g_bForceAspect = false;
bool g_bIgnoreAspect = false;
bool g_fullscreen = false; // initialize video in fullscreen
bool g_bezel_toggle = false;
bool g_bezel_reverse = false;
bool g_bezel_load = false;
bool g_sb_bezel = false;
bool g_scaled = false;
bool g_rotate = false;
bool g_keyboard_bezel = false;
bool g_aux_bezel = false;
bool g_ded_annun_bezel = false;
bool g_annun_lamps = false;
bool g_vertical_orientation = false;
bool g_mousemotion = false;
bool g_sboverlay_white = false;

int g_scalefactor = 100;   // by RDG2010 -- scales the image to this percentage
int g_aspect_ratio = 0;
int sboverlay_characterset = 2;
int g_texture_access = SDL_TEXTUREACCESS_TARGET;
int g_yuv_display = YUV_VISIBLE;

int8_t g_sb_bezel_alpha = 0;
int8_t g_aux_bezel_alpha = 0;
int8_t g_rescale = 0;

int g_sb_bezel_scale = 14;
int g_aux_bezel_scale = 12;
int g_bezel_scalewidth = 0;

uint8_t g_scanline_alpha = 128;
uint8_t g_scanline_shunt = 2;

// Move subtitle rendering to SDL_RenderPresent(g_renderer);
bool g_bSubtitleShown = false;
char g_window_title[TITLE_LENGTH] = "";
char *subchar = NULL;

std::string g_bezel_path = "bezels";
std::string g_bezel_file;
std::string g_tqkeys;

// degrees in clockwise rotation
float g_fRotateDegrees = 0.0f;
double g_aux_ratio = 1.0f;

// YUV stretching
float g_yuv_scale[2] = { 1.0f, 1.0f };

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
bool g_aux_needs_update        = false;
bool g_yuv_skip                = true;

////////////////////////////////////////////////////////////////////////////////////////////////////

// initializes the window in which we will draw our BMP's
// returns true if successful, false if failure
bool init_display()
{
    bool result = false;
    static bool notify = false;
    constexpr char title[] = "HYPSEUS Singe: Multiple Arcade Laserdisc Emulator";

    SDL_SysWMinfo info;
    Uint32 sdl_flags = SDL_WINDOW_SHOWN;
    Uint32 sdl_sb_flags = SDL_WINDOW_BORDERLESS;
    Uint8 sdl_render_flags = SDL_RENDERER_TARGETTEXTURE;

    // if we were able to initialize the video properly
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) >= 0) {

        if (g_fullscreen)
            sdl_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        else if (g_fakefullscreen && g_alloc_screen == 0)
            sdl_flags |= SDL_WINDOW_MAXIMIZED | SDL_WINDOW_BORDERLESS;

        if (g_forcetop)
            sdl_flags |= SDL_WINDOW_ALWAYS_ON_TOP;
        else
            sdl_sb_flags |= SDL_WINDOW_ALWAYS_ON_TOP;

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
        g_probe_width = std::max((int)g_probe_width, 320);
        g_probe_height = std::max((int)g_probe_height, 240);

        if (g_vid_resized) {
            g_viewport_width  = g_vid_width;
            g_viewport_height = g_vid_height;

        } else {

            if (!g_bIgnoreAspect && g_aspect_width > 0) {
                g_viewport_width  = g_aspect_width;
                g_viewport_height = g_aspect_height;

            } else {
                g_viewport_width  = g_probe_width;
                g_viewport_height = g_probe_height;
            }
        }

        if (g_aspect_ratio == ASPECTPD) {
            if (!g_game->get_manymouse()) {
                if (!g_fullscreen) {
                    SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
                }
                g_mousemotion = true;
            }
        }

        // Enforce 4:3 aspect ratio
        if (g_bForceAspect) {
            double dCurAspect = (double)g_viewport_width / g_viewport_height;
            const double dTARGET_ASPECT_RATIO = 4.0 / 3.0;

            if (dCurAspect < dTARGET_ASPECT_RATIO) {
                g_viewport_height = (g_viewport_width * 3) / 4;
            }
            else if (dCurAspect > dTARGET_ASPECT_RATIO) {
                g_viewport_width = (g_viewport_height * 4) / 3;
            }
        }

        if (g_window) resize_cleanup();

        if (g_fRotateDegrees != 0 && g_fRotateDegrees != 180) {
            switch(g_aspect_ratio) {
            case ASPECTWS:
                g_scalefactor = (!g_scaled) ? 56 : g_scalefactor;
                break;
            case ASPECTPD:
                g_scalefactor = (!g_scaled) ? 100 : g_scalefactor;
                break;
            default:
                g_scalefactor = (!g_scaled) ? 75 : g_scalefactor;
                break;
            }
        }

        g_displays = SDL_GetNumVideoDisplays();
        SDL_Rect displayDimensions[g_displays];

        for (int i = 0; i < g_displays; i++) {
            SDL_GetDisplayBounds(i, &displayDimensions[i]);
        }

        if (g_alloc_screen > g_display && g_alloc_screen < g_displays)
            g_display = g_alloc_screen;

        if (notify && g_window && !g_teardown) {
            SDL_SetWindowSize(g_window, g_viewport_width, g_viewport_height);
            SDL_SetWindowPosition(g_window, displayDimensions[g_display].x +
                ((displayDimensions[g_display].w - g_viewport_width) >> 1),
		    displayDimensions[g_display].y +
                        ((displayDimensions[g_display].h - g_viewport_height) >> 1));
        } else {

            if (g_teardown && g_window) {
                SDL_DestroyWindow(g_window);
                SDL_Delay(40);
            }

            g_window = SDL_CreateWindow(title,
	        displayDimensions[g_display].x +
                    ((displayDimensions[g_display].w - g_viewport_width) >> 1),
	        displayDimensions[g_display].y +
                    ((displayDimensions[g_display].h - g_viewport_height) >> 1),
	        g_viewport_width, g_viewport_height, sdl_flags);
        }

        if (!g_window) {
            LOGE << fmt("Could not initialize window: %s", SDL_GetError());
            g_game->set_game_errors(SDL_ERROR_INIT);
            set_quitflag();
            goto exit;
        } else {

            if (g_game->m_sdl_software_rendering)
                sdl_render_flags |= SDL_RENDERER_SOFTWARE;
            else sdl_render_flags |= SDL_RENDERER_ACCELERATED;

            if (strlen(g_window_title) > 0)
                SDL_SetWindowTitle(g_window, g_window_title);

            SDL_RWops* ops = SDL_RWFromConstMem(ghci, sizeof(ghci));
            SDL_Surface *rep = IMG_Load_RW(ops, 1);

            if (rep != NULL) {
                SDL_SetWindowIcon(g_window, rep);
                SDL_FreeSurface(rep);
            }

            Uint8 sdl_sb_render_flags = sdl_render_flags;

            if (g_vsync && (sdl_render_flags & SDL_RENDERER_ACCELERATED))
                sdl_render_flags |= SDL_RENDERER_PRESENTVSYNC;

            g_renderer = SDL_CreateRenderer(g_window, -1, sdl_render_flags);

            SDL_RaiseWindow(g_window);
            SDL_SetWindowInputFocus(g_window);

            if (!g_renderer) {
                LOGE << fmt("Could not initialize renderer: %s", SDL_GetError());
                g_game->set_game_errors(SDL_ERROR_MAINRENDERER);
                set_quitflag();
                goto exit;
            } else {

                if (g_keyboard_bezel) {

                    g_aux_needs_update = true;

                    g_tqkeys = g_bezel_path + "/tqkeys.png";

                    if (!mpo_file_exists(g_tqkeys.c_str()))
                        g_tqkeys = "pics/tqkeys.png";
                }

                SDL_VERSION(&info.version);

                // Set linear filtering
                if (g_scale_linear)
                    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

                // Create a 32-bit surface with alpha component.
                int surfacebpp;
                Uint32 Rmask, Gmask, Bmask, Amask;
                SDL_PixelFormatEnumToMasks(SDL_PIXELFORMAT_RGBA8888, &surfacebpp,
                                &Rmask, &Gmask, &Bmask, &Amask);

                if (g_game->m_software_scoreboard) {

                    if (!g_sb_blit_surface)
                        g_sb_blit_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, g_sb_w, g_sb_h,
                                               surfacebpp, Rmask, Gmask, Bmask, Amask);

                    if (g_sb_bezel_alpha > 1)
                        SDL_SetColorKey(g_sb_blit_surface, SDL_TRUE, 0x000000ff);

                    if (g_sb_bezel) {

                        if (!g_sb_bezel_alpha)
                            SDL_FillRect(g_sb_blit_surface, NULL, 0x000000ff);

                    } else if (SDL_GetWindowWMInfo(g_window, &info) && !g_sb_window) {

                        double scale = double((g_sb_bezel_scale >> 1) / 7.0f);

                        g_sb_window = SDL_CreateWindow(NULL, sb_window_pos_x,
                                          sb_window_pos_y, (scale * g_sb_w), (scale * g_sb_h),
                                              sdl_sb_flags);

                        if (!g_sb_window) {
                            LOGE << fmt("Could not initialize scoreboard window: %s", SDL_GetError());
                            g_game->set_game_errors(SDL_ERROR_SCOREWINDOW);
                            set_quitflag();
                        }

			if (g_displays > 1) {

                            int s = 1;
                            if (g_score_screen > s && g_score_screen <= g_displays)
                                s = g_score_screen;

                            s = (g_display == 1 && s == 1) ? 0 : s;

                            SDL_SetWindowPosition(g_sb_window,
                               displayDimensions[s].x + sb_window_pos_x,
                                  displayDimensions[s].y + sb_window_pos_y);
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

                // DBX: Fullscreen mode, get the logical render stats
                if ((sdl_flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0 ||
                               (sdl_flags & SDL_WINDOW_MAXIMIZED) != 0) {

                    if ((sdl_flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) {
                        SDL_GetDisplayBounds(g_display, &g_logical_rect);
                    } else {

                        if (SDL_GetDesktopDisplayMode(0, &g_displaymode) != 0) {
                            LOGE << fmt("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
                            set_quitflag();
                            goto exit;
                        }
                        g_logical_rect.w = g_displaymode.w;
                        g_logical_rect.h = g_displaymode.h;
                    }

                    if (!g_bezel_file.empty()) g_bezel_toggle = true;

                    format_fullscreen_render();

                } else
                    format_window_render();

                if (g_aux_bezel) {

                    if (g_ded_annun_bezel) g_aux_bezel_scale--;

                    g_aux_ratio = (float)g_an_h / (float)g_an_w;

                    g_aux_blit_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, g_an_w, g_an_h,
                                        surfacebpp, Rmask, Gmask, Bmask, Amask);

                    if (!g_aux_blit_surface) {
                        LOGE << "Failed to load annunicator surface";
                        set_quitflag();
                    }

                    if (g_aux_bezel_alpha > 1)
                        SDL_SetColorKey(g_aux_blit_surface, SDL_TRUE, 0x000000ff);

                    calc_annun_rect();

                    if (g_ded_annun_bezel)
                        LogicalPosition(&g_logical_rect, &g_aux_rect, 99, 10);
                    else
                        LogicalPosition(&g_logical_rect, &g_aux_rect, 100, 90);

                    // argument override
                    if (aux_bezel_pos_x || aux_bezel_pos_y) {
                        g_aux_rect.x = aux_bezel_pos_x;
                        g_aux_rect.y = aux_bezel_pos_y;
                    }

                    draw_annunciator(0);
                    if (!g_ded_annun_bezel)
                        draw_ranks();
                }

		// Always hide the mouse cursor
                SDL_ShowCursor(SDL_DISABLE);

                if (g_grabmouse)
                    SDL_SetWindowGrab(g_window, SDL_TRUE);

                if (g_scanlines)
                    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);

                if (g_game->get_sinden_border())
                    g_scale_h_shift = g_scale_v_shift = 100;

		g_overlay_blitter =
		    SDL_CreateRGBSurface(SDL_SWSURFACE, g_overlay_width, g_overlay_height,
					surfacebpp, Rmask, Gmask, Bmask, Amask);

		// Check for game overlay enhancements (depth and size)
		g_enhance_overlay = g_game->get_overlay_upgrade();
		g_overlay_resize = g_game->get_dynamic_overlay();

		g_blit_surface =
		    SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 240,
					surfacebpp, Rmask, Gmask, Bmask, Amask);

                // Convert the LEDs surface to the destination surface format for faster
                // blitting, and set it's color key to NOT copy 0x000000ff pixels. We
                // couldn't do it earlier in load_bmps() because we need the
                // g_screen_blitter format.
                ConvertSurface(&g_other_bmps[B_OVERLAY_LEDS], g_overlay_blitter->format);
                SDL_SetColorKey(g_other_bmps[B_OVERLAY_LEDS], SDL_TRUE, 0x000000ff);

                if (g_aux_blit_surface) {
                    ConvertSurface(&g_aux_blit_surface, g_overlay_blitter->format);
                }

                ConvertSurface(&g_other_bmps[B_OVERLAY_LDP1450], g_overlay_blitter->format);
                SDL_SetColorKey(g_other_bmps[B_OVERLAY_LDP1450], SDL_TRUE, 0x000000ff);

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
                if (!notify && g_intro) splash();
                result = true;
            }
        }
    } else {
        LOGE << fmt("Could not initialize SDL: %s", SDL_GetError());
        g_game->set_game_errors(SDL_ERROR_INIT);
        set_quitflag();
        goto exit;
    }

exit:
    notify = true;
    return (result);
}

void reset_yuv_overlay()
{
    vid_setup_yuv_overlay(g_probe_width, g_probe_height);
    SDL_Delay(2);
}

void vid_free_yuv_overlay () {
    // Here we free both the YUV surface and YUV texture.
    SDL_DestroyMutex (g_yuv_surface->mutex);
   
    free(g_yuv_surface->Yplane);
    free(g_yuv_surface->Uplane);
    free(g_yuv_surface->Vplane);
    free(g_yuv_surface);

    SDL_DestroyTexture(g_yuv_texture);

    g_yuv_surface = NULL;
    g_yuv_texture = NULL;
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
    if (g_aux_blit_surface) SDL_FreeSurface(g_aux_blit_surface);

    g_sb_texture = NULL;
    g_sb_renderer = NULL;
    g_sb_window = NULL;
    g_sb_blit_surface = NULL;
    g_aux_blit_surface = NULL;

    SDL_FreeSurface(g_overlay_blitter);
    SDL_FreeSurface(g_blit_surface);

    g_overlay_blitter = NULL;
    g_blit_surface  = NULL;

    FC_FreeFont(g_font);
    TTF_CloseFont(g_ttfont);

    if (g_bezel_texture)
        SDL_DestroyTexture(g_bezel_texture);

    if (g_aux_texture)
        SDL_DestroyTexture(g_aux_texture);

    for (int i = 0; i < 2; ++i) {
        delete g_yuv_rect[i];
        g_yuv_rect[i] = NULL;
    }

    g_bezel_texture = NULL;
    g_aux_texture = NULL;

    if (g_rtr_texture) SDL_DestroyTexture(g_rtr_texture);

    SDL_DestroyTexture(g_overlay_texture);
    SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_window);

    g_rtr_texture = NULL;
    g_overlay_texture = NULL;
    g_renderer = NULL;
    g_window = NULL;

    return (true);
}

void resize_cleanup()
{
    g_rotate = false;
    g_bezel_load = false;

    if (g_overlay_blitter) SDL_FreeSurface(g_overlay_blitter);
    if (g_blit_surface) SDL_FreeSurface(g_blit_surface);
    if (g_aux_blit_surface) SDL_FreeSurface(g_aux_blit_surface);

    g_overlay_blitter = NULL;
    g_blit_surface = NULL;
    g_aux_blit_surface = NULL;

    if (g_bezel_texture) SDL_DestroyTexture(g_bezel_texture);
    if (g_aux_texture) SDL_DestroyTexture(g_aux_texture);
    if (g_sb_texture) SDL_DestroyTexture(g_sb_texture);

    g_bezel_texture = NULL;
    g_aux_texture = NULL;
    g_sb_texture = NULL;

    if (g_rtr_texture) SDL_DestroyTexture(g_rtr_texture);
    if (g_overlay_texture) SDL_DestroyTexture(g_overlay_texture);

    if (g_renderer) {
        SDL_RenderClear(g_renderer);
        SDL_RenderPresent(g_renderer);
        SDL_DestroyRenderer(g_renderer);
    }

    g_rtr_texture = NULL;
    g_overlay_texture = NULL;
    g_renderer = NULL;

    SDL_Delay(100);
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

void colorLeds(SDL_Surface *src)
{
    if (g_sboverlay_white) return;
    SDL_Surface* tmp = SDL_CreateRGBSurface(0, src->w, src->h,
                           src->format->BitsPerPixel,
                           src->format->Rmask, src->format->Gmask,
                           src->format->Bmask, src->format->Amask);

    uint8_t r, g, b, a;
    SDL_LockSurface(tmp);
    SDL_LockSurface(src);

    for (int y = 0; y < src->h; ++y) {
        for (int x = 0; x < src->w; ++x) {

            uint8_t* pixel = ((uint8_t*)src->pixels) + y * src->pitch + x * src->format->BytesPerPixel;

            SDL_GetRGBA(*(uint32_t*)pixel, src->format, &r, &g, &b, &a);

            if (r == 0xff && g == 0xff && b == 0xff)
	        { r = 0xc8; g = 0x0d; b = 0x0d; }
            if (r == 0x50 && g == 0x4a && b == 0x4a)
	        { r = 0x38; g = 0x00; b = 0x00; }

            uint8_t* tmpPixel = ((uint8_t*)tmp->pixels) + y * tmp->pitch + x * src->format->BytesPerPixel;
            *(uint32_t*)tmpPixel = SDL_MapRGBA(tmp->format, r, g, b, a);
        }
    }

    SDL_UnlockSurface(tmp);
    SDL_UnlockSurface(src);

    SDL_BlitSurface(tmp, NULL, src, NULL);
    SDL_FreeSurface(tmp);
}

SDL_Surface *load_led_strip(const char *filename, bool bezel)
{
    std::string filepath = fmt("pics/%s", filename);

    SDL_Surface *result  = SDL_LoadBMP(filepath.c_str());

    if (!result) {
        LOGE << fmt("Could not load bitmap: %s", filepath.c_str());
    } else colorLeds(result);

    return (result);
}

// loads all the .bmp's
// returns true if they were all successfully loaded, or a false if they weren't
bool load_bmps()
{

    bool result = true; // assume success unless we hear otherwise
    int index   = 0;
    char filename[81];

    for (; index < LED_RANGE; index++) {
        snprintf(filename, sizeof(filename), "led%d.bmp", index);

        g_led_bmps[index] = load_one_bmp(filename, false);

        // If the bit map did not successfully load
        if (g_led_bmps[index] == 0) {
            result = false;
        }
    }

    g_other_bmps[B_DL_PLAYER1]     = load_one_bmp("player1.bmp", false);
    g_other_bmps[B_DL_PLAYER2]     = load_one_bmp("player2.bmp", false);
    g_other_bmps[B_DL_LIVES]       = load_one_bmp("lives.bmp", false);
    g_other_bmps[B_DL_CREDITS]     = load_one_bmp("credits.bmp", false);
    g_other_bmps[B_TQ_TIME]        = load_one_bmp("time.bmp", false);

    if (sboverlay_characterset != 2)
	g_other_bmps[B_OVERLAY_LEDS] = load_led_strip("overlayleds1.bmp", false);
    else
	g_other_bmps[B_OVERLAY_LEDS] = load_led_strip("overlayleds2.bmp", false);
   
    g_other_bmps[B_OVERLAY_LDP1450] = load_one_bmp("ldp1450font.bmp", false);

    g_other_bmps[B_ACE_CADET]   = load_one_bmp("cadet.bmp", true);
    g_other_bmps[B_ACE_CAPTAIN] = load_one_bmp("captain.bmp", true);
    g_other_bmps[B_ACE_SPACE]   = load_one_bmp("spaceace.bmp", true);

    g_other_bmps[B_CADET_OFF]   = load_one_bmp("offcadet.bmp", true);
    g_other_bmps[B_CAPTAIN_OFF] = load_one_bmp("offcaptain.bmp", true);
    g_other_bmps[B_ACE_OFF]     = load_one_bmp("offspaceace.bmp", true);
    g_other_bmps[B_CADET_ON]    = load_one_bmp("oncadet.bmp", true);
    g_other_bmps[B_CAPTAIN_ON]  = load_one_bmp("oncaptain.bmp", true);
    g_other_bmps[B_ACE_ON]      = load_one_bmp("onspaceace.bmp", true);

    g_other_bmps[B_ANUN_ON]     = load_one_png("annunon.png");
    g_other_bmps[B_ANUN_OFF]    = load_one_png("annunoff.png");

    // check to make sure they all loaded
    for (index = 0; index < B_EMPTY; index++) {
        if (g_other_bmps[index] == NULL && index != B_MIA) {
            result = false;
        }
    }

    return (result);
}

SDL_Surface *load_one_bmp(const char *filename, bool bezel)
{
    std::string filepath;

    if (bezel)
        filepath = fmt("%s/%s", g_bezel_path.c_str(), filename);

    if (!mpo_file_exists(filepath.c_str()))
        filepath = fmt("pics/%s", filename);

    SDL_Surface *result  = SDL_LoadBMP(filepath.c_str());

    if (!result) {
        LOGE << fmt("Could not load bitmap: %s", filepath.c_str());
    }

    return (result);
}

SDL_Surface *load_one_png(const char *filename)
{
    std::string filepath = fmt("%s/%s", g_bezel_path.c_str(), filename);

    if (!mpo_file_exists(filepath.c_str()))
        filepath = fmt("pics/%s", filename);

    SDL_Surface *result = IMG_Load(filepath.c_str());

    if (!result) {
        LOGE << fmt("Could not load png: %s", filepath.c_str());
    }

    return (result);
}

// Draw's one of our LED's to the screen
// value contains the bitmap to draw (0-9 is valid)
// x and y contain the coordinates on the screen
// This function is called from img-scoreboard.cpp
// 1 is returned on success, 0 on failure
bool draw_led(int value, int x, int y, unsigned char end)
{
    g_sb_surface = g_led_bmps[value];
    static char led = 0;

    SDL_Rect dest;
    dest.x = (short) x;
    dest.y = (short) y;
    dest.w = (unsigned short) g_sb_surface->w;
    dest.h = (unsigned short) g_sb_surface->h;

    SDL_BlitSurface(g_sb_surface, NULL, g_sb_blit_surface, &dest);

    if (led == end) {
        if (g_sb_bezel)
            g_sb_texture = SDL_CreateTextureFromSurface(g_renderer,
                               g_sb_blit_surface);
        else {
            g_sb_texture = SDL_CreateTextureFromSurface(g_sb_renderer,
                               g_sb_blit_surface);
            SDL_RenderCopy(g_sb_renderer, g_sb_texture, NULL, NULL);
            g_softsboard_needs_update = true;
        }
        led = -1;
    }
    led++;
    return true;
}

bool draw_annunciator(int which)
{
    if (!g_ded_annun_bezel)
        draw_annunciator1(which);
    else draw_annunciator2(which);

    return true;
}

bool draw_ranks()
{
    if (g_annun_lamps) return false;

    SDL_Rect dest;
    dest.x = 10;
    dest.y = dest.x - 1;

    for (int i = B_ACE_SPACE; i < B_MIA; i++) {

        g_sb_surface = g_other_bmps[i];

        dest.w = (unsigned short) g_sb_surface->w;
        dest.h = (unsigned short) g_sb_surface->h;
        SDL_BlitSurface(g_sb_surface, NULL, g_aux_blit_surface, &dest);
        dest.y = dest.y + (ANUN_CHAR_HEIGHT << 1) + (ANUN_CHAR_HEIGHT);
    }

    return true;
}

bool draw_annunciator1(int which)
{
    g_sb_surface = g_other_bmps[B_ANUN_OFF];

    SDL_Rect dest;
    uint8_t spacer = 4;

    dest.x = g_aux_blit_surface->w - (g_sb_surface->w
                   + (g_sb_surface->w >> 1));

    dest.w = (unsigned short) g_sb_surface->w;
    dest.h = (unsigned short) g_sb_surface->h;

    for (int i = 0; i < ANUN_LEVELS; i++) {
        dest.y = ((dest.h + ANUN_CHAR_HEIGHT) * i) + spacer;
        if (g_annun_lamps) dest.x = 110 - (-i * 15);
        SDL_FillRect(g_aux_blit_surface, &dest, 0x00000000);
        SDL_BlitSurface(g_sb_surface, NULL, g_aux_blit_surface, &dest);
    }

    if (which) {
        g_sb_surface = g_other_bmps[B_ANUN_ON];
        if (g_annun_lamps) dest.x = 110 - ((-which + 1) * 15);
        dest.y = ((dest.h + ANUN_CHAR_HEIGHT) * --which) + spacer;
        SDL_BlitSurface(g_sb_surface, NULL, g_aux_blit_surface, &dest);
    }

    g_aux_needs_update = true;
    return true;
}

bool draw_annunciator2(int which)
{
    SDL_Rect dest;

    dest.x = 0;
    dest.h = 40;
    dest.w = 220;

    for (int i = B_ACE_OFF; i < B_EMPTY; i++) {
        g_sb_surface = g_other_bmps[i];
        dest.y = (ANUN_RANK_HEIGHT * (i - B_ACE_OFF));
        SDL_FillRect(g_aux_blit_surface, &dest, 0x00000000);
        SDL_BlitSurface(g_sb_surface, NULL, g_aux_blit_surface, &dest);
    }

    if (which) {
        g_sb_surface = g_other_bmps[B_MIA + which];
        dest.y = ANUN_RANK_HEIGHT * --which;
        SDL_BlitSurface(g_sb_surface, NULL, g_aux_blit_surface, &dest);
    }

    g_aux_needs_update = true;
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
        SDL_FillRect(g_blit_surface, &dest, 0x00000000);
        SDL_BlitSurface(g_other_bmps[B_OVERLAY_LEDS], &src, g_blit_surface, &dest);

        dest.x += OVERLAY_LED_WIDTH;
    }

    g_scoreboard_needs_update = true;

    // MAC: Even if we updated the overlay surface here, there's no need to do not-thread-safe stuff
    // like SDL_UpdateTexture(), SDL_RenderCopy(), etc... until we are going to compose a final frame
    // with the YUV texture and the overlay on top (which is issued from vldp for now) in VIDEO_RUN_BLIT.
}

void draw_singleline_LDP1450(char *LDP1450_String, int start_x, int y)
{
    SDL_Rect src, dest;

    int i = 0;
    int value = 0;
    int LDP1450_strlen;
    g_scoreboard_needs_update = true;

    if (g_aspect_ratio == ASPECTSD &&
             g_probe_width == NOSQUARE)
        start_x = (start_x - (start_x >> 2));

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
         SDL_FillRect(g_blit_surface, &dest, 0x00000000);
         SDL_BlitSurface(g_other_bmps[B_OVERLAY_LDP1450], &src, g_blit_surface, &dest);
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
SDL_Surface *get_screen_blitter() { return g_overlay_blitter; }
SDL_Texture *get_yuv_screen() { return g_yuv_texture; }
SDL_Surface *get_screen_leds() { return g_blit_surface; }
SDL_Rect get_sb_rect() { return g_sb_bezel_rect; }
SDL_Rect get_aux_rect() { return g_aux_rect; }

FC_Font *get_font() { return g_font; }

Uint16 get_video_width() { return g_vid_width; }
Uint16 get_video_height() { return g_vid_height; }
Uint16 get_viewport_width() { return g_viewport_width; }
Uint16 get_viewport_height() { return g_viewport_height; }

int get_score_bezel_scale() { return g_sb_bezel_scale; }
int get_aux_bezel_scale() { return g_aux_bezel_scale; }

int get_scale_h_shift() { return g_scale_h_shift; };
int get_scale_v_shift() { return g_scale_v_shift; };
int get_textureaccess() { return g_texture_access; }
int get_scalefactor() { return g_scalefactor; }
unsigned int get_logical_width() { return g_logical_rect.w; }
unsigned int get_logical_height() { return g_logical_rect.h; }
float get_fRotateDegrees() { return g_fRotateDegrees; }

bool get_opengl() { return g_opengl; }
bool get_vulkan() { return g_vulkan; }
bool get_fullscreen() { return g_fullscreen; }
bool get_aux_bezel() { return (g_aux_texture != NULL); }
bool get_fullwindow() { return g_fakefullscreen; }
bool get_singe_blend_sprite() { return g_singe_blend_sprite; }
bool get_video_resized() { return g_vid_resized; }
bool use_old_font() { return g_game->use_old_overlay(); }

void set_fullscreen(bool value) { g_fullscreen = value; }
void set_fakefullscreen(bool value) { g_fakefullscreen = value; }
void set_opengl(bool value) { g_opengl = value; }
void set_vulkan(bool value) { g_vulkan = value; }
void set_teardown() { g_teardown = true; }
void set_grayscale(bool value) { g_yuv_grayscale = value; }
void set_forcetop(bool value) { g_forcetop = value; }
void set_textureaccess(int value) { g_texture_access = value; }
void set_grabmouse(bool value) { g_grabmouse = value; }
void set_vsync(bool value) { g_vsync = value; }
void set_intro(bool value) { g_intro = value; }
void set_yuv_blue(bool value) { g_yuv_blue = value; }
void set_scanlines(bool value) { g_scanlines = value; }
void set_shunt(uint8_t value) { g_scanline_shunt = value; }
void set_alpha(uint8_t value) { g_scanline_alpha = value; }
void set_queue_screenshot(bool value) { queue_take_screenshot = value; }
void set_scale_linear(bool value) { g_scale_linear = value; }
void set_singe_blend_sprite(bool value) { g_singe_blend_sprite = value; }
void set_sboverlay_characterset(int value) { sboverlay_characterset = value; }
void set_sboverlay_white(bool value) { g_sboverlay_white = value; }
void set_subtitle_display(char *s) { subchar = strdup(s); }
void set_force_aspect_ratio(bool value) { g_bForceAspect = value; }
void set_ignore_aspect_ratio(bool value) { g_bIgnoreAspect = value; }
void set_aspect_ratio(int fRatio) { g_aspect_ratio = fRatio; }
void set_detected_height(int pHeight) { g_probe_height = pHeight; }
void set_detected_width(int pWidth) { g_probe_width = pWidth; }
void set_score_bezel_alpha(int8_t value) { g_sb_bezel_alpha = value; }
void set_score_bezel_scale(int value) { g_sb_bezel_scale = value; }
void set_aux_bezel_scale(int value) { g_aux_bezel_scale = value; }
void set_aux_bezel_alpha(int8_t value) { g_aux_bezel_alpha = value; }
void set_ded_annun_bezel(bool value) { g_ded_annun_bezel = value; }
void set_scale_h_shift(int value) { g_scale_h_shift = value; }
void set_scale_v_shift(int value) { g_scale_v_shift = value; }
void set_display_screen(int value) { g_alloc_screen = value; }
void set_score_screen(int value) { g_score_screen = value; }

void toggle_grabmouse()
{
     g_grabmouse = !g_grabmouse;
     SDL_SetWindowGrab(g_window, g_grabmouse ? SDL_TRUE : SDL_FALSE);
}

void set_yuv_blank(int value)
{
    g_yuv_display = value;

    if (value == YUV_BLANK)
        vid_update_yuv_overlay(g_yuv_surface->Yplane, g_yuv_surface->Uplane,
        g_yuv_surface->Vplane, g_yuv_surface->Ypitch, g_yuv_surface->Upitch,
        g_yuv_surface->Vpitch);
}

void set_scalefactor(int value)
{
     g_scalefactor = value;
     g_scaled = true;
}

void set_vertical_orientation(bool value)
{
     g_vertical_orientation = value;
     g_scaled = true;
}

void set_window_title(char* value)
{
     strncpy(g_window_title, value, TITLE_LENGTH - 1);

     g_window_title[TITLE_LENGTH - 1] = '\0';
     if (g_window) SDL_SetWindowTitle(g_window, g_window_title);
}

void set_game_window(const char* value)
{
     std::string game(value);
     std::string title = "Hypseus Singe: [" + game + "]";
     strncpy(g_window_title, title.c_str(), TITLE_LENGTH - 1);

     g_window_title[TITLE_LENGTH - 1] = '\0';
     if (g_window) SDL_SetWindowTitle(g_window, g_window_title);
}

void set_rotate_degrees(float fDegrees)
{
     g_fullscreen = true;
     g_fRotateDegrees = fDegrees;
}

void set_score_bezel(bool bEnabled)
{
     if (bEnabled) {
         if (!g_fullscreen && g_yuv_surface)
             vid_toggle_fullscreen();
         g_fullscreen = true;
     }
     g_sb_bezel = bEnabled;
}

void set_aux_bezel(bool bEnabled)
{
     if (bEnabled) {
         g_fullscreen = g_sb_bezel = true;
         g_game->m_software_scoreboard = true;
     }
     g_aux_bezel = bEnabled;
}

void set_annun_lamponly(bool bEnabled)
{
     if (bEnabled) {
         g_annun_lamps = true;
         g_aux_bezel_alpha = 2;
     }
}

void set_bezel_reverse(bool d)
{
     char s[] = "reverse priority";
     g_bezel_reverse = !g_bezel_reverse;
     if (d) draw_subtitle(s, true, true);
}

void set_tq_keyboard(bool bEnabled)
{
     if (bEnabled) {

         g_scalefactor = (g_scalefactor == 100) ? 75 : g_scalefactor;
         g_fullscreen = g_sb_bezel = true;
         g_game->m_software_scoreboard = true;
     }
     g_keyboard_bezel = bEnabled;
}

void set_bezel_file(const char *bezelFile)
{
     g_bezel_file = bezelFile;

     if (!g_bezel_file.empty()) {
         g_fullscreen = true;
     }
}

void set_bezel_path(const char *bezelPath)
{
     g_bezel_path = bezelPath;
     if (g_bezel_path.back() == '/')
         g_bezel_path.pop_back();
}

void set_fRotateDegrees(float fDegrees, bool verbose)
{
     if (!g_fullscreen && g_scaled) return;

     char d[8]; // In-game
     if (fDegrees == 0) SDL_SetRenderTarget(g_renderer, NULL);

     if (verbose) {
         snprintf(d, sizeof(d), "%d\xC2\xB0", int(fDegrees));
         draw_subtitle(d, true, true);
     }

     g_fRotateDegrees = fDegrees;
}

SDL_Rect* copy_rect(const SDL_Rect* src)
{
     if (src == NULL) return NULL;

     SDL_Rect* dst = new SDL_Rect(*src);
     return dst;
}

bool allocate_rect(SDL_Rect** rect)
{
     if (*rect == NULL) {
         *rect = new(std::nothrow) SDL_Rect();
         if (*rect == nullptr) {
             printerror("Failed in allocate_rect: g_yuv_rect");
             set_quitflag();
             return false;
         }
     }
     return true;
}

void reset_yuv_rect()
{
     delete g_yuv_rect[0];
     g_yuv_rect[0] = copy_rect(g_yuv_rect[1]);
}

void set_yuv_rect(int x, int y, int w, int h)
{
     if (allocate_rect(&g_yuv_rect[0]))
         *g_yuv_rect[0] = {x, y, w, h};
}

void set_yuv_scale(int v, uint8_t axis)
{
     if (allocate_rect(&g_yuv_rect[0]))
         g_yuv_scale[axis] = -0.01043f * v + 1.00043f;
}

void calc_annun_rect()
{
     double scale = 9.0f - double((g_aux_bezel_scale << 1) / 10.0f);
     g_aux_rect.w = (g_bezel_scalewidth / scale);
     g_aux_rect.h = (g_aux_rect.w * g_aux_ratio);
}

void reset_scalefactor(int value, uint8_t bezel, bool verbose)
{
     bool rst = true;

     switch(bezel) {
     case 1: // Scoreboard
         if (!g_sb_texture) { rst = false ; break; }
         g_sb_bezel_scale = value;
         break;
     case 2: // Aux
         if (!g_aux_texture) { rst = false ; break; }
         g_aux_bezel_scale = value;
         calc_annun_rect();
         break;
     default:
         g_scalefactor = value;
         break;
     }

     char s[32]; // In-game
     if (rst)
     {
         g_rescale = g_fullscreen ? 1 : 2;
         g_scaled = true;

         if (!verbose) return;
         snprintf(s, sizeof(s), "scale: %d", value);
     }
     else snprintf(s, sizeof(s), "invalid layer");

     draw_subtitle(s, true, true);
}

void calc_kb_rect()
{
     double scale = (double)((g_aux_bezel_scale / 25.0f) + 0.52f);
     g_aux_rect.w = (g_bezel_scalewidth / 2.25f) * scale;
     g_aux_rect.h = (g_aux_rect.w * g_aux_ratio);
}

void scalekeyboard(int value)
{
     if (!g_aux_texture) return;

     g_aux_bezel_scale = value;

     calc_kb_rect();

     char s[32]; // In-game
     snprintf(s, sizeof(s), "scale: %d", value);
     draw_subtitle(s, true, true);
}

void reset_shiftvalue(int value, bool vert, uint8_t bezel)
{
     int h, v;
     bool rst = true;

     switch(bezel) {
     case 1: // Scoreboard
         if (!g_sb_texture) { rst = false ; break; }
         if (vert) sb_window_pos_y = value;
         else sb_window_pos_x = value;
         h = sb_window_pos_x;
         v = sb_window_pos_y;
         break;
     case 2: // Aux
         if (!g_aux_texture) { rst = false ; break; }
         if (vert) g_aux_rect.y = value;
         else g_aux_rect.x = value;
         h = g_aux_rect.x;
         v = g_aux_rect.y;
         break;
     default:
         if (g_game->get_sinden_border()) return;
         if (g_keyboard_bezel) g_scaled = true;
         if (vert) g_scale_v_shift = value;
         else g_scale_h_shift = value;
         h = g_scale_h_shift - 0x64;
         v = g_scale_v_shift - 0x64;
         break;
     }

     char s[32]; // In-game
     if (rst) {
         g_rescale = g_fullscreen ? 1 : 2;
         snprintf(s, sizeof(s), "x:%d, y:%d", h, v);
     }
     else snprintf(s, sizeof(s), "invalid layer");

     draw_subtitle(s, true, true);
}

// position annunicator bezel
void set_aux_bezel_position(int xValue, int yValue)
{
    aux_bezel_pos_x = xValue;
    aux_bezel_pos_y = yValue;
}

// position sb_scoreboard
void set_sb_window_position(int xValue, int yValue)
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

    dest.x = (short)(col * (g_game->use_old_overlay() ? 6 : 5));

    SDL_FillRect(surface, &dest, 0x00000000);
    SDL_Color color = {0xf0, 0xf0, 0xf0, 0xff};
    text_surface = TTF_RenderText_Solid(g_ttfont, t, color);

    SDL_BlitSurface(text_surface, NULL, surface, &dest);
    SDL_FreeSurface(text_surface);
}

void draw_subtitle(char *s, bool insert, bool center)
{
    int y = (int)(g_scaling_rect.h * 0.92f) + g_scaling_rect.y;
    static int m_message_timer;
    static bool align = false;
    const int timeout = 200;

    if (insert) {
        m_message_timer = 0;
        g_bSubtitleShown = true;
        set_subtitle_display(s);
        align = center;

    } else if (m_message_timer > timeout) {
        g_bSubtitleShown = false;
    }

    if (align) {
        SDL_Color color = {0xff, 0xff, 0xff, 0xff};
        FC_DrawEffect(g_font, g_renderer, (int)((g_scaling_rect.w >> 1) + g_scaling_rect.x), y,
			FC_MakeEffect(FC_ALIGN_CENTER, FC_MakeScale(1.0f, 1.0f), color), s);
    } else {
        FC_Draw(g_font, g_renderer, (int)(g_scaling_rect.w - (g_scaling_rect.w * 0.97f))
			+ g_scaling_rect.x, y, s);
    }
    m_message_timer++;
}

// toggles fullscreen mode
void vid_toggle_fullscreen()
{
    if (g_rtr_texture) return;

    g_bezel_toggle = false;

    Uint32 current = SDL_GetWindowFlags(g_window);
    Uint32 flags = (current ^ SDL_WINDOW_FULLSCREEN_DESKTOP);
    bool flip = current & SDL_WINDOW_FULLSCREEN_DESKTOP;

    if (SDL_SetWindowFullscreen(g_window, flip ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP) < 0) {
        LOGW << fmt("Toggle fullscreen failed: %s", SDL_GetError());
        return;
    }

    if ((flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0 ||
         (flags & SDL_WINDOW_MAXIMIZED) != 0) {

        if ((flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) {
            SDL_GetDisplayBounds(g_display, &g_logical_rect);
        } else {
            g_logical_rect.w = g_displaymode.w;
            g_logical_rect.h = g_displaymode.h;
        }
        g_fullscreen = true;
        format_fullscreen_render();
        notify_stats(g_overlay_width, g_overlay_height, "u");

        if (g_aux_texture) g_aux_needs_update = true;

        if (!g_bezel_file.empty()) g_bezel_toggle = true;
        return;
    }
    g_fullscreen = false;
    format_window_render();
    notify_stats(g_overlay_width, g_overlay_height, "u");

    SDL_Rect dimensions;
    SDL_GetDisplayBounds(g_display, &dimensions);

    SDL_SetWindowSize(g_window, g_viewport_width, g_viewport_height);
    SDL_SetWindowPosition(g_window, dimensions.x + ((dimensions.w - g_viewport_width) >> 1),
                dimensions.y + ((dimensions.h - g_viewport_height) >> 1));
}

void vid_toggle_scanlines()
{
    char s[16] = "scanlines off";

    if (g_scanlines) {
        if (g_scanline_shunt < 10) {
            g_scanline_shunt++;
        } else {
            g_scanlines = false;
            SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);
            g_scanline_shunt = 2;
        }
    } else {
        g_scanlines = true;
        SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
    }

    if (g_scanlines) snprintf(s, sizeof(s), "shunt: %d", g_scanline_shunt);

    draw_subtitle(s, true, true);
}

void vid_toggle_bezel()
{
    if (g_fullscreen) {
        g_bezel_toggle = g_bezel_texture || g_aux_texture || g_sb_bezel ?
                 !g_bezel_toggle : g_bezel_toggle;

        char s[] = "bezel toggle";
        draw_subtitle(s, true, true);
    }
}

void vid_scoreboard_switch()
{
    if (!g_sb_window) return;

    char s[16] = "screen: 0";

    if (g_displays > 1) {
        SDL_Rect displayDimensions[g_displays];
        int winId = SDL_GetWindowDisplayIndex(g_sb_window);

        for (int i = 0; i < g_displays; i++)
            SDL_GetDisplayBounds(i, &displayDimensions[i]);

        if (++winId == g_displays) winId = 0;
        snprintf(s, sizeof(s), "screen: %d", (unsigned char)winId);

        SDL_SetWindowPosition(g_sb_window,
           displayDimensions[winId].x + sb_window_pos_x,
              displayDimensions[winId].y + sb_window_pos_y);
    } else
        SDL_SetWindowPosition(g_sb_window, sb_window_pos_x,
                                 sb_window_pos_y);

    draw_subtitle(s, true, false);
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

    g_yuv_surface->Ypitch = g_yuv_surface->width;
    g_yuv_surface->Upitch = g_yuv_surface->width / 2;
    g_yuv_surface->Vpitch = g_yuv_surface->width / 2;

    // Setup the threaded access stuff, since this surface is accessed from the vldp thread, too.
    g_yuv_surface->mutex = SDL_CreateMutex();

    if (g_yuv_rect[0]) {
        g_yuv_rect[0]->w = width * g_yuv_scale[YUV_H];
        g_yuv_rect[0]->h = height * g_yuv_scale[YUV_V];
        g_yuv_rect[0]->x = (width >> 1) - (g_yuv_rect[0]->w >> 1);
        g_yuv_rect[0]->y = (height >> 1) - (g_yuv_rect[0]->h >> 1);
        g_yuv_rect[1] = copy_rect(g_yuv_rect[0]);
    }
}

void vid_blank_yuv_surface () {

    // Blue: YUV#1DEB6B - Black: YUV#108080
    uint8_t Y_value = g_yuv_blue ? 0x1d : 0x10;
    uint8_t U_value = g_yuv_blue ? 0xeb : 0x80;
    uint8_t V_value = g_yuv_blue ? 0x6b : 0x80;

    memset(g_yuv_surface->Yplane, Y_value, g_yuv_surface->Ysize);
    memset(g_yuv_surface->Uplane, U_value, g_yuv_surface->Usize);
    memset(g_yuv_surface->Vplane, V_value, g_yuv_surface->Vsize);
}

 void vid_blank_yuv_texture()
 {
    int w, h;
    SDL_QueryTexture(g_yuv_texture, NULL, NULL, &w, &h);

    uint8_t *y_plane = NULL;
    uint8_t *u_plane = NULL;
    uint8_t *v_plane = NULL;

    uint8_t Y_value = g_yuv_blue ? 0x1d : 0x10;
    uint8_t U_value = g_yuv_blue ? 0xeb : 0x80;
    uint8_t V_value = g_yuv_blue ? 0x6b : 0x80;

    y_plane = (uint8_t *)malloc(w * h);
    u_plane = (uint8_t *)malloc(w * h / 4);
    v_plane = (uint8_t *)malloc(w * h / 4);

    memset(y_plane, Y_value, w * h);
    memset(u_plane, U_value, w * h / 4);
    memset(v_plane, V_value, w * h / 4);

    SDL_UpdateYUVTexture(g_yuv_texture, NULL, y_plane, w,
                             u_plane, w / 2, v_plane, w / 2);
    free(y_plane);
    free(u_plane);
    free(v_plane);
}

SDL_Texture *vid_create_yuv_texture (int width, int height) {
    g_yuv_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_YV12,
        g_texture_access, width, height);

    g_yuv_skip = true;

    return g_yuv_texture;
}

// REMEMBER it updates the YUV surface ONLY: the YUV texture is updated on vid_blit().
int vid_update_yuv_overlay(uint8_t *Yplane, uint8_t *Uplane, uint8_t *Vplane,
	int Ypitch, int Upitch, int Vpitch)
{
    // This function is called from the vldp thread, so access to the
    // yuv surface is protected (mutexed).
    // As a reminder, mutexes are very simple: this fn tries to lock(=get)
    // the mutex, but if it's already taken, LockMutex doesn't return
    // until the mutex is free and we can lock(=get) it here.
    SDL_LockMutex(g_yuv_surface->mutex);

    switch(g_yuv_display) {
    case YUV_BLANK:
        vid_blank_yuv_surface();
        break;
    case YUV_SHUTTER:
        vid_blank_yuv_surface();
        g_yuv_display = YUV_VISIBLE;
        break;
    default:
        if (g_yuv_grayscale) {

            uint8_t* dst_row = g_yuv_surface->Yplane;
            uint8_t* src_row = Yplane;

            for (int y = 0; y < g_yuv_surface->height; ++y) {
                if ((dst_row + g_yuv_surface->width <= g_yuv_surface->Yplane + g_yuv_surface->Ysize) &&
                        (src_row + g_yuv_surface->width <= Yplane + g_yuv_surface->Ysize)) {
                    memcpy(dst_row, src_row, g_yuv_surface->width);

                } else break;

                dst_row += g_yuv_surface->Ypitch;
                src_row += Ypitch;
            }
            memset(g_yuv_surface->Uplane, 0x80, g_yuv_surface->Usize);
            memset(g_yuv_surface->Vplane, 0x80, g_yuv_surface->Vsize);

        } else {
            memcpy(g_yuv_surface->Yplane, Yplane, g_yuv_surface->Ysize);
            memcpy(g_yuv_surface->Uplane, Uplane, g_yuv_surface->Usize);
            memcpy(g_yuv_surface->Vplane, Vplane, g_yuv_surface->Vsize);
        }

        g_yuv_surface->Ypitch = Ypitch;
        g_yuv_surface->Upitch = Upitch;
        g_yuv_surface->Vpitch = Vpitch;
        break;
    }

    g_yuv_video_needs_update = true;

    SDL_UnlockMutex(g_yuv_surface->mutex);

    return 0;
}

void vid_update_overlay_surface (SDL_Surface *tx) {

    if (g_scoreboard_needs_update) {
        return;
    }

    g_overlay_size_rect.x = 0;
    g_overlay_size_rect.y = 0;
    g_overlay_size_rect.w = tx->w;
    g_overlay_size_rect.h = tx->h;

    if (g_overlay_resize) {
        g_render_size_rect = g_overlay_size_rect;
        SDL_SetColorKey (tx, SDL_TRUE, 0x00000000);
        SDL_FillRect(g_overlay_blitter, NULL, 0x00000000);
    }

    SDL_Rect* dst = g_enhance_overlay ? &g_render_size_rect : NULL;
    SDL_BlitSurface(tx, NULL, g_overlay_blitter, dst);
    g_overlay_needs_update = true;
}

void vid_render_aux () {

    if (g_aux_texture) {
        SDL_UpdateTexture(g_aux_texture, &g_annu_rect,
            (void *)g_aux_blit_surface->pixels, g_aux_blit_surface->pitch);

        g_aux_needs_update = false;

    } else {

        if (g_keyboard_bezel) {

            g_aux_needs_update = false;
            g_aux_texture = IMG_LoadTexture(g_renderer, g_tqkeys.c_str());

            if (!g_aux_texture) {
                LOGE << fmt("Failed on keyboard texture: %s, %s",
                             g_tqkeys.c_str(), SDL_GetError());
                set_quitflag();
                return;
            }

            SDL_Point size;
            SDL_QueryTexture(g_aux_texture, NULL, NULL, &size.x, &size.y);

            g_aux_ratio = (float)size.y / (float)size.x;

            calc_kb_rect();
            LogicalPosition(&g_logical_rect, &g_aux_rect, 50, 100);

            // argument override
            if (aux_bezel_pos_x || aux_bezel_pos_y) {
                g_aux_rect.x = aux_bezel_pos_x;
                g_aux_rect.y = aux_bezel_pos_y;
            }
        }
        else
        {
            g_aux_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGBA8888,
                                g_texture_access, g_an_w, g_an_h);

            if (!g_aux_texture) {
                LOGE << fmt("Failed on annunciator texture: %s", SDL_GetError());
                set_quitflag();
            } else if (g_aux_bezel_alpha)
                SDL_SetTextureBlendMode(g_aux_texture, SDL_BLENDMODE_BLEND);
        }
    }
}

void vid_render_texture (SDL_Texture *texture, SDL_Rect rect) {

    if (texture) SDL_RenderCopy(g_renderer, texture, NULL, &rect);
}

void vid_render_bezels () {

    if (!g_bezel_load) {
        if (!g_bezel_file.empty() && !g_bezel_texture) {
            std::string bezelpath =  g_bezel_path + std::string(PATH_SEPARATOR) + g_bezel_file;
            g_bezel_texture = IMG_LoadTexture(g_renderer, bezelpath.c_str());

            if (g_bezel_texture) {
                LOGI << fmt("Loaded bezel file: %s", bezelpath.c_str());
            } else {
                LOGW << fmt("Failed to load bezel: %s", bezelpath.c_str());
            }
        }
        g_bezel_load = true;
    }

    if (g_bezel_reverse)
    {
        vid_render_texture(g_sb_texture, g_sb_bezel_rect);
        vid_render_texture(g_aux_texture, g_aux_rect);
        vid_render_texture(g_bezel_texture, SDL_Rect{0, 0, g_logical_rect.w, g_logical_rect.h});
    }
    else
    {
        vid_render_texture(g_bezel_texture, SDL_Rect{0, 0, g_logical_rect.w, g_logical_rect.h});
        vid_render_texture(g_aux_texture, g_aux_rect);
        vid_render_texture(g_sb_texture, g_sb_bezel_rect);
    }
}

void vid_render_rotate () {

    if (!g_rtr_texture) {

        g_rtr_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGBA8888,
                  SDL_TEXTUREACCESS_TARGET, g_logical_rect.w, g_logical_rect.h);

        if (!g_rtr_texture) {
            LOGE << fmt("Rotation texture creation failed: %s", SDL_GetError());
            g_fRotateDegrees = 0.0f;
            return;
        }
    }

    SDL_SetRenderTarget(g_renderer, NULL);
    SDL_RenderClear(g_renderer);

    SDL_RenderCopyEx(g_renderer, g_rtr_texture, NULL, NULL, g_fRotateDegrees, NULL, SDL_FLIP_NONE);

    SDL_SetRenderTarget(g_renderer, g_rtr_texture);
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

    // Handle any rescaling that occurred.
    switch(g_rescale) {
    case 1:
       format_fullscreen_render();
       break;
    case 2:
       format_window_render();
       break;
    default:
       break;
    }

    // Clear the renderer before the SDL_RenderCopy() calls for this frame.
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
		g_yuv_texture = vid_create_yuv_texture(g_yuv_surface->width,
				    g_yuv_surface->height);
	    }

	    g_yuv_video_needs_update = false;

	    if (g_yuv_skip)
	    {
		if (g_yuv_display == YUV_VISIBLE)
		    g_yuv_skip = false;

		vid_blank_yuv_texture();
	    }
	    else SDL_UpdateYUVTexture(g_yuv_texture, NULL,
		g_yuv_surface->Yplane, g_yuv_surface->Ypitch,
		g_yuv_surface->Uplane, g_yuv_surface->Vpitch,
		g_yuv_surface->Vplane, g_yuv_surface->Vpitch);
	}
	SDL_UnlockMutex(g_yuv_surface->mutex);
    }

    // Does OVERLAY texture need update from the scoreboard surface?
    if (g_scoreboard_needs_update) {
        SDL_UpdateTexture(g_overlay_texture, &g_blit_size_rect,
	    (void *)g_blit_surface->pixels, g_blit_surface->pitch);
    }

    // Does OVERLAY texture need update from the overlay surface?
    if (g_overlay_needs_update) {
        SDL_UpdateTexture(g_overlay_texture, &g_overlay_size_rect,
	    (void *)g_overlay_blitter->pixels, g_overlay_blitter->pitch);

	g_overlay_needs_update = false;
    }

    // Sadly, we have to RenderCopy the YUV texture on every blitting strike, because
    // the image on the renderer gets "dirty" with previous overlay frames on top of the yuv.
    if (g_yuv_texture)
        SDL_RenderCopy(g_renderer, g_yuv_texture, g_yuv_rect[0], &g_scaling_rect);

    // If there's an overlay texture, it means we are using some kind of overlay,
    // be it LEDs or any other thing, so RenderCopy it to the renderer ON TOP of the YUV video.
    if (g_overlay_texture)
            SDL_RenderCopy(g_renderer, g_overlay_texture, &g_render_size_rect, &g_scaling_rect);

    if (g_aux_needs_update) vid_render_aux();

    if (g_scanlines)
        draw_scanlines(g_scanline_shunt);

    // If there's a subtitle overlay
    if (g_bSubtitleShown) draw_subtitle(subchar, false, false);

    if (g_bezel_toggle) vid_render_bezels();

    if (g_game->get_sinden_border())
        draw_border(g_game->get_sinden_border(),
            g_game->get_sinden_border_color());

    if (g_fRotateDegrees != 0) vid_render_rotate();

    SDL_RenderPresent(g_renderer);

    if (g_softsboard_needs_update) {
        SDL_RenderPresent(g_sb_renderer);
        g_softsboard_needs_update = false;
    }

    if (queue_take_screenshot) {
        queue_take_screenshot = false;
        take_screenshot();
    }

    g_rescale = 0;
}

int get_yuv_overlay_width() {

    if (g_yuv_surface) return g_yuv_surface->width;
    return 0;
}

int get_yuv_overlay_height() {

    if (g_yuv_surface) return g_yuv_surface->height;
    return 0;
}

bool get_yuv_overlay_ready() {

    if (g_yuv_surface && g_yuv_texture) return true;
    return false;
}

void set_overlay_offset(int offset) {

    g_render_size_rect.y = abs(offset);
}

void take_screenshot()
{
    struct       stat info;
    char         filename[64];
    int32_t      screenshot_num = 0;
    const char   dir[12] = "screenshots";

    if (stat(dir, &info ) != 0 )
        { LOGW << fmt("'%s' directory does not exist.", dir); return; }
    else if (!(info.st_mode & S_IFDIR))
        { LOGW << fmt("'%s' is not a directory.", dir); return; }
    else if (g_display != 0)
        { LOGE << "Screenshots require the primary display."; return; }

    SDL_Surface *screenshot = NULL;

    if (g_fullscreen) SDL_GetDisplayBounds(0, &g_logical_rect);
    else SDL_GetRendererOutputSize(g_renderer, &g_logical_rect.w, &g_logical_rect.h);

    screenshot = SDL_CreateRGBSurface(0, g_logical_rect.w, g_logical_rect.h, 32, 0, 0, 0, 0);

    if (!screenshot) { LOGE << "Cannot allocate screenshot surface"; return; }

    SDL_SetRenderTarget(g_renderer, NULL);

    if (SDL_RenderReadPixels(g_renderer, &g_logical_rect, screenshot->format->format,
        screenshot->pixels, screenshot->pitch) != 0)
        { LOGE << fmt("Cannot ReadPixels - Something bad happened: %s", SDL_GetError());
             g_game->set_game_errors(SDL_ERROR_SCREENSHOT);
             set_quitflag(); }

    for (;;) {
        screenshot_num++;
        snprintf(filename, sizeof(filename), "%s%shypseus-%d.png",
          dir, PATH_SEPARATOR, screenshot_num);

        if (!mpo_file_exists(filename))
            break;
    }

    if (IMG_SavePNG(screenshot, filename) == 0) {
        LOGI << fmt("Wrote screenshot: %s", filename);
    } else {
        LOGE <<  fmt("Could not write screenshot: %s !!", filename);
    }

    SDL_FreeSurface(screenshot);
}

void load_fonts() {

    const char *ttfont;
    int fs = g_scaling_rect.w / ((g_aspect_ratio == ASPECTPD) ? 24 : 36);

    FC_FreeFont(g_font);
    g_font = FC_CreateFont();
    FC_LoadFont(g_font, g_renderer, "fonts/default.ttf", fs,
                FC_MakeColor(0xff, 0xff, 0xff, 0xff), TTF_STYLE_NORMAL);

    TTF_CloseFont(g_ttfont);
    ttfont = g_game->use_old_overlay() ? "fonts/daphne.ttf" : "fonts/digital.ttf";
    g_ttfont = TTF_OpenFont(ttfont, g_game->use_old_overlay() ? 12 : 14);

    if (g_ttfont == NULL) {
        LOG_ERROR << fmt("Cannot load TTF font: '%s'", (char*)ttfont);
        g_game->set_game_errors(SDL_ERROR_FONT);
        set_quitflag();
    }
}

void format_window_render() {

    g_scaling_rect.w = (g_viewport_width * g_scalefactor) / 100;
    g_scaling_rect.h = (g_viewport_height * g_scalefactor) / 100;

    int diff_w = (g_viewport_width - g_scaling_rect.w) >> 1;
    int diff_h = (g_viewport_height - g_scaling_rect.h) >> 1;

    g_scaling_rect.x = (diff_w * g_scale_h_shift) / 100;
    g_scaling_rect.y = (diff_h * g_scale_v_shift) / 100;

    g_logical_rect.w = g_scaling_rect.w;
    g_logical_rect.h = g_scaling_rect.h;

    if (g_mousemotion)
        SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);

    if (!g_rescale) {
        g_border_rect = (SDL_Rect){0, 0, g_viewport_width, g_viewport_height};
        load_fonts();
    }
}

void format_fullscreen_render() {
    int w, h;
    double ratio = static_cast<double>(g_viewport_width) / g_viewport_height;

    if (g_vid_resized) {
        w = g_viewport_width;
        h = g_viewport_height;
    } else {
        h = g_vertical_orientation ? g_logical_rect.w : g_logical_rect.h;

        switch (g_aspect_ratio) {
            case ASPECTWS:  w = (h * 16) / 9; break;
            case ASPECTSD:  w = (h * 4) / 3; break;
            case ASPECTPD:
                if (g_vertical_orientation) h = static_cast<int>(h * 1.33);
                w = (h * 3) / 4;
                if (g_mousemotion) SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
                break;
            default: w = static_cast<int>(h * ratio); break;
        }
    }

    w = g_bForceAspect ? (h * 4) / 3 : (g_bIgnoreAspect ? static_cast<int>(h * ratio) : w);

    g_scaling_rect.w = (w * g_scalefactor) / 100;
    g_scaling_rect.h = (h * g_scalefactor) / 100;
    g_scaling_rect.x = (g_logical_rect.w - g_scaling_rect.w) / 2;
    g_scaling_rect.y = (g_logical_rect.h - g_scaling_rect.h) / 2;

    if (g_keyboard_bezel && !g_scaled) {
        g_scaling_rect.y /= 4;
        g_scale_v_shift = 0;
    } else {
        g_scaling_rect.x = (g_scaling_rect.x * g_scale_h_shift) / 100;
        g_scaling_rect.y = (g_scaling_rect.y * g_scale_v_shift) / 100;
    }

    if (g_sb_bezel) {
        g_bezel_scalewidth = w;
        double bezel_ratio = static_cast<double>(g_sb_h) / g_sb_w;
        double scale = 9.0 - (static_cast<double>(g_sb_bezel_scale) * 2.0 / 10.0);

        g_sb_bezel_rect = {sb_window_pos_x, sb_window_pos_y,
                           static_cast<int>(g_bezel_scalewidth / scale),
                           static_cast<int>((g_bezel_scalewidth / scale) * bezel_ratio)};
        g_bezel_toggle = true;
    }

    if (!g_rescale) {
        g_border_rect = {
            (g_logical_rect.w - w) / 2,
            (g_logical_rect.h - h) / 2,
            w, h
        };
        load_fonts();
    }
}

void draw_scanlines(int l) {

    int xe = g_scaling_rect.x + g_scaling_rect.w;
    int ye = g_scaling_rect.y + g_scaling_rect.h;

    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, g_scanline_alpha);

    if (g_aspect_ratio == ASPECTPD) {
        for (int i = 0; i < g_scaling_rect.w; i += l) {
            int x = g_scaling_rect.x + i;
            SDL_RenderDrawLine(g_renderer, x, g_scaling_rect.y, x, ye);
        }
    } else {
        for (int i = 0; i < g_scaling_rect.h; i += l) {
            int y = g_scaling_rect.y + i;
            SDL_RenderDrawLine(g_renderer, g_scaling_rect.x, y, xe, y);
        }
    }

    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
}

void notify_stats(int overlaywidth, int overlayheight, const char* input) {

    char s[4] = {0};
    if (!g_overlay_resize) snprintf(s, sizeof(s), "[s]");

    LOGI << fmt("Viewport Stats:|w:%dx%d|v:%dx%d|o:%dx%d%s|l:%dx%d|%s",
         g_viewport_width, g_viewport_height, g_probe_width,
           g_probe_height, overlaywidth, overlayheight, s,
             g_logical_rect.w, g_logical_rect.h, input);
}

void notify_positions() {

    char s[] = "positions logged";

    LOGI << fmt("Scale video (0): %d", g_scalefactor);
    LOGI << fmt("Shift video (0): x:%d, y:%d", (g_scale_h_shift - 0x64),
                         (g_scale_v_shift - 0x64));

    if (g_sb_texture) {
        LOGI << fmt("Scale score (1): %d", g_sb_bezel_scale);
        LOGI << fmt("Shift score (1): x:%d, y:%d", sb_window_pos_x, sb_window_pos_y);
    }

    if (g_aux_texture) {
        LOGI << fmt("Scale aux (2): %d", g_aux_bezel_scale);
        LOGI << fmt("Shift aux (2): x:%d, y:%d", g_aux_rect.x, g_aux_rect.y);
    }

    LOGI << fmt("Rotation: %d\xC2\xB0", (int)g_fRotateDegrees);

    draw_subtitle(s, true, true);
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

    tb.x = lb.x = bb.x = g_border_rect.x;
    tb.y = lb.y = rb.y = g_border_rect.y;
    rb.x = (g_border_rect.w + g_border_rect.x) - s;
    bb.y = (g_border_rect.h - s) + g_border_rect.y;
    tb.w = bb.w = g_border_rect.w;
    tb.h = bb.h = lb.w = rb.w = s;
    lb.h = rb.h = g_border_rect.h;

    SDL_RenderFillRect(g_renderer, &tb);
    SDL_RenderFillRect(g_renderer, &lb);
    SDL_RenderFillRect(g_renderer, &rb);
    SDL_RenderFillRect(g_renderer, &bb);

    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
}

}
