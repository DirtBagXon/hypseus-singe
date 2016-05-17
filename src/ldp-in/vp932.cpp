/*
 * ____ DAPHNE COPYRIGHT NOTICE ____
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
#include <stdlib.h> // for atoi
#include <string.h> // memset, strcpy
#include <queue>    // memset, strcpy
#include "../game/game.h"
#include "vp932.h"
#include "../io/conout.h"
#include "../ldp-out/ldp.h"
#include "../video/video.h"
#include <plog/Log.h>

using namespace std;

#define MAX_COMMAND_SIZE 31

namespace vp932
{
Uint8 command[MAX_COMMAND_SIZE + 1];
int command_pointer = 0;
queue<Uint8> status_queue;
bool g_search_pending = false;
bool g_play_pending   = false;
bool gAudioLeftEn           = true;
bool gAudioRightEn          = true;

// write a byte to the vp932
void write(Uint8 data)
{
    // check if the current character is a carrage return
    // this signifies that we have a complete command
    if (data == 0x0d) {
        process_command();
    }
    // otherwise we have a part of our command so add it to the queue
    else {
        // filter out leading zeros
        if (!((data >= '0' && data <= '9') && command_pointer == 0)) {
            command[command_pointer++] = data;
        }
    }
}

// read a byte from the vp932
Uint8 read()
{
    Uint8 temp = 0;
    if (!status_queue.empty()) {
        temp = status_queue.front();
        status_queue.pop();
    } else {
        LOGE << "Error, status read when empty!";
    }
    return temp;
}

// check to see if data is available to read
bool data_available()
{
    bool result = false;
    if (g_search_pending) {
        // we don't want the response seen until the search is complete
        int stat = g_ldp->get_status();
        // if we finished seeking and found success
        if (stat == LDP_PAUSED) {
            g_search_pending = false;
            if (g_play_pending) {
                restoreAudio();
                g_ldp->pre_play();
                g_play_pending = false;
            }
        }
    }
    if (!g_search_pending) {
        result = !status_queue.empty();
    }

    return result;
}

void process_command()
{
    command[command_pointer] = 0;
    LOGD << fmt("VP932: Got command %s", (const char*)command);
    Uint16 current_frame = 0;
    char frame_string[6];

    switch (command[0]) {
    case '$': {
        if (command[1] == '0') {
            LOGD << "Replay switch disable (ignored)";
        } else if (command[1] == '1') {
            LOGD << "Replay switch enable (ignored)";
        } else {
            LOGE << fmt("Error in '$' command %s", (const char *)command);
        }
    } break;
    case '\'': {
        LOGD << "Eject (ignored)";
    } break;
    case '(': {
        if (command[1] == '0') {
            LOGD << "CX off (ignored)";
        } else if (command[1] == '1') {
            LOGD << "CX on (ignored)";
        } else if (command[1] == 'X') {
            LOGD << "CX normal (ignored)";
        } else {
            LOGE << fmt("Error in '(' command %s", (const char *)command);
        }
    } break;
    case ')': {
        if (command[1] == '0') {
            LOGD << "Transmittion Delay off (ignored)";
        } else if (command[1] == '1') {
            LOGD << "Transmittion Delay on (ignored)";
        } else {
            LOGE << fmt("VP932: Error in ')' command %s", (const char *)command);
        }
    } break;
    // '*' Halt (still mode)
    case '*': {
        g_ldp->pre_pause();
    } break;
    case ',': {
        if (command[1] == '0') {
            LOGD << "Standby (unload) (ignored)";
        }
        // ',1' On (load)
        else if (command[1] == '1') {
            status_queue.push('S'); // ACK - Disc up to speed
            status_queue.push(0x0d);
        } else {
            LOGE << fmt("Error in ')' command %s", (const char *)command);
        }
    } break;
    // '/' Pause (halt + all muted)
    case '/': {
        g_ldp->pre_pause();
    } break;
    case ':': {
        LOGD << "Reset to default values (ignored)";
    } break;
    case '?': {
        // '?F' Picture Number Request (ignored)
        if (command[1] == 'F') {
            current_frame = g_ldp->get_current_frame();
            sprintf(frame_string, "%05d", current_frame);
            status_queue.push('F'); // Frame
            status_queue.push(frame_string[0]);
            status_queue.push(frame_string[1]);
            status_queue.push(frame_string[2]);
            status_queue.push(frame_string[3]);
            status_queue.push(frame_string[4]);
            status_queue.push(0x0d); // Carrage return
        } else if (command[1] == 'C') {
            LOGD << "Chapter Number Request (ignored)";
        } else if (command[1] == 'T') {
            LOGD << "Time Code Request (ignored)";
        } else if (command[1] == 'N') {
            LOGD << "Track Number Information Request (ignored)";
        } else if (command[1] == 'S') {
            LOGD << "Track Start Time Request (ignored)";
        } else if (command[1] == 'I') {
            LOGD << "Disc I.D. request (ignored)";
        } else if (command[1] == 'D') {
            LOGD << "Disc program status request (ignored)";
        } else if (command[1] == 'P') {
            LOGD << "Drive status request (ignored)";
        } else if (command[1] == 'E') {
            LOGD << "Disc lead-out start request (ignored)";
        } else if (command[1] == 'U') {
            LOGD << "User code request (ignored)";
        } else if (command[1] == '=') {
            LOGD << "Revision level request (ignored)";
        } else {
            LOGE << fmt("Error in '?' command %s", (const char *)command);
        }
    } break;
    case 'A': {
        // Audio-1 off
        if (command[1] == '0') {
            g_ldp->disable_audio1();
            gAudioLeftEn = false;
        }
        // Audio-1 on
        else if (command[1] == '1') {
            g_ldp->enable_audio1();
            gAudioLeftEn = true;
        } else {
            LOGE << fmt("Error in 'A' command %s", (const char *)command);
        }
    } break;
    case 'B': {
        // Audio-2 off
        if (command[1] == '0') {
            g_ldp->disable_audio2();
            gAudioRightEn = false;
        }
        // Audio-2 on
        else if (command[1] == '1') {
            g_ldp->enable_audio2();
            gAudioRightEn = true;
        } else {
            LOGE << fmt("Error in 'B' command %s", (const char *)command);
        }
    } break;
    case 'C': {
        if (command[1] == '0') {
            LOGD << "Chapter number display off (ignored)";
        } else if (command[1] == '1') {
            LOGD << "Chapter number display on (ignored)";
        } else {
            LOGE << fmt("Error in 'C' command %s", (const char *)command);
        }
    } break;
    case 'D': {
        if (command[1] == '0') {
            LOGD << "Picture Number/time code display off (ignored)";
        } else if (command[1] == '1') {
            LOGD << "Picture Number/time code display on (ignored)";
        } else if (command[1] == '/') {
            LOGD << "Text on screen (ignored)";
        } else {
            LOGE << fmt("Error in 'D' command %s", (const char *)command);
        }
    } break;
    case 'E': {
        if (command[1] == '0') {
            LOGD << "Video Off (ignored)";
        } else if (command[1] == '1') {
            LOGD << "Video On (ignored)";
        } else if (command[1] == 'M') {
            LOGD << "Video multistandard (ignored)";
        } else if (command[1] == 'P') {
            LOGD << "Video transcoded PAL (ignored)";
        } else if (command[1] == 'N') {
            LOGD << "Video transcoded NTSC (ignored)";
        } else {
            LOGE << fmt("Error in 'E' command %s", (const char *)command);
        }
    } break;
    case 'F': {
        if (command_pointer == 7) {
            if (command[6] == 'I') {
                LOGD << "Load picture number information register (ignored)";
            } else if (command[6] == 'S') {
                LOGD << "Load picture number stop register (ignored)";
            } else if (command[6] == 'R') {
                // terminate the frame
                command[6] = 0;
                g_ldp->pre_search((const char *)&command[1], false);
                g_search_pending = true;

                status_queue.push('A'); // ACK - Search complete
                status_queue.push('0');
                status_queue.push(0x0d);
            } else if (command[6] == 'N') {
                // terminate the frame
                command[6] = 0;
                g_ldp->pre_search((const char *)&command[1], false);
                g_search_pending = true;
                g_play_pending   = true;

                status_queue.push('A'); // ACK - Search complete
                status_queue.push('1');
                status_queue.push(0x0d);
            } else if (command[6] == 'Q') {
                LOGD << "Goto picture number and continue previous "
                        "play mode (ignored)";
            } else if (command[6] == 'A') {
                LOGD << "Load picture number autostop register (ignored)";
            } else if (command[6] == 'P') {
                LOGD << "Goto picture number then still mode (ignored)";
            } else {
                LOGE << fmt("Error in 'FxxxxxX' command %s", (const char *)command);
            }
        } else if (command_pointer == 13) {
            if (command[12] == 'S') {
                LOGD << "Goto picture number xxxxx and play until "
                        "yyyyy then halt (ignored)";
            } else if (command[12] == 'A') {
                LOGD << "Goto picture number xxxxx and play until "
                        "yyyyy then repeat until cleared (ignored)";
            } else {
                LOGE << fmt("Error in 'FxxxxxNyyyyyX' command %s", (const char *)command);
            }
        } else {
            LOGE << fmt("Error in 'F' command", (const char *)command);
        }
    } break;
    case 'H': {
        if (command[1] == '0') {
            LOGD << "Remote control not routed to computer (ignored)";
        } else if (command[1] == '1') {
            LOGD << "Remote control routed to computer (ignored)";
        } else {
            LOGE << fmt("Error in 'H' command %s", (const char *)command);
        }
    } break;
    case 'I': {
        if (command[1] == '0') {
            LOGD << "Local front-panel buttons disabled (ignored)";
        } else if (command[1] == '1') {
            LOGD << "Local front-panel buttons enabled (ignored)";
        } else {
            LOGE << fmt("Error in 'I' command", (const char *)command);
        }
    } break;
    case 'J': {
        if (command[1] == '0') {
            LOGD << "Remote control disabled for drive control (ignored)";
        } else if (command[1] == '1') {
            LOGD << "Remote control enabled for drive control (ignored)";
        } else {
            LOGE << fmt("Error in 'J' command %s", (const char *)command);
        }
    } break;
    case 'L': {
        LOGD << "Still forward (ignored)";
    } break;
    case 'M': {
        LOGD << "Still reverse (ignored)";
    } break;
    case 'N': {
        restoreAudio();
        g_ldp->pre_play();
    } break;
    case 'O': {
        LOGD << "Normal play reverse (ignored)";
    } break;
    case 'Q': {
        if (command[command_pointer - 1] == 'R') {
            LOGD << "Goto chapter number and halt (ignored)";
        } else if (command[command_pointer - 1] == 'N') {
            LOGD << "Goto chapter number and play (ignored)";
        } else if (command[command_pointer - 1] == 'S') {
            LOGD << "Play chapter/track (sequence) and halt (ignored)";
        } else if (command[command_pointer - 1] == 'N') {
            LOGD << "Goto track number [time code in track] and play "
                    "(ignored)";
        } else if (command[command_pointer - 1] == 'P') {
            LOGD << "Goto track number and pause (ignored)";
        } else if (command[command_pointer - 1] == 'A') {
            LOGD << "Play chapter/track (sequence) then repeat until "
                    "cleared (ignored)";
        } else {
            LOGE << fmt("Error in 'Q' command %s", (const char *)command);
        }
    } break;
    case 'S': {
        if (command[command_pointer - 1] == 'F') {
            LOGD << "Set fast speed value 3, 4, or 8 (ignored)";
        } else if (command[command_pointer - 1] == 'S') {
            LOGD << "Set slow speed value 1/2, 1/4, 1/8, 1/16, 1 "
                    "step/sec, 1 step/3 sec (ignored)";
        } else if (command[command_pointer - 1] == 'A') {
            LOGD << "Sound forced analog (4 channel NTSC disc) (ignored)";
        } else if (command[command_pointer - 1] == 'N') {
            LOGD << "Sound normal (digital sound if available) (ignored)";
        } else {
            LOGE << fmt("Error in 'S' command %s", (const char *)command);
        }
    } break;
    case 'T': {
        LOGE << fmt("Error in 'T' command %s", (const char *)command);
    } break;
    case 'U': {
        // This is supposed to start forward at the speed requested
        // by the 'S' command. Since all I'm aware of is DLEURO using
        // this for playing without sound I'm going to cheat
        g_ldp->disable_audio1();
        g_ldp->disable_audio2();
        g_ldp->pre_play();
        // LOGD << "Slow motion forward (ignored)");
    } break;
    case 'V': {
        LOGD << "Slow motion reverse (ignored)";
    } break;
    case 'W': {
        LOGD << "Fast forward (ignored)";
    } break;
    case 'X': {
        LOGD << "Clear (ignored)";
    } break;
    case 'Z': {
        LOGD << "Fast reverse (ignored)";
    } break;
    default: {
        LOGE << fmt("Error in command %s", (const char *)command);
    } break;
    }
    command_pointer = 0;
}

void restoreAudio()
{
    if (gAudioLeftEn) {
        g_ldp->enable_audio1();
    }
    if (gAudioRightEn) {
        g_ldp->enable_audio2();
    }
}
}
