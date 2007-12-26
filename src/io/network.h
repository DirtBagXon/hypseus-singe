/*
 * network.h
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
#define	OS_UNKNOWN 0
#define OS_X86_LINUX 1
#define	OS_WIN32 2
#define	OS_X86_FREEBSD 3
#define	OS_PS2_LINUX 4
#define	OS_SPARC_SOLARIS 5
#define	OS_MAC_OSX 6
#define	OS_XBOX_NATIVE 7
///////////////////////////////////////////////////////////////////////

#define NET_STRSIZE 15
#define NET_LONGSTRSIZE 80

struct net_packet
{
	unsigned char protocol;
	unsigned int user_id;
	unsigned char os;
	char os_desc[NET_LONGSTRSIZE];	// long OS description
	unsigned int mhz;
	unsigned int mem;
	char cpu_name[NET_LONGSTRSIZE];	// long cpu description
	char video_desc[NET_LONGSTRSIZE];	// long video description
	char daphne_version[NET_STRSIZE];
	char gamename[NET_STRSIZE];
	char ldpname[NET_STRSIZE];
	
	// keep this always at the end
	unsigned int crc32;	// a checksum of the preceeding data
};

void net_no_server_send();
unsigned int get_user_id();
void net_set_gamename(char *gamename);
void net_set_ldpname(char *ldpname);
unsigned int get_cpu_mhz();
unsigned int get_sys_mem();
char *get_video_description();
char *get_cpu_name();
char *get_os_description();
void net_send_data_to_server();
