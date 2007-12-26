#include <stdio.h>
#include <stdlib.h>	// for exit
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "interface_940.h"
#include "interface_920.h"

#ifndef GP2X
#include <pthread.h>
pthread_t g_thread940;
// this is our shared vars structure
struct libmpeg2_shared_vars_s g_SharedVars;
#else
// GP2X stuff here

const unsigned int CPU2_MEM_START = 0x3000000;
const unsigned int SHARED_MEM_START = 0x400000;

// for open and mmap
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#endif

struct libmpeg2_shared_vars_s *g_pShared = NULL;

// user-defined callbacks
const struct interface_920_callbacks_s *g_pCallbacks = NULL;

// whether the resolution announce callback has been called
unsigned int g_bResAnnounced = 0;

// whether we're initialized
unsigned int g_bInitialized = 0;

#ifdef GP2X
int g_fdMem;
static unsigned char *g_pSharedMemory = 0;
volatile unsigned short *g_p16Regs;
volatile unsigned int *g_p32Blitter;
#endif // GP2X

// debug function
void mp2_920_print_shared()
{
	printf("uReqCmd : %u, uReqCount : %u, uReqBufCount: %u, uReqFrameCount: %u\n",
		   g_pShared->uReqCmd, g_pShared->uReqCount, g_pShared->uReqBufCount,
		   g_pShared->uReqFrameCount);

	printf("uReqLogCount : %u, uReqTestDelay : %u\n",
		   g_pShared->uReqLogCount, g_pShared->uReqTestDelay);

	printf("uAckCount : %u, uAckBufCount : %u, uAckOutBuf : %u, uAckFrameCount : %u, uAckLogCount : %u\n",
		   g_pShared->uAckCount, g_pShared->uAckBufCount, g_pShared->uAckOutBuf, g_pShared->uAckFrameCount,
		   g_pShared->uAckLogCount);
	printf("uCpu2Running : %u, uCorruptTest : %u\n", g_pShared->uCPU2Running, g_pShared->uCorruptTest);
}

#ifdef GP2X
inline unsigned int mp2_920_get_yuy2_addr()
{
	unsigned int uInBufIdx = (g_pShared->uReqFrameCount + 1) & OUTBUFMASK;

	// absolute (non-virtual) source address
	unsigned int uSrcAddr = ((unsigned int) &g_pShared->outBuf[uInBufIdx][0][0]) -
		((unsigned int) g_pSharedMemory) + CPU2_MEM_START;

	/*
	printf("outbuf's address is %x, g_pSharedMemory is %x, SHARED_MEM_START is %x, CPU2_MEM_START is %x\n",
		   ((unsigned int) &g_pShared->outBuf[uInBufIdx][0][0]),
		   g_pSharedMemory, SHARED_MEM_START, CPU2_MEM_START);
	printf("DBG: uSrcAddr is %x\n", uSrcAddr);
	*/

	return uSrcAddr;
}

void mp2_920_start_fast_yuy2_blit(unsigned int uDestAddr)
{
	// absolute (non-virtual) source address
	unsigned int uSrcAddr = mp2_920_get_yuy2_addr();

	// wait for blitter to not be busy
	//while (mp2_920_is_blitter_busy());

	//Set the destination as 16bit, enable ReadOperation (whatever that means)
	g_p32Blitter[0x0000 >> 2] = (1 << 5) | (1 << 6);

	//Set the address of destination
	g_p32Blitter[0x0004 >> 2] = uDestAddr;
//	printf("DBG: uDestAddr is %x\n", uDestAddr);

	// Set the pitch of destination in bytes.
	// (since it's YUY2, it's 2 bytes per pixel)
	g_p32Blitter[0x0008 >> 2] = g_pShared->uOutX << 1;
//	printf("uOutX << 1 is %x\n", g_pShared->uOutX << 1);

	//Set a 16bit source, enable source and say the source is not controlled by CPU(?)
	g_p32Blitter[0x000C >> 2] = (1 << 8) | (1 << 7) | (1 << 5);

	//Set the source address
	g_p32Blitter[0x0010 >> 2] = uSrcAddr;

	//Set the pitch of source in bytes
	// (since it's YUY2, it's 2 bytes per pixel)
	g_p32Blitter[0x0014 >> 2] = g_pShared->uOutX << 1;
//	printf("uOutX << 1 is %x\n", g_pShared->uOutX << 1);

	//Do nothing with patern
	g_p32Blitter[0x0020 >> 2] = 0;

	// set the image dimensions
	g_p32Blitter[0x002C >> 2] = (g_pShared->uOutY << 16) | g_pShared->uOutX;
//	printf("uOutY << 16 | uOutX is %x\n", (g_pShared->uOutY << 16) | g_pShared->uOutX);

	//Clear the source input FIFO, positive X,Y. And do a copy ROP.
	g_p32Blitter[0x0030 >> 2] = (1 << 10) | (1 << 9) | (1 << 8) | 0xCC;

//	printf("Blitter Regs: 0x34: %08x\n", g_p32Blitter[0x34 >> 2]);
//	printf("090A : %04x\n", g_p16Regs[0x090a>>1]);
//	printf("0904 : %04x\n", g_p16Regs[0x0904>>1]);

	//Make the blitter run.
	g_p32Blitter[0x0034 >> 2] = 0x0001;
}

unsigned int mp2_920_is_blitter_busy()
{
    return (g_p32Blitter[0x0034 >> 2] & 1);
}
#endif // GP2X

// check for incoming messages from cpu2
unsigned int mp2_920_think()
{
	// indicates whether any events were handled
	unsigned int uEventHandled = 0;

	// handle all incoming frames from cpu2 (there usually will be no more than 1)
	while (g_pShared->uAckFrameCount != g_pShared->uReqFrameCount)
	{
		// compute which buffer to show
		unsigned int uInBufIdx = (g_pShared->uReqFrameCount + 1) & OUTBUFMASK;

		// if the resolution has not been announced, then do that first
		if (!g_bResAnnounced)
		{
			g_pCallbacks->ResolutionCallback(g_pShared->uOutX, g_pShared->uOutY);
			g_bResAnnounced = 1;
		}

		// YUY2
		if (g_pShared->uReqYUY2)
		{
			struct yuy2_s yuy2;

			yuy2.YUY2 = g_pShared->outBuf[uInBufIdx][0];
			yuy2.uSize = g_pShared->uOutYSize << 1;
			g_pCallbacks->YUY2Callback(&yuy2);
		}
		// YV12
		else
		{
			struct yv12_s yv12;
			yv12.Y = g_pShared->outBuf[uInBufIdx][0];
			yv12.U = g_pShared->outBuf[uInBufIdx][1];
			yv12.V = g_pShared->outBuf[uInBufIdx][2];
			yv12.uYSize = g_pShared->uOutYSize;
			yv12.uUVSize = g_pShared->uOutUVSize;
			g_pCallbacks->YV12Callback(&yv12);
		}

		// acknowledge that we received (and processed) the frame
		g_pShared->uReqFrameCount++;
		uEventHandled = 1;
	}

	// handle all incoming log messages (usually will be no more than 1)
	while (g_pShared->uAckLogCount != g_pShared->uReqLogCount)
	{
		g_pCallbacks->LogCallback(g_pShared->logStr);

		// clear log message once we've got it
		// (this shouldn't be necessary but we're getting some WEIRD results)
//		memset(g_pShared->logStr, 0, sizeof(g_pShared->logStr));

		// tell cpu2 that we received the log message
		g_pShared->uReqLogCount = g_pShared->uAckLogCount;

//		mp2_920_print_shared();

		uEventHandled = 1;
	}

	assert(g_pShared->uCorruptTest == 55);	// make sure buffer hasn't overflowed

	return uEventHandled;
}

// sleep function that also thinks to ensure that we don't get into a "deadlock" situation with the other cpu
// returns non-zero if an event was handled during the think process
unsigned int mp2_920_sleep_ms(unsigned int uMs)
{
	unsigned int uEventHandled = mp2_920_think();
	usleep(1000 * uMs);
	return uEventHandled;
}

inline unsigned int mp2_920_get_mini_ticks()
{
#ifdef GP2X
	return ((unsigned int *) g_p16Regs)[0xA00 >> 2];
#else
	return (SDL_GetTicks() * 7373);	//convert to gp2x ticks hehe
#endif
}

void mp2_920_send_cmd(int cmd)
{
	unsigned int uStartTicks = mp2_920_get_mini_ticks();

	g_pShared->uReqCmd = cmd;

	// increase counter to let cpu2 know we have a new command ...
	g_pShared->uReqCount++;

	// wait for cpu2 to accept our command
	while (g_pShared->uAckCount != g_pShared->uReqCount)
	{
		unsigned int uTickDiff = mp2_920_get_mini_ticks() - uStartTicks;

		// if ~5 seconds have elapsed with no response, then this is bad ...
		if (uTickDiff > 36865000)
		{
			mp2_920_print_shared();
			printf("mp2_920_send_cmd() timeout on command %u, aborting\n", cmd);
			exit(1);
		}

		mp2_920_sleep_ms(1);

		// don't wait here if we aren't initialized, otherwise we may be stuck here forever
		if (!g_bInitialized)
		{
			break;
		}
	}

	printf("CPU1: command %u was successfully sent\n", cmd);
}

void mp2_920_decode_mpeg(unsigned char *pBufStart, unsigned char *pBufEnd)
{
	unsigned int uInBufIdx = 0;	// which buffer to overwrite
	unsigned int uStartTicks = mp2_920_get_mini_ticks();
	unsigned int uBufLength = pBufEnd - pBufStart;

	if (uBufLength > INBUFSIZE)
	{
		g_pCallbacks->LogCallback("CPU1: decode_mpeg uBufLength is too big");
		return;
	}

	// if we have to wait for cpu2
	if (g_pShared->uReqBufCount != g_pShared->uAckBufCount)
	{
		g_pShared->uReqBufStalls++;
	}
	// else if we don't have to wait for cpu2
	else
	{
		g_pShared->uReqBufNoStalls++;
	}

	// If we're already waiting for cpu2 to finish with the buffer we sent last time, then block
	// (this is actually ideal because it means we are being efficient and cpu2 doesn't have to wait for us)
	while (g_pShared->uReqBufCount != g_pShared->uAckBufCount)
	{
		// this timeout is STUPID!
		/*
		unsigned int uTickDiff = mp2_920_get_mini_ticks() - uStartTicks;

		// if ~5 seconds have elapsed with no response, then this is bad ...
		if (uTickDiff > 36865000)
		{
			mp2_920_print_shared();
			printf("mp2_920_decode_mpeg() timeout, aborting\n");
			exit(1);
		}
		*/

#ifndef GP2X
		// notify the unit test that we have indeed delayed as we were supposed to
		g_pShared->uReqTestDelay1Passed = 1;
#endif // GP2X

		// sleep.. if an event was handled during the sleep, then reset the startTicks so we don't timeout
		if (mp2_920_sleep_ms(1))
		{
			uStartTicks = mp2_920_get_mini_ticks();
		}
	}

	// compute which buffer to overwrite now
	// (we could've also done (uReqBufCount + 1) & 1 which would've given us the same result, but XOR is probably faster.
	uInBufIdx = (g_pShared->uReqBufCount ^ 1) & 1;

	// copy mpeg chunk into buffer for cpu2
	memcpy(g_pShared->inBuf[uInBufIdx], pBufStart, uBufLength);

	// tell cpu2 how big this buffer is
	g_pShared->uInBufSize[uInBufIdx] = uBufLength;

	// Notify CPU2 that we have a new buffer.
	g_pShared->uReqBufCount++;
}

void mp2_920_reset()
{
	mp2_920_send_cmd(C940_LIBMPEG2_RESET);	// do it!
	g_bResAnnounced = 0;	// now that we've reset, the resolution must be announced again
}

void mp2_920_SetDelay(unsigned int uEnabled)
{
	g_pShared->uReqTestDelay = uEnabled;
}

unsigned int mp2_920_GetDelayTest1Result()
{
	return g_pShared->uReqTestDelay1Passed;
}

#ifndef GP2X
extern void *mp2_940_start(void *ignored);
#endif // GP2X

#ifdef GP2X
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

	// set 940's memory to all 0's to compensate for some variables in the 940 binary not being initialized properly.
	memset(g_pSharedMemory, 0, SHARED_MEM_START);	// all the way up to the shared memory boundary

	// load 940 program
	nLen = 0;
	fp = fopen("libmpeg940.bin", "r");
	if(!fp)
	{
		g_pCallbacks->LogCallback("libmpeg940.bin could not be loaded");
		return;
	}
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

	g_pCallbacks->LogCallback("940 should now be running");
}

void Shutdown940()
{
	Reset940(1);
	Pause940(1);
}
#endif // GP2X

int mp2_920_init(const struct interface_920_callbacks_s *pCallbacks, unsigned int uUseYUY2)
{
	int iSuccess = 0;

#ifdef GP2X
	g_fdMem = open("/dev/mem", O_RDWR);

	g_p16Regs = (unsigned short *) mmap(0, 0x10000, 
			PROT_READ|PROT_WRITE, MAP_SHARED, g_fdMem, 0xc0000000);

	g_p32Blitter = (unsigned int *)mmap(0, 0x100, PROT_READ|PROT_WRITE, MAP_SHARED, g_fdMem, 0xE0020000);

	g_pSharedMemory = (unsigned char *) mmap(0, 0xF80000, 
			PROT_READ|PROT_WRITE, MAP_SHARED, g_fdMem,  CPU2_MEM_START);

    // enable all video and graphic devices
	g_p16Regs[0x090a>>1] = 0xFFFF;

	// enable fastio
	g_p16Regs[0x0904>>1] |= (1 << 10);

	// the shared memory variable area lives at g_pSharedMemory + SHARED_MEM_START
	g_pShared = (struct libmpeg2_shared_vars_s *) (g_pSharedMemory + SHARED_MEM_START);

#else
	// for thread testing
	g_pShared = &g_SharedVars;
#endif // GP2X

	// store callbacks
	g_pCallbacks = pCallbacks;
	g_bResAnnounced = 0;	// the resolution needs to be announced at least once

	// clear entire structure to 0
	/*
	memset(g_pShared, 0, sizeof(struct libmpeg2_shared_vars_s) - sizeof(g_pShared->logStr) -
		   sizeof(g_pShared->inBuf) - sizeof(g_pShared->outBuf));
		   */
	memset(g_pShared, 0, sizeof(struct libmpeg2_shared_vars_s));
	g_pShared->uCorruptTest = 55;	// make sure that the buffer is not overflowed

	g_pShared->uReqYUY2 = uUseYUY2;

	assert(g_pShared->uCPU2Running == 0);
	assert(g_pShared->uAckLogCount == 0);
	assert(g_pShared->uAckFrameCount == g_pShared->uReqFrameCount);
	assert(g_pShared->uAckBufCount == g_pShared->uReqBufCount);

#ifdef GP2X
	Startup940();

	while (g_pShared->uCPU2Running == 0)
	{
		printf("Waiting for CPU2 to respond...\n");
		usleep(1000 * 1000);
	}
	printf("CPU2 has responded!\n");

	iSuccess = 1;
#else
	// start 2nd "cpu" (using threads for debugging)
	if (pthread_create(&g_thread940, NULL, mp2_940_start, 0) == 0)
	{
		iSuccess = 1;
	}
	else
	{
		g_pCallbacks->LogCallback("Thread creation failed");
	}
#endif // GP2X

	g_bInitialized = iSuccess;

	// no need to sleep here because cpu2 has already responded
	//usleep(1000 * 1000);

	// debug ...
	mp2_920_print_shared();

	printf("CPU1: Trying to ping...\n");

	// try pinging CPU 2
	mp2_920_send_cmd(C940_NOP);

	printf("CPU1: Got ping response!\n");
	printf("CPU1: libmpeg2 initialization successful\n");

	return iSuccess;
}

void mp2_920_shutdown()
{
	mp2_920_send_cmd(C940_QUIT);	// send quit message
	g_bInitialized = 0;

	printf("Performance Summary:\n");
	printf("Buffer GoodStalls: %6u Bad NoStalls: %6u\n", g_pShared->uReqBufStalls, g_pShared->uReqBufNoStalls);
	printf("Frame      Stalls: %6u     NoStalls: %6u\n", g_pShared->uAckFrameStalls, g_pShared->uAckFrameNoStalls);
	printf("Frame stalls are bad if framerate is too slow, but ok if framerate is maxed out.\n");

#ifdef GP2X
	Shutdown940();

	if(g_pSharedMemory)
	{
		munmap((void *) g_pSharedMemory, 0xF80000);
	}
	g_pSharedMemory = 0;

	if (g_p16Regs)
	{
		munmap((void *) g_p16Regs, 0x10000);
	}
	g_p16Regs = 0;

	if (g_p32Blitter)
	{
		munmap((void *) g_p32Blitter, 0x100);
	}
	g_p32Blitter = 0;

	close(g_fdMem);
#endif // GP2X

	g_pCallbacks->LogCallback("libmpeg2 shutdown complete");
}

