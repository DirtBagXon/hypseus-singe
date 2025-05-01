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

#include "config.h"

#include <stdio.h>
#include <stdlib.h> // for lousy random number generation
#include <sys/types.h>
#include <string.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

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
#include <VersionHelpers.h>
#endif

#ifdef LINUX
#include <sys/utsname.h> // MATT : I'm not sure if this is good for UNIX in general so I put it here
#include <sys/sysinfo.h>
#endif

#ifdef UNIX
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h> // for DNS
#include <sys/time.h>
#include <unistd.h> // for write
#endif

#include <zlib.h> // for crc32 calculation
#include "../io/error.h"
#include "../hypseus.h"
#include "network.h"


bool g_send_data_to_server = false; // whether user allows us to send data to
                                   // server

////////////////////

// this is not the server you are looking for
void net_server_send() { g_send_data_to_server = false; }

int g_sockfd = -1;          // our socket file descriptor
struct net_packet g_packet; // what we're gonna send

void net_set_gamename(char *gamename)
{
    strncpy(g_packet.gamename, gamename, sizeof(g_packet.gamename)-1);
}

void net_set_ldpname(char *ldpname)
{
    strncpy(g_packet.ldpname, ldpname, sizeof(g_packet.ldpname)-1);
}

#if defined(_MSC_VER) && defined(_M_IX86)
// some code I found to calculate cpu mhz
_inline unsigned __int64 GetCycleCount(void) { _asm _emit 0x0F _asm _emit 0x31 }
#endif

// gets the cpu's memory, rounds to nearest 64 megs of RAM
unsigned int get_sys_mem()
{
    unsigned int result    = 0;
    unsigned int mod       = 0;
    unsigned long long mem = 0;
#ifdef LINUX
    struct sysinfo info;
    sysinfo(&info);
    mem = info.totalram * (unsigned long long)info.mem_unit;
#endif

#ifdef WIN32
    MEMORYSTATUSEX memstat;
    memstat.dwLength = sizeof(memstat);
    GlobalMemoryStatusEx(&memstat);
    mem = (unsigned long long)memstat.ullTotalPhys;
#endif

    result = (mem / 1024 / 1024) + 32; // for rounding
    mod    = result % 64;
    result -= mod;

    return result;
}

char *get_video_description()
{
    static char result[NET_LONGSTRSIZE] = {"Unknown video"};

#ifdef LINUX
#if defined(NATIVE_ARM)
     FILE *F;
     char video[64];
     const char *s = "cat /proc/cpuinfo | grep Hardware | sed -e 's/^.*: //' | head -1";
     F = popen(s, "r");
     if (F)
     {
            if (fscanf(F, "%s", video) == 1)
                strcpy(result, video);

            pclose(F);
     }
#elif defined(NATIVE_CPU_X86)
    FILE *F;
    // PCI query fix by Arnaud G. Gibert
    const char *s = "lspci | grep -i \"VGA compatible controller\" | awk -F ': "
                    "' '{print $2}'";
    F = popen(s, "r");
    if (F) {
        unsigned int len = fread(result, 1, 79, F);
        if (len > 1) result[len - 1] = 0; // make sure string is null terminated
        pclose(F);
    }
#endif
#endif

#ifdef WIN32
    typedef BOOL(WINAPI * infoproc)(PVOID, DWORD, PVOID, DWORD);
    infoproc pEnumDisplayDevices;
    HINSTANCE hInstUser32;
    DISPLAY_DEVICE DispDev;
    bool bEnumDisplayOk = true; // if it's ok to enumerate the display device
                                // (winNT may crash when doing this)

    // The call to EnumDisplayDevicesA may crash under WindowsNT.
    // Therefore we acquire the Windows version here, and if it's NT,
    // skip the following code.

    OSVERSIONINFOEX osvi;
    BOOL bOsVersionInfoEx;

    // Try calling GetVersionEx using the OSVERSIONINFOEX structure.
    // If that fails, try using the OSVERSIONINFO structure.

    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if (!(bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO *)&osvi))) {
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

        // if GetVersionEx fails under all circumstances, then we will err on
        // the side of caution and not enumerate
        if (!GetVersionEx((OSVERSIONINFO *)&osvi)) {
            bEnumDisplayOk = false;
        }
    }

    // if it's Windows NT, then don't enumerate the display
    if ((osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) && (osvi.dwMajorVersion <= 4)) {
        bEnumDisplayOk = false;
    }

    // if it's ok to enumerate the display device
    if (bEnumDisplayOk) {
        hInstUser32 = LoadLibrary("user32");
        if (hInstUser32) {
            pEnumDisplayDevices =
                (infoproc)GetProcAddress(hInstUser32, "EnumDisplayDevicesA");
            if (pEnumDisplayDevices) {
                ZeroMemory(&DispDev, sizeof(DISPLAY_DEVICE));
                DispDev.cb = sizeof(DISPLAY_DEVICE);
                if ((*pEnumDisplayDevices)(NULL, 0, &DispDev, 0)) {
                    strncpy(result, (char *)DispDev.DeviceString, sizeof(result) - 1);
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
    static char result[NET_LONGSTRSIZE] = {0};
    strcpy(result, "UnknownCPU"); // default ...

#ifdef NATIVE_CPU_X86
    unsigned int reg_ebx, reg_ecx, reg_edx;
#if defined(_MSC_VER) && defined(_M_IX86)
    _asm
    {
		xor eax, eax
		cpuid
		mov reg_ebx, ebx
		mov reg_ecx, ecx
		mov reg_edx, edx
    }
#else
    asm("xor %%eax, %%eax\n\t"
        "cpuid\n\t"
        : "=b"(reg_ebx), "=c"(reg_ecx), "=d"(reg_edx)
        :             /* no inputs */
        : "cc", "eax" /* a is clobbered upon completion */
        );
#endif

    result[0] = (char)((reg_ebx)&0xFF);
    result[1] = (char)((reg_ebx >> 8) & 0xFF);
    result[2] = (char)((reg_ebx >> 16) & 0xFF);
    result[3] = (char)(reg_ebx >> 24);

    result[4] = (char)((reg_edx)&0xFF);
    result[5] = (char)((reg_edx >> 8) & 0xFF);
    result[6] = (char)((reg_edx >> 16) & 0xFF);
    result[7] = (char)((reg_edx >> 24) & 0xFF);

    result[8]  = (char)((reg_ecx)&0xFF);
    result[9]  = (char)((reg_ecx >> 8) & 0xFF);
    result[10] = (char)((reg_ecx >> 16) & 0xFF);
    result[11] = (char)((reg_ecx >> 24) & 0xFF);
#endif // NATIVE_CPU_X86

#ifdef NATIVE_CPU_MIPS
    strcpy(result, "MIPS R5900 V2.0"); // assume playstation 2 for now
#endif                                 // NATIVE_CPU_MIPS

// On Mac, we can tell what type of CPU by simply checking the ifdefs thanks to
// the universal binary.
#ifdef MAC_OSX
#ifdef __PPC__
    strcpy(result, "PowerPC");
#else
    strcpy(result, "GenuineIntel");
#endif
#endif

#ifdef LINUX
    FILE *F;
    char cpu[64];
    const char *s = "cat /proc/cpuinfo | grep 'model name' | sed -e 's/^.*: //' | head -1";
    F = popen(s, "r");
    if (F)
    {
            if (fscanf(F, "%s", cpu) == 1)
                strcpy(result, cpu);

            pclose(F);
    }
#endif

    return result;
}

char *get_os_description()
{
    static char result[NET_LONGSTRSIZE] = {"Unknown OS"};

#ifdef LINUX
    struct utsname buf;
    int uname_result       = uname(&buf);

    // if uname did not return any error ...
    if (uname_result == 0) {
        strcpy(result, "Linux ");
        snprintf(&result[6], sizeof(result) - 6, "%s", buf.release);
    }

#endif

#ifdef WIN32
    strcpy(result, "Unknown Windows");

    if (IsWindows10OrGreater()) {
        strcpy(result, "Windows 10/11");
    } else if (IsWindows7OrGreater()) {
        strcpy(result, "Windows 7/8");
    } else if (IsWindowsVistaOrGreater()) {
        strcpy(result, "Windows Vista");
    } else if (IsWindowsXPOrGreater()) {
        strcpy(result, "Windows XP/2000");
    }
#endif

#ifdef MAC_OSX
    strcpy(result, "Mac OSX");
#endif

    return result;
}

char *get_sdl_compile()
{
    static char result[NET_LONGSTRSIZE] = {0};

    SDL_version compiled;
    SDL_version imgCompiled;
    SDL_version ttfCompiled;
    SDL_version mixCompiled;

    SDL_VERSION(&compiled);
    SDL_IMAGE_VERSION(&imgCompiled);
    SDL_TTF_VERSION(&ttfCompiled);
    SDL_MIXER_VERSION(&mixCompiled);

    snprintf(result, sizeof(result),
         "(CC) SDL: %d.%d.%d, "
         "IMG: %d.%d.%d, "
         "TTF: %d.%d.%d, "
         "MIX: %d.%d.%d",
         compiled.major, compiled.minor, compiled.patch,
         imgCompiled.major, imgCompiled.minor, imgCompiled.patch,
         ttfCompiled.major, ttfCompiled.minor, ttfCompiled.patch,
         mixCompiled.major, mixCompiled.minor, mixCompiled.patch);

    return result;
}

char *get_sdl_linked()
{
    static char result[NET_LONGSTRSIZE] = {0};

    SDL_version linked;

    SDL_GetVersion(&linked);
    const SDL_version* imgLinked = IMG_Linked_Version();
    const SDL_version* ttfLinked = TTF_Linked_Version();
    const SDL_version* mixLinked = Mix_Linked_Version();

    snprintf(result, sizeof(result),
         "(LD) SDL: %d.%d.%d, "
         "IMG: %d.%d.%d, "
         "TTF: %d.%d.%d, "
         "MIX: %d.%d.%d",
         linked.major, linked.minor, linked.patch,
         imgLinked->major, imgLinked->minor, imgLinked->patch,
         ttfLinked->major, ttfLinked->minor, ttfLinked->patch,
         mixLinked->major, mixLinked->minor, mixLinked->patch);

    return result;
}

char *get_build_time()
{
   static char result[NET_LONGSTRSIZE] = {0};
   static const char *built = __DATE__ " " __TIME__;

   snprintf(result, sizeof(result), "Compiled: %s", built);

   return result;
}

// DBX: Pretty certain MPO's server doesn't want these
// Disabled but rip it out for the paranoid....
void net_send_data_to_server()
{
    return;
}
