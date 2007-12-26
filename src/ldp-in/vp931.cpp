/*
 * vp931.cpp
 *
 * Copyright (C) 2001-2005 Mark Broadhead
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


// vp931.cpp
// part of the DAPHNE emulator
// to be written by Mark Broadhead and Warren Ondras :)
//
// This code emulates the Philips 22VP931 used in Firefox and Freedom Fighter

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../game/game.h"
#include "vp931.h"
#include "../io/conout.h"
#include "../io/numstr.h"	// for DEBUG
#include "../ldp-out/ldp.h"
#include "../cpu/cpu-debug.h"

#ifdef DEBUG
#include <assert.h>
#endif // DEBUG

// True means it's active, which means data is available to be read
// False means it's inactive, which means data has been read
bool g_bVP931_DAV = true;

// True means active, which means that it's ok to send a command
// False means inactive, which means that the buffer is full and the player is processing the byte that was last sent
bool g_bVP931_DAK = true;

bool g_bVP931_ResetLine = false;
bool g_bVP931_ReadLine = false;
bool g_bVP931_WriteLine = false;

unsigned char g_u8VP931InputBuf = 0;

unsigned int g_uVP931CurrentByte = 0;
unsigned char command_byte[3];

#define OUTBUF_SIZE 6

// the output to be sent from LDP
unsigned char g_VP931OutBuf[OUTBUF_SIZE];

// which position we are in the output buf
unsigned int g_uVP931OutBufIdx = 0;

// how many CPU cycles the DAK is inactive for before it becomes active again
unsigned int g_vp931_cycles_per_dak = 0;

///////////////////////////////////////////

// retrieves the status from our virtual VP931
unsigned char read_vp931()
{
	unsigned char u8Result = 0;

	// safety check
	if (g_uVP931OutBufIdx < OUTBUF_SIZE)
	{
		u8Result = g_VP931OutBuf[g_uVP931OutBufIdx];

#ifdef DEBUG
		// DEBUG
		//string strMsg = "read_vp931 returned " + numstr::ToStr(u8Result, 16);
		//printline(strMsg.c_str());
#endif
	}
	else
	{
//		printline("VP931 ERROR: read_vp931 called when there is nothing to be read");
	}

	if (!g_bVP931_DAV)
	{
//		printline("VP931 WARNING: read_vp931 called when DAV wasn't asserted");
	}

	return(u8Result);
}

void write_vp931(unsigned char u8Val)
{
	g_u8VP931InputBuf = u8Val;
}

// returns the number embedded inside the command (5 or 3 digits supported)
unsigned int vp931_get_cmd_num(unsigned int uDigits)
{
	unsigned int uResult = 0;
	unsigned int u = 0;

#ifdef DEBUG
	// we only handle 5 and 3 digit cases
	assert((uDigits == 5) || (uDigits == 3));
#endif // DEBUG

	// if we're only doing 3 digits, then start 1 digit ahead
	if (uDigits == 3)
	{
		u = 1;
	}

	// start with the most dignificant digit which is the low nibble of the command
	uResult = command_byte[u] & 0xF;
	++u;

	while (u < 3)
	{
		uResult *= 10;
		uResult += (command_byte[u] & 0xF0) >> 4;
		uResult *= 10;
		uResult += (command_byte[u] & 0x0F);
		++u;
	}

	return uResult;
}

// don't call this function directly!!!
void process_vp931_cmd (unsigned char value)
{
	char s[81] = { 0 };
	
	// catch the command
	command_byte[g_uVP931CurrentByte] = value;
	++g_uVP931CurrentByte;

	// we've got a complete command so process it
	if (g_uVP931CurrentByte == 3)
	{
#ifdef DEBUG
//		sprintf(s, "VP931 CMD: %x %x %x", command_byte[0], command_byte[1], command_byte[2]);
//		printline(s);
#endif // DEBUG

		g_uVP931CurrentByte = 0;

		if (command_byte[0] == 0x00)
		{
			if (command_byte[1] == 0x00)
			{
				// firefox sends the play command every field, so we don't want the log printing 'play' constantly
				if (g_ldp->get_status() != LDP_PLAYING)
				{
					g_ldp->pre_play();
				}
			}
			else if (command_byte[1] == 0x10)
			{
				printline("VP931: play reverse received - ignored");
			}
			else if (command_byte[1] == 0x20)
			{
				g_ldp->pre_pause();
			}
			else if ((command_byte[1] & 0xF0) == 0xE0)
			{
				unsigned int uSkipFrames = vp931_get_cmd_num(3);
				g_ldp->pre_skip_forward(uSkipFrames);
			}
			// else if it's a skip backward command ...
			else if ((command_byte[1] & 0xF0) == 0xF0)
			{
				unsigned int uSkipFrames = vp931_get_cmd_num(3);
				g_ldp->pre_skip_backward(uSkipFrames);
			}
			else
			{
				sprintf(s,"Unsupported VP931 Command Received: %x %x %x", command_byte[0], command_byte[1], command_byte[2]);
				printline(s);
#ifdef DEBUG
				set_cpu_trace(1);
#endif
			}
		}
		else if (command_byte[0] == 0x02)
		{
			printline("VP931: Audio commands: Implement Me!");
		}
		else if ((command_byte[0] & 0xf0) == 0xd0)
		{
			printline("VP931: Search and Halt command: Implement Me!");
#ifdef DEBUG
			set_cpu_trace(1);
#endif
		}

		// search and play?
		else if ((command_byte[0] & 0xf0) == 0xf0)
		{
			char pszFrame[FRAME_ARRAY_SIZE];

			unsigned int uFrame = vp931_get_cmd_num(5);

			// If the command is received on the first field of the frame, don't try to skip because that throws our current
			//  code off BAD :(
			// The firefox rom is trying to skip on the first and second fields which we aren't prepared to handle.
			if ((g_ldp->get_vblank_mini_count() & 1) == 0)
			{
//				printline("VP931 : Search and Play ignored because it's on the first field (FIXME)");
				g_ldp->framenum_to_frame(uFrame, pszFrame);
				g_ldp->pre_search(pszFrame, true);	// blocking seeking for now while this is developed
				g_ldp->pre_play();
			}
			// else it's the second field which we are prepared to handle
			else
			{		
				unsigned int uCurFrame = g_ldp->get_current_frame();

				// We subtract 1 because if we were trying to search&play forward 1 frame, we would skip 0 frames forward,
				//  because we will arrive at the target frame without doing any skips.
				unsigned int uFrameMin1 = uFrame - 1;

				// If we are searching forward, then do a forward skip
				if (uCurFrame < uFrameMin1)
				{
					string s = "VP931: Search and Play (skip) to frame " + numstr::ToStr(uFrame);
					printline(s.c_str());
					g_ldp->pre_skip_forward(uFrameMin1 - uCurFrame);
				}
				// else take the easy way out until we find a ROM that actually uses this functionality
				else
				{
					g_ldp->framenum_to_frame(uFrame, pszFrame);
					g_ldp->pre_search(pszFrame, true);	// blocking seeking for now while this is developed
					g_ldp->pre_play();
				}
			}
		}
		else
		{
			sprintf(s,"Unsupported VP931 Command Received: %x %x %x", command_byte[0], command_byte[1], command_byte[2]);
			printline(s);
#ifdef DEBUG
			set_cpu_trace(1);
#endif
		}
	}

	// this should never happen if we've implemented this correctly
	else if (g_uVP931CurrentByte > 3)
	{
		printline("VP931 ERROR: g_uVP931CurrentByte > 2");
	}
}

bool vp931_is_dav_active()
{
	return(g_bVP931_DAV);
}

bool vp931_is_dak_active()
{
	return(g_bVP931_DAK);
}

// this is apparently always true if the LDP is running
// (oprt stands for operational?)
bool vp931_is_oprt_active()
{
	return true;
}

void vp931_change_write_line(bool bEnabled)
{
	// if the line has actually changed
	if (bEnabled != g_bVP931_WriteLine)
	{
		// if the write line is enabled, then process the command
		if (bEnabled)
		{
			// safety check: we shouldn't be getting commands if the DAK isn't enabled
			if (!g_bVP931_DAK)
			{
				printline("VP931 WARNING: write line asserted when DAK wasn't active (ie buffer was full)");
			}

			// UPDATE : CPU emulation is too slow using the event callback (because the cpu interleaving has to be so high),
			//  so we will make the DAK 'instant' for now.  If another game requires some DAK delay, we can check elapsed
			//  cycles to accomplish the delay.
			g_bVP931_DAK = true;

			// DAK goes inactive for 15 uS while it is processed, it will become active again when the CPU event fires
//			g_bVP931_DAK = false;
//			cpu_set_event(0, g_vp931_cycles_per_dak, vp931_event_callback, (void *) NULL);

			process_vp931_cmd(g_u8VP931InputBuf);
		}
		g_bVP931_WriteLine = bEnabled;
	}
	// else there is no change, so ignore
}

void vp931_change_read_line(bool bEnabled)
{
	if (bEnabled != g_bVP931_ReadLine)
	{
		// if enabled, it means that the ROM is ready to read the next byte from us
		if (bEnabled)
		{
		}
		// else if the read line is disabled, it means the ROM has finished reading the byte, so we can advance our pointer
		else
		{
			// if we still have stuff remaining to be read ...
			if (g_uVP931OutBufIdx < OUTBUF_SIZE)
			{
				++g_uVP931OutBufIdx;
//				printline("VP931 read line was enabled");
				// TODO : should DAV be disabled, then enabled again?
			}
			// else we're at the end of the output buffer, so now receive commands
			else
			{
				g_bVP931_DAV = false;
			}
		}
		g_bVP931_ReadLine = bEnabled;
	}
}

void vp931_change_reset_line(bool bEnabled)
{
	if (bEnabled != g_bVP931_ResetLine)
	{
		if (bEnabled)
		{
			printline("VP931 RESET received! (ignored)");
		}
		g_bVP931_ResetLine = bEnabled;
	}
}

void vp931_report_vsync()
{
	g_bVP931_DAV = true;	// new status waiting
	g_bVP931_DAK = true;	// not currently processing a command
	g_uVP931CurrentByte = 0;	
	g_uVP931OutBufIdx = 0;	// move to beginning

	// clear buffer
	memset(g_VP931OutBuf, 0, sizeof(g_VP931OutBuf));

	// if this is the first vblank of the frame
	if ((g_ldp->get_vblank_mini_count() & 1) == 0)
	{
		g_VP931OutBuf[0] = 0xF0;
	}
	// else this is the second vblank of the frame
	else
	{
		// NOTE: only firefox has this code on the 2nd field of the disc, so if we support any other games that
		//  use this player (such as freedom fighter) we'll need to improve this
		g_VP931OutBuf[0] = 0xA0;
	}

	unsigned int uCurFrame = g_ldp->get_current_frame();
	unsigned int uBCD = 0;

	// convert current frame into BCD format
	// (only do last 4 digits because most significant digit is part of the first byte)
	for (unsigned int u = 0; u < 4; ++u)
	{
		uBCD >>= 4;	// move to the right one hex digit
		uBCD |= (uCurFrame % 10) << 12;	// get least significant digit and put it high enough that we won't lose it
		uCurFrame /= 10;	// move decimal number to the right one digit
	}

#ifdef DEBUG
	assert(uCurFrame <= 9);
#endif

	g_VP931OutBuf[0] |= uCurFrame;	// add first digit to first byte (uCurFrame & 0xF is implied here)
	g_VP931OutBuf[1] = (uBCD >> 8) & 0xFF;
	g_VP931OutBuf[2] = uBCD & 0xFF;
	// bytes 3, 4, and 5 are already 0
}

void reset_vp931()
{
	unsigned int cpu_hz = get_cpu_hz(0);
	double dCyclesPerUs = cpu_hz / 1000000.0;	// cycles per microsecond

	g_bVP931_DAV = false;	// no status waiting
	g_bVP931_DAK = false;	// won't receive commands

	// How many cycles we pause when we receive a command (DAK goes low then high)
	g_vp931_cycles_per_dak = (unsigned int) ((dCyclesPerUs * 15.0) + 0.5);
}

void vp931_event_callback(void *dontCare)
{
//	printline("vp931 DAK event fired!");

	// right now, the only event is the DAK
	g_bVP931_DAK = true;
}

