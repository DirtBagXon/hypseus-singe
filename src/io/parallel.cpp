/*
 * ____ DAPHNE COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2001 Matt Ownby
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

#include <plog/Log.h>

#include "parallel.h"
#include "numstr.h"

// this code only applies to x86-based systems, I believe
#ifdef NATIVE_CPU_X86

// Code to control the parallel port

unsigned int par::m_uPortIdx = 0;
short par::m_base0[3]        = {0x378, 0x278, 0};
short par::m_base2[3]        = {0x37A, 0x27A, 0};

#include <stdio.h>
#include "conout.h"
#include "stdlib.h"

#ifdef WIN32

#include <windows.h>

typedef void(WINAPI *Out32_t)(short PortAddress, short data);
Out32_t Out32;
HINSTANCE g_hInstInpout;

bool par::init(unsigned int port)
// initializes parallel port for use
// port 0 is LPT1
// port 1 is LPT2
{
    if (port > 1) // if port is not LPT1 or LPT2, a custom address for the port
                  // was specified
    {
        m_base0[2] = port;
        m_base2[2] = port + 2;
        port       = 2; // so that m_uPortIdx becomes 2
    }

    m_uPortIdx = port;

    LOGI << "Opening parallel port at address 0x" << numstr::ToStr(m_base0[m_uPortIdx], 16, 4);

    g_hInstInpout = LoadLibrary("inpout32.dll");
    if (g_hInstInpout == NULL) {
        return (false);
    }

    Out32 = (Out32_t)GetProcAddress(g_hInstInpout, "Out32");
    if (!Out32) {
        FreeLibrary(g_hInstInpout);
        g_hInstInpout = NULL;
        return (false);
    }

    return (true);
}

// writes a byte to the port at base+0
void par::base0(unsigned char data) { Out32(m_base0[m_uPortIdx], data); }

// writes a byte to the port at base+2
void par::base2(unsigned char data) { Out32(m_base2[m_uPortIdx], data); }

void par::close()
{
    // does nothing with current win32 implementation
    LOGI << "Closing parallel port";

    FreeLibrary(g_hInstInpout);
    g_hInstInpout = NULL;
    Out32         = NULL;
}

#endif

// Linux code below

#ifdef UNIX

#ifdef LINUX

#include <sys/io.h>

#endif

// initializes the specified port (0 is LPT1)
// returns 1 if successful, 0 if failted
bool par::init(unsigned int port)
{

    bool result = false;

    LOGI << fmt("Opening parallel port %d", port);

    if (port > 1) // if port is not LPT1 or LPT2, a custom address for the port
                  // was specified
    {
        port            = 2; // set port to the custom address
        par::m_base0[2] = port;
        par::m_base2[2] = port + 2;
    }

    // request access to the small range of ports we need
    // if we get a 0, it means we were successful
    if (ioperm(par::m_base0[port], 3, 1) == 0) {
        m_uPortIdx = port;
        result     = true;
    }

    return (result);
}

// writes a byte to the port at base+0
void par::base0(unsigned char data) { outb(data, par::m_base0[m_uPortIdx]); }

// writes a byte to the port at base+2
void par::base2(unsigned char data) { outb(data, par::m_base2[m_uPortIdx]); }

// closes parallel port
void par::close()
{
    // we don't have to do anything here
}

#endif // end UNIX
#else  // end NATIVE_CPU_X86

// here is the code for systems that have no parallel support
bool par::init(unsigned int port) { return false; }

void par::base0(unsigned char data) {}

void par::base2(unsigned char data) {}

void par::close() {}
#endif
