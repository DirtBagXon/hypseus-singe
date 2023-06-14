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

enum {
    PATH_DAPHNE,
    PATH_FRAMEWORK,
    PATH_END
};

static unsigned char g_retropath = 0;

unsigned char get_retropath() { return g_retropath; }

void lua_set_retropath(unsigned char value) { g_retropath = value; }

unsigned char inPath(const char* src, char* path)
{
    char *s = strstr(src, path);
    if (s != NULL) return 1;
    return 0;
}

void lua_retropath(const char *src, char *dst, int len)
{
    unsigned char r = 0, fin = 0, path = PATH_DAPHNE;

    if (inPath(src, "Framework")) path = PATH_FRAMEWORK;

    for (int i = 0; i < (len - 2); src++, i++) {
        if (!fin) {
            if (*src == '\0') {
                fin = PATH_END;
            }
            if (i == 6) {
                memcpy(dst, "/../", 4);
                dst += 4;
            }
            if (*src == '/' && r < 0xf) {
                r++;
                continue;
            }
            if (r == 2) {
                switch(path) {
                case (PATH_FRAMEWORK):
                    memcpy(dst, "/", 1);
                    dst += 1;
                    break;
                default:
                    memcpy(dst, ".daphne/", 8);
                    dst += 8;
                    break;
                }
                r = 0xf; //bool
            }
            *dst = *src;
            dst++;
        }
    }
    *dst = '\0';
}
