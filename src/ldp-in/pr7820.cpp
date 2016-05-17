/*
* ____ DAPHNE COPYRIGHT NOTICE ____
*
* Copyright (C) 2003 Warren Ondras
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

// based on ldv1000.cpp
// started by Matt Ownby
// contributions made by Mark Broadhead

// This code emulates the Pioneer PR-7820 laserdisc player which is used in
// these games:
// Dragon's Lair US
// Space Ace US
// Thayer's Quest
//
// all games (except for ROM revisions D and earlier of Dragon's Lair) can also
// use an LD-V1000,
// which is preferred due to lower latency and better reporting of status
// information.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../game/game.h"
#include "pr7820.h"
#include "../game/game.h"
#include "../io/conout.h"
#include "../ldp-out/ldp.h"
#include <plog/Log.h>

namespace pr7820
{
bool g_ready = 1; // The /READY line on player interface, not ready while
                         // stopped

Uint16 g_autostop_frame = 0; // which frame we need to stop on (if any)

// whether a non-blocking search is pending
bool g_bPR7820SearchPending = false;

bool audio1 = true; // default audio status is on
bool audio2 = true;

#define PR7820_STACKSIZE 10
unsigned char g_output_stack[PR7820_STACKSIZE];
int g_output_stack_pointer = 0;

char frame[FRAME_ARRAY_SIZE] = {0}; // holds the digits sent to the
                                           // PR-7820
int frame_index = 0;

///////////////////////////////////////////

// retrieves the status from our virtual LD-V1000
bool read_ready()
{
    // if we're waiting for a search to complete
    if (g_bPR7820SearchPending) {
        int stat = g_ldp->get_status();

        // if we finished seeking and found success
        if (stat == LDP_PAUSED) {
            g_ready         = 0; // set to lo (0) which means ready
            g_bPR7820SearchPending = false;
            LOGD << "search succeeded";
        }
        // search failed for whatever reason ...
        else if (stat == LDP_ERROR) {
            g_ready = 1; // leave it as 1 (not ready) to indicate a
                                // problem
            g_bPR7820SearchPending = false;
        }
        // else search is still pending
    }

    // if autostop is active, we need to check to see if we need to stop
    else if (g_autostop_frame != 0) //  none the games actually need
                                           //  autostop anyway...
    {
        //			char f[81];
        //			sprintf(f,"Watching Frame:
        //%05u",g_ldp->get_current_frame());
        //            tms9128nl_outcommand(f,43,23);

        // if we've hit the frame we need to stop on (or gone too far) then stop
        if (g_ldp->get_current_frame() >= g_autostop_frame) {
            g_ldp->pre_pause();
            g_autostop_frame = 0;
        }
    }

    return (g_ready);
}

// pushes a value on the pr7820 stack, returns 1 if successful or 0 if stack is
// full
int stack_push(unsigned char value)
{
    int result = 0;

    // if we still have room to push
    if (g_output_stack_pointer < PR7820_STACKSIZE - 1) {
        g_output_stack[g_output_stack_pointer++] = value;
        result                                                 = 1;
    } else {
        LOGE << "stack overflow (increase its size)";
    }

    return (result);
}

// sends a byte to our virtual PR-7820
void write(unsigned char value)
{

    //	char f[81] = { 0 };
    //	Uint16 curframe = 0;	// current frame we're on

    LOGD << fmt("sent a %x", value);

    switch (value) {
    case 0x3F: // 0
        add_digit('0');
        break;
    case 0x0F: // 1
        add_digit('1');
        break;
    case 0x8F: // 2
        add_digit('2');
        break;
    case 0x4F: // 3
        add_digit('3');
        break;
    case 0x2F: // 4
        add_digit('4');
        break;
    case 0xAF: // 5
        add_digit('5');
        break;
    case 0x6F: // 6
        add_digit('6');
        break;
    case 0x1F: // 7
        add_digit('7');
        break;
    case 0x9F: // 8
        add_digit('8');
        break;
    case 0x5F: // 9
        add_digit('9');
        break;
    case 0xF4: // Audio 1
        pre_audio1();
        break;
    case 0xFC: // Audio 2
        pre_audio2();
        break;
    case 0xA3: // play at 1X
        // it is necessary to set the playspeed because otherwise the ld-v1000
        // ignores skip commands
        g_ldp->pre_change_speed(1, 1);
        break;
    case 0xF9: // Reject - Stop the laserdisc player from playing
        LOGD << "Reject received (ignored)";
        // FIXME: laserdisc state should go into parked mode, but most people
        // probably don't want this to happen (I sure don't)
        break;
    case 0xF3: // Auto Stop (used by Esh's)
        // This command accepts a frame # as an argument and also begins playing
        // the disc
        g_autostop_frame = get_buffered_frame();
        clear();
        g_ldp->pre_play();
        {
            LOGD << fmt("Auto-Stop requested at frame %u", g_autostop_frame);
        }
        break;
    case 0xFD: // Play
        g_ldp->pre_play();
        break;
    case 0xF7:                                // Search
        frame[frame_index] = 0; // null terminate string
        g_ready                   = 1; // set /READY line high to indicate we are busy

        // if search failed
        if (g_ldp->pre_search(frame, false)) {
            g_bPR7820SearchPending = true;
        } else {
            LOGE << "Bad search from player!";
            // leave /READY high (indicates search never completed)
        }

        frame_index = 0; // clear frame buffer

        break;
    case 0xFB: // Stop - this actually just goes into still-frame mode, so we
               // pause
        g_ldp->pre_pause();
        break;
    case 0xFF: // NO ENTRY
        // Does the PR-7820 really support 0xFF? or is this just leftover from
        // the ldv1000 driver...?
        break;
    default: // Unsupported Command
        LOGE << fmt("Unsupported PR-7820 Command Received: %x", value);
        break;
    }
}

/*
// As you can see we are currently not supporting display disable
void pre_display_disable()
{

LOGD << "Display disable received";

}

// We are not currently supporting display enable
void pre_display_enable()
{

LOGD << "Display enable received";

}
*/

// adds a digit to  the frame array that we will be seeking to
// digit should be in ASCII format
void add_digit(char digit)
{
    if (frame_index < FRAME_SIZE) {
        frame[frame_index] = digit;
        frame_index++;
    } else {
        for (int count = 1; count < FRAME_SIZE; count++) {
            frame[count - 1]              = frame[count];
            frame[frame_index - 1] = digit;
        }

        /*
        char s[81] = { 0 };

        sprintf(s, "Too many digits received for frame! (over %d)", FRAME_SIZE);
        printline(s);
        */
    }
}

// Audio channel 1 on or off
void pre_audio1()
{
    // Check if we should just toggle
    if (frame_index == 0) {
        // Check status of audio and toggle accordingly
        if (audio1) {
            audio1 = false;
            g_ldp->disable_audio1();
        } else {
            audio1 = true;
            g_ldp->enable_audio1();
        }
    }
    // Or if we have an explicit audio command
    else {
        switch (frame[0] % 2) {
        case 0:
            audio1 = false;
            g_ldp->disable_audio1();
            break;
        case 1:
            audio1 = true;
            g_ldp->enable_audio1();
            break;
        default:
            LOGE << "Ummm... you shouldn't get this";
        }

        frame_index = 0;
    }
}

// Audio channel 2 on or off
void pre_audio2()
{
    // Check if we should just toggle
    if (frame_index == 0) {
        // Check status of audio and toggle accordingly
        if (audio2) {
            audio2 = false;
            g_ldp->disable_audio2();
        } else {
            audio2 = true;
            g_ldp->enable_audio2();
        }
    }
    // Or if we have an explicit audio command
    else {
        switch (frame[0] % 2) {
        case 0:
            audio2 = false;
            g_ldp->disable_audio2();
            break;
        case 1:
            audio2 = true;
            g_ldp->enable_audio2();
            break;
        default:
            LOGE << "Ummm... you shouldn't get this";
        }

        frame_index = 0;
    }
}

// returns the frame that has been entered in by add_digit thus far
Uint16 get_buffered_frame()
{
    frame[frame_index] = 0; // terminate string
    return ((Uint16)atoi(frame));
}

// clears any received digits from the frame array
void clear(void) { frame_index = 0; }

void reset()
{
    // anything else?
    g_output_stack_pointer = 0;
    g_autostop_frame       = 0; // which frame we need to stop on (if any)

    frame_index = 0;

    g_ready = 1; // PR-7820 is PARK'd and NOT READY (when stopped it's
                        // not ready)
}
}
