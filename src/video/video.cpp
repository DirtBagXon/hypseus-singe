/*
 * ____ HYPSEUS COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2001 Matt Ownby, 2026 DirtBagXon
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

#include "config.h"

// video.cpp
// Part of the HYPSEUS emulator
// This code started by Matt Ownby, May 2000

#include "../hypseus.h"
#include "../game/game.h"
#include "../io/conout.h"
#include "../io/error.h"
#include "../io/mpo_fileio.h"
#include "../io/mpo_mem.h"
#include "icon.h"
#include "video.h"
#include <SDL3_image/SDL_image.h> // screenshot
#include <plog/Log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string> // for some error messages

namespace video {

int g_video_width = 640, g_video_height = 480; // default video dimensions

unsigned int g_probe_width = g_video_width;
unsigned int g_probe_height = g_video_height;

const int g_anun_w = 220, g_anun_h = 128;
const int g_scoreboard_w = 340, g_scoreboard_h = 480;

int g_viewport_width = g_video_width, g_viewport_height = g_video_height;
int g_aspect_width = 0, g_aspect_height = 0;
int g_alloc_screen = 0;
int g_scoreboard_screen = -1;
int g_displays = 0;
int g_display = 0;

int scoreboard_window_pos_x = 0, scoreboard_window_pos_y = 0;
int aux_bezel_pos_x = 0, aux_bezel_pos_y = 0;
int g_scale_h_shift = 100, g_scale_v_shift = 100;

// the current game overlay dimensions
unsigned int g_overlay_width = 0, g_overlay_height = 0;

TTF_Font *g_font                    = nullptr;
TTF_Font *g_ttfont                  = nullptr;
TTF_TextEngine *g_font_engine       = nullptr;
SDL_Surface *g_led_bmps[LED_RANGE]  = {0};
SDL_Surface *g_other_bmps[B_EMPTY]  = {0};
SDL_Window *g_window                = NULL;
SDL_Window *g_scoreboard_window     = NULL;
SDL_Renderer *g_renderer            = NULL;
SDL_Renderer *g_scoreboard_renderer = NULL;
SDL_Texture *g_overlay_texture      = NULL;
SDL_Texture *g_yuv_texture          = NULL;
SDL_Surface *g_overlay_surface      = NULL;
SDL_Surface *g_scoreboard_surface   = NULL;
SDL_Texture *g_scoreboard_texture   = NULL;
SDL_Surface *g_aux_blit_surface     = NULL;
SDL_Texture *g_bezel_texture        = NULL;
SDL_Texture *g_aux_texture          = NULL;
SDL_Texture *g_mix_texture          = NULL;
SDL_Surface *g_scoreboard_blit_surface = NULL;

SDL_Rect g_aux_rect;
SDL_Rect g_border_rect;
SDL_Rect g_scaling_rect          = {0};
SDL_Rect g_logical_rect          = {0};
SDL_Rect g_scoreboard_bezel_rect = {0};
SDL_Rect g_local_size_rect       = {0, 0, 320, 240};

SDL_Rect g_limit_rect = g_local_size_rect;
SDL_Rect g_annu_rect = g_local_size_rect;

std::vector<SDL_Rect> displayDimensions;

SDL_FRect fScaleRect;
SDL_FRect *g_yuv_frect[2] = { NULL };

bool m_isIndex8 = false;
bool m_lua_game = false;
bool m_sboverlay_white = false;
bool m_block_driver_overlay = false;

bool queue_take_screenshot = false;

bool g_scale_linear = false;
bool g_singe_blend_sprite = false;
bool g_scanlines = false;
bool g_softrender = false;
bool g_opengl = false;
bool g_vulkan = false;
bool g_teardown = false;
bool g_intro = true;
bool g_forcetop = false;
bool g_grabmouse = false;
bool g_vsync = true;
bool g_yuv_blue = false;
bool g_video_resized = false;
bool g_enhance_overlay = false;
bool g_overlay_dynamic = false;
bool g_bForceAspect = false;
bool g_bIgnoreAspect = false;
bool g_fullscreen = false;
bool g_bezel_toggle = false;
bool g_bezel_reverse = false;
bool g_bezel_load = false;
bool g_scoreboard_bezel = false;
bool g_scaled = false;
bool g_logo = false;
bool g_rotate = false;
bool g_keyboard_bezel = false;
bool g_aux_bezel = false;
bool g_ded_annun_bezel = false;
bool g_annun_lamps = false;
bool g_vertical_orientation = false;
bool g_legacy_overlay = false;
bool g_toggle_lock = false;

int g_scalefactor = 100;   // by RDG2010
int g_aspect_ratio = 0;
int sboverlay_characterset = 2;
int g_yuv_display = YUV_VISIBLE;

int8_t g_rescale = 0;

int g_scoreboard_bezel_scale = 14;
int g_aux_bezel_scale = 12;
int g_bezel_scalewidth = 0;

uint8_t g_scanline_alpha = 128;
uint8_t g_scanline_shunt = 2;
uint8_t g_yuv_flags = 0;
int8_t g_yuv_luma = YUV_FLAG_LUMA;

uint8_t g_bSubtitleShown = 0;
char g_window_title[TITLE_LENGTH] = "";
char *subchar = NULL;
char *srtchar = NULL;

SDL_TextureAccess g_texture_access = SDL_TEXTUREACCESS_STREAMING;

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
    SDL_Mutex *mutex;
} m_yuv_surface_t;

m_yuv_surface_t *g_yuv_surface;

// Blitting parameters. Textures that need updating in a blitting strike?
bool g_softsboard_needs_update = false;
bool g_yuv_video_needs_update  = false;
bool g_aux_needs_update        = false;
bool g_yuv_skip                = true;

//////////////////////////////////////////////////////////////////////////////

static void ConvertSurface(SDL_Surface **surface, SDL_PixelFormat fmt)
{
    SDL_Surface *tmpSurface = SDL_ConvertSurface(*surface, fmt);
    SDL_DestroySurface(*surface);
    *surface = tmpSurface;
}

static void LogicalPosition(SDL_Rect *port, SDL_Rect *dst, int x, int y)
{
    SDL_Rect tmpRect = {0, 0, dst->w, dst->h};
    tmpRect.x = (((port->w - dst->w) * x) / 100);
    tmpRect.y = (((port->h - dst->h) * y) / 100);

    *dst = tmpRect;
}

static inline void copyPlane(uint8_t *dst, const uint8_t *src, int w, int h, int srcPitch)
{
    if (srcPitch == w) {
        memcpy(dst, src, (size_t)w * h);
    } else {
        for (int y = 0; y < h; ++y)
            memcpy(dst + (size_t)y * w, src + (size_t)y * srcPitch, w);
    }
}

static void calcAuxRect()
{
     double scale = 9.0f - double((g_aux_bezel_scale << 1) / 10.0f);
     g_aux_rect.w = (g_bezel_scalewidth / scale);
     g_aux_rect.h = (g_aux_rect.w * g_aux_ratio);
}

static void resize_cleanup()
{
    g_rotate = false;
    g_bezel_load = false;

    SDL_DestroySurface(g_overlay_surface);
    SDL_DestroySurface(g_aux_blit_surface);

    g_overlay_surface = NULL;
    g_aux_blit_surface = NULL;

    SDL_DestroyTexture(g_bezel_texture);
    SDL_DestroyTexture(g_aux_texture);
    SDL_DestroyTexture(g_scoreboard_texture);

    g_bezel_texture = NULL;
    g_aux_texture = NULL;
    g_scoreboard_texture = NULL;

    TTF_CloseFont(g_font);
    TTF_CloseFont(g_ttfont);
    TTF_DestroyRendererTextEngine(g_font_engine);

    g_font = nullptr;
    g_ttfont = nullptr;
    g_font_engine = nullptr;

    SDL_DestroyTexture(g_mix_texture);
    SDL_DestroyTexture(g_overlay_texture);

    if (g_renderer)
    {
        SDL_RenderClear(g_renderer);
        SDL_RenderPresent(g_renderer);
        SDL_DestroyRenderer(g_renderer);
    }

    g_mix_texture = NULL;
    g_overlay_texture = NULL;
    g_renderer = NULL;

    SDL_Delay(100);
}

static void load_fonts()
{
    if (g_font)
    {
       TTF_CloseFont(g_font);
       g_font = nullptr;
    }

    if (g_ttfont)
    {
        TTF_CloseFont(g_ttfont);
        g_ttfont = nullptr;
    }

    if (g_font_engine)
    {
        TTF_DestroyRendererTextEngine(g_font_engine);
        g_font_engine = nullptr;
    }

    g_font_engine = TTF_CreateRendererTextEngine(g_renderer);

    int fs = g_scaling_rect.w / ((g_aspect_ratio == ASPECTPD) ? 24 : 36);
    const char *base = "fonts/default.ttf";

    const char *ttfont = g_legacy_overlay ? "fonts/daphne.ttf" :
        (m_lua_game ? "fonts/daphne.ttf" : "fonts/digital.ttf");

    int size = g_legacy_overlay ? 12 : (m_lua_game ? 12 : 6);

    g_font = TTF_OpenFont(base, fs);
    g_ttfont = TTF_OpenFont(ttfont, size);

    TTF_SetFontStyle(g_font, TTF_STYLE_NORMAL);

    if (!g_font)
    {
        LOG_ERROR << fmt("Cannot load TTF font: %s", base);
        set_quitflag();
    }

    if (!g_ttfont)
    {
        LOG_ERROR << fmt("Cannot load TTF font: '%s'", (char*)ttfont);
        set_quitflag();
    }
}

static void format_window_render()
{
    g_scaling_rect.w = (g_viewport_width * g_scalefactor) / 100;
    g_scaling_rect.h = (g_viewport_height * g_scalefactor) / 100;

    float diff_w = (g_viewport_width - g_scaling_rect.w) >> 1;
    float diff_h = (g_viewport_height - g_scaling_rect.h) >> 1;

    g_scaling_rect.x = (diff_w * g_scale_h_shift) / 100;
    g_scaling_rect.y = (diff_h * g_scale_v_shift) / 100;

    g_logical_rect.w = g_scaling_rect.w;
    g_logical_rect.h = g_scaling_rect.h;

    if (!g_rescale)
    {
        g_border_rect = (SDL_Rect){0, 0, g_viewport_width, g_viewport_height};
        load_fonts();
    }
}

static void format_fullscreen_render()
{
    int w, h;
    double ratio = static_cast<double>(g_viewport_width) / g_viewport_height;

    if (g_video_resized)
    {
        w = g_viewport_width;
        h = g_viewport_height;
    }
    else
    {
        h = g_vertical_orientation ? g_logical_rect.w : g_logical_rect.h;

        switch (g_aspect_ratio) {
            case ASPECTWS:  w = (h * 16) / 9; break;
            case ASPECTSD:  w = (h * 4) / 3; break;
            case ASPECTPD:
                if (g_vertical_orientation) h = static_cast<int>(h * 1.33);
                w = (h * 3) / 4;
                break;
            default: w = static_cast<int>(h * ratio); break;
        }
    }

    w = g_bForceAspect ? (h * 4) / 3 : (g_bIgnoreAspect ? static_cast<int>(h * ratio) : w);

    g_scaling_rect.w = (w * g_scalefactor) / 100;
    g_scaling_rect.h = (h * g_scalefactor) / 100;
    g_scaling_rect.x = (g_logical_rect.w - g_scaling_rect.w) >> 1;
    g_scaling_rect.y = (g_logical_rect.h - g_scaling_rect.h) >> 1;

    if (g_keyboard_bezel && !g_scaled)
    {
        g_scaling_rect.y /= 4;
        g_scale_v_shift = 0;
    }
    else
    {
        g_scaling_rect.x = (g_scaling_rect.x * g_scale_h_shift) / 100;
        g_scaling_rect.y = (g_scaling_rect.y * g_scale_v_shift) / 100;
    }

    if (g_scoreboard_bezel)
    {
        g_bezel_scalewidth = w;
        double bezel_ratio = static_cast<double>(g_scoreboard_h) / g_scoreboard_w;
        double scale = 9.0 - (static_cast<double>(g_scoreboard_bezel_scale) * 2.0 / 10.0);

        g_scoreboard_bezel_rect = {scoreboard_window_pos_x, scoreboard_window_pos_y,
                           static_cast<int>(g_bezel_scalewidth / scale),
                           static_cast<int>((g_bezel_scalewidth / scale) * bezel_ratio)};
        g_bezel_toggle = true;
    }

    if (!g_rescale)
    {
        g_border_rect = {
            (g_logical_rect.w - w) / 2,
            (g_logical_rect.h - h) / 2,
            w, h
        };
        load_fonts();
    }
}

static bool draw_ranks()
{
    if (g_annun_lamps) return false;

    SDL_Rect dest;
    dest.x = 10;
    dest.y = dest.x - 1;

    for (int i = B_ACE_SPACE; i < B_MIA; i++) {

        g_scoreboard_surface = g_other_bmps[i];

        dest.w = (unsigned short) g_scoreboard_surface->w;
        dest.h = (unsigned short) g_scoreboard_surface->h;
        SDL_BlitSurface(g_scoreboard_surface, NULL, g_aux_blit_surface, &dest);
        dest.y = dest.y + (ANUN_CHAR_HEIGHT << 1) + (ANUN_CHAR_HEIGHT);
    }

    return true;
}

// Initialize the display systems
bool init_display()
{
    bool result = false;
    static bool notify = false;
    constexpr char title[] = "HYPSEUS Singe: Multiple Arcade Laserdisc Emulator";

    Uint32 sdl_flags = 0;
    Uint32 sdl_scoreboard_flags = SDL_WINDOW_BORDERLESS;
    const char *sdl_render_flags = NULL;

    if (g_forcetop)
        sdl_flags |= SDL_WINDOW_ALWAYS_ON_TOP;

    if (g_opengl) {
        sdl_flags |= SDL_WINDOW_OPENGL;
        sdl_scoreboard_flags |= SDL_WINDOW_OPENGL;
    } else if (g_vulkan) {
        sdl_flags |= SDL_WINDOW_VULKAN;
        sdl_scoreboard_flags |= SDL_WINDOW_VULKAN;
    }

    g_overlay_width = g_game->get_video_overlay_width();
    g_overlay_height = g_game->get_video_overlay_height();

    // Enforce a minimum window size
    g_probe_width = std::max((int)g_probe_width, 320);
    g_probe_height = std::max((int)g_probe_height, 240);

    if (g_video_resized) {
        g_viewport_width  = g_video_width;
        g_viewport_height = g_video_height;

    } else {

        if (!g_bIgnoreAspect && g_aspect_width > 0) {
            g_viewport_width  = g_aspect_width;
            g_viewport_height = g_aspect_height;

        } else {
            g_viewport_width  = g_probe_width;
            g_viewport_height = g_probe_height;
        }
    }

    // Enforce 4:3 aspect ratio
    if (g_bForceAspect) {
        double dCurAspect = (double)g_viewport_width / g_viewport_height;
        const double dTARGET_ASPECT_RATIO = 4.0 / 3.0;

        if (dCurAspect < dTARGET_ASPECT_RATIO)
            g_viewport_height = (g_viewport_width * 3) / 4;
        else if (dCurAspect > dTARGET_ASPECT_RATIO)
            g_viewport_width = (g_viewport_height * 4) / 3;
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

    SDL_DisplayID *id = SDL_GetDisplays(&g_displays);
    displayDimensions.resize(g_displays);

    for (int i = 0; i < g_displays; i++) {
        SDL_GetDisplayBounds(id[i], &displayDimensions[i]);
    }

    SDL_free(id);

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

        g_window = SDL_CreateWindow(title, g_viewport_width, g_viewport_height, sdl_flags);
    }

    if (!g_window) {

        LOGE << fmt("Could not initialize window: %s", SDL_GetError());
        set_quitflag();
        goto exit;

    } else {

        if (g_fullscreen)
            SDL_SetWindowFullscreen(g_window, true);
        else if (!notify)
            g_toggle_lock = true;

        if (strlen(g_window_title) > 0)
            SDL_SetWindowTitle(g_window, g_window_title);

        SDL_IOStream* ops = SDL_IOFromConstMem(ghci, sizeof(ghci));
        SDL_Surface *rep = IMG_Load_IO(ops, true);

        if (rep != NULL) {
            SDL_SetWindowIcon(g_window, rep);
            SDL_DestroySurface(rep);
        }

        if (g_softrender) sdl_render_flags = SDL_SOFTWARE_RENDERER;

        g_renderer = SDL_CreateRenderer(g_window, sdl_render_flags);
        SDL_SetRenderVSync(g_renderer, g_vsync ?
            SDL_RENDERER_VSYNC_ADAPTIVE : SDL_RENDERER_VSYNC_DISABLED);

        SDL_RaiseWindow(g_window);

        if (!g_renderer)
        {
            LOGE << fmt("Could not initialize renderer: %s", SDL_GetError());
            set_quitflag();
            goto exit;

        } else {

            if (g_keyboard_bezel) {

                g_aux_needs_update = true;

                g_tqkeys = g_bezel_path + "/tqkeys.png";

                if (!mpo_file_exists(g_tqkeys.c_str()))
                    g_tqkeys = "pics/tqkeys.png";
            }

            m_lua_game = (g_game->get_game_type() == GAME_SINGE);

            Uint32 current = SDL_GetWindowFlags(g_window);

            if ((current & SDL_WINDOW_FULLSCREEN)) {

                g_logical_rect = displayDimensions[g_display];

                if (!g_bezel_file.empty())
                    g_bezel_toggle = true;

                format_fullscreen_render();

            } else
                format_window_render();

            if (g_game->m_software_scoreboard) {

                if (!g_scoreboard_blit_surface)
                    g_scoreboard_blit_surface = SDL_CreateSurface( g_scoreboard_w,
                                                   g_scoreboard_h, SDL_PIXELFORMAT_RGBA32);

                if (!g_scoreboard_blit_surface) {
                    LOGE << fmt("Could not initialize g_scoreboard_blit_surface: %s", SDL_GetError());
                    set_quitflag();
                    goto exit;
                }

                if (!g_scoreboard_bezel && !g_scoreboard_window) {

                    double scale = double((g_scoreboard_bezel_scale >> 1) / 7.0f);

                    g_scoreboard_window = SDL_CreateWindow(NULL,
                        (scale * g_scoreboard_w), (scale * g_scoreboard_h),
                            sdl_scoreboard_flags | SDL_WINDOW_HIDDEN);

                    if (!g_scoreboard_window) {
                        LOGE << fmt("Could not initialize scoreboard window: %s", SDL_GetError());
                        set_quitflag();
                        goto exit;
                    }

                    if (g_displays > 1) {

                        int screen;

                        if (g_scoreboard_screen != -1)
                        {
                            screen = g_scoreboard_screen;
                        }
                            else if (g_display == 0)
                        {
                            screen = 1;
                        }
                        else
                        {
                            screen = g_display;
                        }

                        SDL_SetWindowPosition(g_scoreboard_window,
                           displayDimensions[screen].x + scoreboard_window_pos_x,
                              displayDimensions[screen].y + scoreboard_window_pos_y);
                    } else
                        SDL_SetWindowPosition(g_scoreboard_window, scoreboard_window_pos_x,
                                                  scoreboard_window_pos_y);

                    SDL_ShowWindow(g_scoreboard_window);
                    SDL_RaiseWindow(g_scoreboard_window);

                    g_scoreboard_renderer = SDL_CreateRenderer(g_scoreboard_window,
                                                sdl_render_flags);
                    SDL_SetRenderVSync(g_scoreboard_renderer, g_vsync ?
                                  SDL_RENDERER_VSYNC_ADAPTIVE : SDL_RENDERER_VSYNC_DISABLED);

                    if (!g_scoreboard_renderer) {
                        LOGE << fmt("Could not initialize scoreboard renderer: %s", SDL_GetError());
                        set_quitflag();
                        goto exit;
                    }
                    SDL_SetRenderDrawColor(g_scoreboard_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
                    SDL_RenderClear(g_scoreboard_renderer);
                    SDL_RenderPresent(g_scoreboard_renderer);
                }
            }

            if (g_aux_bezel) {

                if (g_ded_annun_bezel) g_aux_bezel_scale--;

                g_aux_ratio = (float)g_anun_h / (float)g_anun_w;

                g_aux_blit_surface = SDL_CreateSurface(g_anun_w, g_anun_h,
                                           SDL_PIXELFORMAT_RGBA32);

                if (!g_aux_blit_surface) {
                    LOGE << fmt("Failed to create auxillary surface: %s", SDL_GetError());
                    set_quitflag();
                    goto exit;
                }

                calcAuxRect();

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
            SDL_HideCursor();

            if (g_grabmouse)
                SDL_SetWindowMouseGrab(g_window, true);

            if (g_scanlines)
                SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);

            if (g_game->get_outline_border())
                g_scale_h_shift = g_scale_v_shift = 100;

            g_overlay_surface =
                SDL_CreateSurface(g_overlay_width, g_overlay_height,
                                         SDL_PIXELFORMAT_RGBA32);

            if (!g_overlay_surface) {
                LOGE << fmt("Could not initialize g_overlay_surface: %s", SDL_GetError());
                set_quitflag();
                goto exit;
            }

            // Check for game overlay enhancements (depth and size)
            g_enhance_overlay = g_game->has_overlay_upgrade(GAME_OVERLAY_UPGRADE);
            g_overlay_dynamic = g_game->get_dynamic_overlay();

            // Set it's color key to NOT copy 0x000000ff pixels.
            ConvertSurface(&g_other_bmps[B_OVERLAY_LEDS], g_overlay_surface->format);

            Uint32 colorkey = SDL_MapSurfaceRGB(g_other_bmps[B_OVERLAY_LEDS], 0, 0, 0);
            SDL_SetSurfaceColorKey(g_other_bmps[B_OVERLAY_LEDS], true, colorkey);

            if (g_aux_blit_surface)
                ConvertSurface(&g_aux_blit_surface, g_overlay_surface->format);

            ConvertSurface(&g_other_bmps[B_OVERLAY_LDP1450], g_overlay_surface->format);
            colorkey = SDL_MapSurfaceRGB(g_other_bmps[B_OVERLAY_LDP1450], 0, 0, 0);
            SDL_SetSurfaceColorKey(g_other_bmps[B_OVERLAY_LDP1450], true, colorkey);

            if (g_overlay_width && g_overlay_height) {

                g_overlay_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGBA32,
                                        g_texture_access, g_overlay_width, g_overlay_height);

                if (!g_overlay_texture) {
                    LOGE << fmt("Could not initialize g_overlay_texture: %s", SDL_GetError());
                    set_quitflag();
                    goto exit;
                }

                SDL_SetTextureBlendMode(g_overlay_texture, SDL_BLENDMODE_BLEND);
                SDL_SetTextureAlphaMod(g_overlay_texture, 0xff);

                SDL_SetTextureScaleMode(g_overlay_texture, g_scale_linear ?
                          SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
            }

            g_mix_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGBA32,
                      SDL_TEXTUREACCESS_TARGET, g_logical_rect.w, g_logical_rect.h);

            if (!g_mix_texture) {
                LOGE << fmt("Mixing texture creation failed: %s", SDL_GetError());
                set_quitflag();
                goto exit;
            }
            SDL_SetTextureBlendMode(g_mix_texture, SDL_BLENDMODE_BLEND);
            SDL_SetTextureAlphaMod(g_mix_texture, 0xff);

            SDL_SetTextureScaleMode(g_mix_texture, g_scale_linear ?
                      SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);

            SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);

            SDL_RenderClear(g_renderer);
            SDL_RenderPresent(g_renderer);

            m_isIndex8 = (g_game->get_overlay_depth() == GAME_OVERLAY_STD) ? true : false;

            if (!notify && g_intro) splash(g_logo);
            result = true;
        }
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

void vid_free_yuv_overlay()
{
    // Here we free both the YUV surface and YUV texture.
    SDL_DestroyMutex (g_yuv_surface->mutex);
   
    free(g_yuv_surface->Yplane);
    free(g_yuv_surface->Uplane);
    free(g_yuv_surface->Vplane);
    free(g_yuv_surface);

    if (g_yuv_texture)
        SDL_DestroyTexture(g_yuv_texture);

    g_yuv_surface = NULL;
    g_yuv_texture = NULL;
}

///////////////////////////////////////////////////////

// deinitializes the window and renderer we have used.
// returns true if successful, false if failure
bool deinit_display()
{
    SDL_SetWindowMouseGrab(g_window, false);

    SDL_DestroyTexture(g_scoreboard_texture);

    SDL_DestroyRenderer(g_scoreboard_renderer);
    SDL_DestroyWindow(g_scoreboard_window);

    SDL_DestroySurface(g_scoreboard_blit_surface);
    SDL_DestroySurface(g_aux_blit_surface);

    g_scoreboard_texture = NULL;
    g_scoreboard_renderer = NULL;
    g_scoreboard_window = NULL;
    g_scoreboard_blit_surface = NULL;
    g_aux_blit_surface = NULL;

    SDL_DestroySurface(g_overlay_surface);

    g_overlay_surface = NULL;

    TTF_CloseFont(g_font);
    TTF_CloseFont(g_ttfont);
    TTF_DestroyRendererTextEngine(g_font_engine);

    g_font = nullptr;
    g_ttfont = nullptr;
    g_font_engine = nullptr;

    SDL_DestroyTexture(g_bezel_texture);

    SDL_DestroyTexture(g_aux_texture);

    for (int i = 0; i < 2; ++i) {
        delete g_yuv_frect[i];
        g_yuv_frect[i] = NULL;
    }

    g_bezel_texture = NULL;
    g_aux_texture = NULL;

    SDL_DestroyTexture(g_overlay_texture);
    SDL_DestroyTexture(g_mix_texture);
    SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_window);

    g_mix_texture = NULL;
    g_overlay_texture = NULL;
    g_renderer = NULL;
    g_window = NULL;
    free(subchar);
    free(srtchar);

    return (true);
}

// Clear the renderer. Good for avoiding texture mess (YUV, LEDs, Overlay...)
void vid_blank()
{
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(g_renderer);
}

static void colorLeds(SDL_Surface *src)
{
    if (m_sboverlay_white) return;

    const SDL_PixelFormatDetails* fmt = SDL_GetPixelFormatDetails(src->format);

    SDL_Surface* tmp = SDL_CreateSurface(src->w, src->h,
                           fmt->format);
    if (!tmp) return;

    uint8_t r, g, b, a;
    SDL_LockSurface(tmp);
    SDL_LockSurface(src);

    for (int y = 0; y < src->h; ++y) {
        for (int x = 0; x < src->w; ++x) {

            uint8_t* pixel = ((uint8_t*)src->pixels) + y * src->pitch + x * fmt->bytes_per_pixel;

            SDL_GetRGBA(*(uint32_t*)pixel, fmt, nullptr, &r, &g, &b, &a);

            if (r == 0xff && g == 0xff && b == 0xff)
	        { r = 0xc8; g = 0x0d; b = 0x0d; }
            if (r == 0x50 && g == 0x4a && b == 0x4a)
	        { r = 0x38; g = 0x00; b = 0x00; }

            uint8_t* tmpPixel = ((uint8_t*)tmp->pixels) + y * tmp->pitch + x * fmt->bytes_per_pixel;
            *(uint32_t*)tmpPixel = SDL_MapRGBA(fmt, nullptr, r, g, b, a);
        }
    }

    SDL_UnlockSurface(tmp);
    SDL_UnlockSurface(src);

    SDL_BlitSurface(tmp, NULL, src, NULL);
    SDL_DestroySurface(tmp);
}

static SDL_Surface *load_led_strip(const char *filename)
{
    std::string filepath = fmt("pics/%s", filename);

    SDL_Surface *result  = SDL_LoadBMP(filepath.c_str());

    if (!result) {
        LOGE << fmt("Could not load bitmap: %s", filepath.c_str());
    } else colorLeds(result);

    return (result);
}

static SDL_Surface *load_one_bmp(const char *filename, bool bezel)
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

static SDL_Surface *load_one_png(const char *filename)
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

    g_other_bmps[B_DL_PLAYER1]  = load_one_bmp("player1.bmp", false);
    g_other_bmps[B_DL_PLAYER2]  = load_one_bmp("player2.bmp", false);
    g_other_bmps[B_DL_LIVES]    = load_one_bmp("lives.bmp", false);
    g_other_bmps[B_DL_CREDITS]  = load_one_bmp("credits.bmp", false);
    g_other_bmps[B_TQ_TIME]     = load_one_bmp("time.bmp", false);

    g_other_bmps[B_OVERLAY_LEDS] =
    load_led_strip(sboverlay_characterset == 1 ? "overlayleds1.bmp" : "overlayleds2.bmp");
   
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

// Draw's one of our LED's to the screen
// value contains the bitmap to draw (0-9 is valid)
// x and y contain the coordinates on the screen
// This function is called from img-scoreboard.cpp
// 1 is returned on success, 0 on failure
bool draw_led(int value, int x, int y, unsigned char end)
{
    g_scoreboard_surface = g_led_bmps[value];
    static unsigned char led = 0;

    SDL_Rect dest;
    dest.x = (short) x;
    dest.y = (short) y;
    dest.w = (unsigned short) g_scoreboard_surface->w;
    dest.h = (unsigned short) g_scoreboard_surface->h;

    if (!SDL_BlitSurface(g_scoreboard_surface, NULL, g_scoreboard_blit_surface, &dest)) {
        LOGE << fmt("Could not Blit Scoreboard LED's %s", SDL_GetError());
        set_quitflag();
        return false;
    }

    if (led == end) {

        if (g_scoreboard_texture) SDL_DestroyTexture(g_scoreboard_texture);

        SDL_Renderer *renderer = g_scoreboard_bezel ? g_renderer : g_scoreboard_renderer;

        g_scoreboard_texture = SDL_CreateTextureFromSurface(renderer, g_scoreboard_blit_surface);
        if (!g_scoreboard_texture) return false;

        if (!g_scoreboard_bezel) {
            SDL_RenderTexture(g_scoreboard_renderer, g_scoreboard_texture, NULL, NULL);
            g_softsboard_needs_update = true;
        }
        led = -1;
    }
    led++;
    return true;
}

static bool draw_annunciator1(int which)
{
    g_scoreboard_surface = g_other_bmps[B_ANUN_OFF];

    SDL_Rect dest;
    uint8_t spacer = 4;

    dest.x = g_aux_blit_surface->w - (g_scoreboard_surface->w
                   + (g_scoreboard_surface->w >> 1));

    dest.w = (unsigned short) g_scoreboard_surface->w;
    dest.h = (unsigned short) g_scoreboard_surface->h;

    for (int i = 0; i < ANUN_LEVELS; i++) {
        dest.y = ((dest.h + ANUN_CHAR_HEIGHT) * i) + spacer;
        if (g_annun_lamps) dest.x = 110 - (-i * 15);
        SDL_FillSurfaceRect(g_aux_blit_surface, &dest, 0x00000000);
        SDL_BlitSurface(g_scoreboard_surface, NULL, g_aux_blit_surface, &dest);
    }

    if (which) {
        g_scoreboard_surface = g_other_bmps[B_ANUN_ON];
        if (g_annun_lamps) dest.x = 110 - ((-which + 1) * 15);
        dest.y = ((dest.h + ANUN_CHAR_HEIGHT) * --which) + spacer;
        SDL_BlitSurface(g_scoreboard_surface, NULL, g_aux_blit_surface, &dest);
    }

    g_aux_needs_update = true;
    return true;
}

static bool draw_annunciator2(int which)
{
    SDL_Rect dest;

    dest.x = 0;
    dest.h = 40;
    dest.w = 220;

    for (int i = B_ACE_OFF; i < B_EMPTY; i++) {
        g_scoreboard_surface = g_other_bmps[i];
        dest.y = (ANUN_RANK_HEIGHT * (i - B_ACE_OFF));
        SDL_FillSurfaceRect(g_aux_blit_surface, &dest, 0x00000000);
        SDL_BlitSurface(g_scoreboard_surface, NULL, g_aux_blit_surface, &dest);
    }

    if (which) {
        g_scoreboard_surface = g_other_bmps[B_MIA + which];
        dest.y = ANUN_RANK_HEIGHT * --which;
        SDL_BlitSurface(g_scoreboard_surface, NULL, g_aux_blit_surface, &dest);
    }

    g_aux_needs_update = true;
    return true;
}

bool draw_annunciator(int which)
{
    (g_ded_annun_bezel) ? draw_annunciator2(which) : draw_annunciator1(which);

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

    SDL_Rect base = {0, dest.y + 1, dest.w, dest.h};
    static const Uint32 colorkey = SDL_MapSurfaceRGB(g_other_bmps[B_OVERLAY_LEDS], 0xA, 0xA, 0xA);

    // Draw the digit(s) to the overlay surface
    for (int i = 0; i < num_digits; i++)
    {
        src.x = values[i] * OVERLAY_LED_WIDTH;
        base.x = dest.x - 1;

        if (!g_legacy_overlay)
            SDL_FillSurfaceRect(g_overlay_surface, &base, colorkey);

        if (!SDL_BlitSurface(g_other_bmps[B_OVERLAY_LEDS], &src, g_overlay_surface, &dest)) {
            LOGE << fmt("Could not Blit Overlay LED's: %s", SDL_GetError());
            set_quitflag();
            return;
        }
        dest.x += OVERLAY_LED_WIDTH;
    }

    m_block_driver_overlay = true;
}

void draw_singleline_LDP1450(char *LDP1450_String, int start_x, int y)
{
    SDL_Rect src, dest;

    int i = 0;
    int value = 0;
    int LDP1450_strlen;
    m_block_driver_overlay = true;

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
         SDL_FillSurfaceRect(g_overlay_surface, &dest, 0x00000000);
         if (!SDL_BlitSurface(g_other_bmps[B_OVERLAY_LDP1450], &src, g_overlay_surface, &dest))
         {
             LOGE << fmt("Could not draw LDP1450 line: %s", SDL_GetError());
             set_quitflag();
             return;
         }
         dest.x += OVERLAY_LDP1450_CHARACTER_SPACING;
    }
}

//  used to draw non LED stuff like scoreboard text
//  'which' corresponds to enumerated values
bool draw_othergfx(int which, int x, int y)
{
    g_scoreboard_surface = g_other_bmps[which];

    if (g_scoreboard_renderer && which == B_DL_PLAYER1)
        SDL_RenderClear(g_scoreboard_renderer);

    SDL_Rect dest;
    dest.x = (short)x;
    dest.y = (short)y;
    dest.w = (unsigned short) g_scoreboard_surface->w;
    dest.h = (unsigned short) g_scoreboard_surface->h;

    SDL_BlitSurface(g_scoreboard_surface, NULL, g_scoreboard_blit_surface, &dest);

    return true;
}

static void free_one_bmp(SDL_Surface *candidate) {
	SDL_DestroySurface(candidate);
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

//////////////////////////////////////////////////////////////////////////////////////////

std::vector<SDL_Rect> get_displays() { return displayDimensions; }

SDL_Renderer *get_renderer() { return g_renderer; }
SDL_Window *get_window() { return g_window; }
SDL_Texture *get_overlay_texture() { return g_overlay_texture; }
SDL_Surface *get_screen_leds() { return g_overlay_surface; }
SDL_Rect get_scoreboard_rect() { return g_scoreboard_bezel_rect; }
SDL_Rect get_aux_rect() { return g_aux_rect; }

TTF_Font *get_font() { return g_font; }
TTF_TextEngine *get_font_engine() { return g_font_engine; }

Uint16 get_video_width() { return g_video_width; }
Uint16 get_video_height() { return g_video_height; }
Uint16 get_viewport_width() { return g_viewport_width; }
Uint16 get_viewport_height() { return g_viewport_height; }

int get_scoreboard_bezel_scale() { return g_scoreboard_bezel_scale; }
int get_aux_bezel_scale() { return g_aux_bezel_scale; }

bool get_bezelstatus() { return g_bezel_texture ? true : false; }

int get_display_no() { return g_display; }
int get_scale_h_shift() { return g_scale_h_shift; };
int get_scale_v_shift() { return g_scale_v_shift; };
SDL_TextureAccess get_textureaccess() { return g_texture_access; }
int get_scalefactor() { return g_scalefactor; }
unsigned int get_logical_width() { return g_logical_rect.w; }
unsigned int get_logical_height() { return g_logical_rect.h; }
float get_fRotateDegrees() { return g_fRotateDegrees; }

bool get_opengl() { return g_opengl; }
bool get_vulkan() { return g_vulkan; }
bool get_fullscreen() { return g_fullscreen; }
bool get_aux_bezel() { return (g_aux_texture != NULL); }
bool get_singe_blend_sprite() { return g_singe_blend_sprite; }
bool get_video_resized() { return g_video_resized; }
bool use_legacy_font() { return g_legacy_overlay; }

void set_fullscreen(bool value) { g_fullscreen = value; }
void set_software_render(bool value) { g_softrender = value; }
void set_opengl(bool value) { g_opengl = value; }
void set_vulkan(bool value) { g_vulkan = value; }
void set_teardown() { g_teardown = true; }
void set_forcetop(bool value) { g_forcetop = value; }
void set_textureaccess(SDL_TextureAccess value) { g_texture_access = value; }
void set_grabmouse(bool value) { g_grabmouse = value; }
void set_vsync(bool value) { g_vsync = value; }
void set_intro(bool value) { g_intro = value; }
void set_logo(bool value) { g_logo = value; }
void set_yuv_blue(bool value) { g_yuv_blue = value; }
void set_scanlines(bool value) { g_scanlines = value; }
void set_shunt(uint8_t value) { g_scanline_shunt = value; }
void set_alpha(uint8_t value) { g_scanline_alpha = value; }
void set_queue_screenshot(bool value) { queue_take_screenshot = value; }
void set_scale_linear(bool value) { g_scale_linear = value; }
void set_singe_blend_sprite(bool value) { g_singe_blend_sprite = value; }
void set_sboverlay_characterset(int value) { sboverlay_characterset = value; }
void set_sboverlay_white(bool value) { m_sboverlay_white = value; }
void set_legacy_overlay(bool value) { g_legacy_overlay = value; }
void set_force_aspect_ratio(bool value) { g_bForceAspect = value; }
void set_ignore_aspect_ratio(bool value) { g_bIgnoreAspect = value; }
void set_aspect_ratio(int fRatio) { g_aspect_ratio = fRatio; }
void set_detected_height(int pHeight) { g_probe_height = pHeight; }
void set_detected_width(int pWidth) { g_probe_width = pWidth; }
void set_scoreboard_bezel_scale(int value) { g_scoreboard_bezel_scale = value; }
void set_aux_bezel_scale(int value) { g_aux_bezel_scale = value; }
void set_ded_annun_bezel(bool value) { g_ded_annun_bezel = value; }
void set_annun_lamponly(bool bEnabled) { g_annun_lamps = bEnabled; }
void set_scale_h_shift(int value) { g_scale_h_shift = value; }
void set_scale_v_shift(int value) { g_scale_v_shift = value; }
void set_display_screen(int value) { g_alloc_screen = value; }
void set_scoreboard_screen(int value) { g_scoreboard_screen = value; }

static void set_subtitle_display(const char *s)
{
    free(subchar);
    subchar = strdup(s);
}

static void set_srt_display(const char *s)
{
    free(srtchar);
    srtchar = strdup(s);
}


void set_grayscale(bool value)
{
    if (value) g_yuv_flags |= YUV_FLAG_GRAYSCALE;
    else g_yuv_flags &= ~YUV_FLAG_GRAYSCALE;
}

void set_blendfilter(bool value)
{
    if (value) g_yuv_flags |= YUV_FLAG_BLEND;
    else g_yuv_flags &= ~YUV_FLAG_BLEND;
}

void set_luma(bool value, uint8_t luma)
{
    uint8_t mask = (uint8_t)-(value && (luma != YUV_FLAG_LUMA)) & YUV_FLAG_LUMA;
    g_yuv_flags = (g_yuv_flags & ~YUV_FLAG_LUMA) | mask;
    g_yuv_luma = luma - YUV_FLAG_LUMA;
}

void set_overlay_alpha(uint8_t alpha)
{
    SDL_SetTextureAlphaMod(g_overlay_texture, alpha);
}

void toggle_grabmouse()
{
    const bool grabmode = g_game->get_manymouse() ||
          (SDL_GetWindowRelativeMouseMode(g_window) == false);

    g_grabmouse ^= 1;
    const bool state = (g_grabmouse ^ !grabmode) ? true : false;

    if (grabmode)
        SDL_SetWindowMouseGrab(g_window, state);
    else {
        SDL_SetWindowRelativeMouseMode(g_window, state);
        g_grabmouse = !g_grabmouse;
    }
}

void set_yuv_flash()
{
    g_yuv_display = YUV_FLASH;
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

     if (g_window)
         SDL_SetWindowTitle(g_window, g_window_title);
}

void set_game_window(const char* value)
{
     std::string game(value);
     std::string title = "Hypseus Singe: [" + game + "]";
     strncpy(g_window_title, title.c_str(), TITLE_LENGTH - 1);

     g_window_title[TITLE_LENGTH - 1] = '\0';
     if (g_window)
         SDL_SetWindowTitle(g_window, g_window_title);
}

void set_rotate_degrees(float fDegrees)
{
     g_fullscreen = true;
     g_fRotateDegrees = fDegrees;
}

void set_scoreboard_bezel(bool bEnabled)
{
     if (bEnabled) {
         if (!g_fullscreen && g_yuv_surface)
             vid_toggle_fullscreen();
         g_fullscreen = true;
     }
     g_scoreboard_bezel = bEnabled;
}

void set_aux_bezel(bool bEnabled)
{
     if (bEnabled) {
         g_fullscreen = g_scoreboard_bezel = true;
         g_game->m_software_scoreboard = true;
         g_scale_linear = true;
     }
     g_aux_bezel = bEnabled;
}

void set_bezel_reverse(bool d)
{
     char s[] = "reverse priority";
     g_bezel_reverse = !g_bezel_reverse;
     if (d) draw_subtitle(s, 1, true);
}

void set_tq_keyboard(bool bEnabled)
{
     if (bEnabled) {
         g_scalefactor = (g_scalefactor == 100) ? 75 : g_scalefactor;
         g_fullscreen = g_scoreboard_bezel = true;
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
     char d[8]; // In-game

     if (verbose)
     {
         snprintf(d, sizeof(d), "%d\xC2\xB0", int(fDegrees));
         draw_subtitle(d, 1, true);
     }

     g_fRotateDegrees = fDegrees;
}

static SDL_FRect* copy_rect(const SDL_FRect* src)
{
     if (src == NULL) return NULL;

     SDL_FRect* dst = new SDL_FRect(*src);
     return dst;
}

static bool allocate_rect(SDL_FRect** rect)
{
     if (*rect == NULL) {
         *rect = new(std::nothrow) SDL_FRect();
         if (*rect == nullptr) {
             printerror("Failed in allocate_rect: g_yuv_frect");
             set_quitflag();
             return false;
         }
     }
     return true;
}

void reset_yuv_rect()
{
     delete g_yuv_frect[0];
     g_yuv_frect[0] = copy_rect(g_yuv_frect[1]);
}

void set_yuv_rect(float x, float y, float w, float h)
{
     if (allocate_rect(&g_yuv_frect[0]))
         *g_yuv_frect[0] = {x, y, w, h};
}

void set_yuv_scale(int v, uint8_t axis)
{
     if (allocate_rect(&g_yuv_frect[0]))
         g_yuv_scale[axis] = -0.01043f * v + 1.00043f;
}

void reset_scalefactor(int value, uint8_t bezel, bool verbose)
{
     bool rst = true;

     switch(bezel) {
     case 1: // Scoreboard
         if (!g_scoreboard_texture) { rst = false ; break; }
         g_scoreboard_bezel_scale = value;
         break;
     case 2: // Aux
         if (!g_aux_texture) { rst = false ; break; }
         g_aux_bezel_scale = value;
         calcAuxRect();
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

     draw_subtitle(s, 1, true);
}

static void calc_kb_rect()
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
     draw_subtitle(s, 1, true);
}

void reset_shiftvalue(int value, bool vert, uint8_t bezel)
{
     int h, v;
     bool rst = true;

     switch(bezel) {
     case 1: // Scoreboard
         if (!g_scoreboard_texture) { rst = false ; break; }
         if (vert) scoreboard_window_pos_y = value;
         else scoreboard_window_pos_x = value;
         h = scoreboard_window_pos_x;
         v = scoreboard_window_pos_y;
         break;
     case 2: // Aux
         if (!g_aux_texture) { rst = false ; break; }
         if (vert) g_aux_rect.y = value;
         else g_aux_rect.x = value;
         h = g_aux_rect.x;
         v = g_aux_rect.y;
         break;
     default:
         if (g_game->get_outline_border()) return;
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

     draw_subtitle(s, 1, true);
}

// position annunicator bezel
void set_aux_bezel_position(int xValue, int yValue)
{
    aux_bezel_pos_x = xValue;
    aux_bezel_pos_y = yValue;
}

// position scoreboard_scoreboard
void set_scoreboard_window_position(int xValue, int yValue)
{
    scoreboard_window_pos_x = xValue;
    scoreboard_window_pos_y = yValue;
}

// deal with MPEG headers aspect ratio
void set_aspect_change(int aspectWidth, int aspectHeight)
{
    g_aspect_width  = aspectWidth;
    g_aspect_height = aspectHeight;
}

// sets g_video_width
void set_video_width(Uint16 width)
{
    // Let the user specify whatever width s/he wants (and suffer the
    // consequences)
    // We need to support arbitrary resolution to accomodate stuff like screen
    // rotation
    g_video_width = width;
    g_video_resized = true;
}

// sets g_video_height
void set_video_height(Uint16 height)
{
    // Let the user specify whatever height s/he wants (and suffer the
    // consequences)
    // We need to support arbitrary resolution to accomodate stuff like screen
    // rotation
    g_video_height = height;
    g_video_resized = true;
}

void draw_string(const char *t, int col, int row, SDL_Surface *overlay,
                     SDL_Color rgb, bool outline)
{
    SDL_Rect dest;
    dest.y = (short)(row);
    dest.x = (short)(col);
    dest.w = (unsigned short)(6 * strlen(t));
    dest.h = 14;

    SDL_Surface *text_surface = TTF_RenderText_Solid(g_ttfont, t, strlen(t), rgb);

    if (!text_surface) {
        LOGE << fmt("Could not draw_string %s", SDL_GetError());
        set_quitflag();
        return;
    }

    if (outline)
    {
        static const SDL_Color padding = {0x3D, 0x38, 0x36, 0xFF};
        static const int offsets[4][2] = {{-1, 0}, {1, 0}, {0,-1}, {0, 1}};

        SDL_Surface *outline = TTF_RenderText_Solid(g_ttfont, t, strlen(t), padding);
        SDL_Rect r = dest;

        for (int i = 0; i < 4; ++i) {
            r.x = dest.x + offsets[i][0];
            r.y = dest.y + offsets[i][1];
            SDL_BlitSurface(outline, NULL, overlay, &r);
        }

        SDL_DestroySurface(outline);
    }

    SDL_BlitSurface(text_surface, NULL, overlay, &dest);
    SDL_DestroySurface(text_surface);
}

void draw_srt(const char *s, uint8_t func, int pos = -1)
{
    if (!s) return;

    static int message_timer = 0;
    static float base_y = 90.0f;

    const int timeout = 30;
    const int pad = 6;

    int x = (int)(g_scaling_rect.x + g_scaling_rect.w * 0.5f);

    if (pos > 0)
        base_y = g_scaling_rect.y + g_scaling_rect.h * (pos / 100.0f);

    if (func & 1)
    {
        message_timer = 0;
        set_srt_display(s);
        g_bSubtitleShown |= (1 << 1);
    }
    else if (func & 2 || message_timer > timeout)
    {
        g_bSubtitleShown = 0;
        return;
    }

    char *copy = SDL_strdup(s);
    char *saveptr = NULL;

    int max_w = 0;
    int line_count = 0;

    for (char *line = strtok_r(copy, "\n", &saveptr);
         line;
         line = strtok_r(NULL, "\n", &saveptr))
    {
        int w = 0, h = 0;
        TTF_GetStringSize(g_font, line, 0, &w, &h);

        if (w > max_w)
            max_w = w;

        line_count++;
    }

    int line_skip = TTF_GetFontLineSkip(g_font);
    int total_h = line_count * line_skip;

    SDL_free(copy);

    SDL_FRect bg;
    bg.x = x - (max_w / 2.0f) - (pad * 3.0f);
    bg.y = base_y - (pad / 2.0f);
    bg.w = max_w + (pad * 6.0f);
    bg.h = total_h + pad;

    SDL_SetRenderDrawColor(g_renderer, 0x14, 0x14, 0x14, 0xff);
    SDL_RenderFillRect(g_renderer, &bg);

    copy = SDL_strdup(s);
    saveptr = NULL;

    float y = base_y;

    for (char *line = strtok_r(copy, "\n", &saveptr);
         line;
         line = strtok_r(NULL, "\n", &saveptr))
    {
        TTF_Text *text = TTF_CreateText(g_font_engine, g_font, line, 0);

        int w = 0, h = 0;
        TTF_GetStringSize(g_font, line, 0, &w, &h);

        float lx = x - w / 2.0f;

        TTF_DrawRendererText(text, lx, y);

        TTF_DestroyText(text);

        y += line_skip;
    }

    SDL_free(copy);

    message_timer++;
}

void draw_subtitle(const char *s, uint8_t func, bool center = false)
{
    if (!s) return;

    SDL_FPoint p;
    p.y = (g_scaling_rect.h * 0.92f) + g_scaling_rect.y;

    static int m_message_timer;
    static bool align = false;
    const int timeout = 200;

    if (func & 1)
    {
        align = center;
        m_message_timer = 0;
        g_bSubtitleShown |= (1 << 0);
        set_subtitle_display(s);
    }
    else if (m_message_timer > timeout)
    {
        g_bSubtitleShown = 0;
        return;
    }

    const int pad_x = (int)(g_scaling_rect.w * 0.03f);

    int w = 0, h = 0;
    TTF_GetStringSize(g_font, s, 0, &w, &h);

    if (align)
    {
        p.x = (float)((g_scaling_rect.w / 2) + g_scaling_rect.x - (w / 2));
    }
    else
    {
        p.x = (float)(g_scaling_rect.x + pad_x);
    }

    TTF_Text *text = TTF_CreateText(g_font_engine, g_font, s, strlen(s));

    TTF_DrawRendererText(text, p.x, p.y);
    TTF_DestroyText(text);

    m_message_timer++;
}

void vid_toggle_fullscreen()
{
    if (g_toggle_lock) {
        draw_srt("Fullscreen Toggle Locked", 1);
        return;
    }

    g_bezel_toggle = false;

    Uint32 current = SDL_GetWindowFlags(g_window);
    bool flip = current & SDL_WINDOW_FULLSCREEN;

    if (!SDL_SetWindowFullscreen(g_window, flip ? false : SDL_WINDOW_FULLSCREEN)) {
        LOGW << fmt("Toggle fullscreen failed: %s", SDL_GetError());
        return;
    }

    current = SDL_GetWindowFlags(g_window);
    Uint32 flags = (current ^ SDL_WINDOW_FULLSCREEN);

    if ((flags & SDL_WINDOW_FULLSCREEN)) {
        g_logical_rect = displayDimensions[g_display];
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
    SDL_DisplayID id = SDL_GetDisplayForWindow(g_window);
    SDL_GetDisplayBounds(id, &dimensions);

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

    draw_subtitle(s, 1, true);
}

void vid_toggle_bezel()
{
    if (g_fullscreen) {
        g_bezel_toggle = g_bezel_texture || g_aux_texture || g_scoreboard_bezel ?
                 !g_bezel_toggle : g_bezel_toggle;

        char s[] = "bezel toggle";
        draw_subtitle(s, 1, true);
    }
}

void vid_scoreboard_switch()
{
    if (!g_scoreboard_window) return;

    g_scoreboard_screen = max(0, min(g_scoreboard_screen, g_displays - 1));

    char s[16] = "screen: 0";

    if (g_displays > 1)
    {
        snprintf(s, sizeof(s), "screen: %d", (unsigned char)g_scoreboard_screen);

        SDL_SetWindowPosition(g_scoreboard_window,
           displayDimensions[g_scoreboard_screen].x + scoreboard_window_pos_x,
              displayDimensions[g_scoreboard_screen].y + scoreboard_window_pos_y);

        if (++g_scoreboard_screen == g_displays) g_scoreboard_screen = 0;

    } else
        SDL_SetWindowPosition(g_scoreboard_window, scoreboard_window_pos_x,
                                 scoreboard_window_pos_y);

    SDL_RaiseWindow(g_scoreboard_window);
    draw_subtitle(s, 1, true);
}

void vid_setup_yuv_overlay (int width, int height)
{
    // Prepare the YUV overlay, wich means setting up both the YUV surface and YUV texture.

    // If we have already been here, free things first.
    if (g_yuv_surface) {
        // Free both the YUV surface and YUV texture.
        vid_free_yuv_overlay();
    }

    g_yuv_surface = (m_yuv_surface_t*) malloc (sizeof(m_yuv_surface_t));

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

    if (g_yuv_frect[0]) {
        g_yuv_frect[0]->w = (float)(width * g_yuv_scale[YUV_H]);
        g_yuv_frect[0]->h = (float)(height * g_yuv_scale[YUV_V]);
        g_yuv_frect[0]->x = (float)((width / 2) - (g_yuv_frect[0]->w / 2));
        g_yuv_frect[0]->y = (float)((height / 2) - (g_yuv_frect[0]->h / 2));
        g_yuv_frect[1] = copy_rect(g_yuv_frect[0]);
    }
}

static void vid_flash_yuv_surface()
{
    // White: YUV#ED8080 - D4 = 90%
    memset(g_yuv_surface->Yplane, 0xd4, g_yuv_surface->Ysize);
    memset(g_yuv_surface->Uplane, 0x80, g_yuv_surface->Usize);
    memset(g_yuv_surface->Vplane, 0x80, g_yuv_surface->Vsize);
}

static void vid_blank_yuv_surface()
{
    // Blue: YUV#1DEB6B - Black: YUV#108080
    uint8_t Y_value = g_yuv_blue ? 0x1d : 0x10;
    uint8_t U_value = g_yuv_blue ? 0xeb : 0x80;
    uint8_t V_value = g_yuv_blue ? 0x6b : 0x80;

    memset(g_yuv_surface->Yplane, Y_value, g_yuv_surface->Ysize);
    memset(g_yuv_surface->Uplane, U_value, g_yuv_surface->Usize);
    memset(g_yuv_surface->Vplane, V_value, g_yuv_surface->Vsize);
}

static void vid_blank_yuv_texture()
 {
    if (!g_yuv_texture) return;

    float w, h;
    SDL_GetTextureSize(g_yuv_texture, &w, &h);

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

static inline void blendPlane(const uint8_t *src, uint8_t *dst, int w, int h,
	int srcPitch, int dstPitch)
{
    for (int y = 0; y < h; ++y) {
        const uint8_t *line0 = src + ((y > 0 ? y - 1 : y) * srcPitch);
        const uint8_t *line1 = src + ( y * srcPitch);
        const uint8_t *line2 = src + ((y < h - 1 ? y + 1 : y) * srcPitch);
        uint8_t *row = dst + y * dstPitch;

        for (int x = 0; x < w; ++x)
            row[x] = (uint8_t)(((uint32_t)
		    line0[x] + line1[x] + line2[x] + 1U) * 341U >> 10);
    }
}

static inline void lumaControl(const uint8_t *src, uint8_t *dst, int width, int height,
        int srcPitch, int dstPitch)
{
    for (int y = 0; y < height; y++) {
        const uint8_t *s = src + y * srcPitch;
        uint8_t *d = dst + y * dstPitch;

        for (int x = 0; x < width; x++) {
            int yv = s[x];
            int val = yv + ((yv * g_yuv_luma) >> 3);

            if (val < 0) val = 0;
            else if (val > 255) val = 255;

            d[x] = (uint8_t)val;
        }
    }
}

static SDL_Texture *vid_create_yuv_texture(int width, int height)
{
    g_yuv_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_YV12,
        g_texture_access, width, height);

    g_yuv_skip = true;

    if (!g_yuv_texture) {
        LOGE << fmt("Could not initialize g_yuv_texture %s", SDL_GetError());
        set_quitflag();
    }
    else if (g_yuv_flags & YUV_FLAG_BLEND)
            SDL_SetTextureScaleMode(g_yuv_texture, SDL_SCALEMODE_LINEAR);
    else
            SDL_SetTextureScaleMode(g_yuv_texture, SDL_SCALEMODE_NEAREST);

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
    case YUV_FLASH:
        vid_flash_yuv_surface();
        g_yuv_display = YUV_VISIBLE;
        break;
    default:
        if (!g_yuv_flags) {
            copyPlane(g_yuv_surface->Yplane, Yplane,
                g_yuv_surface->width, g_yuv_surface->height, Ypitch);
            copyPlane(g_yuv_surface->Uplane, Uplane,
                g_yuv_surface->width / 2, g_yuv_surface->height / 2, Upitch);
            copyPlane(g_yuv_surface->Vplane, Vplane,
                g_yuv_surface->width / 2, g_yuv_surface->height / 2, Vpitch);
            break;
        }

        if (g_yuv_flags & YUV_FLAG_BLEND) {
            blendPlane(Yplane, g_yuv_surface->Yplane, g_yuv_surface->width,
                g_yuv_surface->height, Ypitch, g_yuv_surface->Ypitch);

        } else {
            copyPlane(g_yuv_surface->Yplane, Yplane,
                g_yuv_surface->width, g_yuv_surface->height, Ypitch);
        }

        if (g_yuv_flags & YUV_FLAG_LUMA) {
            lumaControl(Yplane, g_yuv_surface->Yplane, g_yuv_surface->width,
                g_yuv_surface->height, Ypitch, g_yuv_surface->Ypitch);
        }

        if (g_yuv_flags & YUV_FLAG_GRAYSCALE) {
            memset(g_yuv_surface->Uplane, 0x80, g_yuv_surface->Usize);
            memset(g_yuv_surface->Vplane, 0x80, g_yuv_surface->Vsize);

        } else {
            int w = g_yuv_surface->width  >> 1;
            int h = g_yuv_surface->height >> 1;
            blendPlane(Uplane, g_yuv_surface->Uplane, w, h, Upitch, g_yuv_surface->Upitch);
            blendPlane(Vplane, g_yuv_surface->Vplane, w, h, Vpitch, g_yuv_surface->Vpitch);
        }
        break;
    }

    g_yuv_surface->Ypitch = g_yuv_surface->width;
    g_yuv_surface->Upitch = g_yuv_surface->width / 2;
    g_yuv_surface->Vpitch = g_yuv_surface->width / 2;

    g_yuv_video_needs_update = true;

    SDL_UnlockMutex(g_yuv_surface->mutex);

    return 0;
}

void vid_update_overlay_surface(SDL_Surface *overlay)
{
    if (m_block_driver_overlay)
        return;

    SDL_Rect src = (SDL_Rect){0, 0, overlay->w, overlay->h};
    SDL_Rect* dst = NULL;

    if (g_enhance_overlay) {
        g_limit_rect.w = overlay->w - (g_limit_rect.x << 1);
        g_limit_rect.h = overlay->h - (g_limit_rect.y << 1);
        dst = &g_limit_rect;
    }

    if (g_overlay_dynamic)
        g_limit_rect = src;

    if (m_isIndex8) { // else texture was updated in driver
        SDL_BlitSurface(overlay, NULL, g_overlay_surface, dst);
        SDL_UpdateTexture(g_overlay_texture, &src,
             (void *)g_overlay_surface->pixels, g_overlay_surface->pitch);
    }
}

static void take_screenshot()
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
    else SDL_GetCurrentRenderOutputSize(g_renderer, &g_logical_rect.w, &g_logical_rect.h);

    screenshot = SDL_RenderReadPixels(g_renderer, &g_logical_rect);

    if (!screenshot)
    {
        LOGE << fmt("Cannot ReadPixels - Something bad happened: %s", SDL_GetError());
        set_quitflag();
    }

    for (;;) {
        screenshot_num++;
        snprintf(filename, sizeof(filename), "%s%shypseus-%d.png",
          dir, PATH_SEPARATOR, screenshot_num);

        if (!mpo_file_exists(filename))
            break;
    }

    if (IMG_SavePNG(screenshot, filename)) {
        LOGI << fmt("Wrote screenshot: %s", filename);
    } else {
        LOGE <<  fmt("Could not write screenshot: %s !! - %s", filename, SDL_GetError());
    }

    SDL_DestroySurface(screenshot);
}

static void draw_scanlines(int l)
{
    int xe = g_scaling_rect.x + g_scaling_rect.w;
    int ye = g_scaling_rect.y + g_scaling_rect.h;

    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, g_scanline_alpha);

    if (g_aspect_ratio == ASPECTPD) {
        for (int i = 0; i < g_scaling_rect.w; i += l) {
            int x = (int)(g_scaling_rect.x + i);
            SDL_RenderLine(g_renderer, x, g_scaling_rect.y, x, ye);
        }
    } else {
        for (int i = 0; i < g_scaling_rect.h; i += l) {
            int y = (int)(g_scaling_rect.y + i);
            SDL_RenderLine(g_renderer, g_scaling_rect.x, y, xe, y);
        }
    }

    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
}

static void draw_border(int s, int c)
{
    SDL_FRect tb, lb, rb, bb;
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

static void vid_render_aux()
{
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

            SDL_FPoint size;
            SDL_GetTextureSize(g_aux_texture, &size.x, &size.y);

            g_aux_ratio = size.y / size.x;

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
            g_aux_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGBA32,
                                g_texture_access, g_anun_w, g_anun_h);

            if (!g_aux_texture) {
                LOGE << fmt("Failed on annunciator texture: %s", SDL_GetError());
                set_quitflag();
            } else
                SDL_SetTextureBlendMode(g_aux_texture, SDL_BLENDMODE_BLEND);
        }
    }
}

static void vid_render_texture(SDL_Texture *texture, SDL_Rect rect)
{
    if (texture) {
        SDL_FRect frect;
        SDL_RectToFRect(&rect, &frect);
        SDL_RenderTexture(g_renderer, texture, NULL, &frect);
    }
}

static void vid_render_bezels()
{
    if (!g_bezel_load) {
        if (!g_bezel_file.empty() && !g_bezel_texture) {
            std::string bezelpath =  g_bezel_path + std::string(PATH_SEPARATOR) + g_bezel_file;
            g_bezel_texture = IMG_LoadTexture(g_renderer, bezelpath.c_str());

            if (g_bezel_texture) {
                LOGI << fmt("Loaded bezel file: %s", bezelpath.c_str());
                SDL_SetTextureScaleMode(g_bezel_texture, g_scale_linear ?
                              SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
            } else {
                LOGW << fmt("Failed to load bezel: %s", bezelpath.c_str());
            }
        }
        g_bezel_load = true;
    }

    if (g_bezel_reverse)
    {
        vid_render_texture(g_scoreboard_texture, g_scoreboard_bezel_rect);
        vid_render_texture(g_aux_texture, g_aux_rect);
        vid_render_texture(g_bezel_texture, SDL_Rect{0, 0, g_logical_rect.w, g_logical_rect.h});
    }
    else
    {
        vid_render_texture(g_bezel_texture, SDL_Rect{0, 0, g_logical_rect.w, g_logical_rect.h});
        vid_render_texture(g_aux_texture, g_aux_rect);
        vid_render_texture(g_scoreboard_texture, g_scoreboard_bezel_rect);
    }
}

void vid_blit()
{
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

    // Clear the renderer before the SDL_RenderTexture() calls for this frame.
    // Prevents stroboscopic effects on the background in fullscreen mode,
    // and is recommended by SDL_Rendercopy() documentation.

    SDL_SetRenderTarget(g_renderer, g_mix_texture);
    SDL_RenderClear(g_renderer);

    // Does YUV texture need update from the YUV surface
    if (g_yuv_surface) {

        SDL_LockMutex(g_yuv_surface->mutex);

        if (g_yuv_video_needs_update)
        {
            if (!g_yuv_texture) {
            g_yuv_texture = vid_create_yuv_texture(
                g_yuv_surface->width, g_yuv_surface->height);
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
                g_yuv_surface->Uplane, g_yuv_surface->Upitch,
                g_yuv_surface->Vplane, g_yuv_surface->Vpitch);
        }

        SDL_UnlockMutex(g_yuv_surface->mutex);
    }

    // Does OVERLAY texture need update from the local surfaces
    if (m_block_driver_overlay) {
        SDL_UpdateTexture(g_overlay_texture, &g_local_size_rect,
	    (void *)g_overlay_surface->pixels, g_overlay_surface->pitch);
    }

    SDL_RectToFRect(&g_scaling_rect, &fScaleRect);

    if (g_yuv_texture)
        SDL_RenderTexture(g_renderer, g_yuv_texture, g_yuv_frect[0], &fScaleRect);

    if (g_overlay_texture) {
        SDL_FRect frect;
        SDL_RectToFRect(&g_limit_rect, &frect);
        SDL_RenderTexture(g_renderer, g_overlay_texture, &frect, &fScaleRect);
    }

    if (g_aux_needs_update) vid_render_aux();

    if (g_scanlines)
        draw_scanlines(g_scanline_shunt);

    // If there's a subtitle overlay
    if (g_bSubtitleShown)
    {
        if (g_bSubtitleShown & (1 << 0 ))
            draw_subtitle(subchar, 0);
        else draw_srt(srtchar, 0);
    }

    if (g_game->get_outline_border())
        draw_border(g_game->get_outline_border(),
            g_game->get_outline_border_color());

    SDL_SetRenderTarget(g_renderer, NULL);
    SDL_RenderClear(g_renderer);

    if (g_fRotateDegrees != 0)
        SDL_RenderTextureRotated(g_renderer, g_mix_texture,  &fScaleRect,
             &fScaleRect, g_fRotateDegrees, NULL, SDL_FLIP_NONE);
    else
        SDL_RenderTexture(g_renderer, g_mix_texture, &fScaleRect,  &fScaleRect);

    if (g_bezel_toggle) vid_render_bezels();

    if (queue_take_screenshot) {
        queue_take_screenshot = false;
        take_screenshot();
    }

    SDL_RenderPresent(g_renderer);

    if (g_softsboard_needs_update) {
        SDL_RenderPresent(g_scoreboard_renderer);
        g_softsboard_needs_update = false;
    }

    g_rescale = 0;
}

int get_yuv_overlay_width()
{
    if (g_yuv_surface) return g_yuv_surface->width;
    return 0;
}

int get_yuv_overlay_height()
{
    if (g_yuv_surface) return g_yuv_surface->height;
    return 0;
}

bool get_yuv_overlay_ready()
{
    if (g_yuv_surface && g_yuv_texture) return true;
    return false;
}

void set_overlay_offset(int offsetx, int offsety)
{
    g_limit_rect.x = offsetx;
    g_limit_rect.y = offsety;
}

void notify_stats(int overlaywidth, int overlayheight, const char* input)
{
    char s[4] = {0};
    if (!g_overlay_dynamic) snprintf(s, sizeof(s), "[s]");

    LOGI << fmt("Viewport Stats:|w:%dx%d|v:%dx%d|o:%dx%d%s|l:%dx%d|%s",
         g_viewport_width, g_viewport_height, g_probe_width,
           g_probe_height, overlaywidth, overlayheight, s,
             g_logical_rect.w, g_logical_rect.h, input);
}


void notify_positions()
{
    char s[] = "positions logged";

    LOGI << fmt("Scale video (0): %d", g_scalefactor);
    LOGI << fmt("Shift video (0): x:%d, y:%d", (g_scale_h_shift - 0x64),
                         (g_scale_v_shift - 0x64));

    if (g_scoreboard_texture) {
        LOGI << fmt("Scale score (1): %d", g_scoreboard_bezel_scale);
        LOGI << fmt("Shift score (1): x:%d, y:%d", scoreboard_window_pos_x, scoreboard_window_pos_y);
    }

    if (g_aux_texture) {
        LOGI << fmt("Scale aux (2): %d", g_aux_bezel_scale);
        LOGI << fmt("Shift aux (2): x:%d, y:%d", g_aux_rect.x, g_aux_rect.y);
    }

    LOGI << fmt("Rotation: %d\xC2\xB0", (int)g_fRotateDegrees);

    draw_subtitle(s, 1, true);
}

}
