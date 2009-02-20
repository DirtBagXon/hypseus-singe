/*
 * cmdline.cpp
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

// Parses our command line and sets our variables accordingly
// by Matt Ownby

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cmdline.h"
#include "conout.h"
#include "network.h"
#include "numstr.h"
#include "homedir.h"
#include "input.h"	// to disable joystick use
#include "../io/numstr.h"
#include "../video/video.h"
#include "../video/led.h"
#include "../daphne.h"
#include "../cpu/cpu-debug.h"	// for set_cpu_trace
#include "../game/lair.h"
#include "../game/cliff.h"
#include "../game/game.h"
#include "../game/superd.h"
#include "../game/thayers.h"
#include "../game/speedtest.h"
#include "../game/seektest.h"
#include "../game/releasetest.h"
#include "../game/cputest.h"
#include "../game/multicputest.h"
#include "../game/firefox.h"
#include "../game/ffr.h"
#include "../game/astron.h"
#include "../game/esh.h"
#include "../game/laireuro.h"
#include "../game/badlands.h"
#include "../game/starrider.h"
#include "../game/bega.h"
#include "../game/cobraconv.h"
#include "../game/gpworld.h"
#include "../game/interstellar.h"
#include "../game/benchmark.h"
#include "../game/lair2.h"
#include "../game/mach3.h"
#include "../game/lgp.h"
#include "../game/timetrav.h"
#ifdef BUILD_SINGE
#include "../game/singe.h"
#endif // BUILD_SINGE
#include "../game/test_sb.h"
#include "../ldp-out/ldp.h"
#include "../ldp-out/sony.h"
#include "../ldp-out/pioneer.h"
#include "../ldp-out/ld-v6000.h"
#include "../ldp-out/hitachi.h"
#include "../ldp-out/philips.h"
#include "../ldp-out/ldp-combo.h"
#include "../ldp-out/ldp-vldp.h"
#include "../ldp-out/framemod.h"

#ifdef UNIX
#include <unistd.h>     // for unlink
#endif

int g_argc = 0;
char **g_argv = NULL;
int g_arg_index = 0;

// Win32 doesn't use strcasecmp, it uses stricmp (lame)
#ifdef WIN32
#define strcasecmp stricmp
#endif


// parses the command line looking for the -homedir switch, returns true if found and valid (or not found)
// (this must be done first because the the game and ldp classes may rely on the homedir being set to its proper place)
bool parse_homedir()
{
	bool result = true;
	char s[81] = { 0 };
	bool bHomeDirSet = false;	// whether set_homedir was called
	
	for (;;)
	{
		get_next_word(s, sizeof(s));
		// if there is nothing left for us to parse, break out of the while loop
		if (s[0] == 0)
		{
			break;
		}
	
		// if they are defining an alternate 'home' directory.
		// Primary used for OSX/linux to keep roms, framefiles and saverams in the user-writable space.
		else if (strcasecmp(s, "-homedir")==0)
		{
			//Ignore this one, already handled
			get_next_word(s, sizeof(s));
			if (s[0] == 0)
			{
				printline("Homedir switch used but no homedir specified!");
				result = false;
				break;
			}
			else
			{
				g_homedir.set_homedir(s);
				bHomeDirSet = true;
				printline("Setting alternate home dir:");
				printline(s);
				break;
			}
		}
	}			

	// if user didn't specify a homedir, then we have a default one.
	// (the reason this isn't in the homedir constructor is that we don't want to create
	//  default directories in a default location if the user specifies his/her own homedir)
	if (!bHomeDirSet)
	{

#ifdef UNIX
#ifndef MAC_OSX	// Zaph doesn't seem to like this behavior and he is maintaining the Mac build, so...
		// on unix, if the user doesn't specify a homedir, then we want to default to ~/.daphne
		//  to follow standard unix convention.
#ifndef GP2X	// gp2x has no homedir as far as I can tell, easier to put everything in current dir
		const char *homeenv = getenv("HOME");

		// this is always expected to be non-NULL
		if (homeenv != NULL)
		{
			string strHomeDir = homeenv;
			strHomeDir += "/.daphne";
			g_homedir.set_homedir(strHomeDir);
		}
		// else we couldn't set the homedir, so just leave it as default
#endif // not GP2X
#endif // not MAC_OSX
#else // WIN32
		g_homedir.set_homedir(".");	// for win32 set it to the current directory
		// "." is already the default,
		//  so the the main purpose for this is to ensure that the ram and
		//  framefile directories are created
		
#endif
	} // end if homedir was not set

	//Reset arg index and return
	g_arg_index = 1;
	return result;
}

// parses the game type from the command line and allocates g_game
// returns true if successful or false of failed
bool parse_game_type()
{
	bool result = true;

	// whether SGN (short game name) should match game name from command-line (it almost always should according to agreed-upon developer convention)
	// Exception is for a 'base' game name such as 'dle2' which doesn't specify 2.0, or 2.1 version
	bool bSGNMatches = true;
	char s[81] = { 0 };

	// first thing we need to get from the command line is the game type
	get_next_word(s, sizeof(s));

	net_set_gamename(s);	// report to server the game we are running

	if (strcasecmp(s, "ace") == 0)
	{
		g_game = new ace();
	}
	else if (strcasecmp(s, "ace_a2") == 0)
	{
		g_game = new ace();
		g_game->set_version(2);
	}
	else if (strcasecmp(s, "ace_a") == 0)
	{
		g_game = new ace();
		g_game->set_version(3);
	}
	else if (strcasecmp(s, "ace91")==0)
	{
		g_game = new ace91();
	}
	else if (strcasecmp(s, "ace91_euro")==0)
	{
		g_game = new ace91();
		g_game->set_version(1);
	}
	else if (strcasecmp(s, "aceeuro")==0)
	{
		g_game = new aceeuro();
	}
	else if (strcasecmp(s, "astron")==0)
	{
		g_game = new astronh();
	}
	else if (strcasecmp(s, "astronp")==0)
	{
		g_game = new astron();
	}
	else if (strcasecmp(s, "badlandp")==0)
	{
		g_game = new badlandp();
	}
	else if (strcasecmp(s, "badlands")==0)
	{
		g_game = new badlands();
	}
	else if (strcasecmp(s, "bega")==0)
	{
		g_game = new bega();
	}
	else if (strcasecmp(s, "begar1")==0)
	{
		g_game = new bega();
		g_game->set_version(2);
	}
	else if (strcasecmp(s, "benchmark")==0)
	{
		g_game = new benchmark();
	}
	else if (strcasecmp(s, "blazer")==0)
	{
		g_game = new blazer();
	}
	else if (strcasecmp(s, "cliff")==0)
	{
		g_game = new cliff();
	}
	else if (strcasecmp(s, "cliffalt")==0)
	{
		g_game = new cliffalt();
	}
	// Added by Buzz
	else if (strcasecmp(s, "cliffalt2")==0)
	{
		g_game = new cliffalt2();
	}
	else if (strcasecmp(s, "cobra")==0)
	{
		g_game = new cobra();
	}
	else if (strcasecmp(s, "cobraab")==0)
	{
		g_game = new cobraab();
	}
	else if (strcasecmp(s, "cobraconv")==0)
	{
		g_game = new cobraconv();
	}
	else if (strcasecmp(s, "cobram3")==0)
	{
		g_game = new cobram3();
	}
	else if (strcasecmp(s, "cputest")==0)
	{
		g_game = new cputest();
	}
	else if (strcasecmp(s, "dle11") == 0)
	{
		g_game = new dle11();
	}
	else if (strcasecmp(s, "dle2") == 0)
	{
		bSGNMatches = false;	// shortgame name will be dle20, dle21, etc
		g_game = new dle2();
	}
	// alias with version included
	else if (strcasecmp(s, "dle20") == 0)
	{
		g_game = new dle2();
		g_game->set_version(0);	// set to DLE2.0
	}
	// alias with version included
	else if (strcasecmp(s, "dle21") == 0)
	{
		g_game = new dle2();
		g_game->set_version(1);	// set to DLE2.0
	}
	else if (strcasecmp(s, "esh")==0)
	{
		g_game = new esh();
	}
	else if (strcasecmp(s, "eshalt")==0)
	{
		g_game = new esh();
		g_game->set_version(2);
	}
	else if (strcasecmp(s, "eshalt2")==0)
	{
		g_game = new esh();
		g_game->set_version(3);
	}
	else if (strcasecmp(s, "firefox")==0)
	{
		g_game = new firefox();
	}
	else if (strcasecmp(s, "firefoxa")==0)
	{
		g_game = new firefoxa();
	}
	else if (strcasecmp(s, "ffr")==0)
	{
		g_game = new ffr();
	}
	else if (strcasecmp(s, "galaxy")==0)
	{
		g_game = new galaxy();
	}
	else if (strcasecmp(s, "galaxyp")==0)
	{
		g_game = new galaxyp();
	}
	else if (strcasecmp(s, "gpworld")==0)
	{
		g_game = new gpworld();
	}
	else if (strcasecmp(s, "gtg") == 0)
	{
		g_game = new gtg();
	}
	else if (strcasecmp(s, "interstellar")==0)
	{
		g_game = new interstellar();
	}
	else if (strcasecmp(s, "lair")==0)
	{
		g_game = new lair();
	}
	else if (strcasecmp(s, "lair_f")==0)
	{
		g_game = new lair();
		g_game->set_version(2);
	}
	else if (strcasecmp(s, "lair_e")==0)
	{
		g_game = new lair();
		g_game->set_version(3);
	}
	else if (strcasecmp(s, "lair_d")==0)
	{
		g_game = new lairalt();
		g_game->set_version(4);

	}
	else if (strcasecmp(s, "lair_c")==0)
	{
		g_game = new lairalt();
		g_game->set_version(3);
	}
	else if (strcasecmp(s, "lair_b")==0)
	{
		g_game = new lairalt();
		g_game->set_version(2);
	}
	else if (strcasecmp(s, "lair_a")==0)
	{
		g_game = new lairalt();
		g_game->set_version(1);	// this is default, but what the heck
	}
	else if (strcasecmp(s, "lair_n1")==0)
	{
		g_game = new lairalt();
		g_game->set_version(5);
	}
	else if (strcasecmp(s, "lair_x")==0)
	{
		g_game = new lairalt();
		g_game->set_version(6);
	}
	else if (strcasecmp(s, "lairalt") == 0)
	{
		g_game = new lairalt();
	}
	else if (strcasecmp(s, "laireuro")==0)
	{
		g_game = new laireuro();
	}
	else if (strcasecmp(s, "lair_ita")==0)
	{
		g_game = new laireuro();
		g_game->set_version(2);
	}
	else if (strcasecmp(s, "lair_d2")==0)
	{
		g_game = new laireuro();
		g_game->set_version(3);
	}
	else if (strcasecmp(s, "lair2")==0)
	{
		g_game = new lair2();
	}
	else if (strcasecmp(s, "lair2_319_euro")==0)
	{
		g_game = new lair2();
		g_game->set_version(8);
	}
	else if (strcasecmp(s, "lair2_319_span")==0)
	{
		g_game = new lair2();
		g_game->set_version(9);
	}
	else if (strcasecmp(s, "lair2_318")==0)
	{
		g_game = new lair2();
		g_game->set_version(5);
	}
	else if (strcasecmp(s, "lair2_316_euro")==0)
	{
		g_game = new lair2();
		g_game->set_version(7);
	}
	else if (strcasecmp(s, "lair2_315")==0)
	{
		g_game = new lair2();
		g_game->set_version(0);
	}
	else if (strcasecmp(s, "lair2_314")==0)
	{
		g_game = new lair2();
		g_game->set_version(3);
	}
		else if (strcasecmp(s, "lair2_300")==0)
	{
		g_game = new lair2();
		g_game->set_version(2);
	}
	else if (strcasecmp(s, "lair2_211")==0)
	{
		g_game = new lair2();
		g_game->set_version(1);
	}
	else if (strcasecmp(s, "lgp")==0)
	{
		g_game = new lgp();
	}
	else if (strcasecmp(s, "mach3")==0)
	{
		g_game = new mach3();
	}
	else if (strcasecmp(s, "mcputest")==0)
	{
		g_game = new mcputest();
	}
	else if (strcasecmp(s, "releasetest")==0)
	{
		bSGNMatches = false;
		g_game = new releasetest();
	}
	else if (strcasecmp(s, "roadblaster")==0)
	{
		g_game = new roadblaster();
	}
	else if (strcasecmp(s, "sae") == 0)
	{
		g_game = new sae();
	}
	else if (strcasecmp(s, "seektest")==0)
	{
		g_game = new seektest();
	}
	// singe has a bunch of extra deps so we make it optional to build
#ifdef BUILD_SINGE
	else if (strcasecmp(s, "singe") == 0)
	{
		g_game = new singe();
	}
#endif // BUILD_SINGE
	else if (strcasecmp(s, "speedtest")==0)
	{
		g_game = new speedtest();
	}
	else if (strcasecmp(s, "sdq")==0)
	{
		g_game = new superd();
	}
	else if (strcasecmp(s, "sdqshort")==0)
	{
		g_game = new sdqshort();
	}
	else if (strcasecmp(s, "sdqshortalt")==0)
	{
		g_game = new sdqshortalt();
	}
	else if (strcasecmp(s, "starrider")==0)
	{
		g_game = new starrider();
	}
	else if (strcasecmp(s, "superdon")==0)  //left in for old times' sake
	{
		bSGNMatches = false;
		g_game = new superd();
	}
	else if (strcasecmp(s, "timetrav")==0)
	{
		g_game = new timetrav();
	}
	else if (strcasecmp(s, "test_sb")==0)
	{
		g_game = new test_sb();
	}
	else if (strcasecmp(s, "tq")==0)
	{
		g_game = new thayers();
	}
	else if (strcasecmp(s, "tq_alt")==0)
	{
		g_game = new thayers();
		g_game->set_version(2);
	}	
	else if (strcasecmp(s, "tq_swear")==0)
	{
		g_game = new thayers();
		g_game->set_version(3);
	}
	else if (strcasecmp(s, "uvt")==0)
	{
		g_game = new uvt();
	}
	else
	{
		outstr("ERROR: Unknown game type specified : ");
		printline(s);
		result = false;
	}

	// safety check
	if (!g_game)
	{
		result = false;
	}

	// else if the Short Game Name should match the game name passed in via command-line,
	//  then return an error if it doesn't, to force developers to follow this convention.
	else if (bSGNMatches)
	{
		if (strcasecmp(s, g_game->get_shortgamename()) != 0)
		{
			printline("Developer ERROR : short game name does not match command-line game name and it should!");
			string msg = "Cmdline Game name is: ";
			msg += s;
			msg += "; short game name is: ";
			msg += g_game->get_shortgamename();
			printline(msg.c_str());
			result = false;
		}
	}

	return(result);
}

// parses the LDP type from the command line and allocates g_ldp
// returns true if successful or false if failed
bool parse_ldp_type()
{
	bool result = true;
	char s[81] = { 0 };

	get_next_word(s, sizeof(s));

	net_set_ldpname(s);	// report to server which ldp we are using

	if (strcasecmp(s, "combo")==0)
	{
		g_ldp = new combo();
	}
	else if (strcasecmp(s, "fast_noldp")==0)
	{
		g_ldp = new fast_noldp();	// 'no seek-delay' version of noldp
	}
	else if (strcasecmp(s, "hitachi")==0)
	{
		g_ldp = new hitachi();
	}
	else if (strcasecmp(s, "noldp")==0)
	{
		g_ldp = new ldp();	// generic interface
	}
	else if (strcasecmp(s, "philips")==0)
	{
		g_ldp = new philips();
	}
	else if (strcasecmp(s, "pioneer") == 0)
	{
		g_ldp = new pioneer();
	}
	else if (strcasecmp(s, "sony")==0)
	{
		g_ldp = new sony();
	}
	else if (strcasecmp(s, "v6000")==0)
	{
		g_ldp = new v6000();
	}
	else if (strcasecmp(s, "vldp")==0)
	{
		g_ldp = new ldp_vldp();
	}
	else
	{
		printline("ERROR: Unknown laserdisc player type specified");
		result = false;
	}

	// safety check
	if (!g_ldp)
	{
		result = false;
	}

	return(result);
}

// parses command line
// Returns 1 on success or 0 on failure
bool parse_cmd_line(int argc, char **argv)
{
	bool result = true;
	char s[320] = { 0 };	// in case they pass in a huge directory as part of the framefile
	int i = 0;
	bool log_was_disabled = false;	// if we actually get "-nolog" while going through arguments

	//////////////////////////////////////////////////////////////////////////////////////

	g_argc = argc;
	g_argv = argv;
	g_arg_index = 1;	// skip name of executable from command line

	// if game and ldp types are correct
	if (parse_homedir() && parse_game_type() && parse_ldp_type())
	{		 
  	  // while we have stuff left in the command line to parse
	  for (;;)
	  {
		get_next_word(s, sizeof(s));

		// if there is nothing left for us to parse, break out of the while loop
		if (s[0] == 0)
		{
			break;
		}
		
		// if they are defining an alternate 'home' directory.
		// Primary used for OSX/linux to keep roms, framefiles and saverams in the user-writable space.
		else if (strcasecmp(s, "-homedir")==0)
		{
			//Ignore this one, already handled
			get_next_word(s, sizeof(s));
		}

		// If they are defining an alternate 'data' directory, where all other files aside from the executable live.
		// Primary used for linux to separate binary file (eg. daphne.bin) and other datafiles . 
		else if (strcasecmp(s, "-datadir")==0) 
		{ 
			get_next_word(s, sizeof(s)); 
			change_dir(s);
		}

		// if user wants laserdisc player to blank video while searching (VLDP only)
		else if (strcasecmp(s, "-blank_searches")==0)
		{
			g_ldp->set_search_blanking(true);
		}
		
		// if user wants to laserdisc player to blank video while skipping (VLDP only)
		else if (strcasecmp(s, "-blank_skips")==0)
		{
			g_ldp->set_skip_blanking(true);
		}

		// if they are pointing to a framefile to be used by VLDP
		else if (strcasecmp(s, "-framefile")==0)
		{
			ldp_vldp *cur_ldp = dynamic_cast<ldp_vldp *>(g_ldp);	// see if the currently selected LDP is VLDP
			get_next_word(s, sizeof(s));
			// if it is a vldp, then this option has meaning and we can use it
			if (cur_ldp)
			{
				cur_ldp->set_framefile(s);
			}
			else
			{
				printline("You can only set a framefile using VLDP as your laserdisc player!");
				result = false;
			}
		}
		
		// if they are defining an alternate soundtrack to be used by VLDP
		else if (strcasecmp(s, "-altaudio")==0)
		{
			ldp_vldp *cur_ldp = dynamic_cast<ldp_vldp *>(g_ldp);	// see if the currently selected LDP is VLDP
			get_next_word(s, sizeof(s));
			// if it is a vldp, then this option has meaning and we can use it
			if (cur_ldp)
			{
				cur_ldp->set_altaudio(s);
			}
			else
			{
				printline("You can only set an alternate soundtrack when using VLDP as your laserdisc player!");
				result = false;
			}
		}

		// The # of frames that we can seek per millisecond (to simulate seek delay)
		// Typical values for real laserdisc players are about 30.0 for 29.97fps discs
		//  and 20.0 for 23.976fps discs (dragon's lair and space ace)
		// FLOATING POINT VALUES ARE ALLOWED HERE
		// Minimum value is 12.0 (5 seconds for 60,000 frames), maximum value is
		//  600.0 (100 milliseconds for 60,000 frames).  If you want a value higher than
		//  the max, you should just use 0 (as fast as possible).
		// 0 = disabled (seek as fast as possible)
		else if (strcasecmp(s, "-seek_frames_per_ms")==0)
		{
			get_next_word(s, sizeof(s));
			double d = numstr::ToDouble(s);

			// WARNING : VLDP will timeout if it hasn't received a response after
			// a certain period of time (7.5 seconds at this moment) so you will
			// cause problems by reducing the minimum value much below 12.0
			if ((d > 12.0) && (d < 600.0)) g_ldp->set_seek_frames_per_ms(d);
			else printline("NOTE : Max seek delay disabled");
		}

		// the minimum amount of milliseconds to force a seek to take (artificial delay)
		// 0 = disabled
		else if (strcasecmp(s, "-min_seek_delay")==0)
		{
			get_next_word(s, sizeof(s));
			i = atoi(s);

			if ((i > 0) && (i < 5000)) g_ldp->set_min_seek_delay((unsigned int) i);
			else printline("NOTE : Min seek delay disabled");
		}

		// if the user wants the searching to be the old blocking style instead of non-blocking
		else if (strcasecmp(s, "-blocking")==0)
		{
			g_ldp->set_use_nonblocking_searching(false);
		}

		// to disable any existing joysticks that may be plugged in that may interfere with input
		else if (strcasecmp(s, "-nojoystick")==0)
		{
			set_use_joystick(false);
		}

		// if they are paranoid and don't want data sent to the server
		// (don't be paranoid, read the source code, nothing bad is going on)
		else if (strcasecmp(s, "-noserversend")==0)
		{
			net_no_server_send();
		}
		else if (strcasecmp(s, "-nosound")==0)
		{
			set_sound_enabled_status(false);
			printline("Disabling sound...");
		}
		else if (strcasecmp(s, "-sound_buffer")==0)
		{
			get_next_word(s, sizeof(s));
			Uint16 sbsize = (Uint16) atoi(s);
			set_soundbuf_size(sbsize);
			sprintf(s, "Setting sound buffer size to %d", sbsize);
			printline(s);
		}
		else if (strcasecmp(s, "-volume_vldp")==0)
		{
			get_next_word(s, sizeof(s));
			unsigned int uVolume = atoi(s);
			set_soundchip_vldp_volume(uVolume);
		}
		else if (strcasecmp(s, "-volume_nonvldp")==0)
		{
			get_next_word(s, sizeof(s));
			unsigned int uVolume = atoi(s);
			set_soundchip_nonvldp_volume(uVolume);
		}
		else if (strcasecmp(s, "-nocrc")==0)
		{
			g_game->disable_crc();
			printline("Disabling ROM CRC check...");
		}

		else if (strcasecmp(s, "-scoreboard")==0)
		{
			set_scoreboard(1);
			printline("Enabling external scoreboard...");
		}
		else if (strcasecmp(s, "-scoreport")==0)
		{
			get_next_word(s, sizeof(s));
			set_scoreboard_port((unsigned int)numstr::ToUint32(s, 16));
			sprintf(s, "Setting scoreboard port to %d", i);
			printline(s);
		}
		else if (strcasecmp(s, "-port")==0)
		{
			get_next_word(s, sizeof(s));
			i = atoi(s);
			set_serial_port((unsigned char) i);
			sprintf(s, "Setting serial port to %d", i);
			printline(s);
		}
		else if (strcasecmp(s, "-baud")==0)
		{
			get_next_word(s, sizeof(s));
			i = atoi(s);
			set_baud_rate(i);
			sprintf(s, "Setting baud rate to %d", i);
			printline(s);
		}

		// used to modify the dip switch settings of the game in question
		else if (strcasecmp(s, "-bank")==0)
		{
			get_next_word(s, sizeof(s));
			i = s[0] - '0';	// bank to modify.  We convert to a number this way to catch more errors
			
			get_next_word(s, sizeof(s));
			unsigned char value = (unsigned char) (strtol(s, NULL, 2));	// value to be set is in base 2 (binary)

			result = g_game->set_bank((unsigned char) i, (unsigned char) value);
		}

		else if (strcasecmp(s, "-latency")==0)
		{
			get_next_word(s, sizeof(s));
			i = atoi(s);

			// safety check
			if (i >= 0)
			{
				g_ldp->set_search_latency(i);
				sprintf(s, "Setting Search Latency to %d milliseconds", i);
				printline(s);
			}
			else
			{
				printline("Search Latency value cannot be negative!");
				result = false;
			}
		}
		else if (strcasecmp(s, "-cheat")==0)
		{
			g_game->enable_cheat();	// enable any cheat we have available :)
		}

		// enable keyboard LEDs for use with games such as Space Ace
		else if (strcasecmp(s, "-enable_leds")==0)
		{
			enable_leds(true);
		}
		// hacks roms for certain games such as DL, SA, and CH to bypass slow boot process
		else if (strcasecmp(s, "-fastboot")==0)
		{
			g_game->set_fastboot(true);
		}

		// stretch video vertically by x amount (a value of 24 removes letterboxing effect in Cliffhanger)
		else if (strcasecmp(s, "-vertical_stretch")==0)
		{
			ldp_vldp *the_ldp = dynamic_cast<ldp_vldp *>(g_ldp);

			get_next_word(s, sizeof(s));
			i = atoi(s);
			if (the_ldp != NULL)
			{
				the_ldp->set_vertical_stretch(i);
			}
			else
			{
				printline("Vertical stretch only works with VLDP.");
				result = false;
			}
		}

		// don't force 4:3 aspect ratio regardless of window size
		else if (strcasecmp(s, "-ignore_aspect_ratio")==0)
		{
			set_force_aspect_ratio(false);
		}

		// run daphne in fullscreen mode
		else if (strcasecmp(s, "-fullscreen")==0)
		{
			set_fullscreen(true);
		}

		// disable log file
		else if (strcasecmp(s, "-nolog")==0)
		{
			log_was_disabled = true;
		}

		// disable YUV hardware acceleration
		else if (strcasecmp(s, "-nohwaccel")==0)
		{
			set_yuv_hwaccel(false);
		}

#ifdef USE_OPENGL
		else if (strcasecmp(s, "-opengl")==0)
		{
			set_use_opengl(true);
		}
#endif // USE_OPENGL

		else if (strcasecmp(s, "-pal_dl")==0)
		{
			set_frame_modifier(MOD_PAL_DL);
			printline("Setting DAPHNE up for the PAL Dragon's Lair disc");
			cpu_change_irq(0, 0, LAIR_IRQ_PERIOD * (23.976/25.0)); // change IRQ 0 of CPU 0
			// DL PAL runs at a different speed so we need to overclock the IRQ slightly
		}
		else if (strcasecmp(s, "-pal_sa")==0)
		{
			set_frame_modifier(MOD_PAL_SA);
			printline("Setting DAPHNE up for the PAL Space Ace disc");
		}
		else if (strcasecmp(s, "-pal_dl_sc") == 0)
		{
			set_frame_modifier(MOD_PAL_DL_SC);
			printline("Setting DAPHNE up for the PAL Dragon's Lair Software Corner disc");
			cpu_change_irq(0, 0, LAIR_IRQ_PERIOD * (23.976/25.0));
			// DL Amiga runs at a different speed so we need to overclock the IRQ slightly
		}
		else if (strcasecmp(s, "-pal_sa_sc") == 0)
		{
			set_frame_modifier(MOD_PAL_SA_SC);
			printline("Setting DAPHNE up for the PAL Space Ace Software Corner disc");
		}
		else if (strcasecmp(s, "-spaceace91")==0)
		{
			set_frame_modifier(MOD_SA91);
			printline("Setting DAPHNE to play a Space Ace '91 disc");
		}
		else if (strcasecmp(s, "-preset")==0)
		{
			get_next_word(s, sizeof(s));
			i = atoi(s);
			g_game->set_preset(i);
		}
		else if (strcasecmp(s, "-rotate")==0)
		{
			get_next_word(s, sizeof(s));
			float f = (float) numstr::ToDouble(s);
			set_rotate_degrees(f);
		}

		// continuously updates SRAM (after every seek)  
		// ensures sram is saved even if Daphne is terminated improperly
		else if (strcasecmp(s, "-sram_continuous_update")==0)
		{
			g_ldp->set_sram_continuous_update(true);
		}

		else if (strcasecmp(s, "-version")==0)
		{
			get_next_word(s, sizeof(s));
			i = atoi(s);
			g_game->set_version(i);
		}
		else if (strcasecmp(s, "-x")==0)
		{
			get_next_word(s, sizeof(s));
			i = atoi(s);
			set_video_width((Uint16)i);
			sprintf(s, "Setting screen width to %d", i);
			printline(s);
		}
		else if (strcasecmp(s, "-y")==0)
		{
			get_next_word(s, sizeof(s));
			i = atoi(s);
			set_video_height((Uint16)i);
			sprintf(s, "Setting screen height to %d", i);
			printline(s);
		}
		else if (strcasecmp(s, "-trace")==0)
		{
#ifdef CPU_DEBUG
			printline("CPU tracing enabled");
			set_cpu_trace(1);
#else
			printline("DAPHNE needs to be compiled in debug mode for this to work");
			result = false;
#endif
		}
		// added by JFA for -idleexit
		else if (strcasecmp(s, "-idleexit")==0)
		{
			get_next_word(s, sizeof(s));
			i = atoi(s);
			set_idleexit(i*1000);
			sprintf(s, "Setting idleexit to %d ", i);
			printline(s);
		}
		// end edit

		// added by JFA for -startsilent
		else if (strcasecmp(s, "-startsilent")==0)
		{
			set_startsilent(1);
			printline("Starting silent...");
		}
		// end edit

		// if they are requesting to stop the laserdisc when the program exits
		else if (strcasecmp(s, "-stoponquit")==0)
		{
			g_ldp->set_stop_on_quit(true);
		}
        // this switch only supported by the ldp-vldp player class.
        else if (strcasecmp(s, "-useoverlaysb")==0)
        {
			get_next_word(s, sizeof(s));
			i = atoi(s);

			// make sure that if we read 0 as the argument, that it is really zero.. :)
			if (((i == 0) && (s[0] == '0')) || (i != 0))
			{
				set_sboverlay_characterset(i);

				lair *game_lair_or_sa = dynamic_cast<lair *>(g_game);
				thayers *game_thayers = dynamic_cast<thayers *>(g_game);

				// print a warning instead of an error to make daphne more friendly to non-daphneloader frontends
				if (NULL == game_lair_or_sa && NULL == game_thayers)
				{
					printline("WARNING: -useoverlaysb is not supported for this game and will be ignored");
				}
				else
				{
					if (game_lair_or_sa)
						game_lair_or_sa->init_overlay_scoreboard();
					else
						// Must be Thayer's Quest
						game_thayers->init_overlay_scoreboard();
				}
			}
			else
			{
				outstr("-useoverlaysb requires an argument such as 0 or 1 after it. Instead, found: ");
				printline(s);
				result = false;
			}
        }

        // Playing Thayer's Quest, and don't want speech synthesis?
        else if (strcasecmp(s, "-nospeech")==0)
        {
            thayers *game_thayers = dynamic_cast<thayers *>(g_game);

            if (game_thayers)
                game_thayers->no_speech();
            else
            {
                printline("-nospeech: Switch not supported for this game...");
                result = false;
            }
        }
        else if (strcasecmp(s, "-prefer_samples")==0)
        {
			// If a game doesn't support "prefer samples" then it is not an error.
			// That's why it is _prefer_ samples instead of _require_ samples :)
			g_game->set_prefer_samples(true);
        }

		else if  (strcasecmp(s, "-noissues")==0)
		{
			g_game->set_issues(NULL);
		}

		// Scale the game overlay graphics to the virtual screen dimension
        // this is needed when Daphne is used for overlaying game graphics over the real 
        // laserdisc movie (using a video genlock), and the screen dimension is different
        // from the dimensions of the game.
		else if (strcasecmp(s, "-fullscale")==0)
		{
			ldp_vldp *cur_ldp = dynamic_cast<ldp_vldp *>(g_ldp);	// see if the currently selected LDP is VLDP
			// if it is a vldp, then this option is not supported
			if (cur_ldp)
			{
				printline("Full Scale mode only works with NOLDP.");
				result = false;
			}
			else
			{
				g_game->SetFullScale(true);
			}
		}

		// check for any game-specific arguments ...
		else if (g_game->handle_cmdline_arg(s))
		{
			// don't do anything in here, it has already been handled by the game class ...
		}
		// check for any ldp-specific arguments ...
		else if (g_ldp->handle_cmdline_arg(s))
		{
			// don't do anything in here, it has already been handled by the ldp class ...
		}
		else
		{
			printline("Unknown command line parameter:");
			printline(s);
			result = false;
		}
	  } // end for
	} // end if we know our game type
	
	// if game or ldp was unknown
	else
	{
		result = false;
	}

	// if we didn't receive "-nolog" while parsing, then it's ok to enable the log file now.
	if (!log_was_disabled)
	{
		// first we need to delete the old logfile so we can start fresh
		// (this is the best place to do it since up until this point we don't know where our homedir is)
		string logfile = g_homedir.get_homedir();
		logfile += "/";
		logfile += LOGNAME;
		unlink(logfile.c_str());	// delete logfile if possible, because we're starting over
	
		set_log_enabled(true);
	}

	return(result);
}


// copies a single word from the command line into result (a word is any group of characters separate on both sides by whitespace)
// if there are no more words to be copied, result will contain a null string
void get_next_word(char *result, int result_size)
{
	// make sure we still have command line left to parse
	if (g_arg_index < g_argc)
	{
		strncpy(result, g_argv[g_arg_index], result_size);
		result[result_size-1] = 0;	// terminate end of string just in case we hit the limit
		g_arg_index++;
	}

	// if we have no command line left to parse ...
	else
	{
		result[0] = 0;	// return a null string
	}
}
