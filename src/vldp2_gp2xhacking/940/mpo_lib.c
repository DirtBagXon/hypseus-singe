#include <mpo_lib.h>

static char szHex[16];	// holds result of UToHexStr

#ifdef GP2X
extern unsigned char *g_au8MpoHeap;	// define this in startup to be (shared_mem_start - HEAPSIZE)
#else
static unsigned char g_au8MpoHeap[HEAPSIZE];
#endif // GP2X

unsigned int g_uHeapIndex = 0;	// the position in the heap that we're at

void *mpo_memset(void *s, int c, unsigned int n)
{
	unsigned int u = 0;
	unsigned char *ptr = (unsigned char *) s;

	for (u = 0; u < n; u++)
	{
		ptr[u] = c;
	}

	return s;
}

// accurate, unoptimized memcpy
// (n must be a multiple of 4)
void *mpo_memcpy(void *dest, const void *src, unsigned int n)
{
	unsigned int u = 0;
	unsigned int *p32src = (unsigned int *) src;
	unsigned int *p32dst = (unsigned int *) dest;
	n = n >> 2;	// 4 bytes at a time

	while (u < n)
	{
		*p32dst = *p32src;
		u++;
		p32src++;
		p32dst++;
	}

	return dest;
}

int mpo_memcmp(const void *s1, const void *s2, unsigned int n)
{
	int iResult = 0;
	unsigned int u = 0;
	const unsigned char *ps1 = (const unsigned char *) s1;
	const unsigned char *ps2 = (const unsigned char *) s2;

	for (u = 0; u < n; u++)
	{
		if (ps1[u] < ps2[u])
		{
			iResult = -1;
			break;
		}
		else if (ps1[u] > ps2[u])
		{
			iResult = 1;
			break;
		}
		// else keep comparing ...
	}

	return iResult;
}

void *mpo_malloc(unsigned int size)
{
	void *result = NULL;

	// if we still have some room on the heap ...
	if ((g_uHeapIndex + size) < HEAPSIZE)
	{
		result = g_au8MpoHeap + g_uHeapIndex;
		g_uHeapIndex += size;
	}

	return result;
}

void *mpo_memalign(unsigned int boundary, unsigned int size)
{
	while (dzzIntMod(g_uHeapIndex, boundary) != 0)
	{
		g_uHeapIndex++;
	}

	return mpo_malloc(size);
}

void mpo_free(void *ptr)
{
	// in our primitive scheme, memory cannot be freed
}

// free all allocated memory on the "heap" :o
void mpo_free_all()
{
	g_uHeapIndex = 0;
}

//// STRING FUNCTIONS ////

char *UToHexStr(unsigned int u)
{
	const char *DIGITS = "0123456789ABCDEF";
	unsigned int uIdx = 8;	// start at the last digit+1 (0-7, can't be longer than 8 digits for a 32-bit value)
	// +1 since we immediately decrement in the loop

	szHex[uIdx] = 0;	// null terminate
	do
	{
		uIdx--;
		szHex[uIdx] = DIGITS[u & 0xF];
		u = u >> 4;	// divide by 16
	} while ((u != 0) && (uIdx > 0));

	return &szHex[uIdx];
}

//// MATH FUNCTIONS ////

extern int Divide(int numerator, int denominator);
extern int Mod(int number, int modby);

// I snagged these from dzz's code, so I may as well give him/her credit :)
int dzzIntDivide(int numerator, int denominator)
{
#ifdef GP2X
	return Divide(numerator, denominator);
#else
	return numerator / denominator;
#endif // GP2X
}

int dzzIntMod(int number, int modby)
{
#ifdef GP2X
	return Mod(number, modby);
#else
	return number % modby;
#endif // GP2X
}

