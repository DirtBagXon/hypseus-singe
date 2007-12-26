/*
 * vldp_940_interface.c
 *
 * Copyright (C) 2007 Matt Ownby
 *
 * This file is part of VLDP, a virtual laserdisc player.
 *
 * VLDP is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * VLDP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// contains the "thread" that will eventually run on another cpu (940)

#ifdef WIN32
#include "../vc++/inttypes.h"
#else
#include <inttypes.h>
#endif

#include "mpeg2.h"
#include "video_out.h"
#include "interface_940.h"
#include "mpo_lib.h"

////////////////////////////////////////////////

#ifndef NULL
#define NULL 0
#endif // NULL

static mpeg2dec_t *g_mpeg_data = NULL;	// structure for libmpeg2's state
static vo_instance_t *s_video_output = NULL;

#ifdef GP2X
// must not be static, so that the video outputer can find it via 'extern'
struct libmpeg2_shared_vars_s *g_pShared;
unsigned char *g_au8MpoHeap;
const unsigned int SHARED_MEM_START = 0x400000;
#else
extern struct libmpeg2_shared_vars_s *g_pShared;
#endif // GP2X

////////////////////////////////////////////////

void mpo_strncpy(char *dst, const char *src, unsigned int uLimit)
{
	unsigned int u = 0;

	while (u < uLimit)
	{
		dst[u] = src[u];

		// if we've reached the null terminator, then break
		if (src[u] == 0)
		{
			break;
		}

		u++;
	}

	if (u > 0)
	{
		// null terminate string
		dst[u-1] = 0;
	}
}

void mpo_strncat(char *dst, const char *src, unsigned int uLimit)
{
	unsigned int u = 0;
	for (;;)
	{
		// find the end
		while ((u < uLimit) && (dst[u] != 0))
		{
			u++;
		}

		break;
	}

	mpo_strncpy(dst + u, src, uLimit - u);
}

// for debugging
void do_setvar(unsigned int u)
{
	g_pShared->uCPU2Running = u;
}

int log_received()
{
	return (g_pShared->uReqLogCount == g_pShared->uAckLogCount);
}

// used to get around gcc's optimization bugs
int iDummy = 0;

// lockup in the event of a fatal error or failed test
void lockup()
{
	for (;;);
}

void do_error(const char *s)
{
	mpo_strncpy(g_pShared->logStr, s, sizeof(g_pShared->logStr));

	g_pShared->uAckLogCount++;

	return;	// TO SPEED THIS DANG THING UP

	// this loop has to be written this way so that gcc's optimizer doesn't mess us up
	do
	{
		#ifndef GP2X
		usleep(0);
		#endif // GP2X

		// The only reason we do this is because gcc's optimizer turns this into an unconditional endless loop if we don't.
		// Stupid compiler :)
		iDummy += 2;

	} while (!log_received());
}

/////////////////

// decode_mpeg2 function taken from mpeg2dec.c and optimized a bit
static void decode_mpeg2 (uint8_t * current, uint8_t * end)
{
    const mpeg2_info_t * info;
    int state;
    vo_setup_result_t setup_result;

//	do_error("about to call mpeg2_buffer");

    mpeg2_buffer (g_mpeg_data, current, end);

//	do_error("about to call mpeg2_info");

    info = mpeg2_info (g_mpeg_data);
	// loop until we return (state is -1)
	for (;;)
    {
//		do_error("about to call mpeg2_parse");
		state = mpeg2_parse (g_mpeg_data);
		switch (state)
		{
		case -1:
		    return;
		case STATE_SEQUENCE:
//			do_error("inside STATE_SEQUENCE, about to call s_video_output setup");
		    /* might set nb fbuf, convert format, stride */
		    /* might set fbufs */
		    if (s_video_output->setup (s_video_output, info->sequence->width,
				       info->sequence->height, &setup_result))
			{
				do_error("display setup failed");
		    }
	
		    if (setup_result.convert)
				mpeg2_convert (g_mpeg_data, setup_result.convert, NULL);
					    
		    // if this driver offers a setup_fbuf callback ...
//		    else if (s_video_output->setup_fbuf)
// we KNOW we offer a setup_fbuf function, so get rid of this conditional
		    {
				uint8_t * buf[3];
				void * id;

//				do_error("about to call s_video_output setup_fbuf 3 times");
				s_video_output->setup_fbuf (s_video_output, buf, &id);
				mpeg2_set_buf (g_mpeg_data, buf, id);
				s_video_output->setup_fbuf (s_video_output, buf, &id);
				mpeg2_set_buf (g_mpeg_data, buf, id);
				s_video_output->setup_fbuf (s_video_output, buf, &id);
				mpeg2_set_buf (g_mpeg_data, buf, id);
		    }
		    break;
		case STATE_PICTURE:
		    /* might skip */
		    /* might set fbuf */

			// all possible stuff we could've done here was removed because the null driver
			// doesn't do any of it
		    
		    break;
		case STATE_PICTURE_2ND:
		    /* should not do anything */
		    break;
		case STATE_SLICE:
		case STATE_END:
		    /* draw current picture */
		    /* might free frame buffer */
			// if the init hasn't been called yet, this may fail so we have to put the conditional
		    if (info->display_fbuf)
		    {
//				do_error("about to call s_video_out draw");
				s_video_output->draw (s_video_output, info->display_fbuf->buf,
				      info->display_fbuf->id);
		    }
		    
		    break;
		} // end switch
    } // end endless for loop
}

/////////////////

// returns C940_NO_COMMAND is no command is waiting
int mp2_940_peek_command()
{
	int iResult = C940_NO_COMMAND;

	// if a new command is waiting
	if (g_pShared->uAckCount != g_pShared->uReqCount)
	{
		iResult = g_pShared->uReqCmd;
	}

	return iResult;
}

// tell cpu1 that cpu2 has received the command
void mp2_940_ack_command()
{
	g_pShared->uAckCount = g_pShared->uReqCount;
}

void mp2_940_do_decode()
{
	// copy variables we need before we ack

	// uWhichBuf = the next buffer we haven't handled yet (uAckBufCount + 1) & 1, which is equivalent
	//  to (uAckBufCount ^ 1) & 1.  I think XOR is faster then ADD.
	unsigned int uWhichBuf = (g_pShared->uAckBufCount ^ 1) & 1;

	unsigned int uBufSize = g_pShared->uInBufSize[uWhichBuf];
	unsigned char *pBufStart = g_pShared->inBuf[uWhichBuf];
	unsigned char *pBufEnd = pBufStart + uBufSize;

	// safety test
	if (uBufSize != 0)
	{
#ifndef GP2X
		// if we're testing a delay, then wait...
		if (g_pShared->uReqTestDelay)
		{
			do_error("CPU2: Sleeping to test delay!");
			usleep(1000 * 1000);
		}
#endif // GP2X

//		do_error("CPU2: about to call decode_mpeg2");
		decode_mpeg2(pBufStart, pBufEnd);
//		do_error("CPU2: done calling decode_mpeg2");

		// tell cpu1 that we're done with the buffer
		g_pShared->uAckBufCount++;
	}
	else
	{
		do_error("mp2_940_do_decode: uBufSize is 0!  This can't be right...");
	}
}

void mp2_940_do_reset()
{
	char s[320];
	mpeg2_partial_init(g_mpeg_data);
	mp2_940_ack_command();

	mpo_strncpy(s, "g_mpeg_data address is: ", sizeof(s));
	mpo_strncat(s, UToHexStr(g_mpeg_data), sizeof(s));
	do_error(s);

}

// when building on x86/linux to test this stuff ...
extern struct libmpeg2_shared_vars_s g_SharedVars;

void mp2_940_setup_g_pShared()
{
#ifdef GP2X
	g_pShared = (struct libmpeg2_shared_vars_s *) SHARED_MEM_START;
    g_au8MpoHeap = (unsigned char*) (SHARED_MEM_START - HEAPSIZE);	// we need to make sure the instruction stuff is smaller than this :)
#else
	g_pShared = &g_SharedVars;
#endif // GP2X
}

void mp2_940_tester(int iTrue, const char *cpszMsg)
{
	if (!iTrue)
	{
		do_error(cpszMsg);
		for(;;);	// lockup in disgrace
	}
}

// EXTERNS
extern unsigned int g_uHeapIndex;
extern void * (* mpeg2_malloc_hook) (int size, int reason);
extern int (* mpeg2_free_hook) (void * buf);
//extern int mpeg2_accels;
extern const uint8_t default_intra_quantizer_matrix[64];
extern uint8_t mpeg2_scan_norm[64];
extern uint8_t mpeg2_scan_alt[64];
extern int non_linear_quantizer_scale [];	// from slice.c
extern unsigned int g_uDummy;	// for testing

unsigned int g_uTest1 = 2;
static unsigned int g_uTest2 = 3;

// apparently the global variable initializations aren't done statically, so we need to explicitly do them
void mp2_940_initialize_globals()
{
	const uint8_t mpeg2_scan_norm_values[64] =
	{
		/* Zig-Zag scan pattern */
		 0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
		12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
		35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
		58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
	};

	const uint8_t mpeg2_scan_alt_values[64] =
	{
		/* Alternate scan pattern */
		 0, 8,  16, 24,  1,  9,  2, 10, 17, 25, 32, 40, 48, 56, 57, 49,
		41, 33, 26, 18,  3, 11,  4, 12, 19, 27, 34, 42, 50, 58, 35, 43,
		51, 59, 20, 28,  5, 13,  6, 14, 21, 29, 36, 44, 52, 60, 37, 45,
		53, 61, 22, 30,  7, 15, 23, 31, 38, 46, 54, 62, 39, 47, 55, 63
	};

	unsigned int u = 0;

//	g_uHeapIndex = 0;
	g_mpeg_data = 0;	// structure for libmpeg2's state
	s_video_output = 0;
	mpeg2_malloc_hook = 0;
	mpeg2_free_hook = 0;
//	mpeg2_accels = 0;

	// reset these arrays to default state
	for (u = 0; u < 64; u++)
	{
		mpeg2_scan_norm[u] = mpeg2_scan_norm_values[u];
		mpeg2_scan_alt[u] = mpeg2_scan_alt_values[u];
	}

	// do a bunch of tests to make sure all global variables are initialized correctly
	mp2_940_tester(g_uDummy == 3, "dummy variable");
	mp2_940_tester(default_intra_quantizer_matrix[0] == 8, "quantizer matrix");
	mp2_940_tester(mpeg2_scan_norm[1] == 1, "mpeg2 scan norm");
	mp2_940_tester(mpeg2_scan_alt[1] == 8, "mpeg2 scan alt");
	mp2_940_tester(non_linear_quantizer_scale[1] == 1, "non linear quantizer scale");
	mp2_940_tester(g_uTest1 == 2, "test 1");
	mp2_940_tester(g_uTest2 == 3, "test 2");
	mp2_940_tester(mpeg2_malloc_hook == NULL, "malloc hook");
	mp2_940_tester(mpeg2_free_hook == NULL, "free hook");
//	mp2_940_tester(mpeg2_accels == 0, "mpeg2_accels");
	mp2_940_tester(g_mpeg_data == 0, "g_mpeg_data");
	mp2_940_tester(s_video_output == 0, "s_video_output");
	mp2_940_tester(g_uHeapIndex == 0, "g_uHeapIndex");

#ifdef GP2X
	mp2_940_tester(g_au8MpoHeap == (SHARED_MEM_START - HEAPSIZE), "Heap Pointer");
#endif // GP2X
}

// this is the main function that starts on the 940
void *mp2_940_start(void *ignored)
{
	int done = 0;

	mp2_940_setup_g_pShared();

	// make sure we haven't self-reset
	if (g_pShared->uCPU2Running != 0)
	{
		do_error("CPU2 seems to have reset itself, locking up");

		// lockup
		lockup();
	}

	g_pShared->uCPU2Running = 1;

	mp2_940_initialize_globals();

	do_error("CPU2: logtest1");
	do_error("CPU2: logtest2");

	s_video_output = (vo_instance_t *) vo940_open();	// open 'null' driver (we just pass decoded frames to parent thread)

	g_pShared->uCPU2Running = 2;

	do_error("CPU2: about to call mpeg2_init");

	if (s_video_output)
	{
		char s[80];
		g_mpeg_data = mpeg2_init();
		mpo_strncpy(s, "g_mpeg_data address is: ", sizeof(s));
//		mpo_strncat(s, UToHexStr(g_mpeg_data), sizeof(s));

		g_pShared->uCPU2Running = 3;
	}
	else
	{
		do_error("VLDP : Error opening LIBVO!");
		done = 1;
	}

	g_pShared->uCPU2Running = 4;

	// wait for commands from CPU1
	while (!done)
	{
		int iCmd = mp2_940_peek_command();
		switch (iCmd)
		{
		default:	// no command
			break;
		case C940_QUIT:
			done = 1;
			break;
		case C940_NOP:
			mp2_940_ack_command();	// acknowledge 'ping'
			break;
		case C940_LIBMPEG2_RESET:
			mp2_940_do_reset();
			break;
		}

		// if CPU1 wants us to decode
		if (g_pShared->uReqBufCount != g_pShared->uAckBufCount)
		{
			mp2_940_do_decode();
		}
		// else loop and check for other commands

		// TODO : should we do some kind of delay here so we're not constantly querying memory?

		#ifndef GP2X
		usleep(0);
		#endif // GP2X

	} // end while we have not received a quit command

	mpeg2_close(g_mpeg_data);	// shutdown libmpeg2

	if (s_video_output->close)
	{
		s_video_output->close(s_video_output);		// shutdown video driver
	}

	mpo_free_all();	// free all fake memory

	// acknowledge quit right at the last second
	mp2_940_ack_command();

	return 0;
}

// gp2x code calls into this function
void Main940()
{
	mp2_940_start(0);
}

