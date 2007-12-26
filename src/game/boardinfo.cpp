/*
* boardinfo.cpp
*
* Copyright (C) 2001-2006 Matt Ownby
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


// gives information about the board we are on (for debugging purposes only)

// written by Matt Ownby

#include <string.h>
#include <stdio.h>
#include "game.h"
#include "boardinfo.h"
#include "../io/conout.h"

const struct boardinfo lairstuff[] =
{
	{ "Introduction" },
	{ "Tentacles from the Ceiling" },
	{ "Snake Room" },
	{ "Flaming Ropes" },
	{ "Pool of Water" },
	{ "Bubbling Cauldron" },
	{ "Giddy Goons" },
	{ "Flattening Staircase" },
	{ "Smithee" },
	{ "Grim Reaper" },
	{ "Wind Tunnel" },
	{ "Closing Wall" },
	{ "Room of Fire - Bench Covers Exit" },
	{ "Flying Horse Barding" },
	{ "Robot Knight on Checkered Floor" },
	{ "Crypt Creeps" },
	{ "Catwalk Bats" },
	{ "Flaming Ropes (Mirrored)" },
	{ "Pool of Water (Mirrored)" },
	{ "Giant Bat" },
	{ "Falling Platform - 9 Levels" },
	{ "Smithee (Mirrored)" },
	{ "Flying Horse Barding (Mirrored)" },
	{ "Lizard King" },
	{ "Drink Me" },
	{ "Crypt Creeps (Mirrored)" },
	{ "Grim Reaper (Mirrored)" },
	{ "Tilting Floor" },
	{ "Throne Room" },
	{ "Robot Knight on Checkered Floor (Mirrored)" },
	{ "Falling Platform - 9 Levels (Mirrored)" },
	{ "Underground River" },
	{ "Mudmen" },
	{ "Black Knight on Horse" },
	{ "Rolling Balls" },
	{ "Electric Cage - Geyser" },
	{ "The Dragon's Lair" },
	{ "Attract Mode" },
	{ "Falling Platform - 3 Levels" },
	{ "Small Yellow Room" }
};


// Boardinfo that is different for DLE v2 ROMs
// (this list may not be totally accurate)
const struct boardinfo dle2stuff[] =
{
	{ "Introduction" },
	{ "Tentacles from the Ceiling" },
	{ "Snake Room" },
	{ "Flaming Ropes" },
	{ "Flaming Ropes (Mirrored)" },
	{ "Smithee" },
	{ "Smithee (Mirrored)" },
	{ "Flattening Staircase" },
	{ "Small Yellow Room / Bubbling Cauldron" },
	{ "Grim Reaper" },
	{ "Grim Reaper (Mirrored)" },
	{ "Closing Wall" },
	{ "Room of Fire - Bench Covers Exit" },
	{ "Flying Horse Barding" },
	{ "Flying Horse Barding (Mirrored)" },
	{ "Drink Me" },
	{ "Catwalk Bats" },
	{ "Pool of Water" },
	{ "Pool of Water (Mirrored)" },
	{ "Giant Bat" },
	{ "Falling Platform - 9 Levels" },
	{ "Falling Platform - 9 Levels (Mirrored)" },
	{ "Throne Roome" },
	{ "Lizard King" },
	{ "Crypt Creeps" },
	{ "Crypt Creeps (Mirrored)" },
	{ "Wind Tunnel" },
	{ "Tilting Floor" },
	{ "Robot Knight on Checkered Floor" },
	{ "Robot Knight on Checkered Floor (Mirrored)" },
	{ "Giddy Goons" },
	{ "Underground River" },
	{ "Mudmen" },
	{ "Black Knight on Horse" },
	{ "Rolling Balls" },
	{ "Electric Cage - Geyser" },
	{ "The Dragon's Lair" },
	{ "Attract Mode" },
	{ "Falling Platform - 3 Levels" },
	{ "Small Yellow Room?" }
};


const struct boardinfo acestuff[] =
{
	{ "Introduction" },
	{ "There's Borf's Ship!" },
	{ "I'll Save You, Kimmy!" },
	{ "I'll Save You, Kimmy! (Mirrored)" },
	{ "Ah ha, Borf is here!" },
	{ "Ah ha, Borf is here! (Mirrored)" },
	{ "Searchlight & Moving Platforms" },
	{ "Searchlight & Moving Platforms (Mirrored)" },
	{ "Green Dogs" },
	{ "Green Dogs (Mirrored)" },
	{ "Junk Planet" },
	{ "Junk Planet (Mirrored)" },
	{ "Space Dogfight" },
	{ "Space Dogfight (Mirrored)" },
	{ "Checkered Tunnel" },
	{ "Checkered Tunnel (Mirrored)" },
	{ "Blue Cats & Shag" },
	{ "Blue Cats & Shag (Mirrored)" },
	{ "Beware Your Dark Side!" },
	{ "Beware Your Dark Side (Mirrored) ??" },
	{ "Cycle Chase" },
	{ "Cycle Chase (Mirrored)" },
	{ "Roller Skates & Black Eel" },
	{ "Roller Skates & Black Eel (Mirrored)" },
	{ "Final Showdown" },
	{ "Which Way?" },
	{ "Unknown 1A" },
	{ "I'll Save You, Kimmy! (Partial)" },
	{ "I'll Save You, Kimmy! (Partial, Mirrored)" },
	{ "Moving Platforms (Partial)" },
	{ "Unknown 1E" },
	{ "Unknown 1F" },
	{ "Black Eel (No Rollerskates) (Partial)" },
	{ "Unknown 21" }
};





// prints out the name of a board, its sequence, and any extra attributes



void print_board_info(unsigned char index,	// which board
					  unsigned char sequence,	// which sequence of that board
					  unsigned char type)	// which type of sequence
					  // (type is the 1st byte of the trailer segment, study my documentation on what the trailer segment is if you don't know)
{

	char sequence_type[160] = { 0 };
	char s1[160] = { 0 };
	char board_name[160] = { 0 };

	// if we know that we are playing Dragon's Lair
	if ((g_game->get_game_type() == GAME_LAIR) | (g_game->get_game_type() == GAME_DLE1)
		| (g_game->get_game_type() == GAME_DLE2))
	{
		index = (unsigned char) (index & 0x7F);

		if (type & 0x40)
		{
			strcat(sequence_type, "Attract Mode");
		}
		if (type & 0x20)
		{
			strcat(sequence_type, "Bones Scene ");
		}

		if (type & 0x10)
		{
			strcat(sequence_type, "Death Scene ");
		}

		if (sequence == 1)
		{
			strcat(sequence_type, "Resurrection Scene ");
		}

		// If our current index is for hard difficulty,
		// we need to compensate to get the proper board name
		if (index >= 0x2A)
		{
			index -= 0x2A;
		}

		if (g_game->get_game_type() != GAME_DLE2)
		{
			// make sure we are not out of bounds
			if (index < (sizeof(lairstuff) / sizeof(boardinfo)))
			{
				strcpy(board_name, lairstuff[index].name);
			}
			else
			{
				strcpy(board_name, "OUT OF BOUNDS");
			}
		}
		// else if this is DLE v2
		else
		{
			// make sure we are not out of bounds
			if (index < (sizeof(dle2stuff) / sizeof(boardinfo)))
			{
				strcpy(board_name, dle2stuff[index].name);
			}
			else
			{
				strcpy(board_name, "OUT OF BOUNDS");
			}
		}

		sprintf(s1, "[%2x] %s, Sequence %d %s", index, board_name, sequence, sequence_type);
		printline(s1);
	}


	// if we're playing Space Ace ...
	else if (g_game->get_game_type() == GAME_ACE)
	{
		index = (unsigned char) (index & 0x7F);

		if (type & 0x40)
		{
			strcat(sequence_type, "Attract Mode");
		}

		if (type & 0x20)
		{
			strcat(sequence_type, "Borf Taunt");
		}

		if (type & 0x10)
		{
			strcat(sequence_type, "Death Scene");
		}

		// check to make sure we are within our bounds
		if (index < (sizeof(acestuff) / sizeof(boardinfo)))
		{
			strcpy(board_name, acestuff[index].name);
		}
		else
		{
			strcpy(board_name, "OUT OF BOUNDS");
		}
		sprintf(s1, "[%2x] %s, Sequence %d %s", index, board_name, sequence, sequence_type);
		printline(s1);

	}

	// else if we don't know which game we're playing...
	else
	{
		sprintf(s1, "[%2x] Unknown Name, Sequence %d Type %x", index, sequence, type);
		printline(s1);
	}
}
