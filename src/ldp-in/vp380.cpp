/*
 * ____ DAPHNE COPYRIGHT NOTICE ____
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
#include "ldp1000.h" // text stuff
#include "../io/conout.h"
#include "../ldp-out/ldp.h"
#include <plog/Log.h>

#define VP380_STACKSIZE 10 // the largest response is 8 bytes

namespace vp380
{
unsigned char output_stack[VP380_STACKSIZE];
int output_stack_pointer = 0;
unsigned int curframe;
unsigned int repeat_frame;
unsigned int repeat_start_frame;
bool repeat         = false;
int times_to_repeat = 0;

char frame[FRAME_ARRAY_SIZE] = {0}; // holds the digits sent to the vp380
int frame_index              = 0;

int yslots[3] = {0}; // 3 lines of text

// default status
static unsigned char enter_status = 0x00;
///////////////////////////////////////////

// retrieves the status from our virtual VP380
// returns -1 if there is nothing to return
int read()
{
    if (output_stack_pointer < 1) return (-1);

    output_stack_pointer--;
    return (output_stack[output_stack_pointer]);
}

// pushes a value on the vp380 stack, returns 1 if successful or 0 if stack is
// full
int stack_push(unsigned char value)
{
    int result = 0;

    // if we still have room to push
    if (output_stack_pointer < VP380_STACKSIZE - 1) {
        output_stack[output_stack_pointer++] = value;
        result                                           = 1;
    } else {
        LOGE << "vp380 stack overflow (increase its size)";
    }

    return (result);
}

// functions to see if we have a command
char CommandBuffer[32];
int CommandIndex = 0;

// regular command
int GotCommand(const char* Command)
{
    if (strcmp(Command, &CommandBuffer[0]) == 0) {
        memset(&CommandBuffer, 0, 32);
        return 1;
    } else
        return 0;
}

// search command 'F12345R'
int GotSearch()
{
    if (CommandBuffer[0] == 'F' && CommandBuffer[6] == 'R')
        return 1;
    else
        return 0;
}

// text command "D/Wxxyy@text@"
int GotText()
{
    if (CommandBuffer[0] == 'D' && CommandBuffer[1] == '/' && CommandBuffer[2] == 'W')
        return 1;
    else
        return 0;
}

// sends a byte to our virtual vp380
void write(unsigned char value)
{
    //	char s[81] = { 0 };

    if (value == 0x0C) return; // what is this

    // Store off command char
    if (value != 0x12 && value != 0x0D) {
        CommandBuffer[CommandIndex] = value;
        CommandIndex++;
    } else // parse the command
    {
        CommandBuffer[CommandIndex] = 0; // eos
        CommandIndex                = 0;

        // Frame Request
        if (GotCommand("?F")) {
            curframe = g_ldp->get_current_frame();
            char f[8];
            // itoa(curframe, &f[0], 10);	// should be faster than sprintf
            safe_itoa(curframe, f, sizeof(f)); // MPO : linux doesn't have
                                                     // itoa
            sprintf(f, "%05d", curframe);
            stack_push(0x0D); // M7
            stack_push(f[4]); // M6
            stack_push(f[3]); // M5
            stack_push(f[2]); // M4
            stack_push(f[1]); // M3
            stack_push(f[0]); // M2
            stack_push('F');  // M1
            return;
        }
        // Search
        if (GotSearch()) {
            frame[0] = CommandBuffer[1];
            frame[1] = CommandBuffer[2];
            frame[2] = CommandBuffer[3];
            frame[3] = CommandBuffer[4];
            frame[4] = CommandBuffer[5];
            memset(&CommandBuffer, 0, 32);
            frame_index = 5;
            enter_status = VP380_SEARCH;
            enter();
            return;
        }
        // Clear Screen
        if (GotCommand("D/HCL")) {
            for (int i = 0; i < 3; i++) ldp1000::g_LDP1450_Strings[i].String[0] = 0;
            return;
        }
        // Forward Play
        if (GotCommand("N")) {
            g_ldp->pre_play();
            return;
        }
        // Reverse Play
        //		if (GotCommand ("O"))
        //			return;
        // Still
        if (GotCommand("*")) {
            g_ldp->pause();
            return;
        }
        // Text String
        if (GotText()) {
            char xc[4];
            char yc[4];
            int x;
            int y;

            // get x,y
            strncpy(xc, &CommandBuffer[5], 2);
            xc[2] = 0;
            strncpy(yc, &CommandBuffer[3], 2);
            yc[2] = 0;
            x     = atoi(xc);
            y     = atoi(yc);

            // do we have this line already
            // vp380 expects a char style overwrite.. since we dont have that
            // luxury we have to emulate it.. if its the same line, overwrite it
            for (int j = 0; j < 3; j++) {
                if (ldp1000::g_LDP1450_Strings[j].y == (36 * y) - 20) {
                    ldp1000::g_LDP1450_TextControl.TextLine = j;
                    break;
                }
            }

            ldp1000::g_LDP1450_TextControl.Scale                         = 2.0f;
            ldp1000::g_LDP1450_Strings[ldp1000::g_LDP1450_TextControl.TextLine].x = 34 * x;
            ldp1000::g_LDP1450_Strings[ldp1000::g_LDP1450_TextControl.TextLine].y = (36 * y) - 20;

            int i = 0;

            // ignore the @, but copy text over til the @
            while (1) {
                if (CommandBuffer[8 + i] != '@')
                    ldp1000::g_LDP1450_Strings[ldp1000::g_LDP1450_TextControl.TextLine].String[i] =
                        CommandBuffer[8 + i];
                else {
                    ldp1000::g_LDP1450_TextControl.LDP1450_DisplayOn                     = 1;
                    ldp1000::g_LDP1450_Strings[ldp1000::g_LDP1450_TextControl.TextLine].String[i] = 0;
                    {
                        ldp1000::g_LDP1450_TextControl.TextLine++;
                        if (ldp1000::g_LDP1450_TextControl.TextLine > 2)
                            ldp1000::g_LDP1450_TextControl.TextLine = 0;
                    }
                    break;
                }
                i++;
            }
            return;
        }
        // clear
        if (GotCommand("X")) {
            // clear our strings
            for (int i = 0; i < 3; i++) ldp1000::g_LDP1450_Strings[i].String[0] = 0;

            ldp1000::g_LDP1450_TextControl.TextLine = 0;
            return;
        }
        // load disc
        if (GotCommand(",1")) {
            stack_push('S'); // ack
            return;
        }
        // NTSC video
        //		if (GotCommand ("EN"))
        //			return;
        // TX Off
        //		if (GotCommand (")0"))
        //			return;
        // CX auto on/off
        //		if (GotCommand ("(X"))
        //			return;
        // CX auto on/off
        //		if (GotCommand ("(X"))
        //			return;
        // Text Vertical Size (default)
        //		if (GotCommand ("D/S00"))
        //			return;
        // Text Vertical Size (default)
        //		if (GotCommand ("D/S00"))
        //			return;
        // Text Background Off
        //		if (GotCommand ("D/B0"))
        //			return;
        // Text Offset, vert, horz
        //		if (GotCommand ("D/O0305"))
        //			return;
        // Char Gen Off
        if (GotCommand("D/E0")) {
            ldp1000::g_LDP1450_TextControl.LDP1450_DisplayOn = 0;
            return;
        }
        // Char Gen On
        if (GotCommand("D/E1")) {
            ldp1000::g_LDP1450_TextControl.LDP1450_DisplayOn = 1;
            return;
        }
        // Pic time code display on
        //		if (GotCommand ("D1"))
        //			return;
        // Pic time code display off (default)
        //		if (GotCommand ("D0"))
        //			return;
        // Audio Channel B ON
        if (GotCommand("B1")) {
            g_ldp->enable_audio2();
            return;
        }
        // Audio Channel A ON
        if (GotCommand("A1")) {
            g_ldp->enable_audio1();
            return;
        }
        // Audio Channel B OFF
        if (GotCommand("B0")) {
            g_ldp->disable_audio2();
            return;
        }
        // Audio Channel A OFF
        if (GotCommand("A0")) {
            g_ldp->disable_audio1();
            return;
        }
        // Audio Channel B ON
        if (GotCommand("B1")) {
            g_ldp->enable_audio2();
            return;
        }
    }
}

bool result_ready(void)
{
    bool result = false;

    if (output_stack_pointer) {
        result = true;
    }
    return result;
}

void enter(void)
{
    //	char s[81] = {0};

    if (enter_status == VP380_SEARCH) {
        repeat = false;

        frame[frame_index] = 0;

        // TODO: check for search failure
        g_ldp->pre_search(frame, true);

        frame_index = 0;  // reset frame index
        stack_push(0x0D); // ack
        stack_push('0');  // ack
        stack_push('A');  // ack
        enter_status = 0x00;
    }
}
}
