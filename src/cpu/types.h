/*
** Nofrendo (c) 1998-2000 Matthew Conte (matt@conte.com)
**
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of version 2 of the GNU Library General 
** Public License as published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
**
**
** types.h
**
** Data type definitions
** $Id$
*/

#ifndef _TYPES_H_
#define _TYPES_H_

#include <SDL.h>	// for endian detection

// MPO : added this SDL endian stuff so no source code changes are necessary
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
/* Define this if running on little-endian (x86) systems */
#define HOST_LITTLE_ENDIAN
#endif

// MPO : translate our convention to Nofrendo's convention
#ifdef NATIVE_CPU_PPC
#define TARGET_CPU_PPC
#endif

// MPO : for xcode on OSX, defining NATIVE_CPU_PPC manually is difficult, but gcc defines _ppc_ automatically
#ifdef __ppc__
#define TARGET_CPU_PPC
#endif

/* These should be changed depending on the platform */
typedef  char     int8;
typedef  short    int16;
typedef  int      int32;

typedef  unsigned char  uint8;
typedef  unsigned short uint16;
typedef  unsigned int   uint32;

typedef  uint8    boolean;

#ifndef  TRUE
#define  TRUE     1
#endif
#ifndef  FALSE
#define  FALSE    0
#endif

#endif /* _TYPES_H_ */

/*
** $Log$
** Revision 1.4  2006/08/30 15:37:55  matt
** added an extra #define for __ppc__ to accomodate xcode on OSX
**
** Revision 1.3  2005/08/03 23:37:15  matt
** made minor changes to support Mac OSX (PPC)
**
** Revision 1.2.4.1  2005/08/03 23:04:37  matt
** made minor changes to support Mac OSX (PPC)
**
** Revision 1.2  2003/01/02 21:05:46  matt
**
**
** made it so the endianness is detected automatically (via SDL.h)
**
** Revision 1.1  2001/10/31 19:26:38  markb
** 6502 core from nofrendo
**
** Revision 1.6  2000/06/09 15:12:25  matt
** initial revision
**
*/
