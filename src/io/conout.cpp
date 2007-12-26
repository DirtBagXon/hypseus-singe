/*
 * conout.cpp
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

// CONOUT
// Provides console functionality in a window (replacement for printf, cout, etc)

#include <string>
#include <list>
#include <stdio.h>
#include <string.h>
#include <SDL.h>
#include "conout.h"
#include "../daphne.h"
#include "../video/video.h"
#include "../video/SDL_Console.h"
#include "../timer/timer.h"
#include "input.h"
#include "mpo_fileio.h"
#include "homedir.h"

using namespace std;

char G_curline[CHARS_PER_LINE] = { 0 };	// current line of output

// false = write log entries to g_lsPendingLog, true = write to daphne_log.txt
// This should be false until the command line has finished parsing.
bool g_log_enabled = false;

// used so log can store text until we've been given the green light to write to the log file
// (using a list because it is faster than a vector)
list <string> g_lsPendingLog;

// Prints a "c string" to the screen
// Returns 1 on success, 0 on failure
void outstr(const char *s)
{
// the console consumes quite a bit of cpu, so we don't want to have
//  it enabled by default
#ifdef CPU_DEBUG
	if (get_console_initialized())
	{
		if (strlen(s) + strlen(G_curline) < CHARS_PER_LINE)
		{
			strcat(G_curline,s);
		}
	}
#endif
#ifdef UNIX
	fputs(s, stdout);
#endif

	addlog(s); // add to our logfile
}

// Prints a single character to the screen
// Returns 1 on success, 0 on failure
void outchr(const char ch)
{
	char s[2] = { 0 };

	// the SDL_console output library uses vsprintf so % signs mess it up
	if (ch != '%')
	{
		s[0] = ch;
		s[1] = 0;
	}

	outstr(s);
}

// prints a null-terminated string to the screen
// and adds a line feed
// 1 = success
void printline(const char *s)
{
	outstr(s);
	newline();
}

// moves to the next line without printing anything
void newline()
{

	// Carriage return followed by line feed
	// So we can read the blasted thing in notepad
	static char newline_str[3] = { 13, 10, 0 };

#ifdef CPU_DEBUG
	if (get_console_initialized())
	{
		ConOut(G_curline);
		G_curline[0] = 0;	// erase line
	}
#endif
#ifdef UNIX
	printf("\n");
#endif

	addlog(newline_str);

}

#ifdef CPU_DEBUG
// prints the current line even if we haven't hit a linefeed yet
void con_flush()
{
	ConOutstr(G_curline);
}
#endif


// flood-safe printline
// it only prints every second and anything else is thrown out
void noflood_printline(char *s)
{
	static unsigned int old_timer = 0;
	
	if (elapsed_ms_time(old_timer) > 1000)
	{
		printline(s);
		old_timer = refresh_ms_time();
	}
}

// This is a safe version of ITOA in that prevents buffer overflow, but if your buffer size is too small,
// you will get an incorrect result.
void safe_itoa(int num, char *a, int sizeof_a)
{
	int i = 0,j = 0, n = 0;
	char c = 0;

	// safety check : size needs to be at least 3 to accomodate one number, a minus sign and a null character
	if (sizeof_a > 2)
	{
		n = num;
		
		// if number is negative, reverse its sign to make the math consistent with positive numbers
		if (num < 0)
		{
			n = -n;
		}
		
		do
		{
			a[i++]=n % 10 + '0';
			n = n / 10;
		} while ((n > 0) && (i < (sizeof_a - 2)));
		// sizeof_a - 2 to leave room for a null terminating character and a possible minus sign
		
		// if number was negative, then add a minus sign
		if (num < 0) a[i++]='-';
		
		a[i] = 0;	// terminate string
		
		// reverse string (we had to go in reverse order to do the calculation)
		for (i=0,j=strlen(a)-1; i<j; i++,j--)
		{
			c=a[i];
			a[i]=a[j];
			a[j]=c;
		}
	}	
}

void set_log_enabled(bool val)
{
	g_log_enabled = val;
}

// adds a string to a log file (creates the logfile if it does not exist)
void addlog(const char *s)
{
	// if logging is enabled
	if (g_log_enabled)
	{
		mpo_io *io = NULL;
		string logname = g_homedir.get_homedir();
		logname += "/";
		logname += LOGNAME;

		io = mpo_open(logname.c_str(), MPO_OPEN_APPEND);
		if (io)
		{
			// if our list has stuff in it waiting to be written out to disk, then do so now
			if (!g_lsPendingLog.empty())
			{
				// empty the log of its contents
				for (list<string>::iterator i = g_lsPendingLog.begin();
					i != g_lsPendingLog.end(); i++)
				{
					mpo_write((*i).c_str(), (*i).size(), NULL, io);
				}
				g_lsPendingLog.clear();
			}
			mpo_write(s, strlen(s), NULL, io);
			mpo_close(io);
		}
		// else directory is read-only so we will just ignore for now
		else
		{
#ifdef UNIX
				printf("Cannot write to '%s', do you have write permissions?\n", logname.c_str());
#endif
		}
	}
	// else logging is disabled, so we have to store log entries to memory
	else
	{
		g_lsPendingLog.push_back(s);	// store to RAM for now ...
	}
}
