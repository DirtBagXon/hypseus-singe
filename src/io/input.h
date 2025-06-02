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

#include "limits.h"

#ifndef INPUT_H
#define INPUT_H

enum {
    SWITCH_UP,
    SWITCH_LEFT,
    SWITCH_DOWN,
    SWITCH_RIGHT,
    SWITCH_START1,
    SWITCH_START2,
    SWITCH_BUTTON1,
    SWITCH_BUTTON2,
    SWITCH_BUTTON3,
    SWITCH_COIN1,
    SWITCH_COIN2,
    SWITCH_SKILL1,
    SWITCH_SKILL2,
    SWITCH_SKILL3,
    SWITCH_SERVICE,
    SWITCH_TEST,
    SWITCH_RESET,
    SWITCH_SCREENSHOT,
    SWITCH_QUIT,
    SWITCH_PAUSE,
    SWITCH_CONSOLE,
    SWITCH_TILT,
    SWITCH_COUNT,
    SWITCH_MOUSE_SCROLL_UP,
    SWITCH_MOUSE_SCROLL_DOWN,
    SWITCH_MOUSE_DISCONNECT
}; // hypseus inputs for arcade and additional controls, leave SWITCH_COUNT at
   // the end of key_defs

///////////////////////

#include <SDL.h>

// to be passed into the coin queue
struct coin_input {
    bool coin_enabled;            //	whether the coin was enabled or disabled
    Uint8 coin_val;               // either SWITCH_COIN1 or SWITCH_COIN2
    Uint64 cycles_when_to_enable; // the cycle count that we must have surpassed
                                  // in order to be able to enable the coin
};

////////////////////////

typedef struct
{
    int connected;
    int x;
    int y;
    int relx;
    int rely;
    SDL_Color color;
    char name[64];
    Uint32 buttons;
    Uint32 scrolluptick;
    Uint32 scrolldowntick;
    Uint32 scrolllefttick;
    Uint32 scrollrighttick;
} Mouse;


int SDL_input_init();
void SDL_input_shutdown();

// Filters out mouse events if 'bFilteredOut' is true.
// The purpose is so that games that don't use the mouse don't get a bunch of
// extra mouse
//  events which can hurt performance.
void FilterMouseEvents(bool bFilteredOut);

void SDL_check_input();

void SDL_gamepad_init();

void process_event(SDL_Event *event);
void process_keydown(SDL_Keycode key);
void process_keyup(SDL_Keycode key);
void process_joystick_motion(SDL_Event *event);
void process_controller_motion(SDL_Event *event);
void process_joystick_hat_motion(SDL_Event *event);
void input_enable(Uint8, Sint8);
void input_disable(Uint8, Sint8);
inline void add_coin_to_queue(bool enabled, Uint8 val);
void reset_idle(void); // added by JFA
void set_use_joystick(bool val);
void set_invert_hat(bool val);
void set_open_hat(bool val);
void set_inputini_file(const char *inputFile);
bool set_mouse_mode(int);
void set_use_gamepad(bool value);
void set_gamepad_order(int *c, int);
void do_gamepad_rumble(Uint8, Uint8, Uint8);
void disable_haptics();
void set_haptic(Uint8);
void absolute_only();

void input_toolbox(SDL_Keycode key, SDL_Keycode recent, bool thayers);

#endif // INPUT_H
