/*
 * $Id$
 * Retro path library
 *
 * ____ HYPSEUS COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2026 DirtBagXon
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

static char g_abpath[REWRITE_MAXPATH] = "\0";
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

void lua_espath(const char *src, char *dst, int dstsize)
{
    char *out = dst;
    int rem = dstsize - 1;

    unsigned char bSet = 0, end = 0, folder = 0, path = PATH_DAPHNE;

    if (bPath(src, "singe/Framework"))
        path = PATH_FRAMEWORK;

    if (bPath(src, "singe/")) {
        folder = PATH_SINGE;
        src += 6;
    }

    #define SAFE_APPEND_STR(str)                                   \
        do {                                                       \
            size_t slen = strlen(str);                             \
            if ((int)slen > rem) slen = rem;                       \
            memcpy(out, str, slen);                                \
            out += slen;                                           \
            rem -= slen;                                           \
        } while (0)

    #define SAFE_APPEND_CH(ch)         \
        do {                           \
            if (rem > 0) {             \
                *out++ = (ch);         \
                rem--;                 \
            }                          \
        } while (0)

    for (int i = 0; rem > 0; src++, i++) {
        if (end != PATH_END) {
            if (*src == '\0') {
                end = PATH_END;
            }

            if (i == 0 && folder == PATH_SINGE) {
                if (g_abpath[0] != 0) {
                    SAFE_APPEND_STR(g_abpath);
                } else {
                    const char* traverse = "/../";
                    const char* romdir = get_romdir_path();

                    SAFE_APPEND_STR(romdir);
                    SAFE_APPEND_STR(traverse);
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
                case PATH_FRAMEWORK:
                    SAFE_APPEND_CH('/');
                    break;
                default:
#ifdef ABSTRACT_SINGE
                    SAFE_APPEND_STR(".hypseus/");
#else
                    SAFE_APPEND_STR(".daphne/");
#endif
                    break;
                }
                bSet = 0xf; //bool
            }

            if (*src != '\0') {
	        char ch = *src;
#ifdef _WIN32
                if (ch == '\\') ch = '/';
#endif
                SAFE_APPEND_CH(ch);
            }
            else
                break;
        }
    }

    *out = '\0';

    #undef SAFE_APPEND_STR
    #undef SAFE_APPEND_CH
}

void lua_setmeta(void* d, size_t n, long long s)
{
    uint8_t k[32] = {0};
    uint8_t p[12] = {0};

    lua_settab((uint64_t)s, k, p);
    lua_push(d, n, k, p, 0);
}

void lua_rampath(const char *src, char *dst, int dstsize)
{
    const char *ramdir = get_ramdir_path();
    size_t ramdir_len = strlen(ramdir);

    if (ramdir_len >= (size_t)dstsize) {
        dst[0] = '\0';
        return;
    }

    memcpy(dst, ramdir, ramdir_len);

    char *out = dst + ramdir_len;
    int rem = dstsize - ramdir_len - 1;

    while (*src && rem > 0) {
        char ch = *src++;
#ifdef _WIN32
        if (ch == '\\') ch = '/';
#endif
        *out++ = ch;
        rem--;
    }

    *out = '\0';
}

int lua_chkdir(const char *path)
{
    char tmp[REWRITE_MAXPATH];

    int out = snprintf(tmp, sizeof(tmp), "%s", path);
    if (out < 0 || out >= (int)sizeof(tmp))
        return -1;

    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';

            if (lua_mkdir(tmp) != 0)
                return -1;

            *p = '/';
        }
    }

    return 0;
}
