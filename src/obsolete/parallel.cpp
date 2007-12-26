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

// this code only applies to x86-based systems, I believe
#ifdef NATIVE_CPU_X86

// Code to control the parallel port

unsigned short g_par_base0[2] = { 0x378, 0x278 };	// base port address
unsigned short g_par_base2[2] = { 0x37A, 0x27A };	// base+2 port address

#include <stdio.h>
#include "conout.h"
#include "parallel.h"

#ifdef WIN32

#include <windows.h>

HANDLE lpt_handle[2] = { 0 };

bool par_init(int port)
// initializes parallel port for use
// port 0 is LPT1
// port 1 is LPT2
// returns a 1 if successful, 0 if error
{

	bool result = false;
	char portname[5] = { 0 };

	// make sure port is within our range (see handle declaration)
	if ((port >=0) && (port <=1))
	{

		char s[81] = { 0 };
		sprintf(s, "Opening parallel port %d", port);
		printline(s);

		sprintf(portname, "LPT%d", port+1);

		lpt_handle[port] = CreateFile(portname,
			GENERIC_READ|GENERIC_WRITE,
			0,NULL,
			OPEN_EXISTING,NULL,NULL);

		if(lpt_handle[port] == INVALID_HANDLE_VALUE)
		{
			 printline("Failed to open parallel port!");
		}
		else
		{
			result = true;
		}
	}
	else
	{
		printline("Parallel port specified is out of range!");
	}

	return(result);

}

// writes a byte to the port at base+0
int par_base0 (int port, unsigned char data)
{
	int result = 0;

	if ((port >=0) && (port <= 1))
	{
		result = 1;

		asm_par_base = g_par_base0[port];
		asm_par_data = data;
		asm_par_write();
/*
		unsigned short base = g_par_base0[port];

		_asm
		{
			push ax
			push dx
			mov dx,base
			mov al,data;
			out dx,al
			pop dx
			pop ax
		} //end inline assembly
*/
	}

	return (result);
}

// writes a byte to the port at base+0
int par_base2 (int port, unsigned char data)
{
	int result = 0;

	if ((port >=0) && (port <= 1))
	{
		result = 1;

		asm_par_base = g_par_base2[port];
		asm_par_data = data;
		asm_par_write();

/*
		unsigned short base = g_par_base2[port];
		_asm
		{
			push ax
			push dx
			mov dx,base
			mov al,data;
			out dx,al
			pop dx
			pop ax
		} //end inline assembly
*/
	}

	return(result);

}

void par_close(int port)
// closes the open parallel port
{

	char s[81] = { 0 };

	sprintf(s, "Closing parallel port %d...", port);
	printline(s);
	CloseHandle(lpt_handle[port]);

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
bool par_init (int port)
{

	bool result = false;
	char s[81] = { 0 };

	sprintf(s, "Opening parallel port %d", port);
	printline(s);

	// make sure requested port is in the proper range
	if ((port >= 0) && (port <= 1))
	{
		// request access to the small range of ports we need
		// if we get a 0, it means we were successful
		if (ioperm(g_par_base0[port], 3, 1) == 0)
		{
			result = true;
		}
	}
	else
	{
		printline("Error: Parallel port is out of range!");
	}
	return(result);
}

// writes a byte to the port at base+0
int par_base0 (int port, unsigned char data)
{
	if ((port >=0) && (port <=1))
	{
		outb(data, g_par_base0[port]);
	}
	return(1);
}

// writes a byte to the port at base+2
int par_base2 (int port, unsigned char data)
{
	if ((port >=0) && (port <= 1))
	{
		outb(data, g_par_base2[port]);
	}
	return(1);
}

// closes parallel port
void par_close (int port)
{
	// we don't have to do anything here
}

#endif	// end UNIX
#else	// end NATIVE_CPU_X86

// here is the code for systems that have no parallel support
bool par_init (int port)
{
	return false;
}

int par_base0(int port, unsigned char data)
{
	return 0;
}

int par_base2(int port, unsigned char data)
{
	return 0;
}

void par_close(int port)
{
}
#endif
