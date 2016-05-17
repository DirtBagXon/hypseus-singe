/*
 * ____ DAPHNE COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2002 Mark Broadhead
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

#include "../game/game.h"
#include "../io/conout.h" // for printline
#include "../video/video.h"
#include "palette.h"
#include "rgb2yuv.h"
#include <plog/Log.h>

#ifdef DEBUG
#include <assert.h>
#endif

namespace palette
{
unsigned int g_size = 0;
SDL_Color *g_rgb    = NULL;

// color palette lookup table (to make OpenGL texture conversions faster)
Uint32 g_uRGBAPalette[256];

t_yuv_color *g_yuv = NULL;

bool g_modified = true;

// call this function once to set size of game palette
bool initialize(unsigned int num_colors)
{
    bool result = true;

    g_size = num_colors;
    // we can only have 256 max since all our surfaces are 8-bit
    if (num_colors > 256) {
        LOGE << fmt("Too many colors %d > 256!", num_colors);
        result = false;
    }

    if (result) {
        g_rgb = new SDL_Color[num_colors];
        g_yuv = new t_yuv_color[num_colors];
    }

    if (!((g_rgb) && (g_yuv))) {
        LOGE << "Could not allocate palette arrays!";
        shutdown();
        result = false;
    } else {
        // set all colors to unmodified and black
        for (unsigned int x = 0; x < g_size; x++) {
            // set RGB values to black
            g_rgb[x].r = g_rgb[x].g = g_rgb[x].b = 0;

            g_uRGBAPalette[x] = 0xFF000000; // initialize to opaque black

            // set YUV values to black
            g_yuv[x].y = 0;
            g_yuv[x].u = g_yuv[x].v = 0x7F;
            g_yuv[x].transparent            = false;
        }

        // Default color #0 to be transparent
        // Game drivers can change this individually.
        set_transparency(0, true);
    }

    return result;
}

void set_transparency(unsigned int uColorIndex, bool transparent)
{
#ifdef DEBUG
    // sanity check
    assert(uColorIndex < g_size);
#endif

    g_yuv[uColorIndex].transparent = transparent;

    if (transparent) {
        g_uRGBAPalette[uColorIndex] &= 0x00FFFFFF; // set alpha channel to 0
    } else {
        g_uRGBAPalette[uColorIndex] |= 0xFF000000; // set alpha channel to FF
    }
}

// call this function when a color has changed
void set_color(unsigned int color_num, SDL_Color color_value)
{

#ifdef DEBUG
    assert(color_num < g_size);
#endif

    // make sure the color has really been modified because the RGB2YUV
    // calculations are expensive
    if ((g_rgb[color_num].r != color_value.r) ||
        (g_rgb[color_num].g != color_value.g) ||
        (g_rgb[color_num].b != color_value.b)) {
        g_rgb[color_num] = color_value;
        g_modified       = true;

        // change R,G,B, values, but don't change A
        g_uRGBAPalette[color_num] = (g_uRGBAPalette[color_num] & 0xFF000000) |
                                    color_value.r | (color_value.g << 8) |
                                    (color_value.b << 16);

        // MATT : seems to make more sense to calculate the YUV value of the
        // color here
        rgb2yuv_input[0] = g_rgb[color_num].r;
        rgb2yuv_input[1] = g_rgb[color_num].g;
        rgb2yuv_input[2] = g_rgb[color_num].b;
        rgb2yuv();
        g_yuv[color_num].y = rgb2yuv_result_y;
        g_yuv[color_num].v = rgb2yuv_result_v;
        g_yuv[color_num].u = rgb2yuv_result_u;
    }
}

// call this function right before drawing the current overlay
void finalize()
{
    if (g_modified) {
        // update color palette for all the surfaces that we are using
        for (int i = 0;; i++) {
            SDL_Surface *video_overlay = g_game->get_video_overlay(i);

            // if we have a video overlay to set the colors no ...
            if (video_overlay) {
                SDL_SetPaletteColors(video_overlay->format->palette,
                                     g_rgb, 0, g_size);
            } else {
                break;
            }
        }

        if (g_game->IsFullScaleEnabled()) {
            SDL_SetPaletteColors(g_game->get_scaled_video_overlay()->format->palette,
                                 g_rgb, 0, g_size);
        }
    }

    g_modified = false;
}

// call this function on video shutdown
void shutdown(void)
{
    if (g_rgb) {
        delete[] g_rgb;
        g_rgb = NULL;
    }
    if (g_yuv) {
        delete[] g_yuv;
        g_yuv = NULL;
    }
}

// this function is here temporarily while the game drivers are switch to this
// new method
t_yuv_color *get_yuv(void) { return g_yuv; }

Uint32 *get_rgba(void) { return g_uRGBAPalette; }
}
