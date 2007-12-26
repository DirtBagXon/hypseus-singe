/*
 * ld-v6000.cpp
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


// File to control the Pioneer LD-V6000 series of players
// written by Matt Ownby

#include <string.h>
#include <stdio.h>
#include "../io/serial.h"
#include "../timer/timer.h"
#include "../io/conout.h"
#include "../daphne.h"
#include "../io/input.h"
#include "ld-v6000.h"

#ifdef WIN32
#pragma warning (disable:4100)
#endif

// two-character codes that represent the digits 0 to 9
const char *ldv6000_digits[10] =
{
	"3F", "0F", "8F", "4F", "2F", "AF", "6F", "1F", "9F", "5F"
};

// constructor
v6000::v6000()
{
	need_serial = true;
}

// initializes v6000 (by disabling the flickering white text)
bool v6000::init_player()
{
	serial_tx('E');
	serial_tx('1');	// disable character generation
	make_delay(10);
	// for now I am not sure how to check to see if this command
	// is accepted or not
	
	return(true);
	
}

/*
// does a seek on the v6000
bool v6000::search(char *frame)
{

	int i = 0;
	const int max_attempts = 3;
	int attempts = 0;
	bool success = false;
	unsigned int index = 0;

	while (!success && attempts < max_attempts)
	{
		for (i = 0; i < 5; i++)
		{
			index = frame[i] - '0';	// convert ASCII to a numerical index

			// if we haven't overflowed, send the digits
			if (index <= 9)
			{
				serial_tx(ldv6000_digits[index][0]);
				serial_tx(ldv6000_digits[index][1]);
			}
			else
			{
				printline("Bug in v6000_search function");
			}
		}	// end for loop
		serial_tx('F');
		serial_tx('7');	// search command
		success = wait_for_finished();	// check result & return
		if (!success)
		{
			printline("LD-V6000 search: Failed");
			make_delay(10);	// wait this long (ms) before trying again
			attempts++;
		}
	}

	return(success);
}
*/

bool v6000::nonblocking_search(char *frame)
{
	int i = 0;
	unsigned int index = 0;

	for (i = 0; i < 5; i++)
	{
		index = frame[i] - '0';	// convert ASCII to a numerical index

		// if we haven't overflowed, send the digits
		if (index <= 9)
		{
			serial_tx(ldv6000_digits[index][0]);
			serial_tx(ldv6000_digits[index][1]);
		}
		else
		{
			printline("Bug in v6000_search function");
		}
	}	// end for loop
	serial_tx('F');
	serial_tx('7');	// search command

	return true;
}

int v6000::get_search_result()
{
	int result = SEARCH_FAIL;
	char s[81];

	serial_rxflush();	// flush input buffer
	serial_tx('D');
	serial_tx('4');	// status query command

	bool gotstatus = getstring(s, 1000, true);

	// if we were able to successfully query the LD-V6000
	if (gotstatus)
	{
		// if the LDP is paused (finished searching)
		if (strcmp(s, "65")==0)
		{
			result = SEARCH_SUCCESS;
		}

		// busy searching
		else if (strcmp(s, "50")==0)
		{
			result = SEARCH_BUSY;
		}

		// anything else is an error, so we'll return SEARCH_FAIL
	}

	// else if we don't get a result, then it's an error ...

	return result;
}


// skips forward a certain # of frames
bool v6000::skip_forward(Uint16 frames_to_skip, Uint16 target_frame)
{
	bool result = false;
	char digits[6];
	int count = 0, index = 0;


	if (frames_to_skip < 100)
	{
		sprintf(digits, "%05d", frames_to_skip);

		for (count = 0; count < 5; count++)
		{
			index = digits[count] - '0';	// convert to decimal from ASCII
			serial_tx(ldv6000_digits[index][0]);	// send digit
			serial_tx(ldv6000_digits[index][1]);
		}
		serial_tx('8');
		serial_tx('0');	// skip forward command
	}
	else
	{
		printline("LD-V6000 error : Cannot skip more than 100 frames!");
	}

	return result;
}

// sends play command to ld-v6000
unsigned int v6000::play()
{

	const int max_attempts = 1;
	int attempts = 0;
	bool success = false;

	printline("Sending play command to the LD-V6000");
	while (!success && attempts < max_attempts)
	{
		serial_tx('F');
		serial_tx('D');
		success = wait_for_finished();

		if (!success)
		{
			printline("LD-V6000 play: Failed");
			make_delay(10);	// wait 10 ms before trying again
			attempts++;
		}
	}

	return(refresh_ms_time());
}

// sends PAUSE command to ld-v6000
void v6000::pause()
{
	serial_tx('F');
	serial_tx('B');
	make_delay(10);
}

// sends STOP command to ld-v6000
void v6000::stop()
{
	char status[5] = { 0 };

	// We have to check to see whether the disc is playing before we send a reject
	// command because if the disc is not playing, the player will eject the disc.
	serial_rxflush();	
	serial_tx('D');
	serial_tx('4');
	make_delay(10);
	getstring(status, 1000, false);

	// if disc is currently playing, then it's ok to send a stop command
	if (strcmp(status, "64")==0)
	{
		serial_tx('F');
		serial_tx('9');
		make_delay(10);
	}
	else
	{
		outstr("V6000: Ignoring stop command because disc is not playing -> ");
		printline(status);
	}
}


void v6000::enable_audio1()
{
    serial_tx('0');
	serial_tx('F'); // 1

	serial_tx('F');
	serial_tx('4'); // audio1
}

void v6000::disable_audio1()
{
	serial_tx('3');
	serial_tx('F'); // 0

	serial_tx('F');
	serial_tx('4'); // audio 1
}

void v6000::enable_audio2()
{
	serial_tx('0');
	serial_tx('F');	// 1

	serial_tx('F');
	serial_tx('C');	// audio2
}

void v6000::disable_audio2()
{
	serial_tx('3');
	serial_tx('F');	// 0

	serial_tx('F');
	serial_tx('C');	// audio 2
}



bool v6000::wait_for_finished()
// checks LD-V6000 and waits until it is finished either searching or spinning up to play
// returns a 1 if player is 'finished'
// or a 0 if there is an error
{

	const int time_to_finish = 3;	// wait this long to timeout
	char s[81] = { 0 };
	char debug_string[81] = { 0 };
	int status_timer = 0;
	int finish_timer = 0;
	int bytes_read = 0;
	bool finished = false;
	bool spun_up_before = false;
	bool gotstatus = false;

	// keep querying the LD-V6000 for its status until it is finished searching or until we time out
	while ((!finished) && (finish_timer < time_to_finish))
	{

		serial_rxflush();	// flush input buffer
		serial_tx('D');
		serial_tx('4');	// status query command
		bytes_read = 0;
		status_timer = 0;
		s[0] = 0;

		gotstatus = getstring(s, 1000, true);

		// if we were able to successfully query the LD-V6000
		if (gotstatus)
		{

		  // if the LDP is currently playing the disc
		  if (strcmp(s, "64")==0)
		  {
			finished = true;
		  }

		  // if the LDP is paused (finished searching)
		  else if (strcmp(s, "65")==0)
		  {
			finished = true;
		  }

		  // if the LDP is busy searching
		  else if (strcmp(s, "50")==0)
		  {
			finish_timer = 0;
		  }

		  // if the disc is spinning up
		  else if (strcmp(s, "68")==0)
		  {
		  	// we only want to print the "spin up" message once
			if (!spun_up_before)
			{
				spun_up_before = true;
				printline("Waiting for disc to spin up...");
			}
			finish_timer = 0;
		  }

		  // if the LDP is stopped
		  else if (strcmp(s, "78")==0)
		  {
		  }

		  // if the LDP is spinning down
		  else if (strcmp(s, "7C")==0)
		  {
		  }

		  else
		  {
			outstr("Unknown result code:");
			printline(s);
			serial_tx('F');	// send an extra byte to try to get us back in sync
				// NOTE: This could be a really stupid thing to do
				// This driver is too new to be sure =]
		  }

		}	// end if bytes_read was correct
		else
		{
			sprintf(debug_string, "Got garbage result from player: %s\n", s);
		}

		finish_timer = finish_timer + 1;

	} // end while loop

	if (!finished)
	{
		printline("Operation TIMED OUT");
	}

	return(finished);
}

// gets a 2-character result code from the LD-V6000
// returns code in s
// timeout is how many ms to wait before giving up
// watchquit is whether to abort if the quitflag is set (set it to true if you aren't sure)
// returns false if it could not get the result code
bool v6000::getstring(char *s, Uint32 timeout, bool watchquit)
{
	int index = 0;
	unsigned int cur_time = refresh_ms_time();
	bool result = false;

	// loop while we haven't got our 4 digits
	// and while we haven't received a quit signal	
	while ((index < 4) && !(watchquit && get_quitflag()))
	{
		// if we received a character
		if (serial_rx_char_waiting())
		{
			s[index] = serial_rx();
			index++;
		} // end if we received a character
		
		if (elapsed_ms_time(cur_time) >= timeout)
		{
			printline("V6000 warning: timed out waiting for a response");
			break;
		}
		SDL_check_input();
	} // end while
	
	s[2] = 0;	// terminate string, toss away last two characters (linefeed & CR)
	
	// if we got all 4 digits, we succeeded
	if (index == 4)
	{
		result = true;
	}
	
	return(result);
}
