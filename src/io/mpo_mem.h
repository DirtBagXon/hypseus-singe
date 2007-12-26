#ifndef MPO_MEM_H
#define MPO_MEM_H

#include <SDL.h>	// for endian def

// mpo_mem.h
// by Matt Ownby

// some memory functions/macros to make memory management more manageable :)

// does C++ style allocation ...
#define MPO_MALLOC(bytes) new unsigned char[bytes]

// only frees memory if it hasn't already been freed
#define MPO_FREE(ptr) if (ptr != 0) { delete [] ptr; ptr = 0; }

////////////////

// Endian-macros

// LOAD_LIL_SINT16: loads little-endian 16-bit value
//  Usage: Sint16 val = LOAD_LIL_SINT16(void *ptr);
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
// little endian version
#define LOAD_LIL_SINT16(ptr) (Sint16) *((Sint16 *) (ptr))
#else
// big endian version
#define LOAD_LIL_SINT16(ptr) (Sint16) (*((Uint8 *) (ptr))	| ((*((Uint8 *) (ptr)+1)) << 8))
#endif

// LOAD_LIL_UINT32: loads little-endian 32-bit value
//  Usage: Uint32 val = LOAD_LIL_UINT32(void *ptr);
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
// little endian version
#define LOAD_LIL_UINT32(ptr) (Uint32) *((Uint32 *) (ptr))
#else
// big endian version
#define LOAD_LIL_UINT32(ptr) (Uint32) (*((Uint8 *) (ptr)) | ((*((Uint8 *) (ptr)+1)) << 8) | ((*((Uint8 *) (ptr)+2)) << 16) | ((*((Uint8 *) (ptr)+3)) << 24))
#endif

// STORE_LIL_UINT32: stores 32-bit unsigned 'val' to 'ptr' in little-endian format
//  Usage: STORE_LIL_UINT32(void *ptr, Uint32 val);
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define STORE_LIL_UINT32(ptr,val) *((Uint32 *) (ptr)) = (val)
#else
#define STORE_LIL_UINT32(ptr,val) *((Uint8 *) (ptr)) = (val) & 0xFF; \
	*(((Uint8 *) (ptr))+1) = ((val) >> 8) & 0xFF; \
	*(((Uint8 *) (ptr))+2) = ((val) >> 16) & 0xFF; \
	*(((Uint8 *) (ptr))+3) = ((val) >> 24) & 0xFF
#endif


#endif // MPO_MEM_H
