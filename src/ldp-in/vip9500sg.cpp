/*
 * ____ DAPHNE COPYRIGHT NOTICE ____
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

// vip9500sg.c
// part of the DAPHNE emulator
// written by Mark Broadhead
// contributions made by Matt Ownby
//
// This code emulates the Hitachi VIP-9550SG laserdisc player which is used in
//  Astron Belt, Galaxy Ranger, and others running on the same hardware

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <queue>
#include "../game/game.h"
#include "vip9500sg.h"
#include "../io/conout.h"
#include "../ldp-out/ldp.h"
#include "../hypseus.h" // for set_quitflag
#include <plog/Log.h>

using namespace std;

namespace vip9500sg
{
// holds serial data returns by the VIP9500
queue<unsigned char> g_qu8VIP9500Output;

char frame[FRAME_ARRAY_SIZE] = {0}; // holds the digits sent to the
                                              // VIP9500SG
int frame_index = 0;

// default status
unsigned int enter_status = 0;

// whether the laserdisc player is searching or not
bool g_bVIP9500SG_Searching = false;

///////////////////////////////////////////

// retrieves the status from our virtual VIP9500SG
unsigned char read()
{
    unsigned char result = 0x00;

    if (!g_qu8VIP9500Output.empty()) {
        result = g_qu8VIP9500Output.front();
        g_qu8VIP9500Output.pop();
    } else {
        LOGE << "queue read when empty";
    }

    return (result);
}

// pushes a value on the vip9500sg queue
bool queue_push(unsigned char value)
{
    g_qu8VIP9500Output.push(value);

    // doesn't return an error because this almost never can fail
    return true;
}

// sends a byte to our virtual VIP9500SG
void write(unsigned char value)
{
    switch (value) {
    case 0x25: // Play
        g_ldp->pre_play();
        queue_push(0xa5); // play success
        break;
    case 0x29: // Step Reverse
        // Astron, GR, and Cobra Command only seem to use this for pause
        g_ldp->pre_pause();
        LOGD << "Step Reverse (pause)";
        queue_push(0xa9); // step backwards success
        break;
    case 0x2b: // Search
        enter_status = VIP9500SG_SEARCH;
        break;
    case 0x2f: // Stop
        LOGD << "Reject received (ignored)";
        queue_push(0xaf); // stop success
        break;
    case 0x30: // '0'
    case 0x31: // '1'
    case 0x32: // '2'
    case 0x33: // '3'
    case 0x34: // '4'
    case 0x35: // '5'
    case 0x36: // '6'
    case 0x37: // '7'
    case 0x38: // '8'
    case 0x39: // '9'
        add_digit(value);
        break;
    case 0x41: // Enter
        enter();
        break;
    case 0x46: // Skip Forward
        enter_status = VIP9500SG_SKIP_FORWARD;
        break;
    case 0x4C: // frame counter on, NOTE : if disc is stopped this will return
               // an error 0x1D
        LOGD << "Frame counter on (ignored)";
        queue_push(0xcc); // frame counter on success
        break;
    case 0x4d: // Frame counter off, NOTE : if disc is stopped this will return
               // an error 0x1D
        LOGD << "Frame counter off (ignored)";
        queue_push(0xcd); // frame counter off?
        break;
    case 0x50: // Pause with sound enabled (!!)
    case 0x51: // very slow motion with sound enabled
    case 0x52: // slow motion with sound enabled
    case 0x54: // play faster with sound enabled
    case 0x55: // play even faster with sound enabled
    case 0x56: // " " "
    case 0x57: // " " "
    case 0x58: // " " "
    case 0x59: // " " "
        LOGE << "Unsupported variable speed command requested!";
        break;
    case 0x53: // Play forward at 1X with sound enabled, note that if disc is
               // stopped this will return an error 0x1D
        g_ldp->pre_play();
        queue_push(0xd3); // forward 1X success
        break;
    case 0x68: // Reset
        // Do some reset stuff - turn off frame counter etc
        LOGD << "RESET! (ignored)";
        queue_push(0xe8); // reset success
        break;
    case 0x6b: // Report Current Frame
    {
        unsigned int curframe = g_ldp->get_current_frame();
        queue_push(0x6b);                            // frame
        queue_push((Uint8)((curframe >> 8) & 0xff)); // high byte of
                                                               // frame
        queue_push((Uint8)(curframe & 0xff)); // low byte of frame
    } break;
    case 0x6e:                      // UNKNOWN
        queue_push(0xee); // unknown
        break;
    default:
        LOGE << fmt("Unsupported VIP9500SG Command Received: %x", value);
        break;
    }
}

void think()
{
    // if we're waiting for a search to complete ...
    if (g_bVIP9500SG_Searching) {
        int iStat = g_ldp->get_status();

        // if we're paused, it means we're done seeking
        if (iStat == LDP_PAUSED) {
            LOGD << "Search Complete";
            queue_push(0x41); // search success
            queue_push(0xb0);
            g_bVIP9500SG_Searching = false;
        }
        // else if we're not seeking as we expect to be, return an error
        else if (iStat != LDP_SEARCHING) {
            // if this ever happens, we'll need to find out what the search
            // error result code is :)
            LOGE << "search failed and we don't handle this condition so we're aborting";
            set_quitflag();
        }
        // else we're still searching
    }
    // else nothing to do
}

bool result_ready(void)
{
    bool result = false;

    think();

    // if the queue is not empty, then we have a result ready
    if (!g_qu8VIP9500Output.empty()) {
        result = true;
    }
    return result;
}

void enter(void)
{
    if (enter_status == VIP9500SG_SEARCH) {
        // make sure that the ROM checked the search result before sending
        // another search command
        if (!g_bVIP9500SG_Searching) {
            // TODO: check for search failure
            g_ldp->pre_search(frame, false);
            g_bVIP9500SG_Searching = true;
        } else {
            LOGW << "ROM did not check search result before sending another search command (therefore we are ignoring the second search command)";
        }

        frame_index = 0; // reset frame index
    }

    else if (enter_status == VIP9500SG_SKIP_FORWARD) {
        // TODO: check for skip failure
        frame[frame_index] = 0;

        Uint16 frames_to_skip = (Uint16)atoi(frame);

        g_ldp->pre_skip_forward(frames_to_skip);

        frame_index = 0; // reset frame index

        queue_push(0x41); // skip success
        queue_push(0xc6);
    }

    enter_status = 0x00;
}

// adds a digit to  the frame array that we will be seeking to
// digit should be in ASCII format
void add_digit(char digit)
{
    if (frame_index < FRAME_SIZE) {
        frame[frame_index] = digit;
        frame_index++;
    } else {
        LOGW << fmt("Too many digits received for frame! (over %d)", FRAME_SIZE);
    }
}
}
