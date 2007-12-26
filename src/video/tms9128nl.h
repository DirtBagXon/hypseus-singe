/*
 * tms9128nl.h
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


#ifndef TMS9128NL_H
#define TMS9128NL_H

#include <SDL.h>	// to declare SDL_Color

#define TMS9128NL_OVERLAY_W 320	/* width of overlay */
#define TMS9128NL_OVERLAY_H 240	/* height of the overlay */

#define TMS_VERTICAL_OFFSET 24	/* how many lines down to begin drawing video */

#define TMS_BG_COLOR 0
#define TMS_TRANSPARENT_COLOR 0x7F
#define TMS_FG_COLOR 0xFF

#define TMS_COLOR_COUNT 256

// mixes two colors together 0.75 and 0.25 weights, returning their mixture in 'result'
#define MIX_COLORS_75_25(result,src1,src2) \
	result.r = (Uint8) (((src1.r * 3) + src2.r) >> 2);	\
	result.g = (Uint8) (((src1.g * 3) + src2.g) >> 2);	\
	result.b = (Uint8) (((src1.b * 3) + src2.b) >> 2)

// mix two colors evenly (50% blend)
#define MIX_COLORS_50(result,src1,src2) \
	result.r = (Uint8) ((src1.r + src2.r) >> 1);	\
	result.g = (Uint8) ((src1.g + src2.g) >> 1);	\
	result.b = (Uint8) ((src1.b + src2.b) >> 1)

	
void tms9128nl_reset();
bool tms9128nl_int_enabled();
void tms9128nl_writechar(unsigned char);
unsigned char tms9128nl_getvidmem();
void tms9128nl_write_port1(unsigned char);
void tms9128nl_write_port0(unsigned char Value);
int tms9128nl_setvidmem(unsigned char);
void tms9128nl_convert_color(unsigned char, SDL_Color *);
void tms9128nl_drawchar(unsigned char, int, int);
void tms9128nl_outcommand(char *s,int col,int row);
void tms9128nl_palette_update();
void tms9128nl_palette_calculate();
void tms9128nl_video_repaint();
void tms9128nl_video_repaint_stretched();
void tms9128nl_set_transparency();

#endif
