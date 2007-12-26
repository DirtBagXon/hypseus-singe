/*
 * sony.cpp
 *
 * Copyright (C) 2001 Robert DiNapoli
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


// Code specific for proper operation of the LDP-1450 (and other Sony players)
// Begun by Robert DiNapoli
// Maintained by Matt Ownby

#include <stdio.h>
#include <stdlib.h>	// for atoi
#include "sony.h"
#include "../timer/timer.h"
#include "../io/serial.h"
#include "../io/conout.h"
#include "../daphne.h"
#include "../io/input.h"
#include "../io/numstr.h"
#include <string>

using namespace std;

///////////////////////////////////////////////////////////////////////////

sony::sony()
{
	need_serial = true;
}

/*
// this is the old code which has been replaced
bool sony::search(char *new_frame)
{
	bool result = true; //assume good
	int x = 0;

	serial_tx(0x43); //search
	receive_status(0x0A);

	// send each frame number, waiting for acknowledgement each time
	for (x=0; x < FRAME_SIZE; x++)
	{
		serial_tx(new_frame[x]);
		receive_status(0x0A);	// wait for ACK ...
	} //end for loop

	serial_tx(0x40); //enter
	receive_status(0x0A);	// wait for ACK

	if (!receive_status(0x01))
	{
		printline("Error: Sony LDP search took too long to complete");
		result = false;
	}

	return(result);
}
*/

bool sony::nonblocking_search(char *frame)
{
	bool result = true;

	serial_tx(0x43);	// search command
	receive_status(0x0A);
	for (int i = 0; i < FRAME_SIZE; i++)
	{
		serial_tx(frame[i]);	// send ASCII digits
		receive_status(0x0A);	// wait for ACK
	}
	serial_tx(0x40);	// send 'enter' command
	receive_status(0x0A);	// wait for ACK

	return result;
}

// we assume that this function will only get called after a search has been issued
int sony::get_search_result()
{
	int result = SEARCH_BUSY;

	if (serial_rx_char_waiting())
	{
		unsigned char ch = serial_rx();

		// if search was successful
		if (ch == 0x01)
		{
			result = SEARCH_SUCCESS;
		}

		// if we get any other character, then we can assume that the search failed
		else
		{
			string err_msg = "SONY SEARCH ERROR: Received result 0x";
			err_msg += numstr::ToStr(ch, 16, 2);
			err_msg += ", expected 0x01";
			printline(err_msg.c_str());
			result = SEARCH_FAIL;
		}
	}

	return result;
}

unsigned int sony::play()
{
    serial_tx(58);
    if (!receive_status(0x0A))
	{
		printline("Error: No response from Sony LDP");
	}
	
	return(refresh_ms_time());
}

void sony::pause()
{
	serial_tx(0x4F);	// go into still mode
	if (!receive_status(0x0A))
	{
		printline("Error: No response from Sony LDP");
	}
}


void sony::stop()
{
	const int max_attempts = 1;
	int attempts = 0;
	bool success = false;

	while (!success && attempts < max_attempts)
	{
		serial_rxflush();
		serial_tx(0x3F);

		success = receive_status(0x0A);
		if (!success)
		{
			make_delay(10);
			attempts++;
		}
	}
}

/*
// queries sony player for current frame
// I'm not sure if it's possible to query too quickly or not
// (if you get errors querying for frame number, use the generic function instead)
Uint16 sony::get_current_frame()
{
	char frame[FRAME_ARRAY_SIZE] = { 0 };
	int frame_index = 0;

	serial_tx(0x60);	// frame query

	// get each frame #
	for (frame_index = 0; frame_index < FRAME_SIZE; frame_index++)
	{
		frame[frame_index] = serial_get_one_byte();
	}

	return(static_cast<Uint16>(atoi(frame)));
}
*/

bool sony::receive_status (Uint8 expected_code)
//pass the status code that the player is expected to return
//return true if the code was found, false represents failure
{
	unsigned int cur_time = refresh_ms_time();	// current time
	bool result = false;

	// keep waiting for a response until we timeout or the quitflag gets set
	while ((elapsed_ms_time(cur_time) < 3000) && (!get_quitflag()))
	{
		// if we have a character waiting
		if (serial_rx_char_waiting())
		{
			// if we get our expected code, we're done
			if (serial_rx() == expected_code)
			{
				result = true;
				break;
			}
		}

		SDL_check_input();	// check to see if we've got a quit signal
		make_delay(1);
	}

	return (result);

}

//	Functions to enable/disable the audio channels
void sony::enable_audio1()
{
    serial_tx(0x46);
    if (!receive_status(0x0A))
	{
		printline("Error: No response from Sony LDP");
	}
}

void sony::disable_audio1()
{
    serial_tx(0x47);
    if (!receive_status(0x0A))
	{
		printline("Error: No response from Sony LDP");
	}
}

void sony::enable_audio2()
{
    serial_tx(0x48);
    if (!receive_status(0x0A))
	{
		printline("Error: No response from Sony LDP");
	}
}

void sony::disable_audio2()
{
    serial_tx(0x49);
    if (!receive_status(0x0A))
	{
		printline("Error: No response from Sony LDP");
	}
}
