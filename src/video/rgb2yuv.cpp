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

#include "rgb2yuv.h"

// if we aren't using the assembly version, then use the C++ version instead
#ifndef USE_MMX

// Maintain exact compatibility with the old interface
unsigned int rgb2yuv_input[3] = {0}; // 8-bit values
unsigned int rgb2yuv_result_y = 0;   // 8-bit
unsigned int rgb2yuv_result_u = 0;   // 8-bit
unsigned int rgb2yuv_result_v = 0;   // 8-bit

// Macros to make the math below readable (matches original code style)
#define R ((int)rgb2yuv_input[0])
#define G ((int)rgb2yuv_input[1])
#define B ((int)rgb2yuv_input[2])

// No lookup tables (saves ~12KB cache), uses CPU registers
void rgb2yuv() {
    // 1. Calculate Y (Luma)
    // Formula: Y = (0.299 * R + 0.587 * G + 0.114 * B)
    // Scaled by 15 bits: 9798*R + 19235*G + 3736*B
    int y_val = (9798 * R + 19235 * G + 3736 * B) >> 15;

    rgb2yuv_result_y = (unsigned int)y_val;

    // 2. Calculate U (Chroma Blue)
    // Formula: U = 0.492 * (B - Y) + 128
    // Scaled by 15 bits: 18514
    // We do math directly on the difference rather than looking up an array index
    int u_val = (((B - y_val) * 18514) >> 15) + 128;

    rgb2yuv_result_u = (unsigned int)u_val;

    // 3. Calculate V (Chroma Red)
    // Formula: V = 0.877 * (R - Y) + 128
    // Scaled by 15 bits: 23364
    int v_val = (((R - y_val) * 23364) >> 15) + 128;

    rgb2yuv_result_v = (unsigned int)v_val;
}

// Clean up macros
#undef R
#undef G
#undef B

#endif // not MMX_RGB2YUV
