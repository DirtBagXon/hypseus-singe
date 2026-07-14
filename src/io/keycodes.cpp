/*
 * ____ HYPSEUS COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2021 DirtBagXon
 *
 * This file is part of HYPSEUS SINGE, a laserdisc arcade game emulator
 *
 * HYPSEUS SINGE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HYPSEUS SINGE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "input.h"
#include "conout.h"
#include "keycodes.h"
#include <plog/Log.h>

// Win32 doesn't use strcmp
#ifdef WIN32
#define strcmp stricmp
#define strcasecmp stricmp
#endif

#define INC(N) (N + 1)
#define TRIGGER(N) (N + AXIS_TRIGGER)

// sAn3 subset - this can be extended with decimal keycodes
int sdl3_keycode(const char *str)
{
	if (strcmp(str, "SDLK_BACKSPACE") == 0) return SDLK_BACKSPACE;
	else if (strcmp(str, "SDLK_TAB") == 0) return SDLK_TAB;
	else if (strcmp(str, "SDLK_RETURN") == 0) return SDLK_RETURN;
	else if (strcmp(str, "SDLK_ESCAPE") == 0) return SDLK_ESCAPE;
	else if (strcmp(str, "SDLK_SPACE") == 0) return SDLK_SPACE;
	else if (strcmp(str, "SDLK_HASH") == 0) return SDLK_HASH;
	else if (strcmp(str, "SDLK_QUOTE") == 0) return SDLK_APOSTROPHE;
	else if (strcmp(str, "SDLK_APOSTROPHE") == 0) return SDLK_APOSTROPHE;
	else if (strcmp(str, "SDLK_COMMA") == 0) return SDLK_COMMA;
	else if (strcmp(str, "SDLK_MINUS") == 0) return SDLK_MINUS;
	else if (strcmp(str, "SDLK_PERIOD") == 0) return SDLK_PERIOD;
	else if (strcmp(str, "SDLK_APPLICATION") == 0) return SDLK_APPLICATION;
	else if (strcmp(str, "SDLK_SLASH") == 0) return SDLK_SLASH;
	else if (strcmp(str, "SDLK_0") == 0) return SDLK_0;
	else if (strcmp(str, "SDLK_1") == 0) return SDLK_1;
	else if (strcmp(str, "SDLK_2") == 0) return SDLK_2;
	else if (strcmp(str, "SDLK_3") == 0) return SDLK_3;
	else if (strcmp(str, "SDLK_4") == 0) return SDLK_4;
	else if (strcmp(str, "SDLK_5") == 0) return SDLK_5;
	else if (strcmp(str, "SDLK_6") == 0) return SDLK_6;
	else if (strcmp(str, "SDLK_7") == 0) return SDLK_7;
	else if (strcmp(str, "SDLK_8") == 0) return SDLK_8;
	else if (strcmp(str, "SDLK_9") == 0) return SDLK_9;
	else if (strcmp(str, "SDLK_SEMICOLON") == 0) return SDLK_SEMICOLON;
	else if (strcmp(str, "SDLK_EQUALS") == 0) return SDLK_EQUALS;
	else if (strcmp(str, "SDLK_LEFTBRACKET") == 0) return SDLK_LEFTBRACKET;
	else if (strcmp(str, "SDLK_RIGHTBRACKET") == 0) return SDLK_RIGHTBRACKET;
	else if (strcmp(str, "SDLK_BACKSLASH") == 0) return SDLK_BACKSLASH;
	else if (strcmp(str, "SDLK_BACKQUOTE") == 0) return SDLK_GRAVE;
	else if (strcmp(str, "SDLK_GRAVE") == 0) return SDLK_GRAVE;
	else if (strcasecmp(str, "SDLK_a") == 0) return SDLK_A;
	else if (strcasecmp(str, "SDLK_b") == 0) return SDLK_B;
	else if (strcasecmp(str, "SDLK_c") == 0) return SDLK_C;
	else if (strcasecmp(str, "SDLK_d") == 0) return SDLK_D;
	else if (strcasecmp(str, "SDLK_e") == 0) return SDLK_E;
	else if (strcasecmp(str, "SDLK_f") == 0) return SDLK_F;
	else if (strcasecmp(str, "SDLK_g") == 0) return SDLK_G;
	else if (strcasecmp(str, "SDLK_h") == 0) return SDLK_H;
	else if (strcasecmp(str, "SDLK_i") == 0) return SDLK_I;
	else if (strcasecmp(str, "SDLK_j") == 0) return SDLK_J;
	else if (strcasecmp(str, "SDLK_k") == 0) return SDLK_K;
	else if (strcasecmp(str, "SDLK_l") == 0) return SDLK_L;
	else if (strcasecmp(str, "SDLK_m") == 0) return SDLK_M;
	else if (strcasecmp(str, "SDLK_n") == 0) return SDLK_N;
	else if (strcasecmp(str, "SDLK_o") == 0) return SDLK_O;
	else if (strcasecmp(str, "SDLK_p") == 0) return SDLK_P;
	else if (strcasecmp(str, "SDLK_q") == 0) return SDLK_Q;
	else if (strcasecmp(str, "SDLK_r") == 0) return SDLK_R;
	else if (strcasecmp(str, "SDLK_s") == 0) return SDLK_S;
	else if (strcasecmp(str, "SDLK_t") == 0) return SDLK_T;
	else if (strcasecmp(str, "SDLK_u") == 0) return SDLK_U;
	else if (strcasecmp(str, "SDLK_v") == 0) return SDLK_V;
	else if (strcasecmp(str, "SDLK_w") == 0) return SDLK_W;
	else if (strcasecmp(str, "SDLK_x") == 0) return SDLK_X;
	else if (strcasecmp(str, "SDLK_y") == 0) return SDLK_Y;
	else if (strcasecmp(str, "SDLK_z") == 0) return SDLK_Z;
	else if (strcmp(str, "SDLK_CAPSLOCK") == 0) return SDLK_CAPSLOCK;
	else if (strcmp(str, "SDLK_DELETE") == 0) return SDLK_DELETE;
	else if (strcmp(str, "SDLK_F1") == 0) return SDLK_F1;
	else if (strcmp(str, "SDLK_F2") == 0) return SDLK_F2;
	else if (strcmp(str, "SDLK_F3") == 0) return SDLK_F3;
	else if (strcmp(str, "SDLK_F4") == 0) return SDLK_F4;
	else if (strcmp(str, "SDLK_F5") == 0) return SDLK_F5;
	else if (strcmp(str, "SDLK_F6") == 0) return SDLK_F6;
	else if (strcmp(str, "SDLK_F7") == 0) return SDLK_F7;
	else if (strcmp(str, "SDLK_F8") == 0) return SDLK_F8;
	else if (strcmp(str, "SDLK_F9") == 0) return SDLK_F9;
	else if (strcmp(str, "SDLK_F10") == 0) return SDLK_F10;
	else if (strcmp(str, "SDLK_F11") == 0) return SDLK_F11;
	else if (strcmp(str, "SDLK_F12") == 0) return SDLK_F12;
	else if (strcmp(str, "SDLK_SCROLLLOCK") == 0) return SDLK_SCROLLLOCK;
	else if (strcmp(str, "SDLK_PAUSE") == 0) return SDLK_PAUSE;
	else if (strcmp(str, "SDLK_INSERT") == 0) return SDLK_INSERT;
	else if (strcmp(str, "SDLK_HOME") == 0) return SDLK_HOME;
	else if (strcmp(str, "SDLK_PAGEUP") == 0) return SDLK_PAGEUP;
	else if (strcmp(str, "SDLK_END") == 0) return SDLK_END;
	else if (strcmp(str, "SDLK_PAGEDOWN") == 0) return SDLK_PAGEDOWN;
	else if (strcmp(str, "SDLK_RIGHT") == 0) return SDLK_RIGHT;
	else if (strcmp(str, "SDLK_LEFT") == 0) return SDLK_LEFT;
	else if (strcmp(str, "SDLK_DOWN") == 0) return SDLK_DOWN;
	else if (strcmp(str, "SDLK_UP") == 0) return SDLK_UP;
	else if (strcmp(str, "SDLK_NUMLOCKCLEAR") == 0) return SDLK_NUMLOCKCLEAR;
	else if (strcmp(str, "SDLK_KP_DIVIDE") == 0) return SDLK_KP_DIVIDE;
	else if (strcmp(str, "SDLK_KP_MULTIPLY") == 0) return SDLK_KP_MULTIPLY;
	else if (strcmp(str, "SDLK_KP_MINUS") == 0) return SDLK_KP_MINUS;
	else if (strcmp(str, "SDLK_KP_PLUS") == 0) return SDLK_KP_PLUS;
	else if (strcmp(str, "SDLK_KP_ENTER") == 0) return SDLK_KP_ENTER;
	else if (strcmp(str, "SDLK_KP_1") == 0) return SDLK_KP_1;
	else if (strcmp(str, "SDLK_KP_2") == 0) return SDLK_KP_2;
	else if (strcmp(str, "SDLK_KP_3") == 0) return SDLK_KP_3;
	else if (strcmp(str, "SDLK_KP_4") == 0) return SDLK_KP_4;
	else if (strcmp(str, "SDLK_KP_5") == 0) return SDLK_KP_5;
	else if (strcmp(str, "SDLK_KP_6") == 0) return SDLK_KP_6;
	else if (strcmp(str, "SDLK_KP_7") == 0) return SDLK_KP_7;
	else if (strcmp(str, "SDLK_KP_8") == 0) return SDLK_KP_8;
	else if (strcmp(str, "SDLK_KP_9") == 0) return SDLK_KP_9;
	else if (strcmp(str, "SDLK_KP_0") == 0) return SDLK_KP_0;
	else if (strcmp(str, "SDLK_KP_PERIOD") == 0) return SDLK_KP_PERIOD;
	else if (strcmp(str, "SDLK_VOLUMEUP") == 0) return SDLK_VOLUMEUP;
	else if (strcmp(str, "SDLK_VOLUMEDOWN") == 0) return SDLK_VOLUMEDOWN;
	else if (strcmp(str, "SDLK_LCTRL") == 0) return SDLK_LCTRL;
	else if (strcmp(str, "SDLK_LSHIFT") == 0) return SDLK_LSHIFT;
	else if (strcmp(str, "SDLK_LALT") == 0) return SDLK_LALT;
	else if (strcmp(str, "SDLK_LGUI") == 0) return SDLK_LGUI;
	else if (strcmp(str, "SDLK_RCTRL") == 0) return SDLK_RCTRL;
	else if (strcmp(str, "SDLK_RSHIFT") == 0) return SDLK_RSHIFT;
	else if (strcmp(str, "SDLK_RALT") == 0) return SDLK_RALT;
	else if (strcmp(str, "SDLK_RGUI") == 0) return SDLK_RGUI;
	else if (strcmp(str, "SDLK_MODE") == 0) return SDLK_MODE;
	else {
		LOGW << fmt("Unrecognized key macro in config: %s", str);
		LOGW << "Use decimal values for extended keycodes.";
		return SDLK_UNKNOWN;
	}
}

int sdl3_controller_button(const char *str)
{
	if (strcmp(str, "BUTTON_A") == 0) return INC(SDL_GAMEPAD_BUTTON_SOUTH);
	else if (strcmp(str, "BUTTON_B") == 0) return INC(SDL_GAMEPAD_BUTTON_EAST);
	else if (strcmp(str, "BUTTON_X") == 0) return INC(SDL_GAMEPAD_BUTTON_WEST);
	else if (strcmp(str, "BUTTON_Y") == 0) return INC(SDL_GAMEPAD_BUTTON_NORTH);
	else if (strcmp(str, "BUTTON_BACK") == 0) return INC(SDL_GAMEPAD_BUTTON_BACK);
	else if (strcmp(str, "BUTTON_GUIDE") == 0) return INC(SDL_GAMEPAD_BUTTON_GUIDE);
	else if (strcmp(str, "BUTTON_START") == 0) return INC(SDL_GAMEPAD_BUTTON_START);
	else if (strcmp(str, "BUTTON_LEFTSTICK") == 0) return INC(SDL_GAMEPAD_BUTTON_LEFT_STICK);
	else if (strcmp(str, "BUTTON_RIGHTSTICK") == 0) return INC(SDL_GAMEPAD_BUTTON_RIGHT_STICK);
	else if (strcmp(str, "BUTTON_LEFTSHOULDER") == 0) return INC(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
	else if (strcmp(str, "BUTTON_RIGHTSHOULDER") == 0) return INC(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
	else if (strcmp(str, "BUTTON_DPAD_UP") == 0) return INC(SDL_GAMEPAD_BUTTON_DPAD_UP);
	else if (strcmp(str, "BUTTON_DPAD_DOWN") == 0) return INC(SDL_GAMEPAD_BUTTON_DPAD_DOWN);
	else if (strcmp(str, "BUTTON_DPAD_LEFT") == 0) return INC(SDL_GAMEPAD_BUTTON_DPAD_LEFT);
	else if (strcmp(str, "BUTTON_DPAD_RIGHT") == 0) return INC(SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
	else if (strcmp(str, "AXIS_TRIGGER_LEFT") == 0) return TRIGGER(SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
	else if (strcmp(str, "AXIS_TRIGGER_RIGHT") == 0) return TRIGGER(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
	else {
		LOGW << fmt("Unrecognized button macro in config: %s", str);
		return SDLK_UNKNOWN;
	}
}

int sdl3_controller_axis(const char *str)
{
	if (strcmp(str, "AXIS_LEFT_UP") == 0) return -2;
	else if (strcmp(str, "AXIS_LEFT_DOWN") == 0) return 2;
	else if (strcmp(str, "AXIS_LEFT_LEFT") == 0) return -1;
	else if (strcmp(str, "AXIS_LEFT_RIGHT") == 0) return 1;
	else if (strcmp(str, "AXIS_RIGHT_UP") == 0) return -4;
	else if (strcmp(str, "AXIS_RIGHT_DOWN") == 0) return 4;
	else if (strcmp(str, "AXIS_RIGHT_LEFT") == 0) return -3;
	else if (strcmp(str, "AXIS_RIGHT_RIGHT") == 0) return 3;
	else {
		if (strncmp(str, "0", 1) != 0) {
		    LOGW << fmt("Unrecognized axis macro in config: %s", str);
		}
		return SDLK_UNKNOWN;
	}
}
