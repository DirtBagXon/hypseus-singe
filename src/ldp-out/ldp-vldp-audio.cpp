/*
 * ldp-vldp-audio.cpp
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

// LDP-VLDP-AUDIO.CPP
// by Matt Ownby

// handles the audio portion of VLDP (using Ogg Vorbis)

#ifdef WIN32
#pragma warning (disable:4100)	// disable the warning about unreferenced formal parameters (MSVC++)
#endif

#ifdef UNIX
//#define TRY_MMAP 1	// NOTE : this seems to fail on read-only filesystems (such as NTFS mounted from linux)
#endif

#include "../timer/timer.h"
#include "../io/conout.h"
#include "../io/mpo_fileio.h"
#include "../sound/sound.h"
#include "ldp-vldp.h"

#ifdef DEBUG
#include <assert.h>	// this may include an extra .DLL in windows that I don't want to rely on
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef GP2X
#include <vorbis/codec.h>		// OGG VORBIS specific headers
#include <vorbis/vorbisfile.h>
#else
// gp2x ogg vorbis decoding files
#include <tremor/ivorbiscodec.h>
#include <tremor/ivorbisfile.h>
#endif

#ifdef TRY_MMAP
#include <sys/mman.h>
#endif

// how much uncompressed audio we deal with at a time
#define AUDIO_BUF_CHUNK	4096

// Macros to lock and unlock the mutex for the audio to make sure we aren't playing audio while
// we are loading or seeking
#define OGG_LOCK	SDL_mutexP(g_ogg_mutex)
#define OGG_UNLOCK	SDL_mutexV(g_ogg_mutex)

/////////////////////////////////////////

typedef void *(*audiocopyproc)(void *dest, const void *src, size_t bytes_to_copy);
audiocopyproc paudiocopy = memcpy;	// pointer to the audio copy procedure (defaults to memcpy)

SDL_mutex *g_ogg_mutex = NULL;
mpo_io *g_pIOAudioHandle = NULL;
OggVorbis_File s_ogg;

Uint32 g_audio_filesize = 0;	// total size of the audio stream
Uint32 g_audio_filepos = 0;	// the position in the file of our audio stream
Uint8 *g_big_buf = NULL;	// holds entire Ogg stream in RAM :)
bool g_audio_ready = false;	// whether audio is ready to be parsed
bool g_audio_playing = false;	// whether the audio is to be playing or not
Uint32 g_playing_timer = 0;	// the time at which we began playing audio
Uint32 g_samples_played = 0;	// how many samples have played since we've been timing
bool g_audio_left_muted = false;	// left audio channel enabled
bool g_audio_right_muted = false;	// right audio channel enabled

#ifdef AUDIO_DEBUG
Uint64 g_u64CallbackByteCount = 0;
unsigned int g_uCallbackFloodTimer = 0;
unsigned int g_uCallbackDbgTimer = 0;
#endif

///////////////////////////////////////////////////////////////////////////////////

// resets mm states
void mmreset()
{
	g_audio_filepos = 0;	// rewind back to position #0
}

// replaces fread
size_t mmread (void *ptr, size_t size, size_t nmemb, void *datasource)
{
	size_t bytes_to_read = size * nmemb;	// how many bytes to be read
	Uint8 *src = ((Uint8 *) datasource) + g_audio_filepos;	// where to get the data from

//	printf("mmread being called.. size is %d, nmemb is %d, bytes_to_read is %d\n", size, nmemb, bytes_to_read);
	
	if (g_audio_filepos + bytes_to_read > g_audio_filesize)
	{
		bytes_to_read = 0;
		if (g_audio_filepos < g_audio_filesize)
		{
			bytes_to_read = g_audio_filesize - g_audio_filepos;
		}
	}

	if (bytes_to_read != 0)
	{
		memcpy(ptr, src, bytes_to_read);	// copy the memory
		g_audio_filepos += bytes_to_read;
	}
	
	return (bytes_to_read);
}

#ifdef WIN32
#define int64_t __int64
#endif

int mmseek (void *datasource, int64_t offset, int whence)
{
	int result = -1;

//	printf("mmseek being called, whence is %d\n", whence);

	// just to get rid of warnings
	if (datasource)
	{
	}
	
	switch (whence)
	{
	case SEEK_SET:
		// bug fix by Arnaud Gibert
		if (offset <= g_audio_filesize)
		{
			// make sure offset is positive so we don't get into trouble
			if (offset >= 0)
			{
				g_audio_filepos = (Uint32) offset;
			}
			else
			{
				printline("mmseek, SEEK_SET used with a negative offset!");
			}
			result = 0;
		}
		break;
	case SEEK_CUR:
		if (offset + g_audio_filepos <= g_audio_filesize)
		{
			g_audio_filepos = (unsigned int) (g_audio_filepos + offset);
			result = 0;
		}
		break;
	case SEEK_END:
//		printf("SEEK_END being called, offset is %x, whence is %d!\n", (Uint32) offset, whence);
		if (g_audio_filesize + offset <= g_audio_filesize)
		{
			g_audio_filepos = (unsigned int) (g_audio_filesize + offset);
			result = 0;
		}
		break;
	}

	return result;
}

int mmclose (void *datasource)
{
	// safety check
	if (datasource != g_big_buf)
	{
		printline("ldp-vldp-audio.cpp: datasource != g_bigbuf, this should never happen!");
	}
	
#ifdef TRY_MMAP
	printline("Unmapping audio stream from memory ...");
	munmap(g_big_buf, g_audio_filesize);
	datasource = NULL;
#else
	printline("Freeing memory used to store audio stream...");
	//free(datasource);
	delete [] g_big_buf;
	g_big_buf = NULL;
#endif

	mpo_close(g_pIOAudioHandle);
	g_pIOAudioHandle = NULL;

	return 0;
}

long mmtell (void *datasource)
{
//	printf("mmtell being called, filepos is %x\n", (Uint32) g_audio_filepos);

	if (datasource)
	{
	}

	return g_audio_filepos;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// public audio stuff

void ldp_vldp::enable_audio1()
{
	g_audio_left_muted = false;
	set_audiocopy_callback();
}

void ldp_vldp::enable_audio2()
{
	g_audio_right_muted = false;
	set_audiocopy_callback();
}

void ldp_vldp::disable_audio1()
{
	g_audio_left_muted = true;
	set_audiocopy_callback();
}

void ldp_vldp::disable_audio2()
{
	g_audio_right_muted = true;
	set_audiocopy_callback();
}

///////////////////////////////////////////////////////////////////////////////////////////////

// mute audio data
void *audiocopy_mute(void *dest, const void *src, size_t bytes_to_copy)
{
#ifdef WIN32
	ZeroMemory(dest, bytes_to_copy);
#else
	bzero(dest, bytes_to_copy);
#endif
	return NULL;
}

// copies left-channel audio data to right-channel
void *audiocopy_left_only(void *dest, const void *src, size_t bytes_to_copy)
{
#ifdef DEBUG
	assert(bytes_to_copy % 4 == 0);	// stereo 16-bit audio should always be divisible by 4
#endif
	Uint32 *dst32 = (Uint32 *) dest;
	Uint16 *src16_L = (Uint16 *) src;	// point to first bit of left-channel data

	bytes_to_copy >>= 2;	// divide by 4 because we will be advancing 4 bytes per iteration
	for (unsigned int index = 0; index < bytes_to_copy; index++)
	{
		*dst32 = (*src16_L << 16) | *src16_L;	// we only care about left-channel
		dst32++;
		src16_L += 2;	// skip over right channel data
	}
	return NULL;	// for compatiblity reasons, we don't use this value
}

// copies right-channel audio data to left-channel
void *audiocopy_right_only(void *dest, const void *src, size_t bytes_to_copy)
{
#ifdef DEBUG
	assert(bytes_to_copy % 4 == 0);	// stereo 16-bit audio should always be divisible by 4
#endif

	Uint32 *dst32 = (Uint32 *) dest;
	Uint16 *src16_R = (Uint16 *) src + 1;	// point to the first bit of data that occurs on the right channel

	bytes_to_copy >>= 2;	// divide by 4 because we will be advancing 4 bytes per iteration
	for (unsigned int index = 0; index < bytes_to_copy; index++)
	{
		// fastest method (third attempt)
		*dst32 = (*src16_R << 16) | *src16_R;	// we only care about the right-channel
		dst32++;
		src16_R += 2;	// skip over left channel data
	}
	return NULL;
}

// sets the audio copy callbackto be used, based on which audio channels are muted
// this should only be called when the mute status of the channels is changed
void ldp_vldp::set_audiocopy_callback()
{
	if (g_audio_left_muted)
	{
		if (g_audio_right_muted) paudiocopy = audiocopy_mute;	// both channels are muted
		else paudiocopy = audiocopy_right_only;	// only right channel is on
	}
	else
	{
		if (g_audio_right_muted) paudiocopy = audiocopy_left_only;	// only left channel is on
		else paudiocopy = (audiocopyproc) memcpy;	// default, both channels on
	}
}

// changes file extension of m2vpath so it ends in .ogg (and add suffix for alt soundtrack if needed)
void ldp_vldp::oggize_path(string &oggpath, string m2vpath)
{
	oggpath = m2vpath;
	oggpath.replace(oggpath.length()-4, 4, m_altaudio_suffix);	// append optional alternate audio suffix (if this string is blank it should be ok)
	oggpath += ".ogg";
}

// initializes VLDP audio, returns 1 on success or 0 on failure
bool ldp_vldp::audio_init()
{
	bool result = false;

#ifdef AUDIO_DEBUG
	g_u64CallbackByteCount = 0;
	g_uCallbackFloodTimer = 0;
	g_uCallbackDbgTimer = GET_TICKS();
#endif
	
	// create a mutex to prevent threads from interfering
	g_ogg_mutex = SDL_CreateMutex();
	if (g_ogg_mutex)
	{
		result = true;
	}

	return result;
}

// shuts down VLDP audio
void ldp_vldp::audio_shutdown()
{
	// if we have an audio file still open, close it
	if (g_pIOAudioHandle != 0)
	{
		close_audio_stream();
	}

	// if we successfully created a mutex previously, then destroy it now
	if (g_ogg_mutex)
	{
		SDL_DestroyMutex(g_ogg_mutex);
		g_ogg_mutex = NULL;
	}
}

void ldp_vldp::close_audio_stream()
{
	OGG_LOCK;

	g_audio_ready = false;
	g_audio_playing = false;
	ov_clear(&s_ogg);

	OGG_UNLOCK;
}

bool ldp_vldp::open_audio_stream(const string &strFilename)
{
	bool result = false;
	ov_callbacks mycallbacks =
	{
		mmread,
		mmseek,
		mmclose,
		mmtell
	};

	OGG_LOCK;	// can't have audio callback running during this

	// if an audio stream is already open, close it first
	if (g_pIOAudioHandle != 0)
	{
		close_audio_stream();
	}

	mmreset();	// reset the mm wrappers for new use

	g_pIOAudioHandle = mpo_open((m_mpeg_path + strFilename).c_str(), MPO_OPEN_READONLY);
	// if audio file was opened successfully
	if (g_pIOAudioHandle)
	{
		g_audio_filesize = static_cast<unsigned int>(g_pIOAudioHandle->size & 0xFFFFFFFF);
#ifdef TRY_MMAP
		g_big_buf = (Uint8 *) mmap(NULL, g_audio_filesize, PROT_READ, MAP_PRIVATE, fileno(g_pIOAudioHandle->handle), 0);
		if (!g_big_buf)
		{
			printline("ERROR : mmap failed");
		}
#else
		g_big_buf = new unsigned char[g_audio_filesize];
		if (g_big_buf)
		{
			mpo_read(g_big_buf, g_audio_filesize, NULL, g_pIOAudioHandle);	// read entire stream into RAM
		}
		else
		{
			printline("ERROR : out of memory");
		}
#endif
		if (g_big_buf)
		{
			int open_result = ov_open_callbacks(g_big_buf, &s_ogg, NULL, 0, mycallbacks);

			// if we opening the .OGG succeeded
			if (open_result == 0)
			{
				// now check to make sure it's stereo and the proper sample rate
				vorbis_info *info = ov_info(&s_ogg, -1);

				// if they meet the proper specification, let them proceed
				if ((info->channels == 2) && (info->rate == 44100))
				{
					g_audio_ready = true;
					result = true;
				}
				else
				{
					char s[160];
					printline("OGG ERROR : Your .ogg file needs to have 2 channels and 44100 Hz");
					sprintf(s, "OGG ERROR : Your .ogg file has %u channel(s) and is %ld Hz", info->channels, info->rate);
					printline(s);
					printline("OGG ERROR : Your .ogg file will be ignored (you won't hear any audio)");
				}
			}
			else
			{
				char s[160];
				sprintf(s, "ov_open_callbacks failed!  Error code is %d\n", open_result);
				printline(s);
				sprintf(s, "OV_EREAD=%d OV_ENOTVORBIS=%d OV_EVERSION=%d OV_EBADHEADER=%d OV_EFAULT=%d\n",
					OV_EREAD, OV_ENOTVORBIS, OV_EVERSION, OV_EBADHEADER, OV_EFAULT);
				printline(s);
			}
		}
		// else we've already printed error messages, so we don't need to do anything else

		// close file if we got an earlier error
		if (!result)
		{
			mpo_close(g_pIOAudioHandle);
			g_pIOAudioHandle = NULL;
			
			// if we have memory allocated, de-allocate it
			if (g_big_buf)
			{
#ifdef TRY_MMAP
				munmap(g_big_buf, g_audio_filesize);
#else
				delete [] g_big_buf;
#endif
				g_big_buf = NULL;
			}
		}
			
	} // end if we could open file
	else
	{
		// don't show this message to end-users, a surprising number of them report this as a bug and it's really getting annoying :)
#ifdef DEBUG
		string s;
		s = "No audio file (" + strFilename + ") was found to go with the opened video file";
		printline(s.c_str());
		printline("NOTE : This is not necessarily a problem, some video doesn't have audio!");
#endif
	}

	OGG_UNLOCK;
		
	return result;
}

// seeks to a sample position in the audio stream
// returns true if successful or false if failed
bool ldp_vldp::seek_audio(Uint64 u64Samples)
{
	bool result = false;

	OGG_LOCK;	// can't have audio callback running during this

	if (ov_seekable(&s_ogg))
	{
		ov_pcm_seek(&s_ogg, u64Samples);
		g_audio_playing = false;	// audio should not be playing immediately after a seek
		result = true;
	}
	else
	{
		printline("DOH!  OGG stream is not seekable!");
	}
	
	OGG_UNLOCK;

	return result;
}

// starts playing the audio
void ldp_vldp::audio_play(Uint32 timer)
{
	OGG_LOCK;
	g_playing_timer = timer;
	g_samples_played = 0;
	g_audio_playing = true;
	OGG_UNLOCK;
}

// pauses the audio at the current position
void ldp_vldp::audio_pause()
{
	OGG_LOCK;
	g_audio_playing = false;
	OGG_UNLOCK;
}

////////////////////////////////////////////////////////////////////////////////////////

char g_small_buf[AUDIO_BUF_CHUNK] = { 0 };
Uint8 g_leftover_buf[AUDIO_BUF_CHUNK] = { 0 };
int g_leftover_samples = 0;

// our audio callback
void ldp_vldp_audio_callback(Uint8 *stream, int len, int unused)
{
#ifdef AUDIO_DEBUG
	g_u64CallbackByteCount += len;
	unsigned int uFloodTimer = (GET_TICKS() - g_uCallbackDbgTimer) / 1000;
	if (uFloodTimer != g_uCallbackFloodTimer)
	{
		g_uCallbackFloodTimer = uFloodTimer;
		string s = "audio callback frequency is: " + numstr::ToStr((g_u64CallbackByteCount / uFloodTimer) >> 2);
		printline(s.c_str());
	}
#endif

	OGG_LOCK;	// make sure nothing changes with any ogg stuff while we decode

	// if audio is ready to be read and if it is playing
	if (g_audio_ready && g_audio_playing)
	{
		bool audio_caught_up = false;
		int loop_count = 0;

		// normally we only want to go through this loop once
		// The exception is if we are behind, in which case we want to process audio until we're caught up again
		// we don't want to loop endlessly in here if there is a bug, which is why we have a loop count
		while ((!audio_caught_up) && (loop_count++ < 10))
		{
			long samples_read = 0;
			int samples_copied = 0;
			Uint32 bytes_to_read = 0;
			Uint32 correct_samples = 0;	// how many samples we should have played up to this point
			int nop;
			
			// if we have some samples from last time for the audio stream
			if (g_leftover_samples)
			{
				if (g_leftover_samples <= len)
				{
					paudiocopy(stream, g_leftover_buf, g_leftover_samples);
					samples_copied += g_leftover_samples;
					g_leftover_samples = 0;
				}
				else
				{
					paudiocopy(stream, g_leftover_buf, len);
					memmove(g_leftover_buf, g_leftover_buf + len, g_leftover_samples - len);	// shift remaining buf to front
					// memmove is used because the memory area overlaps
					samples_copied = len;
					g_leftover_samples -= len;
				}
			}

			while (samples_copied < len)
			{
#ifndef GP2X
				samples_read = ov_read(&s_ogg, &g_small_buf[0],
					AUDIO_BUF_CHUNK,0,2,1, &nop);
#else
				// gp2x version
				samples_read = ov_read(&s_ogg, &g_small_buf[0], AUDIO_BUF_CHUNK, &nop);
#endif

				if (samples_read > 0)
				{
					bytes_to_read = len - samples_copied;	// how much space we have left to fill
					// (samples_copied can and often is 0)
				
					// if we have more space to fill than samples available, then we only want to read
					// as many samples as we have available
					if (bytes_to_read > (Uint32) samples_read)
					{
						bytes_to_read = samples_read;
					}
					// else we have to split the buffer
					else
					{
						g_leftover_samples = samples_read - bytes_to_read;
						memcpy(g_leftover_buf, g_small_buf + bytes_to_read, g_leftover_samples);
					}

					paudiocopy(stream + samples_copied, g_small_buf, bytes_to_read);
					samples_copied += bytes_to_read;
				} // end if samples were read

				// if we got an error
				else if (samples_read < 0)
				{
					printline("Problem reading samples!");
					g_audio_playing = false;
					break;
				}

				// else, samples_read == 0 in which case we've come to the end of the stream
				else
				{
					printline("End of audio stream detected!");
					g_audio_playing = false;
					break;
				}

			} // end while we have not filled the buffer

			// NOW WE CHECK TO SEE IF THE AUDIO IS LAGGING TOO FAR BEHIND
			// IF IT IS, WE NEED TO SKIP FORWARD

			g_samples_played += len;	// update stats on how many samples have played so we can make sure audio is in sync

			//unsigned int cur_time = refresh_ms_time();
			unsigned int cur_time = g_ldp->get_elapsed_ms_since_play();
			// if our timer is set to the current time or some previous time
			if (g_playing_timer < cur_time)
			{
				static const Uint64 uBYTES_PER_S = AUDIO_FREQ * AUDIO_BYTES_PER_SAMPLE;	// needs to be uint64 to prevent overflow from subsequent math
				correct_samples = (unsigned int) ((uBYTES_PER_S * (cur_time - g_playing_timer)) / 1000);
				// how many samples should have played
				// 176.4 = 44.1 samples per millisecond * 2 for stereo * 2 for 16-bit
			}
			// our timer is set to some time in the future (used with skipping) so we actually
			// should not have played any samples at this point
			else
			{
//				fprintf(stderr, "LDP-VLDP-AUDIO : Timer is in the future\n");
				correct_samples = 0;
			}
		
			// if we're ahead instead of behind, don't loop
			if (correct_samples <= g_samples_played)
			{
				audio_caught_up = true;
				
				/*
				// warn user if we're too far ahead (this should never happen)
				if ((g_samples_played - correct_samples) > len)
				{
					string s = "ldp-vldp-audio callback: audio is too far ahead! played: " +
						numstr::ToStr(g_samples_played) + ", expected: " + numstr::ToStr(correct_samples);
					printline(s.c_str());
				}
				*/
			}

			// if we're not too far behind, don't loop
			else if ((Sint32) (correct_samples - g_samples_played) < len)
			{
				audio_caught_up = true;
			}

			// if we're too far behind, notify the user for testing purposes
			else
			{
				/*
				char s[160];
				sprintf(s, "AUDIO : played %u, expected %u, timer=%u, curtime=%u", g_samples_played, correct_samples, g_playing_timer, g_ldp->get_elapsed_ms_since_play());
				printline(s);
				*/
				audio_caught_up = false;
				SDL_Delay(0);	// don't starve other processes while trying to catch up
			}
		} // end while we're not caught up

	} // end if audio is playing

	// Either we have no audio file opened OR
	// disc is not playing, pause audio and make sure we don't have any extraneous audio lingering around when
	// we start playing again
	else
	{
		// fill audio stream with silence since it will be expecting to get something back from us
#ifdef WIN32
		ZeroMemory(stream, len);
#else
		bzero(stream, len);
#endif

		g_leftover_samples = 0;
	}

	OGG_UNLOCK;

}
