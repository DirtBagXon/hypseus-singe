/*
 * conin.cpp
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


// Routines to replace stdin routines such as 'gets'
// Purpose: So we can use the Z80 debugger with our GUI

#include <SDL.h>
#include "conout.h"
#include "conin.h"
#include "../daphne.h"
#include "input.h"

#ifdef UNIX
#include <sys/select.h>
#endif

// returns an ASCII character from the keyboard; will wait here forever until a key is pressed
// not all characters are supported
char con_getkey()
{

	unsigned char result = 0;
	SDL_Event event;

#ifdef CPU_DEBUG
		con_flush(); // print current line
#endif

		SDL_EnableUNICODE(1);
		while (!result && !get_quitflag())
		{
			if (SDL_PollEvent (&event))
			{
				switch (event.type)
				{
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
					case SDLK_BACKSPACE:
						result = 8;
						break;
					case SDLK_RETURN:
						result = 13;
						break;
					case SDLK_ESCAPE:
						result = 27;
						break;
					case SDLK_BACKQUOTE:
#ifdef CPU_DEBUG
						toggle_console();
#endif
						result = 0;
						break;
					default:
						result = (unsigned char) event.key.keysym.unicode;
						break;
					}
					break;
				//WO: make any joystick button = enter,
				// so lazy folks (like me) can pass the issues screen
				// without going to the keyboard :)
				case SDL_JOYBUTTONDOWN:
#ifndef GP2X
					result = 13;
#else
					// one of the buttons needs to exit the emulator
					switch(event.jbutton.button)
						{
							// L is #10, and it will map to escape for now ...
							case 10:
								result = 27;
								break;
							case 15:	// Y (hide console)
#ifdef CPU_DEBUG
								toggle_console();
#endif
								result = 0;
								break;
							default:
								result = 13;
								break;
						}
						break;
#endif
					break;
				case SDL_QUIT:
					set_quitflag();
					break;
				default:
					break;
				}
			} // end if

			check_console_refresh();
			SDL_Delay(1);	// sleep to be CPU friendly
			
#ifdef UNIX
			// NOTE : non-blocking I/O should be enabled, so we will get EOF if there is nothing waiting for us ...
			int iRes = getchar();
			if (iRes != EOF)
			{
				result = (unsigned char) iRes;
			}
#endif

			
		} // end while
		SDL_EnableUNICODE(0);

		
	return(result);
}


// returns a null-terminated string from "stdin"; if there was an error, an empty string will be returned
// NOTE: the string is echoed to the screen as the user types it in
// 'length' is the maximum size of 'buf'

void con_getline(char *buf, int length)

{

	int index = 0;
	char ch = 0;

	// loop until we encounter a linefeed or carriage return, or until we run out of space
	while (!get_quitflag())
	{
		ch = con_getkey();

		// if we get a linefeed or carriage return, we're done ...
		if ((ch == 10) || (ch == 13))
		{
			break;
		}

		// backspace, it works but it doesn't look pretty on the screen
		else if (ch == 8)
		{
			if (index >0)
			{
				index = index - 1;
				outstr("[BACK]");
			}
		}
		
		// if we've run out of space, terminate the string prematurely to avoid crashing

		else if (index >= (length - 1))
		{
			break;
		}

		// otherwise if we get a printable character, add it to the string
		else if (ch >= ' ')
		{

			outchr(ch);	// echo it to the screen
			buf[index] = ch;
			index = index + 1;
		}

		// else it was a non-printable character, so we ignore it
	}

	buf[index] = 0;	// terminate the string
	newline();		// go to the next line

}
