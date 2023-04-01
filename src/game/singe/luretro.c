/*
 * $Id$
 * Retro path library
 *
 * ____ HYPSEUS COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2021 DirtBagXon
 *
 * This file is part of HYPSEUS SINGE, a laserdisc arcade game emulator
 *
 * HYPSEUS SINGE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HYPSEUS SINGE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>

static unsigned char g_retropath = 0;

unsigned char get_retropath() { return g_retropath; }

void lua_set_retropath(unsigned char value) { g_retropath = value; }

void lua_retropath(const char *src, char *dst, int len)
{
    unsigned char r = 0;
    for (int i = 0; i < len; src++, i++) {
        if (i == 6) {
            memcpy(dst, "/../", 4);
            dst += 4;
        }
        if (*src == '/' && r < 0x10) {
            r++;
            continue;
        }
        if (r == 2) {
            memcpy(dst, ".daphne/", 8);
            dst += 8;
            r = 0x10;
        }
        *dst = *src;
        dst++;
    }
}
