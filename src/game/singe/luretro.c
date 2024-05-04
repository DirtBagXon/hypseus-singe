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
    PATH_SINGE,
    PATH_FRAMEWORK,
    PATH_END
};

static unsigned char g_retropath = 0;
static unsigned char g_zipath = 0;

unsigned char get_retropath() { return g_retropath; }
unsigned char get_zipath() { return g_zipath; }

void lua_set_retropath(unsigned char value) { g_retropath = value; }
void lua_set_zipath(unsigned char value) { g_zipath = value; }

unsigned char inPath(const char* src, char* path)
{
    char *s = strstr(src, path);
    if (s != NULL) return 1;
    return 0;
}

void lua_retropath(const char *src, char *dst, int len)
{
    unsigned char r = 0, fin = 0, folder = 0, path = PATH_DAPHNE;

    if (inPath(src, "Framework")) path = PATH_FRAMEWORK;
    if (inPath(src, "singe/")) folder = PATH_SINGE;
    else r++;

    for (int i = 0; i < (len - 2); src++, i++) {
        if (fin != PATH_END) {
            if (*src == '\0') {
                fin = PATH_END;
            }
            if (i == 0 && *src == '/') continue;
            if (folder == PATH_SINGE && i == 6) {
                dst -= 5;
                memcpy(dst, "roms/../", 8);
                dst += 8;
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
#ifdef ABSTRACT_SINGE
                    memcpy(dst, ".singe/", 7);
                    dst += 7;
#else
                    memcpy(dst, ".daphne/", 8);
                    dst += 8;
#endif
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

void lua_rampath(const char *src, char *dst, int len)
{
    unsigned char r = 0, fin = 0;

    for (int i = 0; i < (len); src++, i++) {
        if (fin != PATH_END) {
            if (*src == '\0') {
                fin = PATH_END;
            }
            if (r == 0) {
                memcpy(dst, "ram/", 4);
                dst += 4;
                r++;
            }
            *dst = *src;
            dst++;
        }
    }
    *dst = '\0';
}
