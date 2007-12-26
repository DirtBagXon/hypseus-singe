// my_stdio.h
// by Matt Ownby

// common stuff that many .cpp files may need to use

#ifndef MY_STDIO_H
#define MY_STDIO_H

#ifdef WIN32

// we don't want WIN32 using stdio because
// a) it's not available on xbox
// b) it requires MSCVRT.DLL and VS.NET uses a new version that many people don't have
#include <windows.h>

// win32 doesn't have snprintf
#define snprintf _snprintf
#else

// non-win32 can just use regular stdio
#include <stdio.h>

#endif // WIN32

#include "mpo_fileio.h"

#endif // MY_STDIO_H

