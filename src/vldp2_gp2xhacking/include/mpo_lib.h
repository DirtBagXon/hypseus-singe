#ifndef MPO_LIB_H
#define MPO_LIB_H

void *mpo_memset(void *s, int c, unsigned int n);
void *mpo_memcpy(void *dest, const void *src, unsigned int n);
int mpo_memcmp(const void *s1, const void *s2, unsigned int n);
void *mpo_memalign(unsigned int boundary, unsigned int size);
void mpo_free(void *ptr);
void mpo_free_all();
void *mpo_malloc(unsigned int size);
char *UToHexStr(unsigned int u);

int dzzIntDivide(int numerator, int denominator);
int dzzIntMod(int number, int modby);

// this is the bare minimum that we need to handle libmpeg2's dynamic memory allocation
// (the 0x200000 comes from 720 * 480 * 1.5 (yv12) * 3 buffers)
#define HEAPSIZE (0x12B000 + 0x200000)

//#ifdef GP2X
#if 1

#define MPO_MEMSET mpo_memset
#define MPO_MEMCMP mpo_memcmp
#define MPO_MEMALIGN mpo_memalign
#define MPO_FREE mpo_free
#define MPO_MALLOC mpo_malloc

#ifndef NULL
#define NULL 0
#endif // NULL

#else

// if we're not using the gp2x, we can use the standard headers
#include <stdlib.h>
#include <string.h>

#define MPO_MEMSET memset
#define MPO_MEMCMP memcmp

#endif //

#endif // MPO_LIB_H

