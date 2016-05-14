/*
 * ____ DAPHNE COPYRIGHT NOTICE ____
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
// hypseus error handling

#include "config.h"

#ifdef WIN32
#include <windows.h>
#endif

#include <SDL.h>
#include <string.h>
#include <plog/Log.h>
#include "../hypseus.h"
#include "conout.h"
#include "input.h"
#include "../game/game.h"
#include "../sound/sound.h"
#include "../video/video.h"

// const char *instr = "Please read the hypseus_log.txt file for more
// information";
const char *instr = "Read hypseus_log.txt for help";

// notifies the user of an error that has occurred
void printerror(const char *s)
{
    //	SDL_Rect region = { 0, 180, get_video_width(),
    //(Uint16)(get_video_height() >> 1) };

#ifdef WIN32
    MessageBox(NULL, s, "Encountered an error", MB_OK | MB_ICONERROR);
#else
    LOGE << s;
#endif
}

// notifies user that the game does not work correctly and gives a reason
// this should be called after video has successfully been initialized
void printnowookin(const char *s)
{
    LOGE << s;
}

// prints a notice to the screen
void printnotice(const char *s)
{
    LOGE << s;
}
