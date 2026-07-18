/*
 * ____ HYPSEUS COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2002 Mark Broadhead, 2026 DirtBagXon
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
#include <plog/Log.h>

#ifdef DEBUG
#include <assert.h>
#endif

namespace palette
{

unsigned int g_size = 0;
bool g_modified     = true;

std::vector<SDL_Color> g_rgb;

// call this function once to set size of game palette
bool initialize(unsigned int num_colors)
{
    if (num_colors > 256) {
        LOGE << fmt("Too many colors %d > 256!", num_colors);
        return false;
    }

    g_size = num_colors;

    // Allocate and initialise the CPU-side palette
    g_rgb.assign(num_colors, SDL_Color{0, 0, 0, 0xFF});

    // Create and attach SDL palettes to every overlay surface.
    for (int i = 0;; ++i) {
        SDL_Surface *surface = g_game->get_video_overlay(i);
        if (!surface)
            break;

        SDL_Palette *palette = SDL_CreatePalette(g_size);
        if (!palette) {
            LOGE << "SDL_CreatePalette failed";
            shutdown();
            return false;
        }

        SDL_SetPaletteColors(palette, g_rgb.data(), 0, g_size);
        SDL_SetSurfacePalette(surface, palette);
        SDL_DestroyPalette(palette);
    }

    // Default colour 0 is transparent.
    set_transparency(0, true);

    return true;
}

void set_transparency(unsigned int uColorIndex, bool transparent)
{
#ifdef DEBUG
    // sanity check
    assert(uColorIndex < g_size);
#endif

    if (transparent)
    {
        g_rgb[uColorIndex].a = 0x00;
    }
    else
    {
        g_rgb[uColorIndex].a = 0xFF;
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
        g_rgb[color_num].r = color_value.r;
        g_rgb[color_num].g = color_value.g;
        g_rgb[color_num].b = color_value.b;
        g_modified       = true;
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
                SDL_Palette *palette = SDL_GetSurfacePalette(video_overlay);
                SDL_SetPaletteColors(palette, g_rgb.data(), 0, g_size);
            } else {
                break;
            }
        }
    }
    g_modified = false;
}

// call this function on video shutdown
void shutdown(void)
{
    g_rgb.clear();
}

bool get_yuv_overlay_ready()
{
    return video::get_yuv_overlay_ready();
}

void set_yuv_transparency(int state)
{
    if (video::get_yuv_overlay_ready())
        video::set_yuv_blank(state);
}

}
