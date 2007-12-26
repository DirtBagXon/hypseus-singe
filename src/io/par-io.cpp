/*
 * par-io.cpp
 *
 * Copyright (C) 2003 Brad Oldham
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
 *
 *  In addition, as a special exception, Brad Oldham gives permission to link 
 *  the code of this program with the par-io.dll library (or with modified versions of 
 *  par-io.dll that use the same license as par-io.dll), and distribute linked 
 *  combinations including the two.  You must obey the GNU General Public License in 
 *  all respects for all of the code used other than par-io.dll.  If you modify
 *  this file, you may extend this exception to your version of the
 *  file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.

Prototypes for use with par-io.dll (renamed from io.dll to avoid confusion):

void WINAPI PortOut(short int Port, char Data);
void WINAPI PortWordOut(short int Port, short int Data);
void WINAPI PortDWordOut(short int Port, int Data);
char WINAPI PortIn(short int Port);
short int WINAPI PortWordIn(short int Port);
int WINAPI PortDWordIn(short int Port);
void WINAPI SetPortBit(short int Port, char Bit);
void WINAPI ClrPortBit(short int Port, char Bit);
void WINAPI NotPortBit(short int Port, char Bit);
short int WINAPI GetPortBit(short int Port, char Bit);
short int WINAPI RightPortShift(short int Port, short int Val);
short int WINAPI LeftPortShift(short int Port, short int Val);
short int WINAPI IsDriverInstalled();
*/

#include "par-io.h"
#include "dll.h"

#ifdef WIN32
PORTOUT parportout;

HMODULE hpario;

void ParIODLLUnload() 
{
	M_FREE_LIB(hpario);
}

int ParIODLLLoad() 
{
	hpario = M_LOAD_LIB(par-io);
	if (hpario == NULL) return 1;

	parportout = (PORTOUT)M_GET_SYM(hpario, "PortOut");

	// we don't need this because if we've coded our stuff correctly, ParIODLLUnload
	//  will always be called before closing.
//	atexit(ParIODLLUnload);

	return 0;
}
#else
void ParIODLLUnload() 
{

}

int ParIODLLLoad() 
{
	return 0;
}
#endif