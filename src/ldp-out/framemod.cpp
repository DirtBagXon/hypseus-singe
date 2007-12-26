/*
 * framemod.cpp
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


// framemod.cpp
// by Matt Ownby

// contains functions to convert the regular DL/SA NTSC frames to other formats such as PAL

#include "framemod.h"
#include "../daphne.h"	// for get_frame_modifer
#include "../io/conout.h"

// returns 0 if no frame conversion is needed, else it returns a 1
bool need_frame_conversion()
{

	// if we are using a search offset, a SA91 disc, or PAL,
	// return non-zero
	return( (get_search_offset() != 0) || (get_frame_modifier() != MOD_NONE));

}

// returns the Frames Per Kilosecond of the target disc that we are converting frames to
unsigned int get_frame_conversion_fpks()
{
	unsigned int uResult = 0;

	switch (get_frame_modifier())
	{
	case MOD_SA91:
		uResult = 29970;
		break;
	case MOD_PAL_DL:
	case MOD_PAL_SA:
	case MOD_PAL_DL_SC:
	case MOD_PAL_SA_SC:
		uResult = 25000;
		break;
	default:
		printline("Error in get_frame_conversion_fpks, no frame modifier is enabled");
		break;
	};
	
	return uResult;
}

// converts a standard DL/SA '83 NTSC frame to PAL, Space Ace'91, etc ...
// the resulting frame is returned
Uint16 do_frame_conversion(int source_frame)
{

	double result_frame = (double) source_frame;	// it needs to be a float for precise math
	int search_offset = get_search_offset();

	result_frame = result_frame + search_offset;	// apply any existing search offsets

	// if we are using space ace 91 we need to convert from 24 FPS to 30 FPS and add a time constant
	if (get_frame_modifier() == MOD_SA91)
	{
		// if the frame in question is one of the slides at the beginning,
		// we need to do a more thought-out conversion
		if (result_frame <= 145)
		{
			// frames 40 through 75
			if ((result_frame >= 40) && (result_frame <= 75))
			{
				result_frame = 151;
			}
			// frames 112 through 139
			else if ((result_frame >= 112) && (result_frame <= 139))
			{
				result_frame = 151;
			}
			else
			{
				switch ( (int) result_frame)
				{
				case 37:
				case 38:
				case 39:
					result_frame = 145;
					break;
				case 76:
				case 77:
				case 78:
					result_frame = 169;
					break;
				case 79:
				case 80:
				case 81:
					result_frame = 175;
					break;
				case 82:
				case 83:
				case 84:
				case 85:
				case 86:
				case 87:
					result_frame = 217;
					break;
				case 88:
				case 89:
				case 90:
					result_frame = 121;
					break;
				case 91:
				case 92:
				case 93:
				case 94:
				case 95:
				case 96:
				case 100:
				case 101:
				case 102:
					result_frame = 151;
					break;
				case 97:
				case 98:
				case 99:
				case 142:
				case 143:
				case 144:
				case 145:
					result_frame = 145;
					break;

					// if there is no good match, just use space ace logo
				default:
					result_frame = 1;
				} // end switch
			}
		}

		// if frame to be modified is greater than 145
		else
		{
			result_frame = result_frame / 23.976; // convert frames to seconds
			result_frame += 7.80807717679; // add time difference between SA '83 and SA '91
			result_frame *= 29.97; // convert seconds to SA91 frames
			result_frame += 0.5;	// add  0.5 so when truncating, the average becomes the resulting frame
		}
	} // end if Space Ace 91

	// regular Dragon's Lair PAL conversion
	else if (get_frame_modifier() == MOD_PAL_DL)
	{
		result_frame = result_frame - 152;

		// catch potential error
		if (result_frame < 1)
		{
			result_frame = 1;
			printline("NOTE: NTSC frame requested is not available on PAL DL disc");
		}
	}

	// Space Ace PAL conversion
	else if (get_frame_modifier() == MOD_PAL_SA)
	{
			result_frame = result_frame * (25.0 / 23.976); // convert NTSC frames to PAL frames
			// NTSC and PAL discs have a 1:1 time correspondance
			result_frame += 0.5;	// add 0.5 so when truncating, the average becomes the resulting frame
	}

	// Dragon's Lair PAL for the Software Corner conversion
	else if (get_frame_modifier() == MOD_PAL_DL_SC)
	{
			result_frame = result_frame - 230;

			// catch potential error
			if (result_frame < 1)
			{
				result_frame = 1;
				printline("NOTE: NTSC frame requested is not available on DL Software Corner disc");
			}
	}

	// Space Ace PAL Software Corner conversion
	else if (get_frame_modifier() == MOD_PAL_SA_SC)
	{
			result_frame = result_frame * (25.0 / 23.976); // convert NTSC frames to PAL frames
			// NTSC and PAL '83 Space Ace discs have a 1:1 time correspondance
			result_frame += 79.5;
			// Software Corner disc has 79 extra frames than PAL Space Ace
			// add 0.5 to round to the nearest whole number when truncating
	}

	// bug catcher
	else
	{
			printline("Bug in framemod.cpp, unknown frame modifier!");
	}

	return( (Uint16) result_frame);

}
