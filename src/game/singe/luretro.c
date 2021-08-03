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


static unsigned char g_retropath = 0;

unsigned char get_retropath() { return g_retropath; }

void lua_set_retropath(unsigned char value) { g_retropath = value; }

void lua_retropath(const char *src, char *dst, int len)
{
    unsigned char r = 0;
    for (int i = 0; i < len; src++, i++) {

        if (i == 6) {
            *dst   =  47;
            dst++;
            *dst   =  46;
            dst++;
            *dst   =  46;
            dst++;
            *dst   =  47;
            dst++;
        }
        if (*src == 0x2F && r<0x10) {
            r++;
            continue;
        }
        if (r == 2) {
            *dst   =  46;
            dst++;
            *dst   = 100;
            dst++;
            *dst   =  97;
            dst++;
            *dst   = 112;
            dst++;
            *dst   = 104;
            dst++;
            *dst   = 110;
            dst++;
            *dst   = 101;
            dst++;
            *dst   =  47;
            dst++;
            r = 0x10;
        }
        *dst = *src;
        dst++;
    }
}
