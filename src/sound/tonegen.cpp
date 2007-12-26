/*
* tonegen.cpp
*
* Copyright (C) 2005 Mark Broadhead
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

// by Mark Broadhead
// four channel square wave generator
//

#include "sound.h"
#include "tonegen.h"
#include "../io/conout.h"
#include <memory.h>
tonegen g_tonegen;
bool g_tonegen_init = false;

int tonegen_initialize(Uint32 unused)
{
	int result = -1;
	if (!g_tonegen_init)
	{
		result = 0;
	}
	else 
	{
		printline("TONEGEN: Error! You can only initialize one 'chip'!");
	}
	int channel;
	for (channel = 0; channel < VOICES; channel++)
	{
      g_tonegen.amplitude[channel] = 0x7FFF;
		g_tonegen.bytes_to_go[channel] = 4;
		g_tonegen.bytes_per_switch[channel] = 0;
		g_tonegen.flip[channel] = 1;
	}
	g_tonegen_init = true;
	return result;
}

void tonegen_writedata(Uint32 channel, Uint32 frequency, int index)
{
	g_tonegen.bytes_per_switch[channel] = frequency?(int) ((AUDIO_FREQ / frequency * 4 / 2) + .5):0;
}

void tonegen_stream(Uint8* stream, int length, int index)
{
	for (int pos = 0; pos < length; pos += 4)
	{
		// endian-independent! :)
		// NOTE : assumes stream is in little endian format
		Sint16 sample = 0;
		int channel = 0;
		for (channel = 0; channel < VOICES; channel++)
		{
			sample += g_tonegen.amplitude[channel] * g_tonegen.flip[channel] / VOICES;
		}

		stream[pos] = stream[pos+2] = (Uint16) (sample) & 0xff; 
		stream[pos+1] = stream[pos+3] = ((Uint16) (sample) >> 8) & 0xff; 

		for (channel = 0; channel < VOICES; channel++)
		{
         if (g_tonegen.bytes_per_switch[channel] > 4)
			{
            g_tonegen.bytes_to_go[channel] -= 4;
			}
			if (g_tonegen.bytes_to_go[channel] <=0)
			{
				g_tonegen.bytes_to_go[channel] += g_tonegen.bytes_per_switch[channel];
				g_tonegen.flip[channel] = -g_tonegen.flip[channel];
			}
		}
	}
}
