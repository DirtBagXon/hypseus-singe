/*
 * $Id$
 * Auxiliary functions from Lua API
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

#ifndef luretro_h
#define luretro_h

#define LUA_LO 4
#define LUA_HI 6
#define RETRO_PAD  12
#define RETRO_MAXPATH 128

unsigned char get_retropath();

void (os_alter_clocker)(unsigned char value);

void lua_set_retropath(unsigned char value);

void lua_retropath(const char *src, char *dst, int len);

#endif
