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

namespace palette
{
typedef struct {
    Uint8 y;
    Uint8 v;
    Uint8 u;
    bool transparent;
} t_yuv_color;

// Among other things, sets color #0 to be transparent.  You can change this
//  by calling set_transparency.
bool initialize(unsigned int num_colors);

// void set_transparent_color (SDL_Color color_value);
// void set_transparent_number(int color_number);

// sets whether a color (from 0-255) is transparent or not.
// The default is for color #0 to be transparent, and this will be
//  set in initialize.
void set_transparency(unsigned int uColorIndex, bool transparent);
void set_yuv_transparency(int state);
bool get_yuv_overlay_ready();

void set_color(unsigned int color_num, SDL_Color color_value);
void finalize();
void shutdown(void);
}
