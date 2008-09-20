/*
 * dll.h
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

// dll.h
// by Matt Ownby

// just has some macros to make DLL loading uniform across different OS's

#ifdef WIN32
#include <windows.h>	// for DLL loading
#else
#include <dlfcn.h>	// for .so loading
// Mac OSX can get this from http://www.opendarwin.org/projects/dlcompat/
#endif

// macros for making the dynamic library code look cleaner
#ifdef WIN32
#define M_LOAD_LIB(libname)	LoadLibrary(#libname)
#define M_GET_SYM	GetProcAddress
#define M_FREE_LIB	FreeLibrary
#else
#define M_LOAD_LIB(libname)	dlopen("lib" #libname ".so", RTLD_NOW)
#define M_GET_SYM	dlsym
#define M_FREE_LIB	dlclose
#endif

#ifdef WIN32
#define DLL_INSTANCE HINSTANCE
#else
#define DLL_INSTANCE void *
#endif
