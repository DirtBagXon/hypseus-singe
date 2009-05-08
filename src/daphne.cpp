/*
* daphne.cpp
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

// DAPHNE : Dragon's Lair / Space Ace Emulator
// Started by Matt Ownby, many people have contributed since then
// Begun August 3rd, 1999

// ORIGINAL GOAL (we've obviously done much more than this now hehe):

// To emulate the original Dragon's Lair / Space Ace game such that the original ROM's are executed
// properly, the joystick and buttons are emulated, and the Laserdisc Player is properly
// controlled
// In short, so that the original game may be played with the following requirements:
// - Original Dragon's Lair Laserdisc
// - Original Dragon's Lair ROM's (disk files, not the physical ROM's heh)
// - A laserdisc player
// - A common modern computer (i386 Win32, i386 Linux most likely)
// No other original hardware (motherboard, power supply, harness) required!

#include <string>
#include "io/my_stdio.h"

using namespace std;

#ifdef MAC_OSX
#include "mmxdefs.h"
#endif

#ifndef FREEBSD
#include <SDL_main.h>
#else
#include "/usr/local/include/SDL11/SDL_main.h"
#endif

#ifdef WIN32
// win32 doesn't have regular chdir apparently
#define chdir _chdir
#include <direct.h>
#endif

#ifdef UNIX
#include <unistd.h>	// for chdir
#endif

#include "io/homedir.h"
#include "io/input.h"
#include "daphne.h"
#include "timer/timer.h"
#include "io/serial.h"
#include "sound/sound.h"
#include "io/conout.h"
#include "io/conin.h"
#include "io/cmdline.h"
#include "io/network.h"
#include "video/video.h"
#include "video/led.h"
#include "ldp-out/ldp.h"
#include "video/SDL_Console.h"
#include "io/error.h"
#include "cpu/cpu-debug.h"
#include "cpu/cpu.h"
#include "game/game.h"

#include "globals.h"
// some global data is stored in this file

// -------------------------------------------------------------------------------------------------

////////////////////////////////////////

const char *get_daphne_version()
{
	return "1.0.7";
}

unsigned char get_filename(char *s, unsigned char n)
// prepares a filename using any wildcards we may have
// returns filename in s
// returns 1 if this filename contains wild characters
// returns 0 if this filename does not contain any wild characters
{

	unsigned int i = 0;
	unsigned char result = 0;

	for (i = 0; i < strlen(s); i++)
	{
		if (s[i] == '+')
		{
			result = 1;
			s[i] = (char) (n + 0x30);	// convert wildcard to number
		}
	}

	return(result);

}

void set_quitflag()
// sets the quit flag
{

	quitflag = 1;

}

unsigned char get_quitflag()
// returns the quit flag
{

	return(quitflag);

}

bool change_dir(const char *cpszNewDir)
{
	int i = chdir(cpszNewDir);
	return (i == 0);
}

// sets current directory based on the full path of the executable
// This solves the problem of someone running daphne without first being in the daphne directory
void set_cur_dir(const char *exe_loc)
{
	int index = strlen(exe_loc) - 1;	// start on last character
	string path = "";

	// locate the preceeding / or \ character
	while ((index >= 0) && (exe_loc[index] != '/') && (exe_loc[index] != '\\'))
	{
		index--;
	}

	// if we found a leading / or \ character
	if (index >= 0)
	{
		path = exe_loc;
		path.erase(index);	// erase from here to the end
		change_dir(path.c_str());
	}
}

/////////////////////// MAIN /////////////////////

// the main function for both Windows and Linux <grin>
int main(int argc, char **argv)
{
	int result_code = 1;	// assume an error unless we find otherwise

	set_cur_dir(argv[0]);	// set active directory

	reset_logfile(argc, argv);

	// initialize SDL without any subsystems but with the no parachute option so
	// 1 - we can initialize either audio or video first
	// 2 - we can trace segfaults using a debugger
	if (SDL_Init(SDL_INIT_NOPARACHUTE) < 0)
	{
		printerror("Could not initialize SDL!");
		exit(1);
	}

	// parse the command line (which allocates game and ldp) and continue if no errors
	// this is important!  if game_type or ldp_type fails to allocate g_game and g_ldp,
	// then the program will segfault and daphne must NEVER segfault!  hehe
	if (parse_cmd_line(argc, argv))
	{
		// MATT : we have to wait until after the command line is parsed before we do anything with the LEDs in case the
		// user does not enable them
		remember_leds(); // memorizes the status of keyboard leds
		change_led(false, false, false); // turns all keyboard leds off

		// if the display initialized properly
		if (load_bmps() && init_display())
		{
			if (sound_init())
			{
				if (SDL_input_init())
				{
					// if the roms were loaded successfully
					if (g_game->load_roms())
					{
						// if the video was initialized successfully
						if (g_game->video_init())
						{
							// if the game has some problems, notify the user before the user plays the game
							if (g_game->get_issues())
							{
								printnowookin(g_game->get_issues());
							}

							SDL_Delay(1000);
							// delay for a bit before the LDP is intialized to make sure
							// all video is done getting drawn before VLDP is initialized

							// if the laserdisc player was initialized properly
							if (g_ldp->pre_init())
							{
								if (g_game->pre_init())     // initialize all cpu's
								{                         
									printline("Booting ROM ...");
									g_game->start();	// HERE IS THE MAIN LOOP RIGHT HERE
									g_game->pre_shutdown();

									// Send our game/ldp type to server to create stats.
									// This was moved to after the game has finished running because it creates a slight delay (like 1 second),
									//  which throws off the think_delay function in the LDP class.
									net_send_data_to_server();

									result_code = 0;	// daphne will exit without any errors
								}
								else
								{
									//exit if returns an error but don't print error message to avoid repetition
								}
								g_ldp->pre_shutdown();
							}
							else
							{
								printerror("Could not initialize laserdisc player!");
							}
							g_game->video_shutdown();
						} // end if game video was initted properly
						else
						{
							printerror("Game-specific video initialization failed!");
						}
					} // end if roms were loaded properly
					else
					{
						printerror("Could not load ROM images! You must supply these.");
					}
					SDL_input_shutdown();
				}
				else
				{
					printerror("Could not initialize input!");
				}
				sound_shutdown();
			}
			else
			{
				printerror("Sound initialization failed!");
			}
			shutdown_display();	// shut down the display
		} // end init display
		else
		{
			printerror("Video initialization failed!");
		}
	} // end game class allocated properly

	// if command line was bogus, quit
	else
	{
		printerror("Bad command line or initialization problem (see daphne_log.txt for details). \n"
			"To run DAPHNE, you must specify which game to run and which laserdisc player you are using. \n"
			"For example, try 'daphne lair noldp' to run Dragon's Lair in testing mode.");
	}

	// if our g_game class was allocated
	if (g_game)
	{
		delete(g_game);
	}

	// if g_ldp was allocated, de-allocate it
	if (g_ldp)
	{
		delete(g_ldp);
	}

	free_bmps();	// always do this no matter what

	restore_leds();  // sets keyboard leds back how they were (this is safe even if we have the led's disabled)

	SDL_Quit();
	exit(result_code);

}


// sets the serial port to be used to control LDP
void set_serial_port(unsigned char i)
{
	serial_port = i;
}


unsigned char get_serial_port()
{
	return(serial_port);
}

void set_baud_rate(int i)
{
	baud_rate = i;
}

int get_baud_rate()
{
	return(baud_rate);
}

void set_search_offset(int i)
{
	search_offset = i;
}

int get_search_offset()
{
	return(search_offset);
}

unsigned char get_frame_modifier()
{
	return(frame_modifier);
}

void set_frame_modifier(unsigned char value)
{
	frame_modifier = value;
}

void set_scoreboard(unsigned char value)
{
	realscoreboard = value;
}

unsigned char get_scoreboard()
{
	return (realscoreboard);
}

void set_scoreboard_port(unsigned int value)
{
	rsb_port = value;
}

unsigned int get_scoreboard_port()
{
	return(rsb_port);
}

void reset_logfile(int argc, char **argv)
{
	int i = 0;
	char s[160];
	string str;

	snprintf(s, sizeof(s), "--DAPHNE version %s", get_daphne_version());
	printline(s);
	str = "--Command line is: ";
	for (i = 0; i < argc; i++)
	{
		str = str + argv[i] + " ";
	}
	printline(str.c_str());
	snprintf(s, sizeof(s), "--CPU : %s %d MHz || Mem : %d megs", get_cpu_name(), get_cpu_mhz(), get_sys_mem());
	printline(s);
	snprintf(s, sizeof(s), "--OS : %s || Video : %s", get_os_description(), get_video_description());
	printline(s);
	outstr("--OpenGL: ");
#ifdef USE_OPENGL
	printline("Compiled In");
#else
	printline("Not Compiled In");
#endif // USE_OPENGL

	outstr("--RGB2YUV Function: ");
#ifdef USE_MMX
	printline("MMX");
#else
	printline("C");
#endif
	outstr("--Line Blending Function: ");
#ifdef USE_MMX
	printline("MMX");
#else
	printline("C");
#endif // blend MMX

	outstr("--Audio Mixing Function: ");
#ifdef USE_MMX
	printline("MMX");
#else
	printline("C");
#endif // blend MMX
}

// added by JFA for -idleexit
void set_idleexit(unsigned int value)
{
	idleexit = value;
}

unsigned int get_idleexit()
{
	return(idleexit);
}
// end edit

// added by JFA for -startsilent
void set_startsilent(unsigned char value)
{
	startsilent = value;
}

unsigned char get_startsilent()
{
	return(startsilent);
}
// end edit
