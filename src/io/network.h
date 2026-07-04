/*
 * ____ DAPHNE COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2002 Matt Ownby
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

// THESE VALUES MUST NOT CHANGE BECAUSE THE SERVER IS RELYING ON THEM
#define OS_UNKNOWN 0
#define OS_X86_LINUX 1
#define OS_WIN32 2
#define OS_PS2_LINUX 3
#define OS_MAC_OSX 4
///////////////////////////////////////////////////////////////////////

#define NET_STRSIZE 15
#define NET_LONGSTRSIZE 80

void net_server_send(bool);
unsigned int get_sys_mem();
char *get_video_description();
char *get_cpu_name();
char *get_os_description();
char *get_sdl_compile();
char *get_sdl_linked();
char *get_build_time();
const char *get_id();
