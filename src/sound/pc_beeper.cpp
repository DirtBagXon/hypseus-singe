/*
* pc_beeper.cpp
*
* Copyright (C) 2005 Matt Ownby
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

#include "sound.h"	// for get frequency stuff
#include <string.h>	// for memset

#ifdef DEBUG
#include "../io/conout.h"
#include <assert.h>
#endif

// how many beepers have been created
unsigned int g_uBeeperCount = 0;

// if the beeper is generating sound (treat this as a bool, it's an int for performance reasons)
unsigned int g_uBeeperEnabled = 0;

// value provided by game driver to be used to calculate frequency
unsigned int g_uBeeperFreqDiv = 0;

// frequency beeper is supposed to be generating sound at
unsigned int g_uBeeperFreq = 0;

// whether we're waiting for the low byte or the high byte of the frequency-related value
bool g_bWaitFreqLow = false;

// max sample value (gets negated repeatedly)
// Using the absolute max (32767) seemed to loud, so I halved it.  Feel free to play with this.
Sint16 g_s16SampleVal = 16384;

// how many samples have been converted (used to maintain consistency)
unsigned int g_uSampleCount = 0;

// how many samples for half a cycle (global to avoid needless extra calculations)
unsigned int g_uSamplesPerHalfCycle = 0;

// init callback
int beeper_init(Uint32 unused)
{

#ifdef DEBUG
	// a couple of assumptions...
	assert(AUDIO_CHANNELS == 2);
	assert(AUDIO_BYTES_PER_SAMPLE == 4);
#endif

	// we only support 1 beeper for the time being
	if (g_uBeeperCount == 0)
	{
		g_uBeeperCount++;
		return 0;
	}

#ifdef DEBUG
	assert(false);	// force dev to deal with this problem
#endif
	return -1;	// we only support 1 beeper for the time being
}

// should be called from the game driver to control beeper
void beeper_ctrl_data(unsigned int uPort, unsigned int uByte, int internal_id)
{
	switch (uPort)
	{
	case 0x42:	// frequency
		if (g_bWaitFreqLow)
		{
			g_uBeeperFreqDiv = uByte;
			g_bWaitFreqLow = false;
		}
		else
		{
			g_uBeeperFreqDiv |= (uByte << 8);

			// I've seen 3 different variations of this 'magic number' and I do not know how
			//  to calculate the number, so I am using the variation that the DL2 source code uses.
			g_uBeeperFreq = 1193189 / g_uBeeperFreqDiv;

			// Compute how many samples for half a cycle
			// >> 1 because it's half a cycle (divide by 2)
			g_uSamplesPerHalfCycle = (AUDIO_FREQ / g_uBeeperFreq) >> 1;
#ifdef DEBUG
			char s[320];
			sprintf(s, "PC BEEPER : freq %u requested", g_uBeeperFreq);
			printline(s);
#endif
		}
		break;
	case 0x43:	// audio control prep
		if (uByte == 0xB6)
		{
			g_bWaitFreqLow = true;
		}
		break;
	case 0x61:	// speaker on/off
		g_uBeeperEnabled = (uByte & 3);
		break;
	}
}

// called from sound mixer to get audio stream
void beeper_get_stream(Uint8 *stream, int length, int internal_id)
{
#ifdef DEBUG
	// make sure this is in the proper format (stereo 16-bit)
	assert((length % AUDIO_BYTES_PER_SAMPLE) == 0);
#endif

	if (g_uBeeperEnabled)
	{
		// generate square wave at the proper frequency
		for (int byte_pos = 0; byte_pos < length; byte_pos += AUDIO_BYTES_PER_SAMPLE)
		{
			// endian-independent! :)
			// NOTE : assumes stream is in little endian format
			stream[byte_pos] = stream[byte_pos+2] = ((Uint16) g_s16SampleVal) & 0xFF;
			stream[byte_pos+1] = stream[byte_pos+3] = (((Uint16) g_s16SampleVal) >> 8) & 0xFF;

			++g_uSampleCount;	// we've just done 1 sample (4 bytes/sample)

			// if we're halfway through current cycle, or if we've finished the cycle
			//  then it's time to negate the sign of the sample.
			if (g_uSampleCount > g_uSamplesPerHalfCycle)
			{
				g_uSampleCount -= g_uSamplesPerHalfCycle;
				g_s16SampleVal = -g_s16SampleVal;	// negate!
			}
		}
	}

	// return silence
	else
	{
		memset(stream, 0, length);
	}
}
