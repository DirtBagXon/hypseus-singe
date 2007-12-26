#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>	// for memset

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

unsigned char *g_pMem = NULL;
unsigned int *g_p32Blitter = NULL;
unsigned int *g_pRegs32 = NULL;
unsigned short *g_pRegs16 = NULL;

#define UPPER_MEM_START 0x2000000

void blit_test()
{
	unsigned int uImageSize = 320 * 240 * 2;
	memset(g_pMem, uImageSize, uImageSize);	// fill destination with all 1's

	memset(g_pMem + uImageSize, 2, 320 * 240 * 2);	// fill source with all 2's

	//Wait till the blitter is ready
	while (g_p32Blitter[0x0034 >> 2] & 1) usleep(1);

	//Set the destination as 16bit and enable it for use.
	g_p32Blitter[0x0000 >> 2] = (1 << 5) | (1 << 6);

	//Set the address of destination
	g_p32Blitter[0x0004 >> 2] = UPPER_MEM_START;

	//Set the pitch of destination in bytes.
	g_p32Blitter[0x0008 >> 2] = 320*2;

	//Set a 16bit source, enable source and say the source is not controlled by CPU(?)
	g_p32Blitter[0x000C >> 2] = (1 << 8) | (1 << 7) | (1 << 5);

	//Set the source address
	g_p32Blitter[0x0010 >> 2] = UPPER_MEM_START + 320*2*240;

	//Set the pitch of source in bytes
	g_p32Blitter[0x0014 >> 2] = 320*2;

	//Do nothing with patern
	g_p32Blitter[0x0020 >> 2] = 0;

	//Set the size 320 by 240
	g_p32Blitter[0x002C >> 2] = (240 << 16) | (320 << 0);

	//Clear the source input FIFO, positive X,Y. And do a copy ROP.
	g_p32Blitter[0x0030 >> 2] = (1 << 10) | (1 << 9) | (1 << 8) | 0xCC;

	//Make the blitter run.
	g_p32Blitter[0x0034 >> 2] = 0x0001;

	// Wait for blitter to finish
	while (g_p32Blitter[0x0034 >> 2] & 1) usleep(1);

	if (memcmp(g_pMem, g_pMem + uImageSize, uImageSize) == 0)
	{
		printf("It worked!!!\n");
	}
	else
	{
		printf("Mismatch!  The dest image's first byte is %u\n", g_pMem[0]);
	}
}

int main(int argc, char **argv)
{
	int iResult = 0;

	int memfd;
	memfd = open("/dev/mem", O_RDWR);

	g_pRegs32 = (unsigned long*) mmap(0, 0x10000, PROT_READ|PROT_WRITE, MAP_SHARED, memfd, 0xc0000000);
	g_pRegs16 = (unsigned short *) g_pRegs32;

	g_p32Blitter = (unsigned long*)mmap(0, 0x100, PROT_READ|PROT_WRITE, MAP_SHARED, memfd, 0xE0020000);

	g_pMem = (unsigned char*)mmap(0, 0x2000000, PROT_READ | PROT_WRITE, MAP_SHARED,
								  memfd, 0x2000000); //mmap the upper 32MB.

	// enable all video and graphic devices
	g_pRegs16[0x090a >> 1] = 0xFFFF;
	// enable fastio
	g_pRegs16[0x0904 >> 1] |= (1 << 10);

	blit_test();

	munmap(g_pMem, 0x2000000);
	munmap(g_pg_p32Blitter, 0x100);
	munmap(g_pRegs32, 0x10000);

	return iResult;
}
