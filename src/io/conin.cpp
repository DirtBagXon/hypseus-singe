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

// returns an ASCII character from the keyboard; will wait here forever until a
// key is pressed
// not all characters are supported
SDL_Scancode con_getkey()
{

    SDL_Scancode result;
    SDL_Event event;

    while (!result && !get_quitflag()) {
        if (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYUP:
                result = event.key.keysym.scancode;
                break;
            // WO: make any joystick button = enter,
            // so lazy folks (like me) can pass the issues screen
            // without going to the keyboard :)
            case SDL_JOYBUTTONDOWN:
                result = SDL_SCANCODE_RETURN;
                break;
            case SDL_QUIT:
                set_quitflag();
                break;
            default:
                break;
            }
        } // end if

        SDL_Delay(1); // sleep to be CPU friendly

    } // end while

    return (result);
}

// returns a null-terminated string from "stdin"; if there was an error, an
// empty string will be returned
// NOTE: the string is echoed to the screen as the user types it in
// 'length' is the maximum size of 'buf'

void con_getline(char *buf, int length)
{

    int index = 0;
    char ch = 0;
    SDL_Event event;

    SDL_StartTextInput();
    // loop until we encounter a linefeed or carriage return, or until we run
    // out of space
    while (!get_quitflag()) {
        if (SDL_PollEvent(&event)) {
            if(event.type == SDL_TEXTINPUT) {
                ch = con_getkey();

                // if we get a linefeed or carriage return, we're done ...
                if ((ch == 10) || (ch == 13)) {
                    break;
                }

                // backspace, it works but it doesn't look pretty on the screen
                else if (ch == 8) {
                    if (index > 0) {
                        index = index - 1;
                        outstr("[BACK]");
                    }
                }

                // if we've run out of space, terminate the string prematurely to avoid
                // crashing
        
                else if (index >= (length - 1)) {
                    break;
                }
        
                // otherwise if we get a printable character, add it to the string
                else if (ch >= ' ') {
        
                    outchr(ch); // echo it to the screen
                    buf[index] = ch;
                    index      = index + 1;
                }

                // else it was a non-printable character, so we ignore it
            }
        }
    }
    SDL_StopTextInput();

    buf[index] = 0; // terminate the string
    newline();      // go to the next line
}
