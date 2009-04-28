/*
* sound.cpp
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


// DAPHNE
// The sound code in this file uses SDL

#ifdef DEBUG
#include <assert.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"
#include "SDL_audio.h"

#include "sound.h"
#include "sn_intf.h"
#include "pc_beeper.h"
#include "gisound.h"
#include "dac.h"
#include "tonegen.h"
#include "samples.h"
#include "mix.h"
#include "../io/conout.h"
#include "../io/mpo_mem.h"
#include "../io/numstr.h"
#include "../game/game.h"
#include "../daphne.h"
#include "../ldp-out/ldp-vldp.h" // added by JFA for -startsilent

sample_s g_samples[MAX_NUM_SOUNDS] = { { 0 } };
sample_s g_sample_saveme;	// the special saveme wav which is loaded independently of any game

bool g_sound_enabled = true;	// whether sound is enabled

bool g_bSoundMuted = false;	// whether sound is muted

struct sounddef *g_soundchip_head = NULL;	// pointer to the first sound chip in our linked list of chips's
unsigned int g_uSoundChipNextID = 0;	// the idea that the next soundchip to get added will get (also usually indicates how many sound chips have been added, but not if a soundchip gets deleted)

// callback to actually do the mixing
void (*g_soundmix_callback)(Uint8* stream, int length) = mixNone;

// The # of samples in the sound buffer
// Matt prefers 1024 but some people (cough Warren) can't handle it haha
//  but now it can be changed from the command line
Uint16 g_u16SoundBufSamples = 2048;

// # of bytes each individual sound chip should be allocated for its buffer
unsigned int g_uSoundChipBufSize = g_u16SoundBufSamples * AUDIO_BYTES_PER_SAMPLE;

// the volume (user adjustable) of the VLDP audio stream
unsigned int g_uVolumeVLDP = AUDIO_MAX_VOLUME;

// the volume (user adjustable) of all over sound streams besides VLDP
unsigned int g_uVolumeNonVLDP = AUDIO_MAX_VOLUME;

int cur_wave = 0;	// the current wave being played (0 to NUM_DL_BEEPS-1)
bool g_sound_initialized = false;	// whether the sound will work if we try to play it

// Macros to help automatically verify that our locks and unlocks are correct
#ifdef DEBUG
bool g_bAudioLocked = false;
#define LOCK_AUDIO assert (!g_bAudioLocked); g_bAudioLocked = true; SDL_LockAudio
#define UNLOCK_AUDIO assert (g_bAudioLocked); g_bAudioLocked = false; SDL_UnlockAudio
#else
#define LOCK_AUDIO SDL_LockAudio
#define UNLOCK_AUDIO SDL_UnlockAudio
#endif // lock audio macros

// added by JFA for -startsilent
void set_sound_mute(bool bMuted)
{
	g_bSoundMuted = bMuted;

	// only proceed if sound has been initialized
	if (g_sound_initialized)
	{
		LOCK_AUDIO();
		// this should set the mixing callback back to something that isn't muted
		update_soundchip_volumes();
		UNLOCK_AUDIO();
	}
}
// end edit

void set_soundbuf_size(Uint16 newbufsize)
{
	g_u16SoundBufSamples = newbufsize;
	g_uSoundChipBufSize = newbufsize * AUDIO_BYTES_PER_SAMPLE;

	// re-allocate all sound buffers since the size has changed
	struct sounddef *cur = g_soundchip_head;
	while (cur)
	{
		delete cur->buffer;
		cur->buffer = new Uint8 [g_uSoundChipBufSize];
      memset(cur->buffer, 0, g_uSoundChipBufSize);
		cur->buffer_pointer = cur->buffer;
		cur->bytes_left = g_uSoundChipBufSize;
		cur = cur->next_soundchip;
	}
}

static SDL_AudioSpec specDesired, specObtained;

bool sound_init()
// returns a true on success, false on failure
{
	
	bool result = false;
	int audio_rate = AUDIO_FREQ; // rate to mix audio at.  This cannot be changed without resampling all .wav's and all .ogg's
	
	Uint16 audio_format = AUDIO_FORMAT;
	int audio_channels = AUDIO_CHANNELS;
	
	printline("Initializing sound system ... ");
	
	// if the user has not disabled sound from the command line
	if (is_sound_enabled())
	{
		// if SDL audio initialization was successful
		if (SDL_InitSubSystem(SDL_INIT_AUDIO) >= 0)
		{
			specDesired.callback = audio_callback;
			specDesired.channels = audio_channels;
			specDesired.format = audio_format;
			specDesired.freq = audio_rate;
			specDesired.samples = g_u16SoundBufSamples;
			specDesired.userdata = NULL;

			// this stuff doesn't need to be filled in supposedly ...
			specDesired.padding = 0;
			specDesired.size = 0;

			// if we can open the audio device
			if (SDL_OpenAudio(&specDesired, &specObtained) >= 0)
			{
				// make sure we got what we asked for
				if ((specObtained.channels == audio_channels) &&
					(specObtained.format == audio_format) &&
					(specObtained.freq == audio_rate) &&
					(specObtained.callback == audio_callback))
				{
					// if we can load all our waves, we're set
					if (load_waves())
					{
						// If we are supposed to start without playing any sound, then set muted bool here.
						// It must come here because add_soundchip (which comes right afterwards) will set the sound mixing callback.
						if (get_startsilent())
						{
							g_bSoundMuted = true;
						}

						// right before initialization, add the samples 'sound chip', which can (and should be)
						//  only added once, so we need not track its ID (we call its functions directly)
						struct sounddef soundchip;
						soundchip.type = SOUNDCHIP_SAMPLES;
						add_soundchip(&soundchip);

						// initialize sound chips
						init_soundchip();

						if (specObtained.samples != g_u16SoundBufSamples)
						{
							string strWarning = "WARNING : requested " + numstr::ToStr(g_u16SoundBufSamples) +
								" samples for sound buffer, but got " + numstr::ToStr(specObtained.samples) + " samples";
							printline(strWarning.c_str());

							// reset memory allocations
							set_soundbuf_size(specObtained.samples);
						}

						result = true;
						g_sound_initialized = true;

						// enable the audio callback (this should come last to be safe)
						SDL_PauseAudio(0);	// start mixing! :)
					}
					// else if loading waves failed
					else
					{
						printline("ERROR: one or more required sound sample files could not be loaded!");
					}
				} // end if audio specs are correct
				else
				{
					printline("ERROR: unable to obtain desired audio configuration");
				}
			} // end if audio device could be opened ...
			
			// if audio device could not be opened (ie no sound card)
			else
			{
				outstr("WARNING: Audio device could not be opened: ");
				printline(SDL_GetError());
				g_sound_enabled = false;
			}
		} // end if sound initializtion worked	  
	} // end if sound is enabled
	
	// if sound isn't enabled, then we act is if sound initialization worked so daphne doesn't quit
	if (!is_sound_enabled())
	{
		result = true;
	}
	
	return(result);
	
}

// shuts down the sound subsystem
void sound_shutdown()
{
	// shutdown sound only if we previously initialized it
	if (g_sound_initialized)
	{
		printline("Shutting down sound system...");
		SDL_PauseAudio(1);
		SDL_CloseAudio();
		free_waves();
		shutdown_soundchip();
		g_sound_initialized = 0;
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
	}
}

// plays a sample, returns true on success
bool sound_play(Uint32 whichone)
{	
	bool result = false;
	
	// only play a sound if sound has been initialized (and if whichone points to a valid wav)
	if (is_sound_enabled() && (whichone < MAX_NUM_SOUNDS))
	{
		samples_play_sample(g_samples[whichone].pu8Buf, g_samples[whichone].uLength, g_samples[whichone].uChannels);
		result = true;
	}
	
	return(result);
	
}

// plays the 'saveme' sound
bool sound_play_saveme()
{
	bool result = false;
	
	if (is_sound_enabled())
	{
		samples_play_sample(g_sample_saveme.pu8Buf, g_sample_saveme.uLength);
		result = true;
	}
	
	return result;
}

// loads the wave files into the wave structure
// returns 0 if failure, or non-zero if success
int load_waves()
{
	
	Uint32 i = 0;
	int result = 1;
	string filename = "";
	SDL_AudioSpec spec;

	for (; (i < g_game->get_num_sounds()) && result; i++)
	{
		filename = "sound/";
		filename += g_game->get_sound_name(i);

		// initialize so that shutting down succeeds
		g_samples[i].pu8Buf = NULL;
		g_samples[i].uLength = 0;

		// if loading the .wav file succeeds
		if (SDL_LoadWAV(filename.c_str(), &spec, &g_samples[i].pu8Buf, &g_samples[i].uLength))
		{
			// make sure audio specs are correct
			if (((spec.channels == AUDIO_CHANNELS) || (spec.channels == 1)) &&
				(spec.freq == AUDIO_FREQ) &&
				(spec.format == AUDIO_S16))
			{
				g_samples[i].uChannels = spec.channels;
			}
			// else specs are not correct
			else
			{
				outstr("ERROR: Audio specs are not correct for ");
				printline(filename.c_str());
				result = 0;
			}
		} // end if loading worked ...

		// TODO : add .OGG loading support in here

		else
		{
			outstr("ERROR: Could not open sample file ");
			printline(filename.c_str());
			result = 0;
		}
	}
	
	// load "saveme" sound in
	if (!SDL_LoadWAV("sound/saveme.wav", &spec, &g_sample_saveme.pu8Buf, &g_sample_saveme.uLength))
	{
		printline("Loading 'saveme.wav' failed...");
		result = 0;
	}

	// if something went wrong, eject ...
	if (!result)
	{
		free_waves();
	}
	
	return(result);
	
}

// free all allocated waves
void free_waves()
{
	unsigned int i = 0;
	
	for (; i < g_game->get_num_sounds(); i++)
	{
		// don't de-allocate a sound if it hasn't been allocated
		if (g_samples[i].pu8Buf)
		{
			SDL_FreeWAV(g_samples[i].pu8Buf);
			g_samples[i].pu8Buf = NULL;
		}
	}

	if (g_sample_saveme.pu8Buf)
	{
		SDL_FreeWAV(g_sample_saveme.pu8Buf);
		g_sample_saveme.pu8Buf = NULL;
	}
}

int get_sound_initialized()
{
	return(g_sound_initialized);
}

void set_sound_enabled_status (bool value)
{
	g_sound_enabled = value;
}

bool is_sound_enabled()
{
	return g_sound_enabled;
}

// NOTE : this is called by the game driver, so it can be called even if sound is disabled
unsigned int add_soundchip(struct sounddef *candidate)
{
	LOCK_AUDIO();	// safety precaution, we don't want callback running during this function

	struct sounddef *cur = NULL;
	
	// if this is the first sound chip to be added to the list
	if (!g_soundchip_head)
	{
		g_soundchip_head = new struct sounddef;	// allocate a new sound chip, assume allocation is successful
		cur = g_soundchip_head;	// point to the new sound chip so we can populate it with info
	}
	// else we have to move to the end of the list
	else
	{
		cur = g_soundchip_head;
		
		// go to the last sound chip in the list
		while (cur->next_soundchip)
		{
			cur = cur->next_soundchip;
		}
		
		cur->next_soundchip = new struct sounddef;	// allocate a new sound chip at the end of our list
		cur = cur->next_soundchip;	// point to the new sound chip so we can populate it with info
	}
	
	// now we must copy over the relevant info
	memcpy(cur, candidate, sizeof(struct sounddef));	// copy entire thing over
	cur->id = g_uSoundChipNextID;
	cur->internal_id = 0;	// sensible initial value

	// This function will be called before the user has a chance to modify the volumes,
	//  so we will initialize them all to sensible defaults.
	cur->uDriverVolume[0] = cur->uDriverVolume[1] =
		cur->uBaseVolume[0] = cur->uBaseVolume[1] =
		cur->uVolume[0] = cur->uVolume[1] = AUDIO_MAX_VOLUME;

	++g_uSoundChipNextID;

	cur->next_soundchip = NULL;
	cur->bNeedsConstantUpdates = false;	// sensible default
	// create a buffer for each chip
	cur->buffer = new Uint8 [g_uSoundChipBufSize];
	cur->buffer_pointer = cur->buffer;
	cur->bytes_left = g_uSoundChipBufSize;
	cur->init_callback = NULL;
	cur->shutdown_callback = NULL;
	cur->stream_callback = NULL;
	cur->writedata_callback = NULL;
	cur->write_ctrl_data_callback = NULL;

	memset(cur->buffer, 0, g_uSoundChipBufSize);
	
	// now we must assign the appropriate callbacks
	switch (cur->type)
	{
	case SOUNDCHIP_SAMPLES:
		cur->init_callback = samples_init;
		cur->shutdown_callback = samples_shutdown;
		cur->stream_callback = samples_get_stream;
		break;
	case SOUNDCHIP_VLDP:
		// we only need to define stream_callback, everything else is handled by ldp-vldp
		cur->stream_callback = ldp_vldp_audio_callback;
		break;
	case SOUNDCHIP_SN76496:
		cur->bNeedsConstantUpdates = true;	// doesn't sound good without it
		cur->init_callback = tms9919_initialize;
		cur->shutdown_callback = tms9919_shutdown;
		cur->writedata_callback = tms9919_writedata;
		cur->stream_callback = tms9919_stream;
		break;
	case SOUNDCHIP_AY_3_8910:
		cur->bNeedsConstantUpdates = true;	// doesn't sound good without it
		cur->init_callback = gisound_initialize;
		cur->shutdown_callback = gisound_shutdown;
		cur->write_ctrl_data_callback = gisound_writedata;
		cur->stream_callback = gisound_stream;
		break;
	case SOUNDCHIP_PC_BEEPER:	// used by DL2/SA91
		cur->bNeedsConstantUpdates = true;	// for now we'll have it this way
		cur->init_callback = beeper_init;
		cur->write_ctrl_data_callback = beeper_ctrl_data;
		cur->stream_callback = beeper_get_stream;
		break;
	case SOUNDCHIP_DAC:	// used by MACK 3
		cur->bNeedsConstantUpdates = true;
		cur->init_callback = dac_init;
		cur->write_ctrl_data_callback = dac_ctrl_data;
		cur->stream_callback = dac_get_stream;
		break;
	case SOUNDCHIP_TONEGEN:	// generic 4 voice tone generator
		cur->bNeedsConstantUpdates = true;
		cur->init_callback = tonegen_initialize;
		cur->write_ctrl_data_callback = tonegen_writedata;
		cur->stream_callback = tonegen_stream;
		break;
	default:
		printline("FATAL ERROR : unknown sound chip added");
		set_quitflag();	// force user to deal with this problem
		break;
	}

	// calculate mixing callback, adjust volume, recalculate rshift
	// NOTE : this should come last in this function
	update_soundchip_volumes();

	UNLOCK_AUDIO();

	return cur->id;
}

bool delete_soundchip(unsigned int id)
{
	bool bSuccess = false;
	struct sounddef *cur = g_soundchip_head;
	struct sounddef *prev = NULL;

	LOCK_AUDIO();
	// if 1 or more sound chips exists ...
	while (cur)
	{
		struct sounddef *pNext = cur->next_soundchip;

		// if we found a match, then delete it...
		if (cur->id == id)
		{
			// if there is a shutdown callback defined, call it
			if (cur->shutdown_callback)
			{
				cur->shutdown_callback(cur->internal_id);
			}

			// if cur != g_soundchip_head in other words ...
			if (prev != NULL)
			{
				// restore the chain that we're about to break
				prev->next_soundchip = cur->next_soundchip;
			}
			
			delete [] cur->buffer;
			delete cur;

			// if we just deleted the head, then make the next soundchip be the head
			if (cur == g_soundchip_head)
			{
				g_soundchip_head = pNext;
			}
			
			bSuccess = true;
			break;
		}
		prev = cur;
		cur = cur->next_soundchip;
	}
	UNLOCK_AUDIO();
	
	return bSuccess;
}

void init_soundchip()
{
#ifdef DEBUG
	assert(is_sound_enabled());
#endif
	LOCK_AUDIO();	// safety precaution, we don't want callback running during this function
	if (g_soundchip_head)
	{
		struct sounddef *cur = g_soundchip_head;
		
		while (cur)
		{
			// only initialize if the callback exists
			if (cur->init_callback)
			{
				cur->internal_id = cur->init_callback(cur->hz);
				if (cur->internal_id == -1)
				{
					printline("sound.cpp Error : sound chip failed to initialize");
					set_quitflag();	// force dev to deal with this problem
				}
				// else everything initialized correctly
			}
			cur = cur->next_soundchip;
		}
	}
	UNLOCK_AUDIO();
}

// Mixing callback
// USED WHEN : all audio is muted
void mixMute(Uint8 *stream, int length)
{
	memset(stream, 0, length);
}

// Mixing callback
// USED WHEN: there is only 1 sound chip, then there is no need to do any mixing
void mixNone(Uint8 *stream, int length)
{
	if (g_soundchip_head != NULL)
	{
		memcpy(stream, g_soundchip_head->buffer, length);
	}
}

// Mixing callback
// USED WHEN: there are more than 1 sound chip, but all volumes are maximum
void mixWithMaxVolume(Uint8 *stream, int length)
{
#ifdef DEBUG
	assert(g_soundchip_head);
#endif // DEBUG

	// this is a dangerous trick (casting one struct to another) in order to get us extra speed
	g_pMixBufs = (struct mix_s *) g_soundchip_head;
	g_pSampleDst = stream;
	g_uBytesToMix = length;
	g_mix_func();

	/*
	struct sounddef *cur;

	// mix all sound chip buffers together
	// NOTE : this algorithm is accurate but NOT OPTIMIZED and probably should be optimized,
	//  because it is called constantly.
	for (int sample = 0; sample < (length >> 1); sample += 2)
	{
		Sint32 mixed_sample_1 = 0, mixed_sample_2 = 0;	// left/right channels
		cur = g_soundchip_head;
		while (cur)
		{
			mixed_sample_1 += LOAD_LIL_SINT16(((Sint16 *) cur->buffer) + sample);
			mixed_sample_2 += LOAD_LIL_SINT16(((Sint16 *) cur->buffer) + sample + 1);
			cur = cur->next_soundchip;
		}

		DO_CLIP(mixed_sample_1);
		DO_CLIP(mixed_sample_2);

		// note: sample2 needs to be on top because this is little endian, hence LSB
		Uint32 val_to_store = (((Uint16) mixed_sample_2) << 16) | (Uint16) mixed_sample_1;

		STORE_LIL_UINT32(stream, val_to_store);
		stream += 4;
	}
	*/
}

// Mixing callback
// USED WHEN: there are more than 1 sound chip, and volumes are variable
//  (this is the slowest callback)
void mixWithMults(Uint8 *stream, int length)
{
	struct sounddef *cur;

	// mix all sound chip buffers together
	// NOTE : this algorithm is accurate but NOT OPTIMIZED and probably should be optimized,
	//  because it is called constantly.
	for (int sample = 0; sample < (length >> 1); sample += 2)
	{
		Sint32 mixed_sample_1 = 0, mixed_sample_2 = 0;	// left/right channels, 32-bit to support adding many 16-bit samples
		cur = g_soundchip_head;
		while (cur)
		{
			// multiply by the volume and then dividing by the max volume (by shifting right, which is much faster)
			mixed_sample_1 += (Sint16) ((LOAD_LIL_SINT16(((Sint16 *) cur->buffer) + sample) * cur->uVolume[0]) >> AUDIO_MAX_VOL_POWER);
			mixed_sample_2 += (Sint16) ((LOAD_LIL_SINT16(((Sint16 *) cur->buffer) + sample + 1) * cur->uVolume[1]) >> AUDIO_MAX_VOL_POWER);
			cur = cur->next_soundchip;
		}

		DO_CLIP(mixed_sample_1);
		DO_CLIP(mixed_sample_2);

		Uint32 val_to_store = (((Uint16) mixed_sample_2) << 16) | (Uint16) mixed_sample_1;
		STORE_LIL_UINT32(stream, val_to_store);
		stream += 4;
	}
}

void audio_callback ( void *data, Uint8 *stream, int length )
{
	// now go through the sound chips and mix them in
	struct sounddef *cur = g_soundchip_head;

	// fill remaining buffer space for each sound chip
	while (cur)
	{
#ifdef DEBUG
		assert(cur->stream_callback != NULL);	// every sound chip will have to supply this
#endif
		cur->stream_callback(cur->buffer_pointer, cur->bytes_left, cur->internal_id);
		cur->buffer_pointer = cur->buffer;
		cur->bytes_left = g_uSoundChipBufSize;
		cur = cur->next_soundchip;
	}

	// do the actual mixing now
	g_soundmix_callback(stream, length);
}

void audio_writedata(Uint8 id, Uint8 data)
{
	// if sound isn't initialized, then the soundchips aren't initialized either
	if (g_sound_initialized)
	{
		LOCK_AUDIO();	// safety precaution, we don't want callback running during this function
		struct sounddef *cur = g_soundchip_head;
		while (cur)
		{
			if (cur->id == id)
			{
				cur->writedata_callback(data, cur->internal_id);
			}      
			cur = cur->next_soundchip;
		}
		UNLOCK_AUDIO();
	}
}

// in case audio_writedata doesn't cut it ...
void audio_write_ctrl_data(unsigned int uCtrl, unsigned int uData, Uint8 id)
{
	// if sound isn't initialized, then the soundchips aren't initialized either
	if (g_sound_initialized)
	{
		LOCK_AUDIO();
		struct sounddef *cur = g_soundchip_head;
		while (cur)
		{
			if (cur->id == id)
			{
				cur->write_ctrl_data_callback(uCtrl, uData, cur->internal_id);
			}
			cur = cur->next_soundchip;
		}
		UNLOCK_AUDIO();
	}
}

void set_soundchip_volume(Uint8 id, unsigned int uChannel, unsigned int uVolume)
{
	struct sounddef *cur = g_soundchip_head;
	while (cur)
	{
		if (cur->id == id)
		{
			set_soundchip_volume(cur, uChannel, uVolume);
			break;
		}
		cur = cur->next_soundchip;
	}
}

void set_soundchip_volume(struct sounddef *cur, unsigned int uChannel, unsigned int uVolume)
{
	// safety check
	if (uChannel < AUDIO_CHANNELS)
	{
		// safety check
		if (uVolume <= AUDIO_MAX_VOLUME)
		{
			cur->uDriverVolume[uChannel] = uVolume;
			LOCK_AUDIO();
			update_soundchip_volumes();
			UNLOCK_AUDIO();
		}
		else
		{
			printline("sound.cpp, set_soundchip_volume() ERROR: volume is out of range, shutting down");
			set_quitflag();	// force dev to deal with this :)
		}
	}
	// channel was out of range
	else
	{
		printline("sound.cpp, set_soundchip_volume() ERROR : channel is out of range");
		set_quitflag();	// force dev to fix this
	}
}


void set_soundchip_vldp_volume(unsigned int uVolume)
{
	if (uVolume <= AUDIO_MAX_VOLUME)
	{
		g_uVolumeVLDP = uVolume;
		LOCK_AUDIO();
		update_soundchip_volumes();
		UNLOCK_AUDIO();
	}
	else
	{
		printline("WARNING : request VLDP volume is out of range");
	}
}

void set_soundchip_nonvldp_volume(unsigned int uVolume)
{
	if (uVolume <= AUDIO_MAX_VOLUME)
	{
		g_uVolumeNonVLDP = uVolume;
		LOCK_AUDIO();
		update_soundchip_volumes();
		UNLOCK_AUDIO();
	}
	else
	{
		printline("WARNING : request non-VLDP volume is out of range");
	}
}

// IMPORTANT : assumes LOCK_AUDIO has already been called!!!!
void update_soundchip_volumes()
{	
	bool bNonMaxVolume = false;
	unsigned int uSoundchipCount = 0;

#ifdef DEBUG
	assert (g_bAudioLocked == true);
#endif

	// If sound is not muted then do some calculations
	if (!g_bSoundMuted)
	{

		struct sounddef *cur = g_soundchip_head;
		while (cur)
		{
			// if this isn't a VLDP chip ...
			if (cur->type != SOUNDCHIP_VLDP)
			{
				cur->uBaseVolume[0] = cur->uBaseVolume[1] = g_uVolumeNonVLDP;
			}
			// else this is the VLDP chip
			else
			{
				cur->uBaseVolume[0] = cur->uBaseVolume[1] = g_uVolumeVLDP;
			}

			for (unsigned int uChannel = 0; uChannel < 2; ++uChannel)
			{
				// The actual volume must take into account the user-defined BaseVolume,
				//  in case the user requested that the volume be 50% of whatever it would normally be.
				// If the base volume is AUDIO_MAX_VOLUME, then the new volume becomes uVolume
				cur->uVolume[uChannel] = (cur->uDriverVolume[uChannel] * cur->uBaseVolume[uChannel]) / AUDIO_MAX_VOLUME;

				// if we've wound up with a volume that is less than the max
				if (cur->uVolume[uChannel] < AUDIO_MAX_VOLUME)
				{
					bNonMaxVolume = true;
				}
			}

			cur = cur->next_soundchip;
			++uSoundchipCount;
		}

		// if we need to use the most expensive mixing callback...
		if (bNonMaxVolume)
		{
			g_soundmix_callback = mixWithMults;
		}
		else if (uSoundchipCount > 1)
		{
			g_soundmix_callback = mixWithMaxVolume;
		}
		// just 1 soundchip? we can mix super fast in that case
		else
		{
			g_soundmix_callback = mixNone;
		}

	} // end if sound is not muted
	// else sound is muted
	else
	{
		g_soundmix_callback = mixMute;
	}

}

void shutdown_soundchip()
{
#ifdef DEBUG
	assert(g_sound_initialized);
#endif
	LOCK_AUDIO();	// safety precaution, we don't want callback running during this function
	struct sounddef *cur = g_soundchip_head;
	while (cur)
	{
		// if there is a shutdown callback defined, call it
		if (cur->shutdown_callback)
		{
			cur->shutdown_callback(cur->internal_id);
		}
		struct sounddef *temp = cur;
		cur = cur->next_soundchip;
		delete [] temp->buffer;
		delete temp;
	}
	UNLOCK_AUDIO();	
}

void update_soundbuffer()
{
	// we don't want to update the sound buffer, if sound isn't initialized
	if (g_sound_initialized)
	{
		// to ensure that the audio callback doesn't get called while we're in this function
		LOCK_AUDIO();
		struct sounddef *cur = g_soundchip_head;
		while (cur)
		{
			// only update if needed, to save CPU cycles
			if (cur->bNeedsConstantUpdates)
			{
				if (cur->bytes_left >= G_1MS_BUF_SIZE)
				{
					cur->stream_callback(cur->buffer_pointer, G_1MS_BUF_SIZE, cur->internal_id);
					cur->bytes_left -= G_1MS_BUF_SIZE;
					cur->buffer_pointer += G_1MS_BUF_SIZE;
				}
				// else we throw away the new data
				// should we handle this some other way?
			}
			// else doesn't need to be updated so often, so don't do it ...
			cur = cur->next_soundchip;
		}
		UNLOCK_AUDIO();
	}
}
