/*
 * ldp1000.h
 *
 * Copyright (C) 2001 Mark Broadhead
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


// LDP1000.H

#ifndef LDP1000_H
#define LDP1000_H

// if the next argument will be used to search to a frame
#define LDP1000_SEARCH	(1 << 0)

// if the next argument will be used to specify how many times to play through
#define LDP1000_REPEAT_NUMBER	(1 << 1)

// if the next argument will be used to specify which frame to begin playing to repeat
#define LDP1000_REPEAT		(1 << 2)

// mutually exclusive states that LDP1000 can be in
enum ldp1000_state
{
	// neither searching nor repeating
	LDP1000_STATE_NORMAL,

	// waiting for search to complete
	LDP1000_STATE_SEARCHING,

	// waiting to get to a certain frame to repeat or stop
	LDP1000_STATE_REPEATING,

	// if we get a C.L. command in the middle of a search
	LDP1000_STATE_SEARCH_ABORTING,
};

// setup variables for use
void reset_ldp1000();

// call ldp1000_result_ready before calling this function!
unsigned char read_ldp1000();

void write_ldp1000(unsigned char value);
void ldp1000_enter(void);

// function should be called regularly to check on search completion and repeat status
void ldp1000_think();

bool ldp1000_result_ready(void);
int ldp1000_stack_push(unsigned char value);
void ldp1000_add_digit(char);

// LDP1450 - Text Display (up to 3 lines)
typedef struct {
	char	String[128];
	int		line;
	int		mode;
	int		x;
	int		y;
}ldp_text;

typedef struct {
	int		LDP1450_DisplayOn;	// turn on/off

	char	TextString[128];		// temp storage
	int		TextStringPointer;	// 
	bool		TextCommand;	// text x080	// MPO : changed to a bool since that seems to be what it is being used for
	
	bool	bGotSetWindow;
	int		TextGot00;
	int		TextGotXpos;
	int		TextGotYpos;
	int		TextGetXY;
	int		TextGotString;
	int		TextXPos;
	int		TextYPos;
	int		TextLine;
	int		TextMode;
	float	Scale;
}ldp_text_control;

extern	ldp_text	g_LDP1450_Strings[3];
extern  ldp_text_control	g_LDP1450_TextControl;
#endif
