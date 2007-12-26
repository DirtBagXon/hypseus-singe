/*
 * pioneer.cpp
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


// pioneer.c
// contains code to control most Pioneer LD players
// excluding the LD-V6000 and some of the really early
// ones (such as the LD-V1000 hehe, piece of trash)

// written by Matt Ownby, "Ace64", and Mark Broadhead

#include <stdio.h>
#include <stdlib.h>	// for atoi
#include <string.h>
#include "../io/serial.h"
#include "../timer/timer.h"
#include "../io/conout.h"
#include "pioneer.h"
#include "../daphne.h"
#include "../io/input.h"

#include <string>
using namespace std;

// define strcasecmp in case we're lame and compiling under windows =]
#ifdef WIN32
#define strcasecmp stricmp
#endif

#ifdef WIN32
#pragma warning (disable:4100)	// disable the warning about unrereferenced parameters
#endif

// pioneer constructor
pioneer::pioneer()
{
	need_serial = true;
}

// initializes the pioneer player
// returns true if initialization was successful
// returns false if initialization failed
bool pioneer::init_player()
{

	bool success = false;

	outstr("Initializing Pioneer Laserdisc Player...");
	send_tx_string("CL"); // clear buffer  
	success = check_result(1000, true);
	serial_rxflush();

	if (success)
	{
		send_tx_string("3AD"); // enable all sound
		pioneer_audio1 = true;
		pioneer_audio2 = true;
		success = check_result(1000, true);

		if (success)
		{
			send_tx_string("1VD"); // "ViDeo control 'ON'"
			success = check_result(1000, true);

			if (success)
			{
				send_tx_string("0KL"); // "Key Lock 'OFF'"
				success = check_result(1000, true);
			}
			else
			{
				printline("Pioneer error, VD command not supported");
			}
		}
		else
		{
			printline("Pioneer error, AD command not supported");
		}
	}
	else
	{
		printline("Pioneer error, CL command not support (you probably aren't communicating with your player)");
	}

	if (success)
	{
		printline("Success!");
		printmodel();
	}
	else
	{
		printline("FAILED =(");
	}

	return(success);

}

/*
// searches to "frame" (frame is ASCII)
bool pioneer::search(char *frame)
{

	int i = 0;
	const int max_attempts = 1;
	int attempts = 0;
	bool success = false;

	while (!success && attempts < max_attempts)
	{
		serial_rxflush();	// clear receive buffer

		for (i = 0; i < FRAME_SIZE; i++)
		{
			serial_tx(frame[i]);	// send frame #
		}
		send_tx_string("SE");	// Pioneer search command

		success = check_result(5000, true);	// check result & return
		if (!success)
		{
			printline("Pioneer search: Failed");
			make_delay(10);	// wait this long (ms) before trying again
			attempts++;
		}
	}

	return(success);

}
*/

bool pioneer::nonblocking_search(char *frame)
{
	serial_rxflush();	// clear receive buffer

	// send frame
	for (int i = 0; i < FRAME_SIZE; i++)
	{
		serial_tx(frame[i]);
	}
	send_tx_string("SE");	// pioneer search command

	return true;	// only error here would be unplugged serial cable
}

// this is only called immediately after a search command
int pioneer::get_search_result()
{
	char ch = 0;
	int result = SEARCH_BUSY;

	// we don't want to spend any time waiting for a result unless we have one to wait for
	if (serial_rx_char_waiting())
	{
		// the result isn't complete until it gets a CR/LF at the end.
		// We therefore want to wait until we get the CR/LF, which we assume will be instant,
		// but it is safe to put a long timeout anyway even though we never expect to wait
		// very long.
		if (check_result(5000, true))
		{
			result = SEARCH_SUCCESS;
		}
		else
		{
			string err_msg = "PIONEER SEARCH ERROR : Expected R but received ";
			err_msg += ch;
			printline(err_msg.c_str());
			result = SEARCH_FAIL;
		}
		serial_rxflush();	// clear out other characters from receive buffer (such as LF or CR)
	}
	return result;
}

// starts playing the disc
unsigned int pioneer::play()
{

	const int max_attempts = 1;
	int attempts = 0;
	int success = 0;

	// loop while we haven't gotten a result yet,
	// while we haven't given up,
	// and while we haven't received a quit signal
	while ((!success) && (attempts < max_attempts) && (!get_quitflag()))
	{
		serial_rxflush();
		send_tx_string("PL");	// pioneer play command
		success = check_result(15000, true); // gotta wait for disc to spin up

		if (!success)
		{
			printline("Pioneer play: Failed");
			make_delay(10);	// wait 10 ms before trying again
			attempts++;
		}
		SDL_check_input();
	}
	
	return (refresh_ms_time());
}

// sends a still command to the Pioneer player
void pioneer::pause()
{

	int success = 0;

	serial_rxflush();
	send_tx_string("ST");	// still frame
	success = check_result(1000, true);

	if (!success)
	{
		printline("Pioner pause failed");
	}

}

// stops a pioneer player
void pioneer::stop()
{
	char s[81] = { 0 };
	
	serial_rxflush();
	send_tx_string("?P");	// check to see if disc is playing

	getstring(s, 80, 3000, false);	// get a string, ignore quitflag
	
	// result P04 means the disc is playing
	// we only want to send a RJ command if the disc is playing,
	// because otherwise it will eject the disc	
	if (strcasecmp(s, "P04")==0)
	{
		serial_rxflush();
		send_tx_string("RJ");	// reject
		
		// we need to wait a while for the disc to stop
		if (!check_result(10000, false))
		{
			printline("Pioneer Reject command (RJ) failed!");
		}
	}
	else
	{
		printline("Pioneer: ignoring stop command because disc is not playing");
		printline(s);
	}
}


// returns current frame # pioneer player is on
Uint16 pioneer::get_real_current_frame()
{
	char s[81] = { 0 };
	bool gotframe = false;

	serial_rxflush();
	send_tx_string("?F");	// pioneer get frame

	gotframe = getstring(s, 80, 3000, true);
	
	return(static_cast<Uint16>(atoi(s)));
}


// checks for result codes from the pioneer player
// returns a true if Pioneer is 'ready'
// or a false if there is an error
// timeout_val is how many ms to wait for a response
// watchquit is whether or not to abort if a quit signal is requested
//   (you always want to do this unless you are sending a command
//    after the quitflag has been enabled, such as 'stop')
bool pioneer::check_result(Uint32 timeout_val, bool watchquit)
{

	char s[81] = { 0 };
	bool result = false;
	bool gotstring = false;

	gotstring = getstring(s, 80, timeout_val, watchquit);
	
	if (gotstring && (s[0] == 'R'))
	{
		result = true;
	}

	return(result);
}

// retrieves a LF/CR terminted string from the pioneer
// length is the length of the string
// timeout is the # of ms to wait before giving up
// watchquit is true if we want to stop the loop if the quitflag is set
//  (the only time we don't want to is if we send a stop command which might
//   be after the quitflag has already been set)
// returns true if it got a legit string or false if it gave up
bool pioneer::getstring(char *s, int length, Uint32 timeout, bool watchquit)
{
	int index = 0;
	unsigned int cur_time = refresh_ms_time();
	bool result = false;
	char ch = 0;

	// loop while we haven't filled up the buffer yet,
	// and while we haven't received a quit signal	
	while ((index < length) && !(watchquit && get_quitflag()))
	{
		// if we have a character waiting
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
			printline("Pioneer warning: timed out waiting for a response");
			break;
		}
		SDL_check_input();
	} // end while
	
	s[index] = 0;	// terminate string
	
	return(result);
}

// Pioneer skip_forward
// Only the 4400 and 8000 support this feature 
// Tests if player can skip_forward and if can't uses the generic search function
bool pioneer::skip_forward(Uint16 frames_to_skip, Uint16 target_frame)
{
	bool result = false;
	char frame[6] = {0};

	if (skipping_supported)
	{
		char s[81] = {0};
		
		int i = 0;
//		send_tx_string("0VD"); // squelch video for jump - experimental
//		result = check_result(5000, true);	// check result 
		serial_rxflush();	// clear receive buffer
	
		sprintf(frame, "%d", frames_to_skip); // convert number of frames to string

		while (frame[i])
		{
		 	serial_tx(frame[i]);
			i++;
			if (i > FRAME_SIZE)
			{
				printline("PIONEER::skip_forward Too many digits to skip");
				break;
			}
		}		

		send_tx_string("JF");	// Pioneer multi-track jump forward

		result = check_result(5000, true);	// check result 
		
//		send_tx_string("1VD"); // turn video back on - experimental
//		result = check_result(5000, true);	// check result 		

		sprintf(s, "Pioneer: skipped %d frames", frames_to_skip);
		printline(s);
	}
	else
	{
		sprintf(frame, "%05d", target_frame);

		result = pre_search(frame, true);
		pre_play();
	}

	return(result);
}

// queries pioneer player for its model number and prints it
void pioneer::printmodel()
{

	char id[81] = { 0 };
	char model[81] = { 0 };
	char firmware[2] = { 0 };
	bool gotid = false;
	
	send_tx_string("?X");	// query player for its type
	gotid = getstring(id, 80, 3000, true);

	// this is probably not the smartest way of doing things....
	strcpy(model, id);
	model[5] = 0; // terminate model
	firmware[0] = id[5]; // set firmware
	firmware[1] = id[6];
	firmware[2] = 0;


	// if we actually got a string
	if (gotid)
	{
		if (strcasecmp(model, "P1527") == 0)
		{
			printline("Pioneer CLD-V2600 detected!");
		}

		else if (strcasecmp(model, "P1518") == 0)
		{
			printline("Pioneer CLD-V2400 detected!");
		}

		else if (strcasecmp(model, "P1516") == 0)
		{
			skipping_supported = true; // Only the 4400 and 8000 support the skip command
			printline("Pioneer LD-V4400 detected!");
		}

		else if (strcasecmp(model, "P1515") == 0)
		{
			printline("Pioneer LD-V4300 detected!");
		}

		else if (strcasecmp(model, "P1507") == 0)
		{
			printline("Pioneer LD-V2200 detected!");
		}

		else if (strcasecmp(model, "P1506") == 0) 
		{
			skipping_supported = true; // Only the 4400 and 8000 support the skip command
			printline("Pioneer LD-V8000 detected!");
		}

		else if (strcasecmp(model, "P1505") == 0)
		{
			printline("Pioneer LC-V330 detected!");
		}

		else if (strcasecmp(model, "P1502") == 0)
		{
			outstr("Pioneer LD-V4200");
			if (strcasecmp(firmware, "04") == 0)
			{
				printline("-Enhanced detected!");
			}
			else if (strcasecmp(firmware, "03") == 0)
			{
				printline("-Original detected!");
			}
			else 
			{
				printline("-Unknown revision detected!");
			}
		}

		else
		{
			outstr("Unknown Pioneer model ");
			outstr(id);
			printline(" detected.");
			printline("Please contact Matt Ownby and tell him which Pioneer model you are using!");
		}
	}

	else
	{
		printline("Could not query Pioneer LDP for its model type =(");
	}
}

void pioneer::enable_audio1()
{
	if (pioneer_audio2)
	{
		serial_rxflush();

		send_tx_string("3AD"); // enable all sound
		check_result(1000, true);
		pioneer_audio1 = true;
		pioneer_audio2 = true;
	}
	else
	{
		serial_rxflush();

		send_tx_string("0AD"); // turn off sound
		check_result(1000, true);
		send_tx_string("1AD"); // enable just audio1
		check_result(1000, true);
		pioneer_audio1 = true;
		pioneer_audio2 = false;
	}
}

void pioneer::disable_audio1()
{
	if (pioneer_audio2)
	{
		serial_rxflush();

		send_tx_string("0AD"); // turn off sound
		check_result(1000, true);
		send_tx_string("2AD"); // enable just audio2
		check_result(1000, true);
		pioneer_audio1 = false;
		pioneer_audio2 = true;
	}
	else
	{
		serial_rxflush();

		send_tx_string("0AD"); // disable all sound
		check_result(1000, true);
		pioneer_audio1 = false;
		pioneer_audio2 = false;
	}
}

void pioneer::enable_audio2()
{
	if (pioneer_audio1)
	{
		serial_rxflush();

		send_tx_string("3AD"); // enable all sound
		check_result(1000, true);
		pioneer_audio1 = true;
		pioneer_audio2 = true;
	}
	else
	{
		serial_rxflush();

		send_tx_string("0AD"); // turn off sound
		check_result(1000, true);
		send_tx_string("2AD"); // enable just audio2
		check_result(1000, true);
		pioneer_audio1 = false;
		pioneer_audio2 = true;
	}
}

void pioneer::disable_audio2()
{
	if (pioneer_audio1)
	{
		serial_rxflush();

		send_tx_string("0AD"); // turn off sound
		check_result(1000, true);
		send_tx_string("1AD"); // enable just audio1
		check_result(1000, true);
		pioneer_audio1 = true;
		pioneer_audio2 = false;
	}
	else
	{
		serial_rxflush();

		send_tx_string("0AD"); // disable all sound
		check_result(1000, true);
		pioneer_audio1 = false;
		pioneer_audio2 = false;
	}
}
