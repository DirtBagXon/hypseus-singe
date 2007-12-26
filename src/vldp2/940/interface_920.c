#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "interface_940.h"

// this is our shared vars structure
struct libmpeg2_shared_vars_s g_SharedVars;

struct libmpeg2_shared_vars_s *g_pShared = NULL;

#ifndef GP2X
#include <pthread.h>
pthread_t g_thread940;
#endif

// check for incoming messages from cpu2
void mp2_920_think()
{
	// if CPU2 has an incoming frame for us ...
	if (g_pShared->uAckFrameCount != g_pShared->uReqFrameCount)
	{
		if ((g_pShared->uReqFrameCount % 30) == 0)
		{
			printf("TODO : received incoming frame %u\n", g_pShared->uReqFrameCount);
		}

		// acknowledge that we received the frame
		g_pShared->uReqFrameCount = g_pShared->uAckFrameCount;
	}

	// if we have a log message coming in ...
	if (g_pShared->uAckLogCount != g_pShared->uReqLogCount)
	{
		// TODO : store this somewhere else
		printf("CPU2 LOG: %s\n", g_pShared->logStr);

		// tell cpu2 that we received the log message
		g_pShared->uReqLogCount = g_pShared->uAckLogCount;
	}

	assert(g_pShared->uCorruptTest == 55);	// make sure buffer hasn't overflowed
}

// sleep function that also thinks to ensure that we don't get into a "deadlock" situation with the other cpu
void mp2_920_sleep_ms(unsigned int uMs)
{
	mp2_920_think();
	usleep(1000 * uMs);
}

void mp2_920_send_cmd(int cmd)
{
	g_pShared->uReqCmd = cmd;

	// increase counter to let cpu2 know we have a new command ...
	g_pShared->uReqCount++;

	// wait for cpu2 to accept our command
	while (g_pShared->uAckCount != g_pShared->uReqCount)
	{
		mp2_920_sleep_ms(1);
	}
}

void mp2_920_decode_mpeg(unsigned char *pBufStart, unsigned int uBufLength)
{
	if (uBufLength > INBUFSIZE)
	{
		fprintf(stderr, "decode_mpeg uBufLength is too big\n");
		return;
	}

	// if we need to wait for cpu2 to finish with a buffer before we pass more schlop into it
	while ((g_pShared->uReqBufCount - g_pShared->uAckBufCount) > 1)
	{
		printf("Waiting for cpu2 to finish with the buffer...\n");	// debug
		mp2_920_sleep_ms(1);
	}

	// use an unused buffer
	g_pShared->uReqInBuf++;
	g_pShared->uReqInBuf = g_pShared->uReqInBuf % INBUFCOUNT;	// wraparound
	g_pShared->uReqBufCount++;

	// copy mpeg chunk into buffer for cpu2
	memcpy(g_pShared->inBuf[g_pShared->uReqInBuf], pBufStart, uBufLength);

	// tell cpu2 how big this buffer is
	g_pShared->uInBufSize[g_pShared->uReqInBuf] = uBufLength;

	mp2_920_send_cmd(C940_LIBMPEG2_DECODE);	// do it!
}

void mp2_920_reset_mpeg()
{
	mp2_920_send_cmd(C940_LIBMPEG2_RESET);	// do it!
}

#ifndef GP2X
extern void *mp2_940_start(void *ignored);
#endif // GP2X

int mp2_920_init()
{
	int iSuccess = 0;

#ifdef GP2X
	// TODO : set g_pShared to a memory location
#else
	// for thread testing
	g_pShared = &g_SharedVars;
#endif // GP2X

	// clear entire structure to 0
	memset(g_pShared, 0, sizeof(struct libmpeg2_shared_vars_s));
	printf("shared variables have been cleared\n");
	g_pShared->uCorruptTest = 55;	// make sure that the buffer is not overflowed

#ifdef GP2X
	// TODO : cpu initialization stuff ...
#else
	// start 2nd "cpu" (using threads for debugging)
	if (pthread_create(&g_thread940, NULL, mp2_940_start, 0) == 0)
	{
		iSuccess = 1;
	}
	else
	{
		fprintf(stderr, "Thread creation failed\n");
	}
#endif // GP2X

	return iSuccess;
}

void mp2_920_shutdown()
{
	mp2_920_send_cmd(C940_QUIT);	// send quit message
}

