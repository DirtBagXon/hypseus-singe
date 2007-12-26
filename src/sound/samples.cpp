// samples.cpp

#include "../game/game.h"	// to get sound names
#include "../io/conout.h"
#include "../io/mpo_mem.h"	// for endian-independent macros
#include <string.h>	// for memset
#include "samples.h"

#ifdef DEBUG
#include <assert.h>
#endif

//using namespace std;

struct sample_data_s
{
	// holds audio buffer
	Uint8 *pu8Buf;

	// holds audio length
	Uint32 uLength;

	// how many channels (1=mono, 2=stereo) this sample has
	unsigned int uChannels;

	// position of sample if playing
	unsigned int uPos;

	// whether this dynamic sample is active (playing) or not
	bool bActive;

	// if this is not NULL, it will get called when the sample has finished playing
	void (*finishedCallback)(Uint8 *pu8Buf, unsigned int uSampleIdx);
};

// this number can be any size, but there's no reason to make it huge
const unsigned int MAX_DYNAMIC_SAMPLES = 8;

struct sample_data_s g_SampleStates[MAX_DYNAMIC_SAMPLES];

// so that we don't need to scan through to find a free slot in the dynamic samples array
unsigned int g_uNextSampleIdx = 0;

// init callback
int samples_init(unsigned int unused)
{
	int iResult = 0;
	unsigned int u = 0;

	// make sure these are all initialized
	for (u = 0; u < MAX_DYNAMIC_SAMPLES; ++u)
	{
		sample_data_s *s = &g_SampleStates[u];
		s->pu8Buf = NULL;
		s->uLength = 0;
		s->uPos = 0;
		s->uChannels = 0;
		s->bActive = false;
		s->finishedCallback = NULL;
	}

	return iResult;
}

void samples_shutdown(int unused)
{
}

// called from sound mixer to get audio stream
void samples_get_stream(Uint8 *stream, int length, int unused)
{
#ifdef DEBUG
	assert (length % 4 == 0);	// make sure it's divisible by 4
#endif

	// (each sample is 4 bytes, which is why we divide length by 4)
	unsigned int uTotalSamples = length >> 2;

	// clear buffer so that our addition will work
	memset(stream, 0, length);

	unsigned int u = 0;

	// check to see if any sample is playing ...
	for (u = 0; u < MAX_DYNAMIC_SAMPLES; ++u)
	{
		sample_data_s *data = &g_SampleStates[u];

		if (data->bActive)
		{
			Uint8 *ptrStream = stream;

			// do each sample
			for (unsigned int uSample = 0; uSample < uTotalSamples; ++uSample)
			{
				// if there is still some sample data to be mixed to this stream
				if (data->uPos < data->uLength)
				{
					int iMixedSample1 = LOAD_LIL_SINT16((Sint16 *) ptrStream);
					int iMixedSample2 = LOAD_LIL_SINT16(((Sint16 *) ptrStream) + 1);

					Sint16 i16Sample1 = LOAD_LIL_SINT16(data->pu8Buf + data->uPos);
					iMixedSample1 += i16Sample1;
					data->uPos += 2;

					// if this is a stereo sample
					if (data->uChannels == 2)
					{
						iMixedSample2 += LOAD_LIL_SINT16(data->pu8Buf + data->uPos);
						data->uPos += 2;
					}
					// else this is a mono sample, so just duplicate the last sample we read in
					else
					{
						iMixedSample2 += i16Sample1;
					}

					DO_CLIP(iMixedSample1);	// prevent overflow
					DO_CLIP(iMixedSample2);

					// yes, sample2 should be the most significant, sample1 least significant.. releasetest tests this
					unsigned int val_to_store = (((Uint16) iMixedSample2) << 16) | (Uint16) iMixedSample1;
					STORE_LIL_UINT32(ptrStream, val_to_store);
					ptrStream += 4;
				}
				// else this sample is done, so get rid of the entry ...
				else
				{
					data->bActive = false;

					// if caller has requested to be notified when this sample is done ...
					if (data->finishedCallback != NULL)
					{
						data->finishedCallback(data->pu8Buf, u);
					}
					break;
				}
			} // end going through stream buffer
		} // end while we have states to be addressed
	} // end looping through all sample slots
}

int samples_play_sample(Uint8 *pu8Buf, unsigned int uLength, unsigned int uChannels, int iSlot,
								 void (*finishedCallback)(Uint8 *pu8Buf, unsigned int uSlot))
{
	int iResult = -1;
	sample_data_s *state = NULL;

	// range check
	if ((uChannels == 1) || (uChannels == 2))
	{
		// if we should automatically find a free slot
		if (iSlot < 0)
		{
			unsigned int uStartIdx = g_uNextSampleIdx;

			while (state == NULL)
			{
				state = &g_SampleStates[g_uNextSampleIdx];
				iResult = g_uNextSampleIdx;	// assume this slot is ok until proven otherwise

				// if this sample is currently active, we can't use it, and must move on ...
				if (state->bActive)
				{
					state = NULL;
				}

				++g_uNextSampleIdx;

				// wraparound
				if (g_uNextSampleIdx >= MAX_DYNAMIC_SAMPLES)
				{
					g_uNextSampleIdx = 0;
				}

				// If we've come full circle, break out of the loop and see if we found
				//  an available slot
				if (g_uNextSampleIdx == uStartIdx)
				{
					break;
				}
			}

			// if state is NULL, it means that there are no available slots
			if (!state)
			{
				iResult = -2;
			}
		}
		// else if iSlot is in range, use it ...
		else if ((unsigned int) iSlot < MAX_DYNAMIC_SAMPLES)
		{
			state = &g_SampleStates[iSlot];
			iResult = iSlot;
		}
		// else iSlot is out of range ...

		// if we found a state that we can modify
		if (state != NULL)
		{
			state->bActive = true;
			state->pu8Buf = pu8Buf;
			state->uLength = uLength;
			state->uChannels = uChannels;
			state->uPos = 0;
			state->finishedCallback = finishedCallback;
		}
		// else there's an error so do nothing ...
	} // end if channels are ok
	// else channels are out of range

	return iResult;
}

bool samples_is_sample_playing(unsigned int uSlot)
{
	bool bResult = false;
	if (uSlot < MAX_DYNAMIC_SAMPLES)
	{
		bResult = g_SampleStates[uSlot].bActive;
	}
	else
	{
		printline("ERROR: samples_is_sample_playing() was called with an out-of-range parameter");
	}
	return bResult;
}
