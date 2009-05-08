/*
 * network.cpp
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

#include <stdio.h>
#include <stdlib.h>	// for lousy random number generation
#include <sys/types.h>
#include <string.h>

#ifdef MAC_OSX
#include <mach/host_info.h>
#include <mach/mach_host.h>
#include <mach/mach_init.h>
#include <mach/host_priv.h>
#include <mach/machine.h>
#include <carbon/carbon.h>
#endif

#ifdef WIN32
#include <windows.h>
//#include <strmif.h>
//#include <control.h>
//#include <uuids.h>
#endif

#ifdef FREEBSD
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#ifdef LINUX
#include <sys/utsname.h>	// MATT : I'm not sure if this is good for UNIX in general so I put it here
#endif

#ifdef UNIX
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>	// for DNS
#include <sys/time.h>
#include <unistd.h>	// for write
#endif


#include <zlib.h>	// for crc32 calculation
#include "../io/error.h"
#include "../daphne.h"
#include "network.h"

// arbitrary port I've chosen to send incoming data
#define NET_PORT 7733

// ip address to send data to
// I changed this to 'stats' in case I ever want to change the IP address of the stats
// server without changing the location of the web server.
#define NET_IP "stats.daphne-emu.com"

// which version of this simple protocol we are using (so the server can support multiple versions)
static const unsigned char PROTOCOL_VERSION = 1;

#define USERID_FILENAME "userid.bin"

bool g_send_data_to_server = true;	// whether user allows us to send data to server

////////////////////

// disable sending data to server for paranoid users
// (I don't know why anyone should be paranoid, this is open source for crying out loud)
void net_no_server_send()
{
	g_send_data_to_server = false;
}

// returns a unique (hopefully) randomly generated user id so we can track how many
// different daphne users there are out there
unsigned int get_user_id()
{
	bool id_exists = false;	// id already exists?
	unsigned int id = 0;
	FILE *F = NULL;
	
	F = fopen(USERID_FILENAME, "rb");
	// if they already have a userid generated previously
	if (F)
	{
		unsigned int len = fread(&id, 1, sizeof(id), F);

		// make sure we read something ...
		if (len == sizeof(id))
		{
			id_exists = true;
		}
		fclose(F);
	}

	// if we don't have an ID yet then generate one now
	if (!id_exists)
	{
		F = fopen(USERID_FILENAME, "wb");
		if (F)
		{
#ifdef UNIX
			struct timeval tv;
			gettimeofday(&tv, NULL);	// get a random number (time)
			srand(tv.tv_sec);	// seed randomizer with current usec's elapsed (hopefully super random)
			id = rand() ^ tv.tv_usec;	// hopefully this value will be so random that it will be unique
#endif
#ifdef WIN32
			SYSTEMTIME cur_time;
			FILETIME file_time;
			GetSystemTime(&cur_time);
			SystemTimeToFileTime(&cur_time, &file_time);
			srand(file_time.dwHighDateTime);
			id = rand() ^ file_time.dwLowDateTime;
#endif
			fwrite(&id, sizeof(id), 1, F);
			fclose(F);
		}
		// else we couldn't create so we'll just try again next time
	}

	return id;	
}

int g_sockfd = -1;	// our socket file descriptor
struct net_packet g_packet;	// what we're gonna send

void net_set_gamename(char *gamename)
{
	strncpy(g_packet.gamename, gamename, sizeof(g_packet.gamename));
}

void net_set_ldpname(char *ldpname)
{
	strncpy(g_packet.ldpname, ldpname, sizeof(g_packet.ldpname));
}

#ifdef WIN32
// some code I found to calculate cpu mhz
_inline unsigned __int64 GetCycleCount(void)
{
    _asm    _emit 0x0F
    _asm    _emit 0x31
}
#endif

// gets the cpu's mhz (rounds to nearest 50 MHz)
unsigned int get_cpu_mhz()
{
	unsigned int result = 0;
	unsigned int mod = 0;
#ifdef LINUX
#ifdef NATIVE_CPU_X86
	FILE *F;
	double mhz;
	int iRes = 0;
	const char *s = "cat /proc/cpuinfo | grep MHz | sed -e 's/^.*: //'";
	F = popen(s, "r");
	if (F)
	{
		iRes = fscanf(F, "%lf", &mhz);
		pclose(F);
	}
	result = (unsigned int) mhz;
#endif // NATIVE_CPU_X86
#ifdef NATIVE_CPU_MIPS
	result = 294;	// assume playstation 2 for now :)
#endif // NATIVE_CPU_MIPS
#endif // LINUX

#ifdef FREEBSD
	FILE *F;
	double mhz;
	char command[128]="dmesg | grep CPU | perl -e 'while (<>) {if ($_ =~ /\\D+(\\d+).+-MHz.+/) {print \"$1\n\"}}' > /tmp/result.txt"; 
	system(command);
	F = fopen("/tmp/result.txt", "rb");
	if (F)
	{
		fscanf(F, "%lf", &mhz);
		fclose(F);
		unlink("/tmp/result.txt");
	}
	result = (unsigned int) mhz;
#endif
	

#ifdef WIN32
	unsigned __int64  m_startcycle;
	unsigned __int64  m_res;

	m_startcycle = GetCycleCount();
	Sleep(1000); 
	m_res = GetCycleCount()-m_startcycle;

	result = (unsigned int)(m_res / 1000000);	// convert Hz to MHz
#endif

#ifdef MAC_OSX
	long cpuSpeed = 0;
	Gestalt(gestaltProcClkSpeed, &cpuSpeed);
	result = (unsigned int)(cpuSpeed / 1000000);
	return result;
#endif

	// round to nearest 50 MHz
	result += 25;	// for rounding
	mod = result % 50;
	result -= mod;
	
	return result;
}

// gets the cpu's memory, rounds to nearest 64 megs of RAM
unsigned int get_sys_mem()
{
	unsigned int result = 0;
	unsigned int mod = 0;
	unsigned int mem = 0;
#ifdef LINUX
	FILE *F;
	int iRes = 0;
	const char *s = "ls -l /proc/kcore | awk '{print $5}'";
	F = popen(s, "r");
	if (F)
	{
		iRes = fscanf(F, "%u", &mem);	// this breaks if they have over 2 gigs of ram :)
		pclose(F);
	}

#endif

#ifdef FREEBSD
	size_t len;
	len = sizeof(mem);
	sysctlbyname("hw.physmem", &mem, &len, NULL, NULL);
	
#endif

#ifdef WIN32
	MEMORYSTATUS memstat;
	GlobalMemoryStatus(&memstat);
	mem = memstat.dwTotalPhys;
#endif

	result = (mem / (1024*1024)) + 32;	// for rounding
	mod = result % 64;
	result -= mod;

	return result;
}

char *get_video_description()
{
	static char result[NET_LONGSTRSIZE] = { "Unknown video" };

#ifdef LINUX
#ifdef NATIVE_CPU_X86
	FILE *F;
	// PCI query fix by Arnaud G. Gibert
	const char *s = "lspci | grep -i \"VGA compatible controller\" | awk -F ': ' '{print $2}'";
	F = popen(s, "r");
	if (F)
	{
		unsigned int len = fread(result, 1, 79, F);
		if (len > 1) result[len-1] = 0;	// make sure string is null terminated
		pclose(F);
	}
#endif	// NATIVE_CPU_X86
#ifdef NATIVE_CPU_MIPS
	strcpy(result, "Playstation2");	// assume PS2 for now hehe
#endif // MIPS
#endif

#ifdef FREEBSD
	//I haven't found a nice way to get graphic adaptor informations :( 
#endif

#ifdef WIN32
	typedef BOOL (WINAPI *infoproc)(PVOID, DWORD, PVOID, DWORD);
	infoproc pEnumDisplayDevices;
	HINSTANCE hInstUser32;
	DISPLAY_DEVICE DispDev;
	bool bEnumDisplayOk = true;	// if it's ok to enumerate the display device (winNT may crash when doing this)

    // The call to EnumDisplayDevicesA may crash under WindowsNT.
    // Therefore we acquire the Windows version here, and if it's NT, 
    // skip the following code.

    OSVERSIONINFOEX osvi;
    BOOL bOsVersionInfoEx;

    // Try calling GetVersionEx using the OSVERSIONINFOEX structure.
    // If that fails, try using the OSVERSIONINFO structure.

    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) )
	{
        osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);

		// if GetVersionEx fails under all circumstances, then we will err on the side of caution and not enumerate
        if (! GetVersionEx ( (OSVERSIONINFO *) &osvi) )
		{
            bEnumDisplayOk = false;
        }
    }

	// if it's Windows NT, then don't enumerate the display
	if ((osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) && (osvi.dwMajorVersion <= 4))
	{
		bEnumDisplayOk = false;
	}

	// if it's ok to enumerate the display device
	if (bEnumDisplayOk)
	{
		hInstUser32 = LoadLibrary("user32");
		if (hInstUser32)
		{
			pEnumDisplayDevices = (infoproc) GetProcAddress(hInstUser32, "EnumDisplayDevicesA");
			if (pEnumDisplayDevices)
			{
				ZeroMemory(&DispDev, sizeof(DISPLAY_DEVICE));
				DispDev.cb = sizeof(DISPLAY_DEVICE);
				if ((*pEnumDisplayDevices)(NULL, 0, &DispDev, 0))
				{
					strncpy(result, (char *) DispDev.DeviceString, sizeof(result)-1);
				}
			}
			FreeLibrary(hInstUser32);
		}
	}
	// else we can't enumerate
#endif

	return result;
}

char *get_cpu_name()
{
	static char result[NET_LONGSTRSIZE] = { 0 };
	strcpy(result, "UnknownCPU");	// default ...

#ifdef NATIVE_CPU_X86
	unsigned int reg_ebx, reg_ecx, reg_edx;
#ifdef WIN32
	_asm
	{
		xor eax, eax
		cpuid
		mov reg_ebx, ebx
		mov reg_ecx, ecx
		mov reg_edx, edx
	}
#else
    asm
    (
    	"xor %%eax, %%eax\n\t"
    	"cpuid\n\t"
		: "=b" (reg_ebx), "=c" (reg_ecx), "=d" (reg_edx)
		: /* no inputs */
		: "cc", "eax"	/* a is clobbered upon completion */
	);
#endif

	result[0] = (char) ((reg_ebx) & 0xFF);
	result[1] = (char) ((reg_ebx >> 8) & 0xFF);
	result[2] = (char) ((reg_ebx >> 16) & 0xFF);
	result[3] = (char) (reg_ebx >> 24);

	result[4] = (char) ((reg_edx) & 0xFF);
	result[5] = (char) ((reg_edx >> 8) & 0xFF);
	result[6] = (char) ((reg_edx >> 16) & 0xFF);
	result[7] = (char) ((reg_edx >> 24) & 0xFF);

	result[8] = (char) ((reg_ecx) & 0xFF);
	result[9] = (char) ((reg_ecx >> 8) & 0xFF);
	result[10] = (char) ((reg_ecx >> 16) & 0xFF);
	result[11] = (char) ((reg_ecx >> 24) & 0xFF);
#endif // NATIVE_CPU_X86

#ifdef NATIVE_CPU_MIPS
	strcpy(result, "MIPS R5900 V2.0");	// assume playstation 2 for now
#endif	// NATIVE_CPU_MIPS

#ifdef NATIVE_CPU_SPARC
	strcpy(result, "Sparc");
#endif	// NATIVE_CPU_SPARC

//On Mac, we can tell what type of CPU by simply checking the ifdefs thanks to the universal binary.
#ifdef MAC_OSX
#ifdef __PPC__	
	strcpy(result, "PowerPC");
#else
	strcpy(result, "GenuineIntel");
#endif
#endif

	return result;
}

char *get_os_description()
{
	static char result[NET_LONGSTRSIZE] = { "Unknown OS" };

#ifdef LINUX
	struct utsname buf;
	unsigned int i = 0;	
	int uname_result = uname(&buf);
	int found_first_period = 0;

	// if uname did not return any error ...	
	if (uname_result == 0)
	{
		// find the second period in the linux version, so we can terminate there
		for (i = 0; i < (sizeof(result)-10); i++)
		{
			if (buf.release[i] == '.')
			{
				// if we haven't found the first period in the linux version yet
				if (!found_first_period)
				{
					found_first_period = 1;
				}
				
				// if we have already found the first period, then this is the second period,
				// so get out of the loop
				else
				{
					break;
				}
			}
		}

		strcpy(result, "Linux ");	
		strncpy(&result[6], buf.release, i);
		result[i+6] = 0;	// terminate string just in case
	} // end if uname worked

#endif

#ifdef FREEBSD
	size_t len;
        char buff[16] = { "Unknown" };
        len = sizeof(buff);
        sysctlbyname("kern.ostype", &buff, &len, NULL, NULL);
        strcpy(result,buff);
        strcat(result," ");
        sysctlbyname("kern.osrelease", &buff, &len, NULL, NULL);
        strcat(result,buff);		
#endif
	
#ifdef WIN32
	OSVERSIONINFO info;
	memset(&info, 0, sizeof(OSVERSIONINFO));
	info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&info);
	switch (info.dwPlatformId)
	{
	case VER_PLATFORM_WIN32_WINDOWS:
		switch (info.dwMinorVersion)
		{
		case 0:
			strcpy(result, "Windows 95");
			break;
		case 10:
			strcpy(result, "Windows 98");
			break;
		case 90:
			strcpy(result, "Windows ME");
			break;
		default:
			strcpy(result, "Windows 95 Derivative");
			break;
		}
		break;
	case VER_PLATFORM_WIN32_NT:
		switch (info.dwMinorVersion)
		{
		case 0:
			strcpy(result, "Windows NT/2000");
			break;
		case 1:
			strcpy(result, "Windows XP/.NET");
			break;
		default:
			strcpy(result, "Windows NT Derivative");
			break;
		}
		break;
	default:
		strcpy(result, "Unknown Windows");
		break;
	}
#endif

#ifdef SOLARIS
	strcpy(result, "Solaris");
#endif

#ifdef MAC_OSX
	strcpy(result, "Mac OSX");
#endif

	return result;

}

// send stats to server
void net_send_data_to_server()
{
	struct sockaddr_in saRemote;
	struct hostent *info = NULL;
	char ip[81];

	if (!g_send_data_to_server) return;	// if user forbids data to be sent, don't do it

#ifdef DEBUG
	// I don't wanna mess up the server stats with people trying to debug daphne
	return;
#endif

#ifdef WIN32
	// initialize Winschlock
	WSADATA wsaData;
 
	WSAStartup(MAKEWORD(1,1), &wsaData ); 
#endif

	info = gethostbyname(NET_IP);	// do DNS to convert address to numbers
	
	// if the DNS resolution worked
	if (info)
	{
//		inet_ntop(AF_INET, info->h_addr, ip, sizeof(ip));
		sprintf(ip, "%u.%u.%u.%u", (unsigned char) info->h_addr_list[0][0],
			(unsigned char) info->h_addr_list[0][1],
			(unsigned char) info->h_addr_list[0][2],
			(unsigned char) info->h_addr_list[0][3]);

		g_sockfd = socket(AF_INET, SOCK_STREAM, 0);

		if (g_sockfd != -1)
		{		
			memset(&saRemote, 0, sizeof(saRemote));
			saRemote.sin_family = AF_INET;
			saRemote.sin_addr.s_addr = inet_addr(ip);	// inet_addr is broken and should not be used
			saRemote.sin_port = htons(NET_PORT);

			// if we are able to connect to socket successfully
			if (connect(g_sockfd, (struct sockaddr *) &saRemote, sizeof(saRemote)) == 0)
			{
				g_packet.user_id = get_user_id();
				g_packet.os = OS_UNKNOWN;
#ifdef WIN32
				g_packet.os = OS_WIN32;
#endif
#ifdef LINUX
#ifdef NATIVE_CPU_X86
				g_packet.os = OS_X86_LINUX;
#endif
#ifdef NATIVE_CPU_MIPS
				g_packet.os = OS_PS2_LINUX;
#endif
#endif // end LINUX

#ifdef FREEBSD
				g_packet.os = OS_X86_FREEBSD;
#endif

#ifdef SOLARIS
#ifdef NATIVE_CPU_SPARC
				g_packet.os = OS_SPARC_SOLARIS;
#endif
#endif // end SOLARIS

#ifdef MAC_OSX
				g_packet.os = OS_MAC_OSX;
#endif

				// safety check
				if (g_packet.os == OS_UNKNOWN)
				{
					printerror("your OS is unknown in network.cpp, please fix this");
				}

				strncpy(g_packet.os_desc, get_os_description(), sizeof(g_packet.os_desc));
				g_packet.protocol = PROTOCOL_VERSION;
				g_packet.mhz = get_cpu_mhz();
				g_packet.mem = get_sys_mem();
				strncpy(g_packet.video_desc, get_video_description(), sizeof(g_packet.video_desc));
				strncpy(g_packet.cpu_name, get_cpu_name(), sizeof(g_packet.cpu_name));
				strncpy(g_packet.daphne_version, get_daphne_version(), sizeof(g_packet.daphne_version));

				// now compute CRC32 of the rest of the packet
				g_packet.crc32 = crc32(0L, Z_NULL, 0);
				g_packet.crc32 = crc32(g_packet.crc32, (unsigned char*) &g_packet, sizeof(g_packet) - sizeof(g_packet.crc32));
				send(g_sockfd, (const char *) &g_packet, sizeof(g_packet), 0);
			}
			// else connection was refused (server down)

#ifdef WIN32
			closesocket(g_sockfd);
#else
			close(g_sockfd);	// we're done!
#endif
		}
	} // end if DNS look-up worked
}
