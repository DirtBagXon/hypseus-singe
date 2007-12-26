/*
 * hitachi.cpp
 *
 * Copyright (C) 2001 Andrew Hepburn
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


//////////////////////////////////////////////////////////////
//
// hitachi.cpp
// by Andrew Hepburn
//
//////////////////////////////////////////////////////////////
//
// contributors include:
// Matt Ownby
// Mark Broadhead
//
//////////////////////////////////////////////////////////////

// Code specific for proper operation of the Hitachi VIP9550
// Dip switches for 9600 baud (1 = up, 2 = up, 3 = up, 4 = up)
// The 9550 uses a regular modem cable, NOT a null modem cable
// Original command set by Robert DiNapoli

#include "../timer/timer.h"
#include "../io/serial.h"
#include "../io/conout.h"
#include "../daphne.h"	// for get_quitflag
#include "../game/game.h"	// to get game FPS
#include "hitachi.h"

#ifdef WIN32
#pragma warning (disable:4100)
#endif

// constructor
hitachi::hitachi()
{
	need_serial = true;
	skipping_supported = true;	// the Hitachi does support skipping
	max_skippable_frames = 150;
	skip_instead_of_search = true;
}

//
// Initialize
//
// initializes the Hitachi player
//
bool hitachi::init_player()
{
	bool success = true;

	// send commands to reset player
	serial_tx(0x75); // if player got locked by something like using the LDP.exe program
	serial_tx(0x68); // resets player functions (turns off frame counter)

//	serial_tx(0x4C);	// MATT DEBUG: turn on frame counter

	// send command to turn on response
	serial_tx(0x71);

	// wait for player response
	success = receive_status(0xF1, 3000);

	if (!success)
	{
		printline("Error: No response from Hitachi 9550 during initialization");
	}

	return (success);
}


/*
//
// Search
//
// Called to make the Hitachi seek to a frame
// Return true if you successfully searched and found the frame
// Return false if there was an error, but only as a last resort
// If there is an error, always try to resolve it here before returning false
bool hitachi::search(char *frame)
{

    bool result = true; //assume good
	int x = 0;

	// player does not respond until enter is sent

	// send search command
	serial_tx(0x2B); 

	// send clear command
	serial_tx(0x3A);

	// send frame digits
	for (x=0; x < FRAME_SIZE; x++)
	{
		serial_tx(frame[x]);
	}

	// send enter command
	serial_tx(0x41);

	// wait for response from player
	if (!receive_status(0xB0, 4000))
	{
		printline("Error: Hitachi 9550 search took too long to complete");
		result = false;
	}

	return(result);
}
*/

bool hitachi::nonblocking_search(char *frame)
{
    bool result = true; //assume good
	int x = 0;

	// player does not respond until enter is sent

	// send search command
	serial_tx(0x2B); 

	// send clear command
	serial_tx(0x3A);

	// send frame digits
	for (x=0; x < FRAME_SIZE; x++)
	{
		serial_tx(frame[x]);
	}

	// send enter command
	serial_tx(0x41);
	return result;
}

int hitachi::get_search_result()
{
	int result = SEARCH_BUSY;
	if (serial_rx_char_waiting())
	{
		// we except 0x41 0xB0 to come back
		if (receive_status(0xB0, 1000))
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

// skips forward frames_to_skip number of frames, and returns true if it was successful
bool hitachi::skip_forward(Uint16 frames_to_skip, Uint16 target_frame)
{
	char frame[FRAME_ARRAY_SIZE] = { 0 };
	int i = 0;
	bool result = true;

	// The hitachi only skips forward frames if the disc is 29.97 FPS.  If the disc is 23.976,
	// then we have to convert the number we send to the hitachi (by multiplying it by
	// 29.97/23.976).

	if (g_game->get_disc_fpks() != 29970)
	{
		printline("Hitachi: Disc is not standard 29.97 and therefore we must convert the skip parameter");
		frames_to_skip = (Uint16) ((1.25 * frames_to_skip) + 0.5); // to float multiplication and add 0.5 to round to the nearest whole number
	}

	framenum_to_frame(frames_to_skip, frame);
	serial_tx(0x46);	// command to skip forward

	// send number to skip in frame format
	for (i = 0; i < FRAME_SIZE; i++)
	{
		serial_tx(frame[i]);
	}

	serial_tx(0x41);	// send enter command

	// expect to receive 0x41 followed by 0xC6 on successful search
	if (!receive_status(0xC6, 1000))
	{
		printline("Error: Hitachi 9550 skip failed");
		result = false;
	}
	
	return(result);
}


//
// Play
// Called to make the Hitachi player play
// Return after you have verified that the player is playing
// (if there is an error, you should resolve it before returning)
//
unsigned int hitachi::play()
{
	serial_tx(0x25);
	
	// wait 15 seconds for disc to spin up =]
	if (!receive_status(0xA5, 15000))
	{
		printline("Error: No response from Hitachi 9550");
	}

	return (refresh_ms_time());

}

//
// Pause
//
// Called to make the Hitachi player pause
//
void hitachi::pause()
{
	serial_tx(0x24);	// go into still mode
	if (!receive_status(0xA4, 1000))
	{
		printline("Error: No response from Hitachi 9550 for pause command");
	}
}

// waits 'timeout' ms for 'expected_code' from the player
// if it receives it within that time, it returns 'true', otherwise 'false'
bool hitachi::receive_status (Uint8 expected_code, Uint32 timeout)
{
	unsigned int cur_time = refresh_ms_time();	// current time
	bool result = false;

	// keep waiting for a response until we timeout or the quitflag gets set
	while ((elapsed_ms_time(cur_time) < timeout) && (!get_quitflag()))
	{
		// if there is an incoming character waiting to be retrieved ...
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

//
// STOP
//
// Called to make the Hitachi player STOP
//
void hitachi::stop()
{
	const int max_attempts = 1;
	int attempts = 0;
	bool success = false;

	while (!success && attempts < max_attempts)
	{
	        serial_rxflush();
                serial_tx(0x2F);

                success = receive_status(0xA5, 3000);
                if (!success)
		{
                        make_delay(10);
                        attempts++;
		}
	}
}

// comment this out if the hitachi can't return frames fast enough .. it can get stuck
//Uint16 hitachi::get_current_frame()
//{
//	return get_real_current_frame();
//}

// returns the actual current frame as a 16-bit integer
// There are a few problems with this
// 1 - Sometimes it gets "stuck" and times out after a second or two... this is obviously no good during a game
// 2 - When it works properly it can't return the current frame # fast enough to be useful (no good w/ super don for example)
// So there is really no point to using this unless you _have_ to know the real frame
Uint16 hitachi::get_real_current_frame()
{
	Uint8 highbyte = 0, lowbyte = 0;
	Uint16 result = 0;
	
	serial_rxflush();
	serial_tx(0x6B);	// code to get current frame
	
	// make sure we get a response (0x6B) before we get 2 more bytes
	if (receive_status(0x6B, 1000))
	{
		highbyte = serial_get_one_byte(1000);
		lowbyte = serial_get_one_byte(1000);
	}
	
	result = (Uint16) ((highbyte << 8) | lowbyte);
	return(result);
}


//	Functions to enable/disable the audio channels
//	No error checking is done. Anyone that knows what the 
//	return codes are should add this 
void hitachi::enable_audio1()
{
	serial_tx(0x48);	
}

void hitachi::disable_audio1()
{
	serial_tx(0x49);	
}

void hitachi::enable_audio2()
{
	serial_tx(0x4a);	
}

void hitachi::disable_audio2()
{
	serial_tx(0x4b);	
}
