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

// LD-V1000.CPP
// part of the DAPHNE emulator
// started by Matt Ownby
// contributions made by Mark Broadhead

// This code emulates the Pioneer LD-V1000 laserdisc player which is used in
// these games:
// Dragon's Lair US
// Space Ace US
// Super Don Quixote
// Thayer's Quest
// Badlands
// Esh's Aurunmilla
// Interstellar
// Astron Belt (LD-V1000 roms)
// Starblazer

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <plog/Log.h>
#include "../hypseus.h"
#include "../game/game.h"
#include "ldv1000.h"
#include "../game/game.h"
#include "../io/conout.h"
#include "../io/numstr.h"
#include "../ldp-out/ldp.h"
#ifdef DEBUG
#include <assert.h>
#endif

#define LDV1000_STACKSIZE 10

namespace ldv1000
{
unsigned char g_output_stack[LDV1000_STACKSIZE];
int g_output_stack_pointer = 0;
Uint16 g_autostop_frame    = 0; // which frame we need to stop on (if any)

bool g_instant_seeking = false; // whether we can make the LD-V1000 complete
                                // searches instantly
// Some games permit instantaneous seeking (Dragon's Lair) and
// therefore should use it to improve performance.
// Other games expect a little search lag (Esh's, Astron Belt)
// and therefore should not use fast seeking.
// Slow seeking is conservative and therefore, the default.

bool audio1 = true; // default audio status is on
bool audio2 = true;

bool audio_temp_mute = false; // flag set on FORWARD 1X, 2X, etc., which don't
                              // play audio unless a PLAY command was given
                              // first

char frame[FRAME_ARRAY_SIZE] = {0}; // holds the digits sent to the
                                            // LD-V1000

unsigned char g_output = 0xFC; // LD-V1000 is PARK'd and READY

bool g_search_pending = false; // whether the LD-V1000 is currently in
                                       // the middle of a search operation or
                                       // not (NOT USED WITH BLOCKING SEEKING)
Uint64 g_search_begin_cycles =
    0; // the current cycle count when LD-V1000 began searching (used to force
       // it to be busy for a certain period of time)
Uint32 g_cycles_per_search = 0;    // how many cycles a read operation
                                           // should last (some games require a
                                           // simulated delay)
double g_seconds_per_search = 0.5; // minimum # of seconds that a search
                                           // operation should last

///////////////////////////////////////////

// retrieves the status from our virtual LD-V1000
unsigned char read()
{
    unsigned char result = 0;

    // if we don't have anything on the stack to return, then return current
    // player status
    if (g_output_stack_pointer < 1) {
        // we are in the middle of a search operation ...
        if (g_search_pending) {
            // using uint32 because this number should never get big enough to
            // hit the uint64 range
            Uint32 elapsed_cycles =
                (Uint32)(cpu::get_total_cycles_executed(0) - g_search_begin_cycles);

            g_output = 0x50; // default to being busy searching

            // if we are using instant seeking OR if the ld-v1000 has been
            // "searching" for long enough
            // then check to see if it's time to change our search from 'busy'
            // to 'finished'
            if ((g_instant_seeking) || (elapsed_cycles >= g_cycles_per_search)) {
                int stat = g_ldp->get_status();

                // if we finished seeking and found success
                if (stat == LDP_PAUSED) {
                    g_output         = 0xD0;
                    g_search_pending = false;
                    LOGD << "search succeeded d0";
                }
                // search failed for whatever reason ...
                else if (stat == LDP_ERROR) {
                    g_output         = 0x90;
                    g_search_pending = false;
                }
            }
        }

        // if autostop is active, we need to check to see if we need to stop
        else if ((g_output & 0x7F) == 0x54) {
            // if we've hit the frame we need to stop on (or gone too far) then
            // stop
            if (g_ldp->get_current_frame() >= g_autostop_frame) {
                g_ldp->pre_pause();
                g_output         = (Uint8)((g_output & 0x80) | 0x65); // preserve ready bit and set status to paused
                g_autostop_frame = 0;
            }
        }

        // else we can just return the previous output

        result = g_output;
    } else {
        //	or pop a value off the stack and return it
        g_output_stack_pointer--;
        result = g_output_stack[g_output_stack_pointer];
    }

    return (result);
}

// pushes a value on the ldv1000 stack, returns 1 if successful or 0 if stack is
// full
int stack_push(unsigned char value)
{
    int result = 0;

    // if we still have room to push
    if (g_output_stack_pointer < LDV1000_STACKSIZE - 1) {
        g_output_stack[g_output_stack_pointer++] = value;
        result                                                   = 1;
    } else {
        LOGW << "ERROR: LD-V1000 stack overflow (increase its size)";
    }

    return (result);
}

// sends a byte to our virtual LD-V1000
void write(unsigned char value)
{

    char s[81] = {0};
    //	char f[81] = { 0 };
    Uint16 curframe = 0; // current frame we're on

    // if high-bit is set, it means we are ready and so we accept input
    if (g_output & 0x80) {
        g_output &= 0x7F; // clear high bit
        // because when we receive a non 0xFF command we are no longer ready

        switch (value) {
        case 0xBF: // clear
            clear();
            g_output &= 0x7F; // Super Don expects highbit to get
                                      // cleared when we send a Clear command
            break;
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
        case 0xA0: // play at 0X (pause)
            g_ldp->pre_pause();
            break;
        case 0xA1: // play at 1/4X
            g_ldp->pre_change_speed(1, 4);
            break;
        case 0xA2: // play at 1/2X
            g_ldp->pre_change_speed(1, 2);
            break;
        case 0xA3: // play at 1X
            // it is necessary to set the playspeed because otherwise the
            // ld-v1000 ignores skip commands
            if (g_ldp->get_status() != LDP_PLAYING) {
                LOGD << "LDV1000: Forward 1X (muted)";

                // if not already playing, FORWARD 1X plays with no audio (until
                // the next PLAY),
                // so we need to set a temporary mute
                audio_temp_mute = true;
                g_ldp->pre_play();
                g_ldp->disable_audio1();
                g_ldp->disable_audio2();
                g_output = 0x2e; // not ready and in FORWARD (variable
                                         // speed) mode
            }
            g_ldp->pre_change_speed(1, 1);
            break;
        case 0xA4: // play at 2X
            g_ldp->pre_change_speed(2, 1);
            break;
        case 0xA5: // play at 3X
            g_ldp->pre_change_speed(3, 1);
            break;
        case 0xA6: // play at 4X
            g_ldp->pre_change_speed(4, 1);
            break;
        case 0xA7: // play at 5X
            g_ldp->pre_change_speed(5, 1);
            break;
        case 0xF9: // Reject - Stop the laserdisc player from playing
            LOGD << "Reject received (ignored)";
            g_output = 0x7c; // LD-V1000 is PARK'd and NOT READY
            // FIXME: laserdisc state should go into parked mode, but most
            // people probably don't want this to happen (I sure don't)
            break;
        case 0xF3: // Auto Stop (used by Esh's)
            // This command accepts a frame # as an argument and also begins
            // playing the disc
            g_autostop_frame = get_buffered_frame();
            clear();
            g_ldp->pre_play();
            g_output = 0x54; // autostop is active
            {
                LOGD << fmt("Auto-Stop requested at frame %u", g_autostop_frame);
            }
            break;
        case 0xFD: // Play
            g_ldp->pre_play();

            // if a FORWARD 1X caused playing with no audio, PLAY will turn
            // audio back on
            if (audio_temp_mute) {
                audio_temp_mute = false;
                if (audio1) // make sure we don't have a normal mute going as
                            // well
                {
                    g_ldp->enable_audio1();
                }
                if (audio2) {
                    g_ldp->enable_audio2();
                }
            }
            g_output = 0x64; // not ready and playing
            break;
        case 0xFE: // step reverse - we probably just want to pause
        {
            g_ldp->pre_pause();
            g_output = 0x65; // 0x65 is stop
            break;
        }
        case 0xF7: // Search

            // if we are to use the more accurate non-blocking seeking method
            // ...
            if (g_ldp->get_use_nonblocking_searching()) {
                // if search command was not accepted
                if (g_ldp->pre_search(frame, false) == false) {
                    LOGW << "Search command was not accepted!";
                    g_output = 0x90; // search failed code
                }

                // if search command was accepted
                else {
                    g_search_pending      = true;
                    g_output              = 0x50;
                    g_search_begin_cycles = cpu::get_total_cycles_executed(0);
                }
            }

            // else if we are to use the more stable blocking seeking method ...
            else {
                // if we need to 'look busy' for the benefit of some games
                // (Esh's, Astron Belt) then do so
                if (!g_instant_seeking) {
                    stack_push(0x50); // 0x50 means we are busy
                                              // searching
                    stack_push(0x50);
                    stack_push(0x50);
                    stack_push(0x50);
                    LOGD << "using slower seeking";
                }

                // if seek failed ...
                if (g_ldp->pre_search(frame, true) == false) {
                    g_output = 0x90;
                }

                // if seek succeeded
                else {
                    g_output = 0xD0;
                }
            }

            clear();
            break;
        case 0xC2: // get current frame
            // curframe = g_ldp->get_current_frame();
            curframe = g_ldp->get_adjusted_current_frame(); // in an effort to
                                                            // fix SDQ death
                                                            // scene overrun

            g_ldp->framenum_to_frame(curframe, s);
            stack_push(s[4]);
            stack_push(s[3]);
            stack_push(s[2]);
            stack_push(s[1]);
            stack_push(s[0]); // high digit goes last

            break;
        case 0xB1: // Skip Forward 10
        case 0xB2: // Skip Forward 20
        case 0xB3: // Skip Forward 30
        case 0xB4: // Skip Forward 40
        case 0xB5: // Skip Forward 50
        case 0xB6: // Skip Forward 60
        case 0xB7: // Skip Forward 70
        case 0xB8: // Skip Forward 80
        case 0xB9: // Skip Forward 90
        case 0xBa: // Skip Forward 100
            // FIXME: ignore skip command if forward command has not been issued
            {
                Uint16 frames_to_skip = (Uint16)((10 * (value & 0x0f)) + 1); // LD-V1000 does add 1 when skipping
                g_ldp->pre_skip_forward(frames_to_skip);
            }
            break;
        case 0xCD: // Display Disable
            pre_display_disable();
            break;
        case 0xCE: // Display Enable
            pre_display_enable();
            break;
        case 0xFB: // Stop - this actually just goes into still-frame mode, so
                   // we pause
            g_ldp->pre_pause();
            g_output = 0x65; // stopped and not ready
            break;
        case 0x20: // Badlands custom command (see below)
            LOGD << "Got a 0x20";
                   // UPDATE : This doesn't work so I am disabling it for now :)
                   //			g_ldp->pre_skip_backward(16);

            // WO: This should be a relative skip as Matt used, but since that
            // doesn't work in pause mode,
            // I'm using an absolute search instead.  :)
            if (g_game->get_game_type() == GAME_BADLANDS) {
                g_ldp->framenum_to_frame((g_ldp->get_current_frame() - 16), s);
                if (g_ldp->pre_search(s, true) == false) {
                    LOGW << "on Badlands custom skip!";
                    // push search failure code for the LD-V1000 onto stack
                    g_output = 0x90; // does this command really expect
                                             // a response?
                }
                break; // unsupported for all other games, so keep the break in
                       // here to get the default case
            }
            break;
        case 0x31: // Badlands custom command
            // The Badlands programmers modified the LD-V1000 ROM (!!) and added
            // a new
            // command which is 0x20 0x31.  This command skips backwards 16
            // frames.  It is
            // used on scenes such as the twirling axe and the red eyes (bats).
            LOGD << "Got a 0x31";
            break;
        case 0xFF: // NO ENTRY
            // it's legal to send the LD-V1000 as many of these as you want, we
            // just ignore 'em
            g_output |= 0x80; // set highbit just in case
            break;
        default: // Unsupported Command
            LOGW << fmt("Unsupported LD-V1000 Command Received: %x", value);
            break;
        }
    }

    // if we are not ready, we can become ready by receiving a 0xFF command
    else {
        // if we got 0xFF (NO ENTRY) as expected
        if (value == 0xFF) {
            g_output |= 0x80; // set high bit, we are now ready
        }

        // if we got a non NO ENTRY, we just ignore it, only the first non-NO
        // ENTRY matters
        else {
            g_output &= 0x7F; // clear high bit, we are no longer ready
        }
    }
}

// As you can see we are currently not supporting display disable
void pre_display_disable() { LOGW << "Display disable received (unsupported)"; }

// We are not currently supporting display enable
void pre_display_enable() { LOGW << "Display enable received (unsupported)"; }

// Make the LD-V1000 appear to perform instantaneous searches
// NOTE : Doesn't work with all games
// Test this with each game and if it doesn't cause any harm, use it.
// If it breaks the game, don't use it.
void enable_instant_seeking()
{
    // WDO 1/2005 - disabled this message because
    // it occurs before the -nolog option takes effect
    LOGD << "Instantaneous seeking enabled!";
    g_instant_seeking = true;
}

// adds a digit to  the frame array that we will be seeking to
// digit should be in ASCII format
void add_digit(char digit)
{
    // we need to set the high bit for Badlands - it might be caused by some
    // emulation problem
    if (g_game->get_game_type() == GAME_BADLANDS) {
        g_output |= 0x80;
    }

    for (int count = 0; count < FRAME_SIZE - 1; count++) {
        if (!frame[count + 1]) {
            frame[count + 1] = '0';
        }
        frame[count] = frame[count + 1];
    }
    frame[FRAME_SIZE - 1] = digit;
    /*
    char s[81] = { 0 };

    sprintf(s, "Too many digits received for frame! (over %d)", FRAME_SIZE);
    printline(s);
    */
}

// Audio channel 1 on or off
void pre_audio1()
{
    // Check if we should just toggle
    if (!frame[FRAME_SIZE - 1]) {
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
        switch (frame[FRAME_SIZE - 1] % 2) {
        case 0:
            audio1 = false;
            g_ldp->disable_audio1();
            break;
        case 1:
            audio1 = true;
            g_ldp->enable_audio1();
            break;
        default:
            LOGW << "Ummm... you shouldn't get this";
        }
        clear();
    }
}

// Audio channel 2 on or off
void pre_audio2()
{
    // Check if we should just toggle
    if (!frame[FRAME_SIZE - 1]) {
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
        switch (frame[FRAME_SIZE - 1] % 2) {
        case 0:
            audio2 = false;
            g_ldp->disable_audio2();
            break;
        case 1:
            audio2 = true;
            g_ldp->enable_audio2();
            break;
        default:
            LOGW << "Ummm... you shouldn't get this";
        }
        clear();
    }
}

// returns the frame that has been entered in by add_digit thus far
Uint16 get_buffered_frame()
{
    frame[FRAME_SIZE] = 0; // terminate string
    return ((Uint16)atoi(frame));
}

// clears any received digits from the frame array
void clear(void) { memset(frame, 0, FRAME_SIZE); }

//////////////////////////////////
// BEGIN STROBE TIMING SECTION
// This section is for those game drivers that poll for the status of the status
// and command strobes of the ld-v1000
// There are a few of them (lair, ace, tq, cobraconv)
// To get accurate strobe values, simply call report_vsync on every
// vsync pulse
// Then you can query the other two strobe functions and always get the status
// of the strobe

// Uint64 g_vsync_timer = 0;
Uint32 g_until_vsync_end    = 0; // # of cpu cycles until vsync ends
Uint32 g_until_status_start = 0; // # of cpu cycles until status strobe
                                         // begins
Uint32 g_until_status_end = 0;   // # of cpu cycles u ntil status strobe
                                         // ends
Uint32 g_until_command_start = 0; // # of cpu cycles until command
                                          // strobe begins
Uint32 g_until_command_end = 0; // # of cpu cycles until command strobe
                                        // ends

const unsigned int LDV1000_EVENT_VSYNC_START  = 0; // when vsync starts
const unsigned int LDV1000_EVENT_VSYNC_END    = 1;
const unsigned int LDV1000_EVENT_STATUS_START = (1 << 1); // when status strobe
                                                          // starts
const unsigned int LDV1000_EVENT_STATUS_END    = (1 << 2);
const unsigned int LDV1000_EVENT_COMMAND_START = (1 << 3); // when command
                                                           // strobe starts
const unsigned int LDV1000_EVENT_COMMAND_END = (1 << 4);

// so we know what state we're in
unsigned int g_last_event = 0;

void event_callback(void *eventType)
{
    g_last_event = (uint64_t)eventType; // changed to long for x64
                                                // support (thanks Joker)

    switch ((uint64_t)eventType) {
    case LDV1000_EVENT_VSYNC_END:
#ifdef DEBUG
        assert(is_vsync_active() == false);
#endif
        cpu::set_event(0, g_until_status_start, event_callback,
                      (void *)LDV1000_EVENT_STATUS_START);
        break;
    case LDV1000_EVENT_STATUS_START:
        g_game->OnLDV1000LineChange(true, true);
#ifdef DEBUG
        assert(is_status_strobe_active() == true);
#endif
        cpu::set_event(0, g_until_status_end, event_callback,
                      (void *)LDV1000_EVENT_STATUS_END);
        break;
    case LDV1000_EVENT_STATUS_END:
        g_game->OnLDV1000LineChange(true, false);
#ifdef DEBUG
        assert(is_status_strobe_active() == false);
#endif
        cpu::set_event(0, g_until_command_start, event_callback,
                      (void *)LDV1000_EVENT_COMMAND_START);
        break;
    case LDV1000_EVENT_COMMAND_START:
        g_game->OnLDV1000LineChange(false, true);
#ifdef DEBUG
        assert(is_command_strobe_active() == true);
#endif
        cpu::set_event(0, g_until_command_end, event_callback,
                      (void *)LDV1000_EVENT_COMMAND_END);
        break;
    case LDV1000_EVENT_COMMAND_END:
        g_game->OnLDV1000LineChange(false, false);
#ifdef DEBUG
        assert(is_command_strobe_active() == false);
#endif
        // no more events until next vsync
        break;
    default:
        LOGW << "unhandled event, fix this!";
        set_quitflag();
        break;
    }
}

// call this when vsync begins, your game must have a CPU obviously
// this must be called on EVERY vsync
void report_vsync()
{
    g_last_event = LDV1000_EVENT_VSYNC_START;
#ifdef DEBUG
    assert(is_vsync_active() == true);
#endif
    cpu::set_event(0, g_until_vsync_end, event_callback,
                  (void *)LDV1000_EVENT_VSYNC_END);
    g_last_event = 0;
}

// returns true if vsync is active, false if it isn't
bool is_vsync_active()
{
    return (g_last_event == LDV1000_EVENT_VSYNC_START);
}

// returns true if status strobe is active, false if it isn't
bool is_status_strobe_active()
{
    return (g_last_event == LDV1000_EVENT_STATUS_START);
}

// returns true if command strobe is active, false if it isn't
bool is_command_strobe_active()
{
    return (g_last_event == LDV1000_EVENT_COMMAND_START);
}

// END STROBE TIMING SECTION
/////////////////////////////////////////

void reset()
{
    clear();
    Uint32 cpu_hz       = cpu::get_hz(0);
    double dCyclesPerUs = cpu_hz / 1000000.0; // cycles per microsecond

    // anything else?
    g_output_stack_pointer = 0;
    g_autostop_frame       = 0; // which frame we need to stop on (if any)

    g_output = 0xFC; // LD-V1000 is PARK'd and READY

    // do these expensive calculations once
    g_until_vsync_end =
        (Uint32)((dCyclesPerUs * 27.1) + 0.5); // 27.1 uS, length of vsync
                                               // (according to warren)
    g_until_status_start =
        (Uint32)((dCyclesPerUs * 500.0) + 0.5); // 500-650uS from the time vsync
                                                // starts until the time status
                                                // strobe starts
    g_until_status_end =
        (Uint32)((dCyclesPerUs * 26.0) + 0.5); // status strobe lasts 26 uS
    g_until_command_start =
        (Uint32)((dCyclesPerUs * 54.0) + 0.5); // 54 uS between status end and
                                               // command begin
    g_until_command_end =
        (Uint32)((dCyclesPerUs * 25.0) + 0.5); // cmd strobe lasts 25 uS

    g_cycles_per_search = (Uint32)(cpu::get_hz(0) * g_seconds_per_search);
}

void set_seconds_per_search(double d)
{
    g_seconds_per_search = d;
}

void print_info()
{
    LOGD << fmt("The last LD-V1000 event was %d", g_last_event);
}
}
