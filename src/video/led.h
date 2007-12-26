/*
 * led.h
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

/*LED.CPP - Added by Brad Oldham for Space Ace skill LED emulation
**
**  Portions of this code Copyright 1999 Mark J. McGinty, 
**  All Rights Reserved
**	Free Usage granted to the public domain.
**
*/

#ifdef WIN32

#include <windows.h>

// (some definitions borrowed from ntkbdio.h)

//
// Define the keyboard indicators.
//

#define IOCTL_KEYBOARD_SET_INDICATORS        CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0002, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_QUERY_TYPEMATIC       CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0008, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_QUERY_INDICATORS      CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0010, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _KEYBOARD_INDICATOR_PARAMETERS {
    USHORT UnitId;		// Unit identifier.
    USHORT LedFlags;		// LED indicator state.

} KEYBOARD_INDICATOR_PARAMETERS, *PKEYBOARD_INDICATOR_PARAMETERS;

#define KEYBOARD_CAPS_LOCK_ON     4
#define KEYBOARD_NUM_LOCK_ON      2
#define KEYBOARD_SCROLL_LOCK_ON   1

#endif

void change_led(bool, bool, bool);
void remember_leds();
void restore_leds();
void enable_leds(bool value);

#ifdef WIN32

int FlashKeyboardLight(HANDLE hKbdDev, UINT LightFlag, BOOL);
HANDLE OpenKeyboardDevice(int *ErrorNumber);
int CloseKeyboardDevice(HANDLE hndKbdDev);

#endif
