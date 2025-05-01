/*
 * ____ HYPSEUS COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2025 DirtBagXon
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

 #ifndef DEFAULTCONFIGS_H
 #define DEFAULTCONFIGS_H

 static const char k_defaultJoystick[] =
 {
    "#                Key1            Key2      Button    (Axis)\n"
    "[KEYBOARD]\n"
    "KEY_UP         = SDLK_UP         0         0          -002\n"
    "KEY_DOWN       = SDLK_DOWN       0         0          +002\n"
    "KEY_LEFT       = SDLK_LEFT       0         0          -001\n"
    "KEY_RIGHT      = SDLK_RIGHT      0         0          +001\n"
    "KEY_COIN1      = SDLK_5          0         0\n"
    "KEY_COIN2      = SDLK_6          0         0\n"
    "KEY_START1     = SDLK_1          0         0\n"
    "KEY_START2     = SDLK_2          0         0\n"
    "KEY_BUTTON1    = SDLK_LCTRL      0         0\n"
    "KEY_BUTTON2    = SDLK_LALT       0         0\n"
    "KEY_BUTTON3    = SDLK_SPACE      0         0\n"
    "KEY_SKILL1     = SDLK_LSHIFT     0         0\n"
    "KEY_SKILL2     = SDLK_z          0         0\n"
    "KEY_SKILL3     = SDLK_x          0         0\n"
    "KEY_SERVICE    = SDLK_9          0         0\n"
    "KEY_TEST       = SDLK_F2         SDLK_F4   0\n"
    "KEY_RESET      = SDLK_0          0         0\n"
    "KEY_SCREENSHOT = SDLK_F12        0         0\n"
    "KEY_QUIT       = SDLK_ESCAPE     0         0\n"
    "KEY_PAUSE      = SDLK_p          0         0\n"
    "KEY_CONSOLE    = SDLK_BACKSLASH  0         0\n"
    "KEY_TILT       = SDLK_t          0         0\n"
    "END\n"
};

 static const char k_defaultGamePad[] =
{
    "#                Key1            Key2      Pad0 Button        (Axis Pad0)       Pad1 Button      (Axis Pad1)\n"
    "[KEYBOARD]\n"
    "KEY_UP         = SDLK_UP         0         BUTTON_DPAD_UP     AXIS_LEFT_UP      0                0\n"
    "KEY_DOWN       = SDLK_DOWN       0         BUTTON_DPAD_DOWN   AXIS_LEFT_DOWN    0                0\n"
    "KEY_LEFT       = SDLK_LEFT       0         BUTTON_DPAD_LEFT   AXIS_LEFT_LEFT    0                0\n"
    "KEY_RIGHT      = SDLK_RIGHT      0         BUTTON_DPAD_RIGHT  AXIS_LEFT_RIGHT   0                0\n"
    "KEY_COIN1      = SDLK_5          0         BUTTON_BACK        0                 0\n"
    "KEY_COIN2      = SDLK_6          0         0                  0                 BUTTON_BACK\n"
    "KEY_START1     = SDLK_1          0         BUTTON_START       0                 0\n"
    "KEY_START2     = SDLK_2          0         0                  0                 BUTTON_START\n"
    "KEY_BUTTON1    = SDLK_LCTRL      0         BUTTON_A           0                 0\n"
    "KEY_BUTTON2    = SDLK_LALT       0         BUTTON_B           0                 0\n"
    "KEY_BUTTON3    = SDLK_SPACE      0         AXIS_TRIGGER_RIGHT 0                 0\n"
    "KEY_SKILL1     = SDLK_LSHIFT     0         0                  0                 0\n"
    "KEY_SKILL2     = SDLK_z          0         0                  0                 0\n"
    "KEY_SKILL3     = SDLK_x          0         0                  0                 0\n"
    "KEY_SERVICE    = SDLK_9          0         0                  0                 0\n"
    "KEY_TEST       = SDLK_F2         SDLK_F4   0                  0                 0\n"
    "KEY_RESET      = SDLK_0          0         0                  0                 0\n"
    "KEY_SCREENSHOT = SDLK_F12        0         0                  0                 0\n"
    "KEY_QUIT       = SDLK_ESCAPE     0         0                  0                 0\n"
    "KEY_PAUSE      = SDLK_p          0         0                  0                 0\n"
    "KEY_CONSOLE    = SDLK_BACKSLASH  0         0                  0                 0\n"
    "KEY_TILT       = SDLK_t          0         0                  0                 0\n"
    "END\n"
};

#endif
