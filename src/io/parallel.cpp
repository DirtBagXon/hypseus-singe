/*
 * parallel.cpp
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

#include "parallel.h"

// this code only applies to x86-based systems, I believe
#ifdef NATIVE_CPU_X86

// Code to control the parallel port

unsigned int par::m_uPortIdx = 0;
short par::m_base0[3] = { 0x378, 0x278, 0 };
short par::m_base2[3] = { 0x37A, 0x27A, 0 };

#include <stdio.h>
#include "conout.h"
#include "stdlib.h"

#ifdef WIN32

void _stdcall Out32(short PortAddress, short data);

bool par::init(unsigned int port, ILogger *pLogger)
// initializes parallel port for use
// port 0 is LPT1
// port 1 is LPT2
{
	if (port > 1) // if port is not LPT1 or LPT2, a custom address for the port was specified
	{
		m_base0[2] = port;
		m_base2[2] = port + 2;
		port = 2; // so that m_uPortIdx becomes 2
	}

	m_uPortIdx = port;

	char sAddr[81];
	sprintf(sAddr, "%x", m_base0[m_uPortIdx]);
	string s = "Opening parallel port at address 0x";
	s += sAddr;
	pLogger->Log(s);

	return(true);
}

// writes a byte to the port at base+0
void par::base0 (unsigned char data)
{
	Out32(m_base0[m_uPortIdx], data);
}

// writes a byte to the port at base+2
void par::base2 (unsigned char data)
{
	Out32(m_base2[m_uPortIdx], data);
}

void par::close(ILogger *pLogger)
{
	// does nothing with current win32 implementation
	pLogger->Log("Closing parallel port");
}

#endif

// Linux code below

#ifdef UNIX

#ifdef LINUX

#include <sys/io.h>

#endif


//FreeBSD code 
#ifdef FREEBSD

#include <machine/sysarch.h>
#include <sys/types.h>
#include <machine/cpufunc.h>
#define ioperm i386_set_ioperm

#endif


// initializes the specified port (0 is LPT1)
// returns 1 if successful, 0 if failted
bool par::init(unsigned int port, ILogger *pLogger)
{

	bool result = false;
	char s[81] = { 0 };

	sprintf(s, "Opening parallel port %d", port);
	pLogger->Log(s);

	if (port > 1) // if port is not LPT1 or LPT2, a custom address for the port was specified
	{
		port = 2; // set port to the custom address
		par::m_base0[2] = port;
		par::m_base2[2] = port + 2;
	}

	// request access to the small range of ports we need
	// if we get a 0, it means we were successful
	if (ioperm(par::m_base0[port], 3, 1) == 0)
	{
		m_uPortIdx = port;
		result = true;
	}

	return(result);
}

// writes a byte to the port at base+0
void par::base0 (unsigned char data)
{
	outb(data, par::m_base0[m_uPortIdx]);
}

// writes a byte to the port at base+2
void par::base2 (unsigned char data)
{
	outb(data, par::m_base2[m_uPortIdx]);
}

// closes parallel port
void par::close (ILogger *pLogger)
{
	// we don't have to do anything here
}

#endif	// end UNIX
#else	// end NATIVE_CPU_X86

// here is the code for systems that have no parallel support
bool par::init (unsigned int port, ILogger *pLogger)
{
	return false;
}

void par::base0(unsigned char data)
{
}

void par::base2(unsigned char data)
{
}

void par::close(ILogger *pLogger)
{
}
#endif
