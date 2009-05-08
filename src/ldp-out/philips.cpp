/*
 * philips.cpp
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


// philips.cpp
// Controls newer Philips laserdisc players
// written by Matt Ownby
// thanks to Matteo Marioni for the command protocol information

#include <stdio.h>
#include <stdlib.h>	// for atoi
#include <string.h>
#include "../io/serial.h"
#include "../timer/timer.h"
#include "../io/conout.h"
#include "philips.h"
#include "../daphne.h"	// for get_quitflag
#include "../io/input.h"	// to check input

// define strcasecmp in case we're lame and compiling under windows =]
#ifdef WIN32
#define strcasecmp stricmp
#endif

// philips constructor
philips::philips()
{
	need_serial = true;
}

bool philips::init_player()
{
	bool success = true;
	
	// MATT : update, Matteo Marioni says that the 22VP835 will work,
	// in addition to the other Philips players, if this init stuff is
	// commented out.

/*
	bool success = false;

	// turn on audio in case it's off
	enable_audio1();
	enable_audio2();
	
	send_tx_string(",1");
	// this command is supposed to start the disc playing and pause on frame one, sending
	// an "S" when it has succeeded
	
	success = check_result("S", 20000, true);	// check result & return
	// wait 20 seconds for success
*/
	
	return(success);
	
}

/*
// searches to "frame" (frame is ASCII)
bool philips::search(char *frame)
{

	int i = 0;
	bool success = false;

	serial_rxflush();	// clear receive buffer

	serial_tx('F');
	for (i = 0; i < FRAME_SIZE; i++)
	{
		serial_tx(frame[i]);	// send frame #
	}
	send_tx_string("R");	// append with an R and a carriage return

	success = check_result("A0", 5000, true);	// check result & return

	if (!success)
	{
		printline("Philips search: Failed");
	}

	return(success);

}
*/

bool philips::nonblocking_search(char *frame)
{
	int i = 0;
	bool success = true;

	serial_rxflush();	// clear receive buffer

	serial_tx('F');
	for (i = 0; i < FRAME_SIZE; i++)
	{
		serial_tx(frame[i]);	// send frame #
	}
	send_tx_string("R");	// append with an R and a carriage return

	return success;
}

int philips::get_search_result()
{
	int result = SEARCH_BUSY;
	if (serial_rx_char_waiting())
	{
		if (check_result("A0", 5000, true))
		{
			result = SEARCH_SUCCESS;
		}
		else
		{
			result = SEARCH_FAIL;
		}
	}
	return result;
}

/*
// skips forward 'frames_to_skip' from current position
// WARNING: This command seems to be inaccurate as it does not work correctly for Super Don
// so it is commented out until someone can figure out what's wrong and fix it
bool philips::skip_forward(Uint16 frames_to_skip)
{

	int i = 0;
	Uint16 cur_frame = get_current_frame();
	bool success = true;
	char frame_ascii[5];

	cur_frame = (Uint16) (cur_frame + frames_to_skip);
	sprintf(frame_ascii, "%05d", cur_frame);

	serial_rxflush();	// clear receive buffer

	serial_tx('F');
	for (i = 0; i < FRAME_SIZE; i++)
	{
		serial_tx(frame_ascii[i]);	// send frame #
	}
	send_tx_string("N");	// append with an N and a carriage return

	success = check_result("A1", 5000, true);	// check result & return

	if (!success)
	{
		printline("Philips skip: Failed");
	}

	return(success);
}
*/

// starts playing the disc
unsigned int philips::play()
{
	send_tx_string("N");	// philips play command
	return (refresh_ms_time());
}

// sends a still command to the philips player
void philips::pause()
{
	send_tx_string("*");	// still frame
}

// stops a philips player
void philips::stop()
{
	send_tx_string(",0");
	// puts the player in standby mode (stops it)
}

// checks for result codes from the philips player
// returns a true if it got the expected result
// or a false if there is an error
// timeout_val is how many ms to wait for a response
// watchquit is whether or not to abort if a quit signal is requested
//   (you always want to do this unless you are sending a command
//    after the quitflag has been enabled, such as 'stop')
bool philips::check_result(const char *result_string, Uint32 timeout_val, bool watchquit)
{

	char s[81] = { 0 };
	bool result = false;
	bool gotstring = false;

	gotstring = getstring(s, 80, timeout_val, watchquit);

	// if we got a result and the result matches what we were expecting	
	if (gotstring)
	{
		// if we have a match
		if (strcasecmp(result_string, s)==0)
		{
			result = true;
		}
		else
		{
			outstr("PHILIPS player returned unexpected response.  We wanted ");
			outstr(result_string);
			outstr(" but we got ");
			printline(s);
		}
	}

	return(result);
}

// retrieves a LF/CR terminted string from the philips
// length is the length of the string
// timeout is the # of ms to wait before giving up
// watchquit is true if we want to stop the loop if the quitflag is set
//  (the only time we don't want to is if we send a stop command which might
//   be after the quitflag has already been set)
// returns true if it got a legit string or false if it gave up
bool philips::getstring(char *s, int length, Uint32 timeout, bool watchquit)
{
	int index = 0;
	unsigned int cur_time = refresh_ms_time();
	bool result = false;
	char ch = 0;

	// loop while we haven't filled up the buffer yet,
	// and while we haven't received a quit signal	
	while ((index < length) && !(watchquit && get_quitflag()))
	{
		// if we received a character
		if (serial_rx_char_waiting())
		{
			ch = serial_rx();
			// if we've got the complete string, we're done
			if ((ch == 0x0D) || (ch == 0x0A))
			{
				result = true;
				break;
			}
			else
			{
				s[index] = ch;
				index++;
			}
		} // end if we received a character
		
		if (elapsed_ms_time(cur_time) >= timeout)
		{
			printline("philips warning: timed out waiting for a response");
			break;
		}
		SDL_check_input();
	} // end while
	
	s[index] = 0;	// terminate string
	
	return(result);
}


void philips::enable_audio1()
{
	printline("PHILIPS: Enabling left channel");
	send_tx_string("A1");
}

void philips::disable_audio1()
{
	printline("PHILIPS: Disabling left channel");
	send_tx_string("A0");
}

void philips::enable_audio2()
{
	printline("PHILIPS: Enabling right channel");
	send_tx_string("B1");
}

void philips::disable_audio2()
{
	printline("PHILIPS: Disabling right channel");
	send_tx_string("B0");
}

// returns the actual current frame
// This is not fast enough to be used in some games like Super Don
Uint16 philips::get_real_current_frame()
{
	char s[81] = { 0 };
	
	serial_rxflush();
	send_tx_string("?F");	// frame number request
	
	getstring(s, 80, 1000, true);	// get result
	
	return((Uint16) atoi(&s[1]));	// first character is an F, followed by the frame
}
