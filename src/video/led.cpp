/*
 * led.cpp
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

#include "led.h"

bool g_save_numlock = false, g_save_capital = false, g_save_scroll = false;
bool g_leds_enabled = false;	// LEDs are disabled by default since they don't work on all platforms

#ifdef LINUX
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <linux/kd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#endif

#ifdef WIN32
#include <windows.h>
#include <winioctl.h>

int FlashKeyboardLight(HANDLE hKbdDev, UINT LedFlag, BOOL Toggle)
{

	KEYBOARD_INDICATOR_PARAMETERS InputBuffer;	  // Input buffer for DeviceIoControl
	KEYBOARD_INDICATOR_PARAMETERS OutputBuffer;	  // Output buffer for DeviceIoControl
	UINT				LedFlagsMask;
	ULONG				DataLength = sizeof(KEYBOARD_INDICATOR_PARAMETERS);
	ULONG				ReturnedLength; // Number of bytes returned in output buffer

	InputBuffer.UnitId = 0;
	OutputBuffer.UnitId = 0;

	// Preserve current indicators' state
	//
	if (!DeviceIoControl(hKbdDev, IOCTL_KEYBOARD_QUERY_INDICATORS,
				&InputBuffer, DataLength,
				&OutputBuffer, DataLength,
				&ReturnedLength, NULL))
		return GetLastError();

	// Mask bit for light to be manipulated
	//
	LedFlagsMask = (OutputBuffer.LedFlags & (~LedFlag));

	// Set toggle variable to reflect current state.
	//
	
	InputBuffer.LedFlags = (unsigned short) (LedFlagsMask | (LedFlag * Toggle));
	
	if (!DeviceIoControl(hKbdDev, IOCTL_KEYBOARD_SET_INDICATORS,
		&InputBuffer, DataLength,
		NULL,	0,	&ReturnedLength, NULL))
		return GetLastError();

	return 0;

}


HANDLE OpenKeyboardDevice(int *ErrorNumber)
{

	HANDLE	hndKbdDev;
	int		*LocalErrorNumber;
	int		Dummy;

	if (ErrorNumber == NULL)
		LocalErrorNumber = &Dummy;
	else
		LocalErrorNumber = ErrorNumber;

	*LocalErrorNumber = 0;
	
	if (!DefineDosDevice (DDD_RAW_TARGET_PATH, "Kbd",
				"\\Device\\KeyboardClass0"))
	{
		*LocalErrorNumber = GetLastError();
		return INVALID_HANDLE_VALUE;
	}

	hndKbdDev = CreateFile("\\\\.\\Kbd", GENERIC_WRITE, 0,
				NULL,	OPEN_EXISTING,	0,	NULL);
	
	if (hndKbdDev == INVALID_HANDLE_VALUE)
		*LocalErrorNumber = GetLastError();

	return hndKbdDev;

}

int CloseKeyboardDevice(HANDLE hndKbdDev)
{

	int e = 0;

	if (!DefineDosDevice (DDD_REMOVE_DEFINITION, "Kbd", NULL))
		e = GetLastError();

	if (!CloseHandle(hndKbdDev))					
		e = GetLastError();

	return e;
}

#endif

// end Win32 specific code

// changes keyboard LED lights to simulate the skill LEDs in Space Ace
// changed_led (false, false, false) set all 3 lights to off
// changed_led (true, true, true) sets all 3 lights to on.

void change_led(bool num_lock, bool caps_lock, bool scroll_lock)
{

	// are the LED's enabled
	if (g_leds_enabled)
	{
#ifdef WIN32

		HANDLE			hndKbdDev;

		hndKbdDev = OpenKeyboardDevice(NULL);

		FlashKeyboardLight(hndKbdDev, KEYBOARD_SCROLL_LOCK_ON, scroll_lock);
		FlashKeyboardLight(hndKbdDev, KEYBOARD_NUM_LOCK_ON, num_lock);
		FlashKeyboardLight(hndKbdDev, KEYBOARD_CAPS_LOCK_ON, caps_lock);

		CloseKeyboardDevice(hndKbdDev);				

#endif

#ifdef LINUX
		int keyval = 0;	// holds LED bit values
		int fd = open("/dev/console", O_NOCTTY);
	
		// if we were able to open console ok (must be root)
		if (fd != -1)
		{
			if (num_lock) keyval |= LED_NUM;
			if (scroll_lock) keyval |= LED_SCR;
			if (caps_lock) keyval |= LED_CAP;

			// set the LED's here and check for error
			if (ioctl(fd, KDSETLED, keyval) == -1)
			{
				perror("Could not set keyboard leds because");
			}
			close(fd);
		}
		else
		{
			perror("Could not open /dev/console, are you root? Error is ");
		}
#endif
	} // end whether LED's are enabled
}

void remember_leds()
{

	if (g_leds_enabled)
	{
#ifdef WIN32
		if (GetKeyState(VK_NUMLOCK) != 0x00) g_save_numlock = true;
		if (GetKeyState(VK_CAPITAL) != 0x00) g_save_capital = true;
		if (GetKeyState(VK_SCROLL) != 0x00) g_save_scroll = true;			
#endif
#ifdef LINUX
		long int keyval = 0;	// holds LED bit values
		int fd = open("/dev/console", O_NOCTTY);
	
		// if we were able to open console ok (must be root)
		if (fd != -1)
		{
			// get the LED's here
			if (ioctl(fd, KDGETLED, &keyval) != -1)
			{
				if (keyval & LED_SCR) g_save_scroll = true;
				if (keyval & LED_CAP) g_save_capital = true;
				if (keyval & LED_NUM) g_save_numlock = true;
			}
			else
			{
				perror("Could not get keyboard leds because");
			}
			close(fd);
		}
		else
		{
			perror("Could not open /dev/console, are you root? Error is ");
		}
#endif
	}
}

void restore_leds() 
{
	change_led(g_save_numlock, g_save_capital, g_save_scroll);
}

// enables the LEDs for use since they are disabled by default
void enable_leds(bool value)
{
	g_leds_enabled = value;
}
