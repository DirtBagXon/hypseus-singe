/*
 * error.cpp
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


// error.cpp
// daphne error handling

#ifdef WIN32
#include <windows.h>
#endif

#include <SDL.h>
#include <string.h>
#include "../daphne.h"
#include "conout.h"
#include "conin.h"
#include "input.h"
#include "../game/game.h"
#include "../sound/sound.h"
#include "../video/video.h"
#include "../video/SDL_DrawText.h"

#ifndef GP2X
//const char *instr = "Please read the daphne_log.txt file for more information";
const char *instr = "Read daphne_log.txt for help";
#else
const char *instr = "Read daphne_log.txt for help";
#endif

const char CRLF[3] = { 13, 10, 0 };	// carriage return / linefeed combo, for the addlog statements in this file

// notifies the user of an error that has occurred
void printerror(const char *s)
{
//	SDL_Rect region = { 0, 180, get_video_width(), (Uint16)(get_video_height() >> 1) };

	addlog(s);
	addlog(CRLF);

	// if video has been initialized
	// FIXME: if bmp hasn't been loaded this might blow up
	if (get_console_initialized())
	{
		SDL_Surface *srfScreen = get_screen_blitter();
		unsigned int uXOffset = 0;

		vid_blank();	// needed for opengl

		// only draw this graphic if our width is at least 640 (it can be smaller on handheld)
		if (srfScreen->w >= 640)
		{
			uXOffset = 80;	// shift to the right to compensate for graphic
			draw_othergfx(B_DAPHNE_SAVEME, 0, 180);
		}

		// make sure text is centered
		SDLDrawText(s, srfScreen, FONT_SMALL, ((srfScreen->w >> 1) + uXOffset -((strlen(s) >> 1)*6)), (srfScreen->h >> 1)-13);
		SDLDrawText(instr, srfScreen, FONT_SMALL, ((srfScreen->w >> 1) + uXOffset -((strlen(instr) >> 1)*6)), (srfScreen->h >> 1));

		vid_blit(srfScreen, 0, 0);
		vid_flip();

		// play a 'save me' sound if we've got sound
		if (get_sound_initialized())
		{
			sound_play_saveme();
		}
		
		con_getkey();	// wait for keypress

		vid_blank();
		vid_flip();

		// MATT : we can't call video_repaint because the video might not have been initialized
		// yet.  We can either comment this out, or add safety checks in video_repaint (which
		// is kind of a hassle right now since we have a bunch of redundant copies everywhere)

	} // end if video has been initialized

	// if video has not been initialized, print an error any way that we can
	else
	{
#ifdef WIN32
	MessageBox(NULL, s, "DAPHNE encountered an error", MB_OK | MB_ICONERROR);
#else
		printf("%s\n",s);
#endif
	}	
}

// notifies user that the game does not work correctly and gives a reason
// this should be called after video has successfully been initialized
void printnowookin(const char *s)
{
	if (get_console_initialized())
	{
		SDL_Surface *srfScreen = get_screen_blitter();

		vid_blank();
		// don't draw 'no wookin' graphic if display is too small (can happen on handheld)
		if (srfScreen->w >= 640) draw_othergfx(B_GAMENOWOOK, 0, 180);
		SDLDrawText(s, srfScreen, FONT_SMALL, ((srfScreen->w >> 1) -((strlen(s) >> 1)*6)), (srfScreen->h >> 1));
		vid_blit(srfScreen, 0, 0);
		vid_flip();
		con_getkey();	// wait for keypress

		// repaint the disrupted overlay (ie Dragon's Lair Scoreboard when using a real LDP)
		display_repaint();
	}
}

// prints a notice to the screen
void printnotice(const char *s)
{
	if (get_console_initialized())
	{
		char ch = 0;
		SDL_Surface *srfScreen = get_screen_blitter();

		SDL_FillRect(srfScreen, NULL, 0);	// draw black background first
		SDLDrawText(s, srfScreen, FONT_SMALL, ((srfScreen->w >> 1)-((strlen(s) >> 1)*6)), (srfScreen->h >> 1));

		vid_blank();	// needed by opengl
		vid_blit(srfScreen, 0, 0);
		vid_flip();

		ch = con_getkey();	// wait for keypress

		// if they pressed escape, quit
		if (ch == 27)
		{
			set_quitflag();
		}
	}
}
