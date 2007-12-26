/*
 * vp932.cpp
 *
 * Copyright (C) 2005-2007 Mark Broadhead
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

// part of the DAPHNE emulator
// written by Mark Broadhead
//
// this code is to emulate the Phillips serial command set as used on the 
// 22vp932 and 22vp380 as used in Euro Dragon's Lair and Euro Dragon's Lair 2
// respectively

#include <stdio.h>
#include <stdlib.h>	// for atoi
#include <string.h>	// memset, strcpy
#include <queue>	// memset, strcpy
#include "../game/game.h"
#include "vp932.h"
#include "../io/conout.h"
#include "../ldp-out/ldp.h"
#include "../video/video.h"
using namespace std;

#define MAX_COMMAND_SIZE 31 

Uint8 vp932_command[MAX_COMMAND_SIZE + 1];
int vp932_command_pointer = 0;
queue <Uint8> vp932_status_queue;
bool g_vp932_search_pending = false;
bool g_vp932_play_pending = false;
bool gAudioLeftEn = true;
bool gAudioRightEn = true;

// write a byte to the vp932
void vp932_write(Uint8 data)
{
	// check if the current character is a carrage return
	// this signifies that we have a complete command
	if (data == 0x0d) 	
	{
		vp932_process_command();
	}
	// otherwise we have a part of our command so add it to the queue
	else
	{
		// filter out leading zeros
      if (!((data >= '0' && data <= '9') && vp932_command_pointer == 0))
      {
         vp932_command[vp932_command_pointer++] = data;
      }
	}
}

// read a byte from the vp932
Uint8 vp932_read()
{	
   Uint8 temp = 0;
	if (!vp932_status_queue.empty())
	{
      temp = vp932_status_queue.front();
		vp932_status_queue.pop();
	}
	else 
	{
		printline("VP932: Error, status read when empty!");
	}
	return temp;
}

// check to see if data is available to read
bool vp932_data_available()
{
	bool result = false;
	if (g_vp932_search_pending)
	{
		// we don't want the response seen until the search is complete
		int stat = g_ldp->get_status();
		// if we finished seeking and found success
		if (stat == LDP_PAUSED)
		{
			g_vp932_search_pending = false;
			if (g_vp932_play_pending)
			{
				restoreAudio();
				g_ldp->pre_play();
				g_vp932_play_pending = false;
			}
		}		
	}
	if (!g_vp932_search_pending)
	{
		result = !vp932_status_queue.empty();
	}

	return result;
}

void vp932_process_command()
{
	char s[81] = "";
	sprintf(s,"VP932: Got command ");
	printline(s);
	vp932_command[vp932_command_pointer] = 0;
	printline((const char*)vp932_command);
	Uint16 current_frame = 0;
	char frame_string[6];

	switch (vp932_command[0])
	{
	case '$':
		{
			if (vp932_command[1] == '0')
			{
				printline("VP932: Replay switch disable (ignored)");
			}
			else if (vp932_command[1] == '1')
			{
				printline("VP932: Replay switch enable (ignored)");
			}
			else
			{
				printline("VP932: Error in '$' command");
				printline((const char*)vp932_command);
			}
		}
		break;
	case '\'':
		{
			printline("VP932: Eject (ignored)");
		}
		break;
	case '(':
		{
			if (vp932_command[1] == '0')
			{
				printline("VP932: CX off (ignored)");
			}
			else if (vp932_command[1] == '1')
			{
				printline("VP932: CX on (ignored)");
			}
			else if (vp932_command[1] == 'X')
			{
				printline("VP932: CX normal (ignored)");
			}
			else
			{
				printline("VP932: Error in '(' command");
				printline((const char*)vp932_command);
			}
		}
		break;
	case ')':
		{
			if (vp932_command[1] == '0')
			{
				printline("VP932: Transmittion Delay off (ignored)");
			}
			else if (vp932_command[1] == '1')
			{
				printline("VP932: Transmittion Delay on (ignored)");
			}
			else
			{
				printline("VP932: Error in ')' command");
				printline((const char*)vp932_command);
			}
		}
		break;
	// '*' Halt (still mode) 
	case '*':
		{
			g_ldp->pre_pause();
		}
		break;
	case ',':
		{
			if (vp932_command[1] == '0')
			{
				printline("VP932: Standby (unload) (ignored)");
			}
			// ',1' On (load) 
			else if (vp932_command[1] == '1')
			{
				vp932_status_queue.push('S'); // ACK - Disc up to speed
				vp932_status_queue.push(0x0d);				
			}
			else
			{
				printline("VP932: Error in ')' command");
				printline((const char*)vp932_command);
			}
		}
		break;
	// '/' Pause (halt + all muted)
	case '/':
		{
			g_ldp->pre_pause();
		}
		break;
	case ':':
		{
			printline("VP932: Reset to default values (ignored)");
		}
		break;
	case '?':
		{
			// '?F' Picture Number Request (ignored)
			if (vp932_command[1] == 'F')
			{
				current_frame = g_ldp->get_current_frame();
				sprintf(frame_string, "%05d", current_frame);
				vp932_status_queue.push('F'); // Frame				
				vp932_status_queue.push(frame_string[0]);
				vp932_status_queue.push(frame_string[1]);
				vp932_status_queue.push(frame_string[2]);
				vp932_status_queue.push(frame_string[3]);
				vp932_status_queue.push(frame_string[4]);
				vp932_status_queue.push(0x0d); // Carrage return
			}
			else if (vp932_command[1] == 'C')
			{
				printline("VP932: Chapter Number Request (ignored)");
			}
			else if (vp932_command[1] == 'T')
			{
				printline("VP932: Time Code Request (ignored)");
			}
			else if (vp932_command[1] == 'N')
			{
				printline("VP932: Track Number Information Request (ignored)");
			}
			else if (vp932_command[1] == 'S')
			{
				printline("VP932: Track Start Time Request (ignored)");
			}
			else if (vp932_command[1] == 'I')
			{
				printline("VP932: Disc I.D. request (ignored)");
			}
			else if (vp932_command[1] == 'D')
			{
				printline("VP932: Disc program status request (ignored)");
			}
			else if (vp932_command[1] == 'P')
			{
				printline("VP932: Drive status request (ignored)");
			}
			else if (vp932_command[1] == 'E')
			{
				printline("VP932: Disc lead-out start request (ignored)");
			}
			else if (vp932_command[1] == 'U')
			{
				printline("VP932: User code request (ignored)");
			}
			else if (vp932_command[1] == '=')
			{
				printline("VP932: Revision level request (ignored)");
			}
			else
			{
				printline("VP932: Error in '?' command");
				printline((const char*)vp932_command);
			}
		}
		break;
	case 'A':
		{
			// Audio-1 off
			if (vp932_command[1] == '0')
			{
				g_ldp->disable_audio1();
				gAudioLeftEn = false;
			}
			// Audio-1 on
			else if (vp932_command[1] == '1')
			{
				g_ldp->enable_audio1();
				gAudioLeftEn = true;
			}
			else
			{
				printline("VP932: Error in 'A' command");
				printline((const char*)vp932_command);
			}
		}
		break;
	case 'B':
		{
			// Audio-2 off
			if (vp932_command[1] == '0')
			{
				g_ldp->disable_audio2();
				gAudioRightEn = false;
			}
			// Audio-2 on
			else if (vp932_command[1] == '1')
			{
				g_ldp->enable_audio2();
				gAudioRightEn = true;
			}
			else
			{
				printline("VP932: Error in 'B' command");
				printline((const char*)vp932_command);
			}
		}
		break;
	case 'C':
		{
			if (vp932_command[1] == '0')
			{
				printline("VP932: Chapter number display off (ignored)");
			}
			else if (vp932_command[1] == '1')
			{
				printline("VP932: Chapter number display on (ignored)");
			}
			else
			{
				printline("VP932: Error in 'C' command");
				printline((const char*)vp932_command);
			}
		}
		break;
	case 'D':
		{
			if (vp932_command[1] == '0')
			{
				printline("VP932: Picture Number/time code display off (ignored)");
			}
			else if (vp932_command[1] == '1')
			{
				printline("VP932: Picture Number/time code display on (ignored)");
			}
			else if (vp932_command[1] == '/')
			{
				printline("VP932: Text on screen (ignored)");
			}
			else
			{
				printline("VP932: Error in 'D' command");
				printline((const char*)vp932_command);
			}
		}
		break;
	case 'E':
		{
			if (vp932_command[1] == '0')
			{
				printline("VP932: Video Off (ignored)");
			}
			else if (vp932_command[1] == '1')
			{
				printline("VP932: Video On (ignored)");
			}
			else if (vp932_command[1] == 'M')
			{
				printline("VP932: #Video multistandard (ignored)");
			}
			else if (vp932_command[1] == 'P')
			{
				printline("VP932: #Video transcoded PAL (ignored)");
			}
			else if (vp932_command[1] == 'N')
			{
				printline("VP932: #Video transcoded NTSC (ignored)");
			}
			else
			{
				printline("VP932: Error in 'E' command");
				printline((const char*)vp932_command);
			}
		}
		break;
	case 'F':
		{
			if (vp932_command_pointer == 7)
			{
				if (vp932_command[6] == 'I')
				{
					printline("VP932: Load picture number information register (ignored)");
				}
				else if (vp932_command[6] == 'S')
				{
					printline("VP932: Load picture number stop register (ignored)");
				}
				else if (vp932_command[6] == 'R')
				{
					// terminate the frame
					vp932_command[6] = 0;
					g_ldp->pre_search((const char*)&vp932_command[1], false);
					g_vp932_search_pending = true;

					vp932_status_queue.push('A'); // ACK - Search complete
					vp932_status_queue.push('0');
					vp932_status_queue.push(0x0d);
				}
				else if (vp932_command[6] == 'N')
				{
					// terminate the frame
					vp932_command[6] = 0;
					g_ldp->pre_search((const char*)&vp932_command[1], false);
					g_vp932_search_pending = true;
					g_vp932_play_pending = true;

					vp932_status_queue.push('A'); // ACK - Search complete
					vp932_status_queue.push('1');
					vp932_status_queue.push(0x0d);
				}
				else if (vp932_command[6] == 'Q')
				{
					printline("VP932: Goto picture number and continue previous play mode (ignored)");
				}
				else if (vp932_command[6] == 'A')
				{
					printline("VP932: Load picture number autostop register (ignored)");
				}
				else if (vp932_command[6] == 'P')
				{
					printline("VP932: Goto picture number then still mode (ignored)");
				}
				else
				{
					printline("VP932: Error in 'FxxxxxX' command");
					printline((const char*)vp932_command);
				}
			}
			else if (vp932_command_pointer == 13)
			{
				if (vp932_command[12] == 'S')
				{
					printline("VP932: Goto picture number xxxxx and play until yyyyy then halt (ignored)");
				}
				else if (vp932_command[12] == 'A')
				{
					printline("VP932: Goto picture number xxxxx and play until yyyyy then repeat until cleared (ignored)");
				}
				else
				{
					printline("VP932: Error in 'FxxxxxNyyyyyX' command");
					printline((const char*)vp932_command);
				}
			}
			else
			{
				printline("VP932: Error in 'F' command");
				printline((const char*)vp932_command);
			}
		}
		break;
	case 'H':
		{
			if (vp932_command[1] == '0')
			{
				printline("VP932: Remote control not routed to computer (ignored)");
			}
			else if (vp932_command[1] == '1')
			{
				printline("VP932: Remote control routed to computer (ignored)");
			}
			else
			{
				printline("VP932: Error in 'H' command");
				printline((const char*)vp932_command);
			}
		}
		break;
	case 'I':
		{
			if (vp932_command[1] == '0')
			{
				printline("VP932: Local front-panel buttons disabled (ignored)");
			}
			else if (vp932_command[1] == '1')
			{
				printline("VP932: Local front-panel buttons enabled (ignored)");
			}
			else
			{
				printline("VP932: Error in 'I' command");
				printline((const char*)vp932_command);
			}
		}
		break;
	case 'J':
		{
			if (vp932_command[1] == '0')
			{
				printline("VP932: Remote control disabled for drive control (ignored)");
			}
			else if (vp932_command[1] == '1')
			{
				printline("VP932: Remote control enabled for drive control (ignored)");
			}
			else
			{
				printline("VP932: Error in 'J' command");
				printline((const char*)vp932_command);
			}
		}
		break;
	case 'L':
		{
			printline("VP932: Still forward (ignored)");
		}
		break;
	case 'M':
		{
			printline("VP932: Still reverse (ignored)");
		}
		break;
	case 'N':
		{
			restoreAudio();
			g_ldp->pre_play();
		}
		break;
	case 'O':
		{
			printline("VP932: Normal play reverse (ignored)");
		}
		break;
	case 'Q':
		{
			if (vp932_command[vp932_command_pointer - 1] == 'R')
			{
				printline("VP932: Goto chapter number and halt (ignored)");
			}
			else if (vp932_command[vp932_command_pointer - 1] == 'N')
			{
				printline("VP932: Goto chapter number and play (ignored)");
			}
			else if (vp932_command[vp932_command_pointer - 1] == 'S')
			{
				printline("VP932: Play chapter/track (sequence) and halt (ignored)");
			}
			else if (vp932_command[vp932_command_pointer - 1] == 'N')
			{
				printline("VP932: Goto track number [time code in track] and play (ignored)");
			}
			else if (vp932_command[vp932_command_pointer - 1] == 'P')
			{
				printline("VP932: Goto track number and pause (ignored)");
			}
			else if (vp932_command[vp932_command_pointer - 1] == 'A')
			{
				printline("VP932: Play chapter/track (sequence) then repeat until cleared (ignored)");
			}
			else
			{
				printline("VP932: Error in 'Q' command");
				printline((const char*)vp932_command);
			}		
		}
		break;
	case 'S':
		{
			if (vp932_command[vp932_command_pointer - 1] == 'F')
			{
				printline("VP932: Set fast speed value 3, 4, or 8 (ignored)");
			}
			else if (vp932_command[vp932_command_pointer - 1] == 'S')
			{
				printline("VP932: Set slow speed value 1/2, 1/4, 1/8, 1/16, 1 step/sec, 1 step/3 sec (ignored)");
			}
			else if (vp932_command[vp932_command_pointer - 1] == 'A')
			{
				printline("VP932: Sound forced analog (4 channel NTSC disc) (ignored)");
			}
			else if (vp932_command[vp932_command_pointer - 1] == 'N')
			{
				printline("VP932: Sound normal (digital sound if available) (ignored)");
			}
			else
			{
				printline("VP932: Error in 'S' command");
				printline((const char*)vp932_command);
			}		
		}
		break;
	case 'T':
		{
			printline("VP932: Error in 'T' command");
			printline((const char*)vp932_command);
		}
		break;
	case 'U':
		{
			// This is supposed to start forward at the speed requested 
			// by the 'S' command. Since all I'm aware of is DLEURO using 
			// this for playing without sound I'm going to cheat
			g_ldp->disable_audio1();
			g_ldp->disable_audio2();
			g_ldp->pre_play();
			//printline("VP932: Slow motion forward (ignored)");
		}
		break;
	case 'V':
		{
			printline("VP932: Slow motion reverse (ignored)");
		}
		break;
	case 'W':
		{
			printline("VP932: Fast forward (ignored)");
		}
		break;
	case 'X':
		{
			printline("VP932: Clear (ignored)");
		}
		break;
	case 'Z':
		{
			printline("VP932: Fast reverse (ignored)");
		}
		break;
	default:
		{
			printline("VP932: Error in command");
			printline((const char*)vp932_command);
		}
		break;
	}
	vp932_command_pointer = 0;
}

void restoreAudio()
{
	if (gAudioLeftEn)
	{
		g_ldp->enable_audio1();
	}
	if (gAudioRightEn)
	{
		g_ldp->enable_audio2();
	}
}
