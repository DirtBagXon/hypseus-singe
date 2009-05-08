/*
 * vp380.cpp
 *
 * Copyright (C) 2003 Paul Blagay
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


// vp380.c
// part of the DAPHNE emulator
// written by Paul Blagay 
//
// This code emulates the Philips VP380 laserdisc player which is used in
// Dragon's Lair 2 / Space Ace '91 Euro 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../game/game.h"
#include "vp380.h"
#include "ldp1000.h"	// text stuff
#include "../io/conout.h"
#include "../ldp-out/ldp.h"

#define VP380_STACKSIZE 10 // the largest response is 8 bytes

unsigned char vp380_output_stack[VP380_STACKSIZE];
int vp380_output_stack_pointer = 0;
unsigned int vp380_curframe;
unsigned int vp380_repeat_frame;
unsigned int vp380_repeat_start_frame;
bool vp380_repeat = false;
int vp380_times_to_repeat = 0;

char vp380_frame[FRAME_ARRAY_SIZE] = {0}; // holds the digits sent to the vp380
int vp380_frame_index = 0; 

int vp380_yslots[3] = {0};	// 3 lines of text

// default status
static unsigned char enter_status = 0x00;
///////////////////////////////////////////

// retrieves the status from our virtual VP380
// returns -1 if there is nothing to return
int read_vp380()
{
	if (vp380_output_stack_pointer < 1)
		return(-1);
	
	vp380_output_stack_pointer--;
	return (vp380_output_stack[vp380_output_stack_pointer]);
}


// pushes a value on the vp380 stack, returns 1 if successful or 0 if stack is full
int vp380_stack_push(unsigned char value)
{
	int result = 0;

	// if we still have room to push
	if (vp380_output_stack_pointer < VP380_STACKSIZE-1)
	{
		vp380_output_stack[vp380_output_stack_pointer++] = value;
		result = 1;
	}
	else
	{
		printline("ERROR: vp380 stack overflow (increase its size)");
	}

	return(result);
}

// functions to see if we have a command
char	CommandBuffer[32];
int		CommandIndex = 0;

// regular command
int vp380_GotCommand ( const char* Command )
{
	if(strcmp(Command, &CommandBuffer[0]) == 0)
	{
		memset (&CommandBuffer,0,32);
		return 1;
	}
	else
		return 0;
}

// search command 'F12345R'
int vp380_GotSearch ( )
{
	if (CommandBuffer[0] == 'F' && CommandBuffer[6] == 'R')
		return 1;
	else
		return 0;
}

// text command "D/Wxxyy@text@"
int vp380_GotText ( )
{
	if (CommandBuffer[0] == 'D' && CommandBuffer[1] == '/' && CommandBuffer[2] == 'W')
		return 1;
	else
		return 0;
}

// sends a byte to our virtual vp380
void write_vp380 (unsigned char value)
{
//	char s[81] = { 0 };

	if (value == 0x0C)
		return;			// what is this

	//Store off command char
	if (value != 0x12 && value != 0x0D)
	{
		CommandBuffer[CommandIndex] = value;
		CommandIndex++;
	}
	else	// parse the command
	{
		CommandBuffer[CommandIndex] = 0;	// eos
		CommandIndex = 0;

		// Frame Request
		if (vp380_GotCommand ("?F"))
		{
			vp380_curframe = g_ldp->get_current_frame();
			char f[8];
			//itoa(vp380_curframe, &f[0], 10);	// should be faster than sprintf
			safe_itoa(vp380_curframe, f, sizeof(f));	// MPO : linux doesn't have itoa
			sprintf(f, "%05d", vp380_curframe);
			vp380_stack_push(0x0D); // M7
			vp380_stack_push(f[4]); // M6
			vp380_stack_push(f[3]); // M5
			vp380_stack_push(f[2]); // M4
			vp380_stack_push(f[1]); // M3
			vp380_stack_push(f[0]); // M2
			vp380_stack_push('F'); // M1
			return;
		}
		// Search
		if (vp380_GotSearch ())
		{
			vp380_frame[0] = CommandBuffer[1];
			vp380_frame[1] = CommandBuffer[2];
			vp380_frame[2] = CommandBuffer[3];
			vp380_frame[3] = CommandBuffer[4];
			vp380_frame[4] = CommandBuffer[5];
			memset (&CommandBuffer,0,32);
			vp380_frame_index = 5;
			enter_status = VP380_SEARCH;
			vp380_enter();
			return;
		}
		// Clear Screen
		if (vp380_GotCommand ("D/HCL"))
		{
			for (int i=0; i<3; i++)
				g_LDP1450_Strings[i].String[0] = 0;
			return;
		}
		// Forward Play
		if (vp380_GotCommand ("N"))
		{
			g_ldp->pre_play();
			return;
		}
		// Reverse Play
//		if (vp380_GotCommand ("O"))
//			return;
		// Still
		if (vp380_GotCommand ("*"))
		{
			g_ldp->pause();
			return;
		}
		// Text String
		if (vp380_GotText())
		{
			char	xc[4];
			char	yc[4];
			int		x;
			int		y;
			int		Line = 0;

			// get x,y
			strncpy (xc, &CommandBuffer[5],2);
			xc[2] = 0;
			strncpy (yc, &CommandBuffer[3],2);
			yc[2] = 0;
			x = atoi(xc);
			y = atoi(yc);
	
			// do we have this line already
			// vp380 expects a char style overwrite.. since we dont have that
			// luxury we have to emulate it.. if its the same line, overwrite it
			for (int j=0; j<3;j++)
			{
				if (g_LDP1450_Strings[j].y == (36 * y) - 20)
				{
					g_LDP1450_TextControl.TextLine = j;
					Line = 1;
					break;
				}
			}
	
			g_LDP1450_TextControl.Scale = 2.0f;
			g_LDP1450_Strings[g_LDP1450_TextControl.TextLine].x = 34 * x;
			g_LDP1450_Strings[g_LDP1450_TextControl.TextLine].y = (36 * y) - 20;
			
			int	i = 0; 
	
			// ignore the @, but copy text over til the @
			while (1)
			{
				if (CommandBuffer[ 8 + i] != '@')
					g_LDP1450_Strings[g_LDP1450_TextControl.TextLine].String[i] = CommandBuffer[8 + i];
				else
				{
					g_LDP1450_TextControl.LDP1450_DisplayOn = 1;
					g_LDP1450_Strings[g_LDP1450_TextControl.TextLine].String[i] = 0;
					{
						g_LDP1450_TextControl.TextLine++;
						if (g_LDP1450_TextControl.TextLine >2)
							g_LDP1450_TextControl.TextLine = 0;
					}
					break;
				}
				i++;
			}
			return;
		}
		// clear
		if (vp380_GotCommand ("X"))
		{
			// clear our strings
			for (int i=0; i<3; i++)
				g_LDP1450_Strings[i].String[0] = 0;
	
			g_LDP1450_TextControl.TextLine = 0;
			return;
		}
		// load disc
		if (vp380_GotCommand (",1"))
		{
			vp380_stack_push('S'); // ack
			return;
		}
		// NTSC video
//		if (vp380_GotCommand ("EN"))
//			return;
		// TX Off
//		if (vp380_GotCommand (")0"))
//			return;
		// CX auto on/off
//		if (vp380_GotCommand ("(X"))
//			return;
		// CX auto on/off
//		if (vp380_GotCommand ("(X"))
//			return;
		// Text Vertical Size (default)
//		if (vp380_GotCommand ("D/S00"))
//			return;
		// Text Vertical Size (default)
//		if (vp380_GotCommand ("D/S00"))
//			return;
		// Text Background Off
//		if (vp380_GotCommand ("D/B0"))
//			return;
		// Text Offset, vert, horz
//		if (vp380_GotCommand ("D/O0305"))
//			return;
		// Char Gen Off
		if (vp380_GotCommand ("D/E0"))
		{
			g_LDP1450_TextControl.LDP1450_DisplayOn = 0;
			return;
		}
		// Char Gen On
		if (vp380_GotCommand ("D/E1"))
		{
			g_LDP1450_TextControl.LDP1450_DisplayOn = 1;
			return;
		}
		// Pic time code display on
//		if (vp380_GotCommand ("D1"))
//			return;
		// Pic time code display off (default)
//		if (vp380_GotCommand ("D0"))
//			return;
		// Audio Channel B ON
		if (vp380_GotCommand ("B1"))
		{
			g_ldp->enable_audio2();	
			return;
		}
		// Audio Channel A ON
		if (vp380_GotCommand ("A1"))
		{
			g_ldp->enable_audio1();	
			return;
		}
		// Audio Channel B OFF
		if (vp380_GotCommand ("B0"))
		{
			g_ldp->disable_audio2();	
			return;
		}
		// Audio Channel A OFF
		if (vp380_GotCommand ("A0"))
		{
			g_ldp->disable_audio1();	
			return;
		}
		// Audio Channel B ON
		if (vp380_GotCommand ("B1"))
		{
			g_ldp->enable_audio2();	
			return;
		}
	}
}



bool vp380_result_ready(void)
{
	bool result = false;

	if (vp380_output_stack_pointer)
	{
		result = true;
	}
	return result;
}

void vp380_enter(void)
{
//	char s[81] = {0};
	
	if (enter_status == VP380_SEARCH)
	{
		vp380_repeat = false;
		
		vp380_frame[vp380_frame_index] = 0;
		
		// TODO: check for search failure
		g_ldp->pre_search(vp380_frame, true);

		vp380_frame_index = 0; // reset frame index
		vp380_stack_push(0x0D); // ack
		vp380_stack_push('0'); // ack
		vp380_stack_push('A'); // ack
		enter_status = 0x00;
	}
}

