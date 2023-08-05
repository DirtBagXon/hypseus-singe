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
#include "video.h"
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

namespace video
{
int g_vid_width = 640, g_vid_height = 480; // default video dimensions
const int g_bw = g_vid_width >> 1, g_bh = g_vid_height >> 1;
unsigned int g_probe_width = g_vid_width;
unsigned int g_probe_height = g_vid_height;
const int g_an_w = 220, g_an_h = 128;
const int g_sb_w = 340, g_sb_h = 480;
int s_alpha = 128;
int s_shunt = 2;

int g_viewport_width = g_vid_width, g_viewport_height = g_vid_height;
int g_aspect_width = 0, g_aspect_height = 0;
int g_score_screen = 0;

int sb_window_pos_x = 0, sb_window_pos_y = 0;
int ann_bezel_pos_x = 0, ann_bezel_pos_y = 0;
int g_scale_h_shift = 100, g_scale_v_shift = 100;

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
SDL_Surface *g_overlay_blitter     = NULL; // The main blitter surface
SDL_Surface *g_blit_surface        = NULL;
SDL_Surface *g_sb_surface          = NULL;
SDL_Texture *g_sb_texture          = NULL;
SDL_Surface *g_sb_blit_surface     = NULL;
SDL_Surface *g_aux_blit_surface    = NULL;
SDL_Texture *g_bezel_texture       = NULL;
SDL_Texture *g_aux_texture         = NULL;

SDL_Rect g_aux_rect;
SDL_Rect g_rotate_rect;
SDL_Rect g_overlay_size_rect;
SDL_Rect g_scaling_rect = {0, 0, 0, 0};
SDL_Rect g_logical_rect = {0, 0, 0, 0};
SDL_Rect g_sb_bezel_rect = {0, 0, 0, 0};
SDL_Rect g_blit_size_rect = {0, 0, g_bw, g_bh};
SDL_Rect g_render_size_rect = g_blit_size_rect;
SDL_Rect g_annu_rect = { 0, 0, g_an_w, g_an_h };

SDL_DisplayMode g_displaymode;

LDP1450_CharStruct LDP1450_CharSet[OVERLAY_LDP1450_LINES];

bool queue_take_screenshot = false;
bool g_scale_linear = false;
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
bool g_sb_bezel = false;
bool g_rotate = false;
bool g_rotated_state = false;
bool g_keyboard_bezel = false;
bool g_annun_bezel = false;
bool g_ded_annun_bezel = false;
bool g_annun_lamps = false;
bool g_vertical_orientation = false;

int g_scalefactor = 100;   // by RDG2010 -- scales the image to this percentage
int g_aspect_ratio = 0;
int sboverlay_characterset = 2;
int g_texture_access = SDL_TEXTUREACCESS_TARGET;

int8_t g_sb_bezel_alpha = 0;
int8_t g_annun_bezel_alpha = 0;

int g_sb_bezel_scale = 14;
int g_an_bezel_scale = 12;
int g_bezel_scalewidth = 0;

// Move subtitle rendering to SDL_RenderPresent(g_renderer);
bool g_bSubtitleShown = false;
char *subchar = NULL;

string g_bezel_file;

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
bool g_yuv_video_blank         = false;
bool g_aux_needs_update        = false;

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
    Uint32 sdl_sb_flags = SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_BORDERLESS;
    Uint8 sdl_render_flags = SDL_RENDERER_TARGETTEXTURE;

    // if we were able to initialize the video properly
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) >= 0) {

        if (g_fullscreen)
            sdl_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        else if (g_fakefullscreen)
            sdl_flags |= SDL_WINDOW_MAXIMIZED | SDL_WINDOW_BORDERLESS;

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
            g_viewport_width  = g_vid_width;
            g_viewport_height = g_vid_height;

        } else {

            if (!g_bIgnoreAspectRatio && g_aspect_width > 0) {
                g_viewport_width  = g_aspect_width;
                g_viewport_height = g_aspect_height;

            } else {
                g_viewport_width  = g_probe_width;
                g_viewport_height = g_probe_height;
            }
        }

        // Enforce 4:3 aspect ratio
        if (g_bForceAspectRatio) {
            double dCurAspectRatio = (double)g_viewport_width / g_viewport_height;
            const double dTARGET_ASPECT_RATIO = 4.0 / 3.0;

            if (dCurAspectRatio < dTARGET_ASPECT_RATIO) {
                g_viewport_height = (g_viewport_width * 3) / 4;
            }
            else if (dCurAspectRatio > dTARGET_ASPECT_RATIO) {
                g_viewport_width = (g_viewport_height * 4) / 3;
            }
        }

        if (g_window) resize_cleanup();

        if (g_fRotateDegrees != 0) {
            if (g_fRotateDegrees != 180.0) {
                if (!notify) { LOGW << "Screen rotation: Just a ruse..."; }
                g_viewport_height = g_viewport_width;
            }
	}

	g_window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                         g_viewport_width, g_viewport_height, sdl_flags);

        if (!g_window) {
            LOGE << fmt("Could not initialize window: %s", SDL_GetError());
            g_game->set_game_errors(SDL_ERROR_INIT);
            set_quitflag();
            goto exit;
        } else {
            if (g_game->m_sdl_software_rendering) {
                sdl_render_flags |= SDL_RENDERER_SOFTWARE;
            } else {
                sdl_render_flags |= SDL_RENDERER_ACCELERATED;
            }

            Uint8 sdl_sb_render_flags = sdl_render_flags;

            if (g_vsync && (sdl_render_flags & SDL_RENDERER_ACCELERATED))
                sdl_render_flags |= SDL_RENDERER_PRESENTVSYNC;

            g_renderer = SDL_CreateRenderer(g_window, -1, sdl_render_flags);

            if (!g_renderer) {
                LOGE << fmt("Could not initialize renderer: %s", SDL_GetError());
                g_game->set_game_errors(SDL_ERROR_MAINRENDERER);
                set_quitflag();
                goto exit;
            } else {

                if (g_keyboard_bezel) {
                    string tqkeys = "bezels/tqkeys.png";
                    if (!mpo_file_exists(tqkeys.c_str()))
                        tqkeys = "pics/tqkeys.png";

                    g_aux_texture = IMG_LoadTexture(g_renderer, tqkeys.c_str());

                    if (!g_aux_texture) {
                        LOGE << fmt("Failed to load keyboard graphic: %s - %s",
                                          tqkeys.c_str(), SDL_GetError());
                        set_quitflag();
                    }
                } else if (g_annun_bezel) {
                    g_aux_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGBA8888,
                                       g_texture_access, g_an_w, g_an_h);

                    if (!g_aux_texture) {
                        LOGE << fmt("Failed on annunciator texture: %s", SDL_GetError());
                        set_quitflag();
                    } else if (g_annun_bezel_alpha)
                        SDL_SetTextureBlendMode(g_aux_texture, SDL_BLENDMODE_BLEND);
                }

                if (!g_bezel_file.empty()) {
                    string bezelpath = "bezels/" + g_bezel_file;
                    g_bezel_texture = IMG_LoadTexture(g_renderer, bezelpath.c_str());

                    if (!notify) {
                        if (g_bezel_texture) {
                            LOGI << fmt("Loaded bezel file: %s", bezelpath.c_str());
                        } else {
                            LOGW << fmt("Failed to load bezel: %s", bezelpath.c_str());
                        }
                    }
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

                    int displays = SDL_GetNumVideoDisplays();
                    SDL_Rect displayDimensions[displays];

                    for (int i = 0; i < displays; i++)
                        SDL_GetDisplayBounds(i, &displayDimensions[i]);

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

			if (displays > 1) {

                            int s = 1;
                            if (g_score_screen > s && g_score_screen <= displays)
                                s = g_score_screen;

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
                        SDL_RenderSetViewport(g_renderer, NULL);
                        SDL_RenderGetViewport(g_renderer, &g_logical_rect);

                    } else {

                        if (SDL_GetDesktopDisplayMode(0, &g_displaymode) != 0) {
                            LOGE << fmt("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
                            set_quitflag();
                            goto exit;
                        }
                        g_logical_rect.w = g_displaymode.w;
                        g_logical_rect.h = g_displaymode.h;
                    }

                    if (g_bezel_texture) g_bezel_toggle = true;

                    format_fullscreen_render();

                } else
                    format_window_render();

                if (g_aux_texture) {

                    if (g_keyboard_bezel) {

                        SDL_Point size;
                        SDL_QueryTexture(g_aux_texture, NULL, NULL, &size.x, &size.y);

                        double ratio = (float)size.y / (float)size.x;

                        g_aux_rect.w = (g_bezel_scalewidth / 2.25f);
                        g_aux_rect.h = (g_aux_rect.w * ratio);

                        LogicalPosition(&g_logical_rect, &g_aux_rect, 50, 100);

                    }

                    if (g_annun_bezel) {

                        if (g_ded_annun_bezel) g_an_bezel_scale--;

                        double scale = 9.0f - double((g_an_bezel_scale << 1) / 10.0f);
                        double ratio = (float)g_an_h / (float)g_an_w;

                        g_aux_blit_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, g_an_w, g_an_h,
                                            surfacebpp, Rmask, Gmask, Bmask, Amask);

                        if (!g_aux_blit_surface) {
                            LOGE << "Failed to load annunicator surface";
                            set_quitflag();
                        }

                        if (g_annun_bezel_alpha > 1)
                            SDL_SetColorKey(g_aux_blit_surface, SDL_TRUE, 0x000000ff);

                        g_aux_rect.w = (g_bezel_scalewidth / scale);
                        g_aux_rect.h = (g_aux_rect.w * ratio);

                        if (g_ded_annun_bezel)
                            LogicalPosition(&g_logical_rect, &g_aux_rect, 99, 10);
                        else
                            LogicalPosition(&g_logical_rect, &g_aux_rect, 100, 90);

                        // argument override
                        if (ann_bezel_pos_x || ann_bezel_pos_y) {
                            g_aux_rect.x = ann_bezel_pos_x;
                            g_aux_rect.y = ann_bezel_pos_y;
                        }

                        draw_annunciator(0);
                        if (!g_ded_annun_bezel)
                            draw_ranks();
                    }
                }

		// Always hide the mouse cursor
                SDL_ShowCursor(SDL_DISABLE);

                if (g_grabmouse)
                    SDL_SetWindowGrab(g_window, SDL_TRUE);

                if (g_scanlines)
                    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);

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

                if (g_game->use_old_overlay()) {
                    ConvertSurface(&g_other_bmps[B_OVERLAY_LDP1450],
                                   g_overlay_blitter->format);
                    SDL_SetColorKey(g_other_bmps[B_OVERLAY_LDP1450], SDL_TRUE, 0x000000ff);
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
    vid_setup_yuv_overlay(g_viewport_width, g_viewport_height);
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
    FC_FreeFont(g_fixfont);
    TTF_CloseFont(g_ttfont);

    if (g_bezel_texture)
        SDL_DestroyTexture(g_bezel_texture);

    if (g_aux_texture)
        SDL_DestroyTexture(g_aux_texture);

    g_bezel_texture = NULL;
    g_aux_texture = NULL;

    SDL_DestroyTexture(g_overlay_texture);
    SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_window);

    g_overlay_texture = NULL;
    g_renderer = NULL;
    g_window = NULL;

    return (true);
}

void resize_cleanup()
{
    SDL_SetWindowGrab(g_window, SDL_FALSE);

    g_rotate = false;

    if (g_overlay_blitter) SDL_FreeSurface(g_overlay_blitter);
    if (g_blit_surface) SDL_FreeSurface(g_blit_surface);
    if (g_aux_blit_surface) SDL_FreeSurface(g_aux_blit_surface);

    g_overlay_blitter = NULL;
    g_blit_surface = NULL;
    g_aux_blit_surface = NULL;

    if (g_bezel_texture) SDL_DestroyTexture(g_bezel_texture);
    if (g_aux_texture) SDL_DestroyTexture(g_aux_texture);

    g_bezel_texture = NULL;
    g_aux_texture = NULL;

    if (g_overlay_texture) SDL_DestroyTexture(g_overlay_texture);
    if (g_renderer) SDL_DestroyRenderer(g_renderer);

    SDL_DestroyWindow(g_window);

    g_overlay_texture = NULL;
    g_renderer = NULL;
    g_window = NULL;
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

    if (sboverlay_characterset != 2)
	g_other_bmps[B_OVERLAY_LEDS] = load_one_bmp("overlayleds1.bmp", false);
    else
	g_other_bmps[B_OVERLAY_LEDS] = load_one_bmp("overlayleds2.bmp", false);
   
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

    g_other_bmps[B_SHOOT]       = load_one_bmp("shoot.bmp", true);

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
    string filepath;

    if (bezel)
        filepath = fmt("bezels/%s", filename);

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
    string filepath = fmt("bezels/%s", filename);

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

    for (int i = B_ACE_OFF; i < B_SHOOT; i++) {
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

// Draw LDP1450 overlay characters to the screen - rewrite for SDL2 (DBX)
void draw_charline_LDP1450(char *LDP1450_String, int start_x, int y)
{
    float x;
    int i, j = 0;
    int LDP1450_strlen;
    int fontwidth = FC_GetWidth(g_fixfont, "0123456789");
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

    x = (double)((g_scaling_rect.w - fontwidth) >> 1) + g_scaling_rect.x;

    for (i = 0; i < LDP1450_strlen; i++)
    {
         if (LDP1450_String[i] == 0x13)
             LDP1450_String[i] = 0x5f;

         if (LDP1450_String[i] != 32) j++;
    }

    if (j > 0) {
        LDP1450_CharSet[index].enable = true;
        LDP1450_CharSet[index].x = x;
        LDP1450_CharSet[index].y = (y * (double)(g_scaling_rect.h * 0.004f))
                                       + g_scaling_rect.y;
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

FC_Font *get_font() { return g_font; }

Uint16 get_video_width() { return g_vid_width; }
Uint16 get_video_height() { return g_vid_height; }

int get_textureaccess() { return g_texture_access; }
int get_scalefactor() { return g_scalefactor; }
unsigned int get_logical_width() { return g_logical_rect.w; }
unsigned int get_logical_height() { return g_logical_rect.h; }

bool get_opengl() { return g_opengl; }
bool get_vulkan() { return g_vulkan; }
bool get_singe_blend_sprite() { return g_singe_blend_sprite; }
bool get_video_blank() { return g_yuv_video_blank; }
bool get_video_resized() { return g_vid_resized; }
bool get_rotated_state() { return g_rotated_state; }
bool use_old_font() { return g_game->use_old_overlay(); }

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
void set_vertical_orientation(bool value) { g_vertical_orientation = value; }
void set_queue_screenshot(bool value) { queue_take_screenshot = value; }
void set_scale_linear(bool value) { g_scale_linear = value; }
void set_singe_blend_sprite(bool value) { g_singe_blend_sprite = value; }
void set_yuv_video_blank(bool value) { g_yuv_video_needs_blank = value; }
void set_video_blank(bool value) { g_yuv_video_blank = value; }
void set_sboverlay_characterset(int value) { sboverlay_characterset = value; }
void set_subtitle_display(char *s) { subchar = strdup(s); }
void set_force_aspect_ratio(bool value) { g_bForceAspectRatio = value; }
void set_ignore_aspect_ratio(bool value) { g_bIgnoreAspectRatio = value; }
void set_aspect_ratio(int fRatio) { g_aspect_ratio = fRatio; }
void set_detected_height(int pHeight) { g_probe_height = pHeight; }
void set_detected_width(int pWidth) { g_probe_width = pWidth; }
void set_bezel_file(const char *bezelFile) { g_bezel_file = bezelFile; }
void set_score_bezel_alpha(int8_t value) { g_sb_bezel_alpha = value; }
void set_score_bezel_scale(int value) { g_sb_bezel_scale = value; }
void set_ace_annun_scale(int value) { g_an_bezel_scale = value; }
void set_annun_bezel_alpha(int8_t value) { g_annun_bezel_alpha = value; }
void set_ded_annun_bezel(bool value) { g_ded_annun_bezel = value; }
void set_scale_h_shift(int value) { g_scale_h_shift = value; }
void set_scale_v_shift(int value) { g_scale_v_shift = value; }
void set_scalefactor(int value) { g_scalefactor = value; }
void set_score_screen(int value) { g_score_screen = value; }

void set_rotate_degrees(float fDegrees)
{
     g_fRotateDegrees = fDegrees;
     if (fDegrees != 0) g_rotated_state = true;
}

void set_score_bezel(bool bEnabled)
{
     if (bEnabled) {
         g_fullscreen = true;
     }
     g_sb_bezel = bEnabled;
}

void set_annun_bezel(bool bEnabled)
{
     if (bEnabled) {
         g_fullscreen = g_sb_bezel = true;
         g_game->m_software_scoreboard = true;
     }
     g_annun_bezel = bEnabled;
}

void set_annun_lamponly(bool bEnabled)
{
     if (bEnabled) {
         g_annun_lamps = true;
         g_annun_bezel_alpha = 2;
     }
}

void set_tq_keyboard(bool bEnabled)
{
     if (bEnabled) {
         g_scalefactor = 75;
         g_fullscreen = g_sb_bezel = true;
         g_game->m_software_scoreboard = true;
     }
     g_keyboard_bezel = bEnabled;
}

// position annunicator bezel
void set_annun_bezel_position(int xValue, int yValue)
{
    ann_bezel_pos_x = xValue;
    ann_bezel_pos_y = yValue;
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

    if (g_game->use_old_overlay()) dest.x = (short)((col * 6));
    else dest.x = (short)((col * 5));

    SDL_FillRect(surface, &dest, 0x00000000);
    SDL_Color color={0xf0, 0xf0, 0xf0};
    text_surface=TTF_RenderText_Solid(g_ttfont, t, color);

    SDL_BlitSurface(text_surface, NULL, surface, &dest);
    SDL_FreeSurface(text_surface);
}

void draw_shoot(int col, int row, SDL_Surface *surface)
{
    g_sb_surface = g_other_bmps[B_SHOOT];

    SDL_Rect dest;
    dest.x = (short)(col);
    dest.y = (short)(row);
    dest.w = (unsigned short) g_sb_surface->w;
    dest.h = (unsigned short) g_sb_surface->h;

    SDL_SetColorKey(g_sb_surface, SDL_TRUE, 0x00000000);
    SDL_BlitSurface(g_sb_surface, NULL, surface, &dest);
}

void draw_subtitle(char *s, bool insert)
{
    double h = 0.92, w = 0.97;

    int x = (int)((g_scaling_rect.w) - (g_scaling_rect.w * w)) + g_scaling_rect.x;
    int y = (int)(g_scaling_rect.h * h) + g_scaling_rect.y;
    static int m_message_timer;
    const int timeout = 200;

    if (insert) {
       m_message_timer = 0;
       g_bSubtitleShown = true;
       set_subtitle_display(s);
    } else if (m_message_timer > timeout) {
       g_bSubtitleShown = false;
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
    if (!SDL_RectEmpty(&g_rotate_rect)) return;

    g_bezel_toggle = false;
    Uint32 flags = (SDL_GetWindowFlags(g_window) ^ SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (SDL_SetWindowFullscreen(g_window, flags) < 0) {
        LOGW << fmt("Toggle fullscreen failed: %s", SDL_GetError());
        return;
    }

    if ((flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0 ||
         (flags & SDL_WINDOW_MAXIMIZED) != 0) {

        if ((flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) {
            SDL_RenderSetViewport(g_renderer, NULL);
            SDL_RenderGetViewport(g_renderer, &g_logical_rect);
        } else {
            g_logical_rect.w = g_displaymode.w;
            g_logical_rect.h = g_displaymode.h;
        }
        g_fullscreen = true;
        format_fullscreen_render();
        notify_stats(g_overlay_width, g_overlay_height, "u");

        if (g_bezel_texture) g_bezel_toggle = true;
        return;
    }
    g_fullscreen = false;
    format_window_render();
    notify_stats(g_overlay_width, g_overlay_height, "u");
    SDL_SetWindowSize(g_window, g_viewport_width, g_viewport_height);
    SDL_SetWindowPosition(g_window, SDL_WINDOWPOS_CENTERED,
                          SDL_WINDOWPOS_CENTERED);
}

void vid_toggle_scanlines()
{
    char s[16] = "scanlines off";

    SDL_BlendMode mode;
    SDL_GetRenderDrawBlendMode(g_renderer, &mode);
    if (mode != SDL_BLENDMODE_BLEND && !g_scanlines)
        SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);

    if (g_scanlines) {
        if (s_shunt < 10) {
            s_shunt++;
        } else {
            g_scanlines = false;
            SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);
            s_shunt = 2;
        }
    } else g_scanlines = true;

    if (g_scanlines) snprintf(s, sizeof(s), "shunt: %d", s_shunt);

    draw_subtitle(s, true);
}

void vid_scoreboard_switch()
{
    if (!g_sb_window) return;

    char s[16] = "screen: 0";
    int displays = SDL_GetNumVideoDisplays();

    if (displays > 1) {
        SDL_Rect displayDimensions[displays];
        int winId = SDL_GetWindowDisplayIndex(g_sb_window);

        for (int i = 0; i < displays; i++)
            SDL_GetDisplayBounds(i, &displayDimensions[i]);

        if (++winId == displays) winId = 0;
        snprintf(s, sizeof(s), "screen: %d", (unsigned char)winId);

        SDL_SetWindowPosition(g_sb_window,
           displayDimensions[winId].x + sb_window_pos_x,
              displayDimensions[winId].y + sb_window_pos_y);
    } else
        SDL_SetWindowPosition(g_sb_window, sb_window_pos_x,
                                 sb_window_pos_y);

    draw_subtitle(s, true);
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

    if (g_yuv_video_blank) {

        vid_blank_yuv_texture(false);

    } else if (g_yuv_video_needs_blank) {

        vid_blank_yuv_texture(false);
        g_yuv_video_needs_blank = false;

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
        SDL_FillRect(g_overlay_blitter, NULL, 0x00000000);
        SDL_BlitSurface(tx, NULL, g_overlay_blitter, NULL);

    } else {

        // MAC: 8bpp to RGBA8888 conversion. Black pixels are considered totally transparent so they become 0x00000000;
        for (int i = 0; i < (tx->w * tx->h); i++) {
	    *((uint32_t*)(g_overlay_blitter->pixels)+i) =
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
		g_yuv_texture = vid_create_yuv_texture(g_yuv_surface->width,
				    g_yuv_surface->height);
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
        SDL_RenderCopy(g_renderer, g_yuv_texture, NULL, &g_scaling_rect);

    // If there's an overlay texture, it means we are using some kind of overlay,
    // be it LEDs or any other thing, so RenderCopy it to the renderer ON TOP of the YUV video.
    if (g_overlay_texture)
            SDL_RenderCopy(g_renderer, g_overlay_texture, &g_render_size_rect, &g_scaling_rect);

    if (g_aux_needs_update) {
        SDL_UpdateTexture(g_aux_texture, &g_annu_rect,
            (void *)g_aux_blit_surface->pixels, g_aux_blit_surface->pitch);

        g_aux_needs_update = false;
    }

    // LDP1450 overlay
    if (g_LDP1450_overlay) draw_LDP1450_overlay();

    if (g_scanlines)
        draw_scanlines(s_shunt);

    // If there's a subtitle overlay
    if (g_bSubtitleShown) draw_subtitle(subchar, false);

    if (g_fRotateDegrees != 0) {
        if (!g_rotate) {
            int8_t ar = 2;
            if (g_aspect_ratio == ASPECTWS && g_overlay_resize) ar--;
            SDL_RenderSetLogicalSize(g_renderer, g_viewport_width, g_viewport_height);
            g_rotate_rect.w = g_render_size_rect.h + (g_render_size_rect.w >> ar);
            g_rotate_rect.h = g_render_size_rect.h;
            g_rotate_rect.y = g_render_size_rect.y;
            g_rotate_rect.x = 0;
            g_bezel_toggle = false;
            g_rotate = true;
        }
        SDL_RenderClear(g_renderer);
        if (g_yuv_texture)
            SDL_RenderCopyEx(g_renderer, g_yuv_texture, NULL, NULL,
                      g_fRotateDegrees, NULL, g_flipState);
        if (g_overlay_texture)
            SDL_RenderCopyEx(g_renderer, g_overlay_texture, &g_rotate_rect, NULL,
                      g_fRotateDegrees, NULL, g_flipState);
    } else if (g_game->get_sinden_border())
                draw_border(g_game->get_sinden_border(),
                      g_game->get_sinden_border_color());

    if (g_bezel_toggle) {
        if (g_bezel_texture) {
            SDL_RenderCopy(g_renderer, g_bezel_texture, NULL, NULL);
        }
        if (g_aux_texture) {
            SDL_RenderCopy(g_renderer, g_aux_texture, NULL, &g_aux_rect);
        }
        if (g_sb_bezel) {
            SDL_RenderCopy(g_renderer, g_sb_texture, NULL, &g_sb_bezel_rect);
        }
    }

    SDL_RenderPresent(g_renderer);

    if (g_softsboard_needs_update) {
        SDL_RenderPresent(g_sb_renderer);
        g_softsboard_needs_update = false;
    }

    if (queue_take_screenshot) {
        queue_take_screenshot = false;
        take_screenshot();
    }
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

    SDL_Surface *screenshot = NULL;

    SDL_GetRendererOutputSize(g_renderer, &g_logical_rect.w, &g_logical_rect.h);

    screenshot = SDL_CreateRGBSurface(0, g_logical_rect.w, g_logical_rect.h, 32, 0, 0, 0, 0);

    if (!screenshot) { LOGE << "Cannot allocate screenshot surface"; return; }

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
    int fs = g_scaling_rect.w / 36;
    int ffs = g_aspect_ratio == ASPECTWS ? g_scaling_rect.w / 24 : g_scaling_rect.w / 18;

    FC_FreeFont(g_font);
    g_font = FC_CreateFont();
    FC_LoadFont(g_font, g_renderer, "fonts/default.ttf", fs,
                FC_MakeColor(0xff, 0xff, 0xff, 0xff), TTF_STYLE_NORMAL);

    FC_FreeFont(g_fixfont);
    g_fixfont = FC_CreateFont();
    FC_LoadFont(g_fixfont, g_renderer, "fonts/timewarp.ttf", ffs,
                FC_MakeColor(0xff, 0xff, 0xff, 0xff), TTF_STYLE_NORMAL);

    TTF_CloseFont(g_ttfont);
    if (g_game->use_old_overlay()) {
        ttfont = "fonts/daphne.ttf";
        g_ttfont = TTF_OpenFont(ttfont, 12);
    } else {
        ttfont = "fonts/digital.ttf";
        g_ttfont = TTF_OpenFont(ttfont, 14);
    }

    if (g_ttfont == NULL) {
        LOG_ERROR << fmt("Cannot load TTF font: '%s'", (char*)ttfont);
        g_game->set_game_errors(SDL_ERROR_FONT);
        set_quitflag();
    }
}

void format_window_render() {

    g_scaling_rect.w = (g_viewport_width * g_scalefactor) / 100;
    g_scaling_rect.h = (g_viewport_height * g_scalefactor) / 100;
    g_scaling_rect.x = ((g_viewport_width - g_scaling_rect.w) >> 1);
    g_scaling_rect.y = ((g_viewport_height - g_scaling_rect.h) >> 1);

    g_scaling_rect.x = (g_scaling_rect.x * g_scale_h_shift) / 100;
    g_scaling_rect.y = (g_scaling_rect.y * g_scale_v_shift) / 100;

    g_logical_rect.w = g_scaling_rect.w;
    g_logical_rect.h = g_scaling_rect.h;

    load_fonts();
}

void format_fullscreen_render() {

    int w, h;

    if (g_vid_resized) {

        w = g_viewport_width;
        h = g_viewport_height;

    } else {

        if (g_vertical_orientation)
            h = g_logical_rect.w;
        else h = g_logical_rect.h;

        switch (g_aspect_ratio) {
        case ASPECTWS:
            w = (h * 16) / 9;
            break;
        case ASPECTSD:
            w = (h * 4) / 3;
            break;
        default:
            double ratio = (double)g_viewport_width /
                     (double)g_viewport_height;
            w = h * ratio;
            break;
        }
    }

    g_bezel_scalewidth = w;
    g_scaling_rect.w = (w * g_scalefactor) / 100;
    g_scaling_rect.h = (h * g_scalefactor) / 100;
    g_scaling_rect.x = ((g_logical_rect.w - g_scaling_rect.w) >> 1);
    g_scaling_rect.y = ((g_logical_rect.h - g_scaling_rect.h) >> 1);

    if (g_keyboard_bezel)
        g_scaling_rect.y = g_scaling_rect.y >> 2;
    else
    {
        g_scaling_rect.x = (g_scaling_rect.x * g_scale_h_shift) / 100;
        g_scaling_rect.y = (g_scaling_rect.y * g_scale_v_shift) / 100;
    }

    if (g_sb_bezel) {

        double ratio = (float)g_sb_h / (float)g_sb_w;
        double scale = 9.0f - double((g_sb_bezel_scale << 1) / 10.0f);

        g_sb_bezel_rect.x = sb_window_pos_x;
        g_sb_bezel_rect.y = sb_window_pos_y;
        g_sb_bezel_rect.w = (g_bezel_scalewidth / scale);
        g_sb_bezel_rect.h = (g_sb_bezel_rect.w * ratio);
        g_bezel_toggle = true;
    }

    load_fonts();
}

void draw_scanlines(int l) {

    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, s_alpha);

    for (int i = 0; i < g_logical_rect.h; i+=l) {
       SDL_RenderDrawLine(g_renderer, 0, i, g_logical_rect.w, i);
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

    tb.x = lb.x = bb.x = g_scaling_rect.x;
    tb.y = lb.y = rb.y = g_scaling_rect.y;
    rb.x = (g_scaling_rect.w + g_scaling_rect.x) - s;
    bb.y = (g_scaling_rect.h - s) + g_scaling_rect.y;
    tb.w = bb.w = g_scaling_rect.w;
    tb.h = bb.h = lb.w = rb.w = s;
    lb.h = rb.h = g_scaling_rect.h;

    SDL_RenderFillRect(g_renderer, &tb);
    SDL_RenderFillRect(g_renderer, &lb);
    SDL_RenderFillRect(g_renderer, &rb);
    SDL_RenderFillRect(g_renderer, &bb);

    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
}

}
