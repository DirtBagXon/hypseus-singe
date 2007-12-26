/*
 * palette.cpp
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

#include "../game/game.h"
#include "../io/conout.h" // for printline
#include "palette.h"
#include "rgb2yuv.h"
#include "../video/video.h"
#include "../ldp-out/ldp-vldp-gl.h"	// for palette changing

#ifdef DEBUG
#include <assert.h>
#endif

unsigned int g_palette_size = 0;
SDL_Color *g_rgb_palette = NULL;

// color palette lookup table (to make OpenGL texture conversions faster)
Uint32 g_uRGBAPalette[256];

t_yuv_color *g_yuv_palette = NULL;

bool g_palette_modified = true;

// call this function once to set size of game palette
bool palette_initialize (unsigned int num_colors)
{
	bool result = true;

	g_palette_size = num_colors;
	// we can only have 256 max since all our surfaces are 8-bit
	if (num_colors > 256)
	{
		printline("palette_initialize error: Too many colors > 256!");
		result = false;
	}

	if (result)
	{
		g_rgb_palette = new SDL_Color[num_colors];
		g_yuv_palette = new t_yuv_color[num_colors];
	}

	if (!((g_rgb_palette) && (g_yuv_palette)
		))
	{
		printline("palette_initialize error: Could not allocate palette arrays!");
		palette_shutdown();
		result = false;
	}
	else
	{
		// set all colors to unmodified and black
		for (unsigned int x = 0; x < g_palette_size; x++)
		{
			// set RGB values to black
			g_rgb_palette[x].r = g_rgb_palette[x].g = g_rgb_palette[x].b = 0;

			g_uRGBAPalette[x] = 0xFF000000;	// initialize to opaque black

			// set YUV values to black
			g_yuv_palette[x].y = 0;
			g_yuv_palette[x].u = g_yuv_palette[x].v = 0x7F;
			g_yuv_palette[x].transparent = false;
		}

		// Default color #0 to be transparent
		// Game drivers can change this individually.
		palette_set_transparency(0, true);
	}

	return result;
}

void palette_set_transparency(unsigned int uColorIndex, bool transparent)
{
#ifdef DEBUG
	// sanity check
	assert(uColorIndex < g_palette_size);
#endif

	g_yuv_palette[uColorIndex].transparent = transparent;

	if (transparent)
	{
		g_uRGBAPalette[uColorIndex] &= 0x00FFFFFF;	// set alpha channel to 0
	}
	else
	{
		g_uRGBAPalette[uColorIndex] |= 0xFF000000;	// set alpha channel to FF
	}

#ifdef USE_OPENGL
	// TODO : change this to 'g_palette_modified = true' once we're in a better position to test.
	// if we're using OpenGL, we need to update the palette texture
	if (get_use_opengl())
	{
		on_palette_change_gl();
	}
#endif

}

// call this function when a color has changed
void palette_set_color (unsigned int color_num, SDL_Color color_value)
{

#ifdef DEBUG
	assert (color_num < g_palette_size);
#endif

	// make sure the color has really been modified because the RGB2YUV calculations are expensive
	if ((g_rgb_palette[color_num].r != color_value.r) || (g_rgb_palette[color_num].g != color_value.g) || (g_rgb_palette[color_num].b != color_value.b))
	{
		g_rgb_palette[color_num] = color_value;
		g_palette_modified = true;	

		// change R,G,B, values, but don't change A
		g_uRGBAPalette[color_num] = (g_uRGBAPalette[color_num] & 0xFF000000) |
			color_value.r | (color_value.g << 8) |
			(color_value.b << 16);

		// MATT : seems to make more sense to calculate the YUV value of the color here
		rgb2yuv_input[0] = g_rgb_palette[color_num].r;
		rgb2yuv_input[1] = g_rgb_palette[color_num].g;
		rgb2yuv_input[2] = g_rgb_palette[color_num].b;
		rgb2yuv();
		g_yuv_palette[color_num].y = rgb2yuv_result_y;
		g_yuv_palette[color_num].v = rgb2yuv_result_v;
		g_yuv_palette[color_num].u = rgb2yuv_result_u;
	}

}

// call this function right before drawing the current overlay
void palette_finalize()
{
	if (g_palette_modified)
	{
		// update color palette for all the surfaces that we are using
		for (int i = 0;; i++)
		{
			SDL_Surface *video_overlay = g_game->get_video_overlay(i);

			// if we have a video overlay to set the colors no ...
			if (video_overlay)
			{
				SDL_SetColors(video_overlay, g_rgb_palette, 0, g_palette_size);
			}
			else
			{
				break;
			}
		}

		if (g_game->IsFullScaleEnabled())
		{
			SDL_SetColors(g_game->get_scaled_video_overlay(), g_rgb_palette, 0, g_palette_size);
		}

#ifdef USE_OPENGL
		// if we're using OpenGL, we need to update the palette texture
		if (get_use_opengl())
		{
			on_palette_change_gl();
		}
#endif

	}

	g_palette_modified = false;
}

// call this function on video shutdown
void palette_shutdown (void)
{
	if (g_rgb_palette)
	{
		delete [] g_rgb_palette;
		g_rgb_palette = NULL;
	}
	if (g_yuv_palette)
	{
		delete [] g_yuv_palette;
		g_yuv_palette = NULL;
	}

}

// this function is here temporarily while the game drivers are switch to this new method
t_yuv_color *get_yuv_palette(void)
{
	return g_yuv_palette;
}

Uint32 *get_rgba_palette(void)
{
	return g_uRGBAPalette;
}
