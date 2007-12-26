/*
* dac.cpp
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
#include "../io/mpo_mem.h"

#ifdef DEBUG
#include "../io/conout.h"
#include "../io/mpo_fileio.h"
#include "../cpu/cpu.h"
#include "../io/numstr.h"
#include <assert.h>
#endif

// how many DACs have been created
unsigned int g_uDACCount = 0;

// lookup table to convert 8-bit unsigned sound data to 16-bit signed sound data
Sint16 g_DACTable[256];

// the sample val that is currently active
unsigned int g_u8DACVal = 0;

// buffer that holds 8-bit unsigned mono samples that have been received from the game driver,
//  but have not been converted by dac_get_stream yet.
unsigned char g_u8SampleBuf[10000];

// how many samples are inside u8SampleBuf right this very second
unsigned int g_uDACSampleCount = 0;

// how many samples we've returned without benefit of our buffer
unsigned int g_uDACSamplesWOBuf = 0;

// of # cpu cycles that occur each 1 ms interval
unsigned int g_uCyclesPerInterval = 0;

// # of cycles that have been used so far during the current 1 ms interval
unsigned int g_uCyclesUsedThisInterval = 0;

// to make sure we don't use too many samples per interval
//  (since this # is not necessarily the same as g_uDACSampleCount, since it can have leftovers)
unsigned int g_uSampleCountThisInterval = 0;

// needs to be calculated by dac_init
double g_dSamplesPerCycle = 0.0;

/////////////////////////////////////////////////////////////

//#define OUT_RAW 1

#ifdef OUT_RAW
const char *STREAM_NAME = "stream.raw";
const char *SAMPLE_NAME = "sample.raw";

mpo_io *stream_io = NULL;
mpo_io *sample_io = NULL;

#endif

// init callback
int dac_init(unsigned int uCpuFreq)
{

#ifdef DEBUG
	// a couple of assumptions...
	assert(AUDIO_CHANNELS == 2);
	assert(AUDIO_BYTES_PER_SAMPLE == 4);
	assert(g_uDACCount == 0);	// not designed to handle more than 1 DAC
#endif

#ifdef OUT_RAW
	stream_io = mpo_open(STREAM_NAME, MPO_OPEN_CREATE);
	sample_io = mpo_open(SAMPLE_NAME, MPO_OPEN_CREATE);
#endif

	for (int i = 0; i < 256; i++)
	{
//		g_DACTable[i] = (i - 128) * 256;
// UPDATE : it appears that the DAC cannot do negative values
		g_DACTable[i] = i * 128;
	}

	g_uCyclesPerInterval = uCpuFreq / 1000;	// each interval is 1 ms
	g_dSamplesPerCycle = ((double) AUDIO_FREQ) / uCpuFreq;

	++g_uDACCount;
	return 0;
}

void dac_ctrl_data(unsigned int uCyclesSinceLastChange, unsigned int u8Byte, int internal_id)
{
#ifdef DEBUG
	assert(u8Byte <= 255);	// make sure it is really 8-bit
#endif

#ifdef OUT_RAW
	if (sample_io) mpo_write(&u8Byte, 1, NULL, sample_io);
#endif

	// if this is a recent update to the DAC, then we need to buffer it
	if (uCyclesSinceLastChange < g_uCyclesPerInterval)
	{
		g_uCyclesUsedThisInterval += uCyclesSinceLastChange;

		// calculate how many samples we should have at this point ...
		unsigned int uCorrectSampleCount = (unsigned int) ((g_uCyclesUsedThisInterval * g_dSamplesPerCycle) + 0.5);

#ifdef DEBUG
		assert(uCorrectSampleCount > g_uSampleCountThisInterval);
#endif
		// the # of samples we will be storing depends on how many we've already stored
		unsigned int uSamplesToStore = uCorrectSampleCount - g_uSampleCountThisInterval;

		// if we're in an overflow situation
		// (this should never happen)
		if ((uSamplesToStore + g_uDACSampleCount) >= sizeof(g_u8SampleBuf))
		{
			uSamplesToStore = sizeof(g_u8SampleBuf) - g_uDACSampleCount;
		}

		// if it's ok to write to the buffer
		if (uSamplesToStore > 0)
		{
			memset(g_u8SampleBuf + g_uDACSampleCount, g_u8DACVal, uSamplesToStore);
			g_uDACSampleCount += uSamplesToStore;
			g_uSampleCountThisInterval += uSamplesToStore;
		}
		// else perhaps we were already at our correct sample count

	} // end if recent update
	// else the DAC hasn't been updated for a while, so we do not need to buffer the new value,
	//  we can let the stream callback handle it instead.

	g_u8DACVal = u8Byte;
}

// called from sound mixer to get audio stream
void dac_get_stream(Uint8 *stream, int length, int internal_id)
{
#ifdef DEBUG
	// make sure this is in the proper format (stereo 16-bit)
	assert((length % AUDIO_BYTES_PER_SAMPLE) == 0);
#endif

	/*
	if (g_uDACSampleCount > 45)
	{
		Uint64 total_cycs = get_total_cycles_executed(1);
		string s = "total cycles for CPU #1 is " + numstr::ToStr(total_cycs) +
			" and the sample count is " + numstr::ToStr(g_uDACSampleCount);
		printline(s.c_str());
		int i = 0;
	}
	*/

	int pos = 0;
	unsigned int buf_idx = 0;

	while (pos < length)
	{
		Sint16 mono_sample;	// just one sample value from -32768 to 32767

		// if we have some buffered audio samples to be used
		if (buf_idx < g_uDACSampleCount)
		{
			mono_sample = g_DACTable[g_u8SampleBuf[buf_idx]];
			buf_idx++;
		}
		// if we have no buffered data, then fill remainder of stream with current sample value
		else
		{
			mono_sample = g_DACTable[g_u8DACVal];
			++g_uDACSamplesWOBuf;
		}

		Uint32 uSample = (Uint32) ((((Uint16) mono_sample) << 16) | (Uint16) mono_sample);	// convert to stereo

#ifdef OUT_RAW
		if (stream_io) mpo_write(&uSample, sizeof(uSample), NULL, stream_io);
#endif

		STORE_LIL_UINT32(stream + pos, uSample);	// store to audio stream
		pos += 4;
	}

	// length is in bytes, we want to know how many samples we've just sent
	unsigned int total_samples = (unsigned int) length / AUDIO_BYTES_PER_SAMPLE;

	// if we have leftover samples
	if (g_uDACSampleCount > total_samples)
	{
#ifdef DEBUG
		unsigned char cNewFront = g_u8SampleBuf[total_samples];
#endif
		g_uDACSampleCount -= total_samples;
		
		// move leftover samples to beginning of buffer
		memmove(g_u8SampleBuf, g_u8SampleBuf + total_samples, g_uDACSampleCount);
#ifdef DEBUG
		assert(g_u8SampleBuf[0] == cNewFront);	// make sure we crafted memmove command correctly
#endif
	}
	// else, no leftovers...
	else g_uDACSampleCount = 0;

	// a new interval begins now
	g_uCyclesUsedThisInterval = 0;	
	g_uSampleCountThisInterval = 0;

}
