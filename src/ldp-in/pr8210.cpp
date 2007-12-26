/*
 * pr8210.cpp
 *
 * Copyright (C) 2001 Matt Ownby
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

// pr8210.cpp
// by Matt Ownby
// translates PR-8210 blips into commands
// this should be useful for any game that uses the PR-8210

// PR8210 INFO
// The commands for the PR8210 are listed in the PR8210A service manual under the "serial" section, page 37.
// A .pdf of this manual can be downloaded from the Dragon's Lair Project under game sections that used the PR-8210.
// For example, http://www.dragons-lair-project.com/tech/pages/mach3.asp has a link to this manual.

// As the manual says, a blip of 0 has an interval of 1.05 ms, while a blip of 1 has an interval of 2.11 ms
// At the moment, each game driver calculates these blips internally.
// NOTE : to calculate blips accurately, you need to use elapsed cpu cycles, not elapsed time.

#include <stdio.h>
#include "../io/conout.h"
#include "pr8210.h"
#include "../ldp-out/ldp.h"
#include "../timer/timer.h"

unsigned int g_pr8210_seek_received = 0;	// whether we've received a seek command
static char g_pr8210_frame[6] = {0};
unsigned int g_pr8210_digit_count = 0;

bool g_pr8210_audio1_mute = false;
bool g_pr8210_audio2_mute = false;

// used to determine whether to display errors if a search fails
bool g_pr8210_search_pending = false;

// processes 10 blips into a PR-8210 command
// The blips should be stored in the lowest 10 bits of the integer
// So for example, if my command is 0010000000 then my 32-bit integer would like like
// ???????? ???????? ??????00 10000000
void pr8210_command(unsigned int blips)
{
	char s[80];	// for printing error messages
	static unsigned int old_blips = 0xFFFF;	// the last command we received
	// initialied to 0xFFFF to make sure it is never equal to any new command we get

	blips &= 0x3FF;	// make sure any extra garbage is stripped off

//	printf("Command is ");
//	pr8210_print_binary(blips);
//	printf("\n");

	// if the new blips are not equal to the old ones, then accept command
	if (blips != old_blips)
	{
		// verify header and footer
		// So the format of a command is 001?????00
		if ((blips & 0x383) == 0x80)
		{
			unsigned char command = (unsigned char) ((blips & 0x7C) >> 2);
			// isolate the 5-bit command for comparison
			
			switch (command)
			{
				case 0x14:	// play (10100)
					g_ldp->pre_play();
					break;
				case 0x10: // 3x play forward (10000)
					printline("PR-8210 : 3X play forward (unsupported)");
					break;
				case 0x08: // scan forward (01000)
					printline("PR-8210 : scan forward (unsupported)");
					break;
				case 0x18: // slow forward (11000)
					printline("PR-8210 : slow forward (unsupported)");
					break;
				case 0x4:	// step forward (00100)
					g_ldp->pre_step_forward();
					break;
				case 0xC: // 3x play reverse (01100)
					printline("PR-8210 : 3x play reverse (unsupported)");
					break;
				case 0x1C: // scan reverse (11100)
					printline("PR-8210 : scan reverse (unsupported)");
					break;
				case 0x2: // slow reverse (00010)
					printline("PR-8210 : slow reverse (unsupported)");
					break;
				case 0x12: // step reverse (10010)
					g_ldp->pre_step_backward();
					break;
				case 0xA:	// pause (01010)
					g_ldp->pre_pause();
					break;
				case 0xE:	// Audio 1/L (01110)
					pr8210_audio1();
					break;
				case 0x16:	// Audio 2/R (10110)
					pr8210_audio2();
					break;
				case 0x1E: // Reject (11110)
					// if the player is stopped, reject ejects the player, therefore, we ignore this command
					// because we probably don't ever want this to occur
					printline("PR-8210 : reject received (ignored)");
					break;
				case 0x1A:	// Seek (11010)
					pr8210_seek();
					break;
				case 0x6 : // Chapter (00110)
					printline("PR-8210 : Chapter (unsupported)");
					break;
				case 0x1:	// 0 (00001)
					pr8210_add_digit('0');
					break;
				case 0x11:	// 1 (10001)
					pr8210_add_digit('1');
					break;
				case 0x9:	// 2 (01001)
					pr8210_add_digit('2');
					break;
				case 0x19:	// 3 (11001)
					pr8210_add_digit('3');
					break;
				case 0x5:	// 4 (00101)
					pr8210_add_digit('4');
					break;
				case 0x15:	// 5 (10101)
					pr8210_add_digit('5');
					break;
				case 0xD:	// 6 (01101)
					pr8210_add_digit('6');
					break;
				case 0x1D:	// 7 (11101)
					pr8210_add_digit('7');
					break;
				case 0x3:	// 8 (00011)
					pr8210_add_digit('8');
					break;
				case 0x13:	// 9 (10011)
					pr8210_add_digit('9');
					break;
				case 0xB: // Frame (01011)
					printline("PR-8210 : Frame (unsupported)");
					break;
				case 0x0:	// filler to separate two of the same commands, used by Cliff Hanger
					break;
				default:
					sprintf(s, "PR8210: Unknown command %x", command);
					printline(s);
					break;
			} // end switch
		} // end valid header and footer

		// else if we didn't get filler (all 0's) to separate commands, notify user that something is wrong
		else if (blips != 0)
		{
			printline("PR8210 Error : Bad header or footer");
		}

		// else we got all 0's which is just filler to separate two commands

		old_blips = blips;
	} // end if this is a new command	
}

// Audio 1/L
void pr8210_audio1()
{
	if (g_pr8210_audio1_mute)
	{
		g_ldp->enable_audio1();
		g_pr8210_audio1_mute = false;
	}
	else
	{
		g_ldp->disable_audio1();
		g_pr8210_audio1_mute = true;
	}
}

// Audio 2/R
void pr8210_audio2()
{
	if (g_pr8210_audio2_mute)
	{
		g_ldp->enable_audio2();
		g_pr8210_audio2_mute = false;
	}
	else
	{
		g_ldp->disable_audio2();
		g_pr8210_audio2_mute = true;
	}
}

void pr8210_seek()
{
//	printline("PR8210 Seek Received");

	// if this is our first one...
	if (!g_pr8210_seek_received)
	{
		g_pr8210_seek_received = 1;
	}

	// if we are getting another seek command, it means that we're done receiving digits and to execute the seek
	else
	{
		g_pr8210_frame[g_pr8210_digit_count] = 0;	// terminate string, note the g_pr8210_digit_count should never be out of bounds

		if (g_pr8210_digit_count > 0)
		{
			if (g_ldp->get_status() != LDP_SEARCHING)
			{
				g_pr8210_search_pending = true;
				g_ldp->pre_search(g_pr8210_frame, false);	// non-blocking seeking, drivers SHOULD use pr8210_get_current_frame()
			}
			else printline("PR8210 : got search command before we were done searching.. ignoring..");
		}
		
		// else, I think this is some kind of pr8210 reset
		// Cliff Hanger does this.. seems safe to ignore it.

		g_pr8210_digit_count = 0;	// reset digit count
	}
}

// PR8210 games should use this function instead of calling g_ldp->get_current_frame() directly
//  because this function calls the necessary g_ldp->get_status() automatically.
//  NOTE!! A result of 0 means we are seeking or stopped!
Uint16 pr8210_get_current_frame()
{
	Uint16 result = 0;	// 0 means we are seeking or stopped
	int status = g_ldp->get_status();

	if ((status == LDP_PLAYING) || (status == LDP_PAUSED))
	{
		result = g_ldp->get_current_frame();
		g_pr8210_search_pending = false;	// if there was a search pending, it is over now
	}
	// if the search resulted in an error
	else if (g_pr8210_search_pending && (status != LDP_SEARCHING))
	{
		g_pr8210_search_pending = false;
		
		printline("PR8210 SEARCH ERROR: if you're using VLDP then your framefile may be invalid!");
	}
	// else the search is still pending, or we were never searching

	return result;
}

// ch is the digit to add and it's in ASCII format
void pr8210_add_digit(char ch)
{
	/*
	char s[81];
	sprintf(s, "PR8210 Digit Received : %c\n", ch);
	printline(s);
	*/

	// make sure we are getting digits after we've gotten a seek
	if (g_pr8210_seek_received)
	{
		// safety check
		if (g_pr8210_digit_count < (sizeof(g_pr8210_frame)-1))
		{
			g_pr8210_frame[g_pr8210_digit_count] = ch;
			g_pr8210_digit_count++;
		}
		else
		{
			printline("PR8210 ERROR : Received too many digits, undefined behavior!");
			g_pr8210_digit_count = 0;
		}
	}

	// if we are getting digits without first getting a seek, something
	// is wrong
	else
	{
		printline("PR8210 error: digit received without seek command");
	}
}

void pr8210_reset()  //since the audio commands are toggles, we have to set audio to on (default) on reset
{
	printline("PR8210: Reset");
	if (g_pr8210_audio1_mute)
	{
		g_ldp->enable_audio1();
		g_pr8210_audio1_mute = false;
	}
	else
	{
		g_ldp->disable_audio1();
		g_pr8210_audio1_mute = true;
	}
	if (g_pr8210_audio2_mute)
	{
		g_ldp->enable_audio2();
		g_pr8210_audio2_mute = false;
	}
	else
	{
		g_ldp->disable_audio2();
		g_pr8210_audio2_mute = true;
	}
}

// for debugging only
// prints a 10 bit PR-8210 command
void pr8210_print_binary(unsigned int num)
{
	int i = 0;
	
	for (i = 0; i < 10; i++)
	{
		outchr(((num & 0x200) >> 9) + '0');
		num = num << 1;	// print the next digit ...
	}
}
