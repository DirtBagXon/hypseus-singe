/*
 * samples.h
 *
 * Copyright (C) 2006 Matt Ownby
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

#ifndef SAMPLES_H
#define SAMPLES_H

#include <SDL.h>	// for data-type defs

// init callback
int samples_init(unsigned int unused);

// shutdown
void samples_shutdown(int shutdown);

// called from sound mixer to get audio stream
void samples_get_stream(Uint8 *stream, int length, int internal_id);

// Plays a sample
// The sample's audio specs must match our the audio device's specs
// 'uLength' is how long the sample is IN BYTES (so 4-bytes = 1 sample for 16-bit stereo)
// 'uChannels' is how many channels the sample has (must be 1 for mono or 2 for stereo)
// 'iSlot' specifies which slot to play the sample in, or -1 to just pick the next available one
// Returns the slot that the sample is playing in, or
//  -1 if uChannels or iSlot is out of range, or
//  -2 if there are no slots available
int samples_play_sample(Uint8 *pu8Buf, unsigned int uLength, unsigned int uChannels = AUDIO_CHANNELS, int iSlot = -1,
										 void (*finishedCallback)(Uint8 *pu8Buf, unsigned int uSlot) = NULL);

// returns true if the sample indicated by 'uSlot' is currently playing
//  or false if the sample isn't playing
bool samples_is_sample_playing(unsigned int uSlot);

#endif // SAMPLES_H
