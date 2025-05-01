/*
 * $Id$
 * Auxiliary functions from Lua API
 *
 * ____ HYPSEUS COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2024 DirtBagXon
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

#ifndef luretro_h
#define luretro_h

#define RETRO_PAD  12
#define RETRO_MAXPATH 128

unsigned char get_retropath();
unsigned char get_zipath();

int lua_chkdir(const char *path);

const char* get_romdir_path();
const char* get_ramdir_path();

void lua_set_retropath(unsigned char value);
void lua_set_zipath(unsigned char value);
void lua_set_abpath(const char *value);

void lua_retropath(const char *src, char *dst, int len);
void lua_rampath(const char *src, char *dst, int len);

#endif
