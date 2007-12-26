#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#include "940/940.h"

int g_fdMem;
static unsigned char *g_pSharedMemory = 0;
static SharedVariables *g_pShared;

volatile unsigned short *g_p16Regs;

//////////////////////////////////////////////////////////////////

void Pause940(int iDoPause)
{
	if (iDoPause)
	{
		g_p16Regs[0x0904>>1] &= 0xFFFE;
	}
	else
	{
		g_p16Regs[0x0904>>1] |= 1;
	}
} 

void Reset940(int iYesReset)
{
	g_p16Regs[0x3B48>>1] = ((iYesReset&1) << 7) | (0x03);
}

void Startup940()
{
	int nLen, nRead;
	FILE *fp;
	unsigned char ucData[1000];

	Reset940(1);
	Pause940(1);
	g_p16Regs[0x3B40>>1] = 0;
	g_p16Regs[0x3B42>>1] = 0;
	g_p16Regs[0x3B44>>1] = 0xffff;
	g_p16Regs[0x3B46>>1] = 0xffff;

	// load code940.bin
	nLen = 0;
	fp = fopen("code940.bin", "r");
	if(!fp)
		return;
	while(1)
	{
		nRead = fread(ucData, 1, 1000, fp);
		if(nRead <= 0)
			break;
		memcpy(g_pSharedMemory + nLen, ucData, nRead);
		nLen += nRead;
	}
	fclose(fp);
	
	Reset940(0);
	Pause940(0);
}

void Shutdown940()
{
	Reset940(1);
	Pause940(1);
}

void main_loop()
{
	printf("Attempting to set new command ...\n");
	memset(g_pShared->inBuf, 0, sizeof(g_pShared->inBuf));	// set input buf all to 0's ...
	g_pShared->g_uReqCmd = CMD_FLIP_BUF;	// ask CPU #2 to flip the buffer
	g_pShared->g_uCmdCount++;	// do it!

	printf("Waiting for CPU #2 to complete the operation ...\n");

	while (g_pShared->g_uAckCount != g_pShared->g_uReqCmd)
	{
		usleep(1000);
	}

	// it's done!
	printf("Done!\n");

	if (g_pShared->outBuf[0] == 0xFF)
	{
		printf("Well the first byte is correct, at least...\n");
	}
	else
	{
		printf("Doh!  First byte is wrong! it's %x\n", g_pShared->outBuf[0]);
	}
}

int main(int argc, char *argv[])
{
	g_fdMem = open("/dev/mem", O_RDWR);

	g_p16Regs = (unsigned short *) mmap(0, 0x10000, 
			PROT_READ|PROT_WRITE, MAP_SHARED, g_fdMem, 0xc0000000);

	g_pSharedMemory = (unsigned char *) mmap(0, 0xF80000, 
			PROT_READ|PROT_WRITE, MAP_SHARED, g_fdMem, 0x3000000);

	// the shared memory variable area lives at g_pSharedMemory + 0x200000
	g_pShared = (SharedVariables *) (g_pSharedMemory + 0x200000);

	g_pShared->g_uCmdCount = g_pShared->g_uAckCount = 0;	// make sure these are equal before we start anything ...

	Startup940();

	// main loop is here ...
	main_loop();

	Shutdown940();

	if(g_pSharedMemory)
	{
		munmap(g_pSharedMemory, 0xF80000);
	}
	g_pSharedMemory = 0;

	close(g_fdMem);
	return 0;
}

