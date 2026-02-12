/*
 * $Id$
 * Retro path library
 *
 * ____ HYPSEUS COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2025 DirtBagXon
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

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <unistd.h>
#endif

#include "luretro.h"

enum {
    PATH_DAPHNE,
    PATH_SINGE,
    PATH_FRAMEWORK,
    PATH_END
};

static char g_abpath[RETRO_MAXPATH] = "\0";
static unsigned char g_espath = 0;
static unsigned char g_zipath = 0;

unsigned char get_espath() { return g_espath; }
unsigned char get_zipath() { return g_zipath; }

void lua_set_espath(unsigned char value) { g_espath = value; }
void lua_set_zipath(unsigned char value) { g_zipath = value; }

void lua_set_abpath(const char *value)
{
    strncpy(g_abpath, value, sizeof(g_abpath) - 1);
    g_abpath[sizeof(g_abpath) - 1] = '\0';
}

unsigned char bPath(const char* src, const char* path)
{
    return (strncmp(src, path, strlen(path)) == 0);
}

// We block in lfs, isolate to zip rampath via io
int lua_mkdir(const char *path)
{
    struct stat st = {0};

    if (stat(path, &st) == -1) {
        if (mkdir(path, 0755) != 0) {
            return -1;
        }
    }

    return 0;
}

void lua_espath(const char *src, char *dst, int len)
{
    unsigned char bSet = 0, end = 0, folder = 0, path = PATH_DAPHNE;

    if (bPath(src, "singe/Framework")) {
        path = PATH_FRAMEWORK;
    }

    if (bPath(src, "singe/")) {
        folder = PATH_SINGE;
        src += 6;
    }

    for (int i = 0; i < (len - 2); src++, i++) {
        if (end != PATH_END) {
            if (*src == '\0') {
                end = PATH_END;
            }
            if (i == 0 && folder == PATH_SINGE) {
                if (g_abpath[0] != 0) {
                    memcpy(dst, g_abpath, strlen(g_abpath));
                    dst += strlen(g_abpath);
                } else {
                    const char* traverse = "/../";
                    const char* romdir = get_romdir_path();

                    memcpy(dst, romdir, strlen(romdir));
                    dst += strlen(romdir);

                    memcpy(dst, traverse, strlen(traverse));
                    dst += strlen(traverse);
                }
            }
            if (*src == '/') {
                if (bSet < 0xf) {
                    bSet++;
                    continue;
                }
            }
            if (bSet == 1) {
                switch(path) {
                case (PATH_FRAMEWORK):
                    *dst++ = '/';
                    break;
                default:
#ifdef ABSTRACT_SINGE
                    memcpy(dst, ".hypseus/", 9);
                    dst += 9;
#else
                    memcpy(dst, ".daphne/", 8);
                    dst += 8;
#endif
                    break;
                }
                bSet = 0xf; //bool
            }
            *dst = *src;
            dst++;
        }
    }
    *dst = '\0';
}

void lua_rampath(const char *src, char *dst, int len)
{
    const char *ramdir = get_ramdir_path();
    size_t ramdir_len = strlen(ramdir);
    unsigned char end = 0;

    memcpy(dst, ramdir, ramdir_len);
    dst += ramdir_len;

    while (*src && len-- > 0) {
        if (end != PATH_END) {
            if (*src == '\0') {
                end = PATH_END;
            }
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

int lua_chkdir(const char *path)
{
    char tmp[RETRO_MAXPATH];
    char *p = NULL;
    int len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);

    for (p = tmp + 1; p < tmp + len; p++) {
        if (*p == '/') {
            *p = '\0';

            if (lua_mkdir(tmp) != 0) {
                return -1;
            }
            *p = '/';
        }
    }

    return 0;
}
