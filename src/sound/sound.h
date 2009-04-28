/*
 * sound.h
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

#ifndef SOUND_H
#define SOUND_H

#include <SDL.h>

// header file for sound.c

// max # of samples any game can handle (make this number as small as possible)
#define MAX_NUM_SOUNDS 50

// frequency all our audio runs at.
// In order to change this value, all .wav's and .ogg's will need to be resampled, which
//  is quite involved. Other parts of the code may assume the frequency is 44100 too (such as ldp-vldp-audio.cpp)
#define AUDIO_FREQ 44100

// stereo sound
#define AUDIO_CHANNELS 2

// stereo, 16-bit sound has 4 bytes per sample
#define AUDIO_BYTES_PER_SAMPLE 4

// volume for each stream can be between 0-64
// (keep this as a power of 2, see AUDIO_MAX_VOL_POWER)
#define AUDIO_MAX_VOLUME 64

// if 2^AUDIO_MAX_VOL_POWER = AUDIO_MAX_VOLUME
#define AUDIO_MAX_VOL_POWER 6

// SDL audio format (16-bit signed, little endian)
// This is true even for big-endian platforms
#define AUDIO_FORMAT AUDIO_S16LSB

// how many audio bytes needed to fill 1 millisecond of space
static const unsigned int G_1MS_BUF_SIZE = (AUDIO_FREQ * AUDIO_BYTES_PER_SAMPLE) / 1000;

enum { SOUNDCHIP_UNDEFINED, SOUNDCHIP_SAMPLES, SOUNDCHIP_VLDP, SOUNDCHIP_SN76496, SOUNDCHIP_AY_3_8910,
	SOUNDCHIP_PC_BEEPER, SOUNDCHIP_DAC, SOUNDCHIP_TONEGEN };

struct sample_s
{
	// how many channels (1 = mono, 2 = stereo) that this sample has
	unsigned int uChannels;

	// length of the sample ...
	unsigned int uLength;

	// the sample's buffer
	Uint8 *pu8Buf;
};

// macro to do 16-bit clipping
#define DO_CLIP(i) 	if (i > 32767) { i = 32767; } else if (i < -32768) { i = -32768; }

struct sounddef
{
	// *** THIS SECTION IS DEFINED INTERNALLY
	// IMPORTANT: buffer and next_soundchip MUST come first, because they must match
	//  the structure mix_s defined in mix.h; this is so we can use our MMX optimized
	//  audio mixing function.
	Uint8* buffer; // pointer to buffer used by this sound chip
	struct sounddef *next_soundchip;	// pointer to the next sound chip in this linked list

	Uint8* buffer_pointer; // pointer to where we are in the buffer
	Uint32 bytes_left; // number of bytes left in buffer
	unsigned int id;	// used so game drivers can call audio_writedata (if there are multiple sound chips being used)
	int internal_id;	// internal ID that the sound chips returns when init_callback is called
	unsigned int uVolume[AUDIO_CHANNELS];	// don't modify this value directly, use set_soundchip_volume() to do it ...

	// This is the volume that the game driver has requested (if no request, this is AUDIO_MAX_VOLUME).
	// It is not affected by set_soundchip_volume.  uVolume is calculated based on this value.
	unsigned int uDriverVolume[AUDIO_CHANNELS];

	// This is the volume that the user has requested (if no request, this is AUDIO_MAX_VOLUME).
	// It is not affected by set_soundchip_volume.  uVolume is calculated based on this value.
	unsigned int uBaseVolume[AUDIO_CHANNELS];

	// callback to initialize the sound chip.
	// The callback returns an internal ID which is used in all the other callbacks to
	//  refer to the sound chip that has been created.
	// On success a number >= 0 will be returned. If there is an error, -1 will be returned.
	int (*init_callback)(Uint32 freq_hz);
	void (*shutdown_callback)(int internal_id);	// callback to shutdown the sound chip
	void (*writedata_callback)(Uint8 data, int internal_id);	// callback to write data to the sound chip

	// optional callback for those sound chips that take a 'ctrl' byte to write 'data' to,
	//  such as the PC beeper which writes data to specific ports.  It's up to the game driver
	//  to decide whether to use this callback or not.
	// NOTE : the arguments are unsigned int's for speed, but they usually will probably be Uint8's
	void (*write_ctrl_data_callback)(unsigned int uCtrl, unsigned int uData, int internal_id);

	void (*stream_callback)(Uint8* stream, int length, int internal_id); // callback to write stream to buffer

	// *** THIS SECTION IS DEFINED WHEN SOUND CHIP IS ADDED
	int type;	// type of sound chip (See enum's)
	Uint32 hz;	// speed of sound chip in Hz

	// Should be true if sound quality of chip is better the more often it is updated.
	//  An example is Super Don's sound chip.
	// Should be false if sound doesn't change no matter how fast it is updated.
	//  An example is VLDP which has constant pre-defined audio.
	// FIXME : in the future this should be changed to define how frequent of updates are
	//  needed, because the less frequent of updates, the better.
	bool bNeedsConstantUpdates;	
};

// adds a new soundchip and returns the ID
unsigned int add_soundchip(struct sounddef *);	// add a new cpu

// deletes a soundchip that has previously been added
bool delete_soundchip(unsigned int id);

void init_soundchip(); // inits all the sound chips

// Mixing callback
// USED WHEN : all audio is muted
void mixMute(Uint8 *stream, int length);

// Mixing callback
// USED WHEN: there is only 1 sound chip, then there is no need to do any mixing
// (fastest)
void mixNone(Uint8 *stream, int length);

// Mixing callback
// USED WHEN: there are more than 1 sound chip, but all volumes are maximum
// (pretty fast)
void mixWithMaxVolume(Uint8 *stream, int length);

// Mixing callback
// USED WHEN: there are more than 1 sound chip, and volumes are variable
// (this is the slowest callback, only use it if you must)
void mixWithMults(Uint8 *stream, int length);

void audio_callback ( void *, Uint8 *, int); // audio callback for SDLMixer
void audio_writedata(Uint8, Uint8);

// for game driver to send commands to sound chip
void audio_write_ctrl_data(unsigned int uCtrl, unsigned int uData, Uint8 id);

// Used to set the volume of an audio stream. Valid volume values are 0 to AUDIO_MAX_VOLUME.
// 'id' is the id returned by add_soundchip()
// 'channel' is which audio channel to the set the volume for (0 = left, 1 = right I think)
// WARNING : setting the volume to anything lower than AUDIO_MAX_VOLUME may incur a (small?)
//  performance penalty and should be avoided if there is little benefit to adjusting
//  the volume.
// If there is an error, the logfile will be notified and the quitflag may be set :)
void set_soundchip_volume(Uint8 id, unsigned int uChannel, unsigned int uVolume);

// helper function for the other set_soundchip_volume
void set_soundchip_volume(struct sounddef *cur, unsigned int uChannel, unsigned int uVolume);

// so the user can override the default VLDP volume
void set_soundchip_vldp_volume(unsigned int uVolume);

// so the user can override the default volume of all soundchips besides VLDP
void set_soundchip_nonvldp_volume(unsigned int uVolume);

// Recalculates uVolume based on values of drivervolume and basevolume
// Also calculates which rshift and callback to use
void update_soundchip_volumes();

void shutdown_soundchip();
void update_soundbuffer(); // update the sound buffers with 1 ms worth of data
void set_soundbuf_size(Uint16 newbufsize);
bool sound_init();
void sound_shutdown();
bool sound_play(Uint32 whichone);
bool sound_play_saveme();
int load_waves();
void free_waves();
int get_sound_initialized();
void set_sound_mute(bool bMuted); // added by JFA for -startsilent
void set_sound_enabled_status (bool value);
bool is_sound_enabled();

// (re)calculates the right-shift value to be used to mix sounds (for fast division)
void sound_recalc_rshift();

#endif
