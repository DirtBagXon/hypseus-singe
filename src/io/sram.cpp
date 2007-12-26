/*
 * sram.cpp
 *
 * Copyright (C) 2002 Warren Ondras
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

// sram.cpp -- stores/loads saved static RAM to/from disk
// by Warren Ondras

#include <stdio.h>
#include <zlib.h>	// for compression
#include "homedir.h"
#include "numstr.h"
#include "conout.h"

int sram_load(const char * filename, unsigned char * mem, unsigned int size)
{
	string s;
	gzFile loadfile = NULL;
	int result = 0;
	
	//Use homedir to locate the correct place for saveram
	string sramFile = g_homedir.get_ramfile(filename);

	loadfile = gzopen(sramFile.c_str(), "rb");	// open compressed file

	// if loading the file succeeded
	if (loadfile)
	{
		// read compressed data
		if (gzread(loadfile, (voidp) mem, size) == (int) size)
		{
			s = "Loaded " + numstr::ToStr(size) + " bytes from " + sramFile;
			printline(s.c_str());
			result = 1;
		}
		else
		{
			s = "Error loading from " + sramFile;
			printline(s.c_str());
		}
		gzclose(loadfile);
	}

	// if loading the file failed
	else
	{
		s = "NOTE : RAM file " + sramFile + " was not found (it'll be created)";
		printline(s.c_str());
	}
	
	return result;
}

int sram_save(const char * filename, unsigned char * mem, unsigned int size)
{
	char s[81];
	gzFile savefile;
	int result = 0;

	//Use homedir to locate the correct place for saveram
	string sramFile = g_homedir.get_ramfile(filename);
	
	savefile = gzopen(sramFile.c_str(), "wb");

	// if opening file was successful	
	if (savefile)
	{
		// set it to compress at maximum because it shouldn't take too long and
		// it'll save disk space
		gzsetparams(savefile, Z_BEST_COMPRESSION, Z_DEFAULT_STRATEGY);

		if (gzwrite(savefile, (voidp) mem, size) == (int) size)
		{
			sprintf(s, "Saved %d bytes to %s", size, filename);
			printline(s);
			result = 1;
		}
		else
		{
			sprintf(s, "Error saving %d bytes to %s", size, filename);
			printline(s);
		}
		gzclose(savefile);
	}

	else
	{
		sprintf(s, "Error saving RAM to file ram/%s", filename);
		printline(s);
	}
	
	return result;
}
