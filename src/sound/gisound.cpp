/*
* gisound.cpp
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
// an attempt at a portable AY-3-8910 emulator
// We assume that there are 4 bytes per sample

#include "sound.h"
#include "gisound.h"
#include "../io/conout.h"
#include <memory.h>

#define MAX_GISOUND_CHIPS 4
int g_gisoundchip_count = -1;

gi_sound_chip *g_gi_chips[MAX_GISOUND_CHIPS] = { NULL };
Sint16 g_volumetable[16];

int gisound_initialize(Uint32 core_frequency)
{
   char s[81] = {0};
   sprintf(s, "GI Sound chip initialized at %d Hz", core_frequency);
   printline(s);
   g_gi_chips[++g_gisoundchip_count] = new gi_sound_chip;
   memset(g_gi_chips[g_gisoundchip_count],0,sizeof(gi_sound_chip));
   g_gi_chips[g_gisoundchip_count]->core_clock = core_frequency;
   g_gi_chips[g_gisoundchip_count]->chan_a_bytes_per_switch = 4;
	g_gi_chips[g_gisoundchip_count]->chan_a_bytes_to_go = 0;
   g_gi_chips[g_gisoundchip_count]->chan_a_flip = 1;
   g_gi_chips[g_gisoundchip_count]->chan_b_bytes_per_switch = 4;
	g_gi_chips[g_gisoundchip_count]->chan_b_bytes_to_go = 0;
   g_gi_chips[g_gisoundchip_count]->chan_b_flip = 1;
   g_gi_chips[g_gisoundchip_count]->chan_c_bytes_per_switch = 4;
	g_gi_chips[g_gisoundchip_count]->chan_c_bytes_to_go = 0;
   g_gi_chips[g_gisoundchip_count]->chan_c_flip = 1;
   g_gi_chips[g_gisoundchip_count]->noise_bytes_per_switch = 4;
   g_gi_chips[g_gisoundchip_count]->noise_flip = 1;
   g_gi_chips[g_gisoundchip_count]->random_seed = 0;
   g_gi_chips[g_gisoundchip_count]->envelope_period = 4;

   int i = 0;

   // make my table
   double dReduction = 1.0;

   // the .707 series
   for (i = 15; i > 13; i--)
   {
       g_volumetable[i] = (short) ((0x7FFF * dReduction) + 0.5);
       dReduction *= 0.70710678118654752440084436210485;    // square root of 1/2
   }

   // it's different the rest of the way
   for (i = 13; i > 0; i -= 2)
   {
       g_volumetable[i] = (Sint16)((g_volumetable[i+2] * 0.5) + 0.5);
       g_volumetable[i-1] = (Sint16)((g_volumetable[i] * 0.606) + 0.5);
   }

   g_volumetable[0] = 0;


   return g_gisoundchip_count;
}

void gisound_writedata(Uint32 address, Uint32 data, int index)
{
   Uint16 chan_a_tone_period;
   Uint16 chan_b_tone_period;
   Uint16 chan_c_tone_period;
   g_gi_chips[index]->register_set[address] = data;
   int old_bytes_per_switch = 0;
//	char s[81] = {0};
	bool old_tone_a, old_tone_b, old_tone_c;

   switch (address)
   {
   case CHANNEL_A_TONE_PERIOD_FINE:
   case CHANNEL_A_TONE_PERIOD_COARSE:
      // fine adjustment is bottom 8 bits of 12 bit total value
      // coarse adjustment is top 4 bits of 12 bit total value
      chan_a_tone_period = g_gi_chips[index]->register_set[CHANNEL_A_TONE_PERIOD_FINE] | 
         (g_gi_chips[index]->register_set[CHANNEL_A_TONE_PERIOD_COARSE] << 8);
      old_bytes_per_switch = g_gi_chips[index]->chan_a_bytes_per_switch;
      g_gi_chips[index]->chan_a_bytes_per_switch = (int)((AUDIO_FREQ * (chan_a_tone_period * 16.0) 
         / g_gi_chips[index]->core_clock * 2) + .5);
      if (g_gi_chips[index]->chan_a_bytes_per_switch < 4)
      {
         g_gi_chips[index]->chan_a_bytes_per_switch = 4;
      }
      g_gi_chips[index]->chan_a_bytes_to_go += g_gi_chips[index]->chan_a_bytes_per_switch - old_bytes_per_switch;
      break;
   
   case CHANNEL_B_TONE_PERIOD_FINE:
   case CHANNEL_B_TONE_PERIOD_COARSE:
      // fine adjustment is bottom 8 bits of 12 bit total value
      // coarse adjustment is top 4 bits of 12 bit total value
      chan_b_tone_period = g_gi_chips[index]->register_set[CHANNEL_B_TONE_PERIOD_FINE] | 
         (g_gi_chips[index]->register_set[CHANNEL_B_TONE_PERIOD_COARSE] << 8);
      old_bytes_per_switch = g_gi_chips[index]->chan_b_bytes_per_switch;
      g_gi_chips[index]->chan_b_bytes_per_switch = (int)((AUDIO_FREQ * (chan_b_tone_period * 16.0) 
         / g_gi_chips[index]->core_clock * 2) + .5);
      if (g_gi_chips[index]->chan_b_bytes_per_switch < 4)
      {
         g_gi_chips[index]->chan_b_bytes_per_switch = 4;
      }
      g_gi_chips[index]->chan_b_bytes_to_go += g_gi_chips[index]->chan_b_bytes_per_switch - old_bytes_per_switch;
      break;
   
   case CHANNEL_C_TONE_PERIOD_FINE:
   case CHANNEL_C_TONE_PERIOD_COARSE:
      // fine adjustment is bottom 8 bits of 12 bit total value
      // coarse adjustment is top 4 bits of 12 bit total value
      chan_c_tone_period = g_gi_chips[index]->register_set[CHANNEL_C_TONE_PERIOD_FINE] | 
         (g_gi_chips[index]->register_set[CHANNEL_C_TONE_PERIOD_COARSE] << 8);
      old_bytes_per_switch = g_gi_chips[index]->chan_c_bytes_per_switch;
      g_gi_chips[index]->chan_c_bytes_per_switch = (int)((AUDIO_FREQ * (chan_c_tone_period * 16.0) 
         / g_gi_chips[index]->core_clock * 2) + .5);
      if (g_gi_chips[index]->chan_c_bytes_per_switch < 4)
      {
         g_gi_chips[index]->chan_c_bytes_per_switch = 4;
      }
      g_gi_chips[index]->chan_c_bytes_to_go += g_gi_chips[index]->chan_c_bytes_per_switch - old_bytes_per_switch;
      break;
   
   case NOISE_PERIOD:
      // noise period is 5 bits
      g_gi_chips[index]->noise_period = data & 0x1f;
      old_bytes_per_switch = g_gi_chips[index]->noise_bytes_per_switch;
      g_gi_chips[index]->noise_bytes_per_switch = (int)((AUDIO_FREQ * (g_gi_chips[index]->noise_period * 16.0) 
         / g_gi_chips[index]->core_clock * 2) + .5);
      if (g_gi_chips[index]->noise_bytes_per_switch < 4)
      {
         g_gi_chips[index]->noise_bytes_per_switch = 4;
      }
      g_gi_chips[index]->noise_bytes_to_go += g_gi_chips[index]->noise_bytes_per_switch - old_bytes_per_switch;
		g_gi_chips[g_gisoundchip_count]->noise_flip = 1;
      break;

   case ENABLE:
	
		old_tone_a = g_gi_chips[index]->tone_a;
		old_tone_b = g_gi_chips[index]->tone_a;
		old_tone_c = g_gi_chips[index]->tone_a;

		// ENABLE is active low
		g_gi_chips[index]->iob_in  = (Uint8)(~(data >> 7)) & 0x01;
		g_gi_chips[index]->ioa_in  = (Uint8)(~(data >> 6)) & 0x01;
		g_gi_chips[index]->noise_c = (Uint8)(~(data >> 5)) & 0x01;
		g_gi_chips[index]->noise_b = (Uint8)(~(data >> 4)) & 0x01;
		g_gi_chips[index]->noise_a = (Uint8)(~(data >> 3)) & 0x01;
		g_gi_chips[index]->tone_c  = (Uint8)(~(data >> TONE_C_ENABLE)) & 0x01;
		g_gi_chips[index]->tone_b  = (Uint8)(~(data >> TONE_B_ENABLE)) & 0x01;
		g_gi_chips[index]->tone_a  = (Uint8)(~(data >> TONE_A_ENABLE)) & 0x01;
		
		// if these were just enabled reset the counters
		if (g_gi_chips[index]->tone_a && !old_tone_a)
		{
         g_gi_chips[g_gisoundchip_count]->chan_a_flip = 1;
			g_gi_chips[index]->chan_a_bytes_to_go = g_gi_chips[index]->chan_a_bytes_per_switch;
		}
		if (g_gi_chips[index]->tone_b && !old_tone_b)
		{
         g_gi_chips[g_gisoundchip_count]->chan_b_flip = 1;
			g_gi_chips[index]->chan_b_bytes_to_go = g_gi_chips[index]->chan_b_bytes_per_switch;
		}
		if (g_gi_chips[index]->tone_c && !old_tone_c)
		{
         g_gi_chips[g_gisoundchip_count]->chan_c_flip = 1;
			g_gi_chips[index]->chan_c_bytes_to_go = g_gi_chips[index]->chan_c_bytes_per_switch;
		}
		break;
      
   case CHANNEL_A_AMPLITUDE:
      // bits 0-3 are the fixed amplitude level
      // bit 4 is the amplitude level mode
      g_gi_chips[index]->chan_a_amplitude_mode = (data >> 4) & 0x01;
      if (!g_gi_chips[index]->chan_a_amplitude_mode)
      {
         g_gi_chips[index]->chan_a_amplitude = data & 0x0f;
      }
      break;

   case CHANNEL_B_AMPLITUDE:
      // bits 0-3 are the fixed amplitude level
      // bit 4 is the amplitude level mode
      g_gi_chips[index]->chan_b_amplitude_mode = (data >> 4) & 0x01;
      if (!g_gi_chips[index]->chan_b_amplitude_mode)
      {
         g_gi_chips[index]->chan_b_amplitude = data & 0x0f;
      }
		break;

   case CHANNEL_C_AMPLITUDE:
      // bits 0-3 are the fixed amplitude level
      // bit 4 is the amplitude level mode
      g_gi_chips[index]->chan_c_amplitude_mode = (data >> 4) & 0x01;
      if (!g_gi_chips[index]->chan_c_amplitude_mode)
      {
         g_gi_chips[index]->chan_c_amplitude = data & 0x0f;
      }
		break;

   case ENVELOPE_PERIOD_FINE:
   case ENVELOPE_PERIOD_COARSE:
      // Envelope Period is a 16 bit number made up of COURSE<<8|FINE
      g_gi_chips[index]->envelope_period = (int)((AUDIO_FREQ * 256.0 * (g_gi_chips[index]->register_set[ENVELOPE_PERIOD_FINE] 
         | (g_gi_chips[index]->register_set[ENVELOPE_PERIOD_COARSE] << 8)) / g_gi_chips[index]->core_clock / 4) + 0.5);
      // if Envelope Period is set to 0 then it is 1/2 the Envelope Period of 1
      if (g_gi_chips[index]->envelope_period < 4)
      {
         g_gi_chips[index]->envelope_period = 4;
      }
      g_gi_chips[index]->envelope_cycle_complete = false;
		g_gi_chips[index]->envelope_bytes_to_go = g_gi_chips[index]->envelope_period;
		g_gi_chips[index]->envelope_step = 0;
      break;

   case ENVELOPE_SHAPE_CYCLE:
      g_gi_chips[index]->envelope_shape_cycle_cont = (data >> 3) & 0x01;
      g_gi_chips[index]->envelope_shape_cycle_att  = (data >> 2) & 0x01;
      g_gi_chips[index]->envelope_shape_cycle_alt  = (data >> 1) & 0x01;
      g_gi_chips[index]->envelope_shape_cycle_hold = (data >> 0) & 0x01;      
      break;
   
   case IO_PORT_A_DATA_STORE:
      g_gi_chips[index]->port_a_data_store = data;
      break;

   case IO_PORT_B_DATA_STORE:
      g_gi_chips[index]->port_b_data_store = data;
      break;
   }
}

void gisound_stream(Uint8* stream, int length, int index)
{
	for (int pos = 0; pos < length; pos += 4)
	{
		// endian-independent! :)
		// NOTE : assumes stream is in little endian format
		Sint16 sample = 
			(g_volumetable[g_gi_chips[index]->chan_a_amplitude] * 
			((g_gi_chips[index]->tone_a?g_gi_chips[index]->chan_a_flip:1) 
			+ (g_gi_chips[index]->noise_a?g_gi_chips[index]->noise_flip:1)) / 2 +
			g_volumetable[g_gi_chips[index]->chan_b_amplitude] * 
			((g_gi_chips[index]->tone_b?g_gi_chips[index]->chan_b_flip:1) 
			+ (g_gi_chips[index]->noise_b?g_gi_chips[index]->noise_flip:1)) / 2 +
			g_volumetable[g_gi_chips[index]->chan_c_amplitude] * 
			((g_gi_chips[index]->tone_c?g_gi_chips[index]->chan_c_flip:1) 
			+ (g_gi_chips[index]->noise_c?g_gi_chips[index]->noise_flip:1)) / 2) / 3;

		stream[pos] = stream[pos+2] = (Uint16) (sample) & 0xff; 
		stream[pos+1] = stream[pos+3] = ((Uint16) (sample) >> 8) & 0xff; 

		g_gi_chips[index]->chan_a_bytes_to_go -= 4;
		g_gi_chips[index]->chan_b_bytes_to_go -= 4;
		g_gi_chips[index]->chan_c_bytes_to_go -= 4;
		g_gi_chips[index]->noise_bytes_to_go -= 4;
		g_gi_chips[index]->envelope_bytes_to_go -= 4;
         
		// update channel A if it needs it
		if (g_gi_chips[index]->chan_a_bytes_to_go <= 0)
		{
			g_gi_chips[index]->chan_a_bytes_to_go = g_gi_chips[index]->chan_a_bytes_per_switch
				+ g_gi_chips[index]->chan_a_bytes_to_go;
			g_gi_chips[index]->chan_a_flip = -g_gi_chips[index]->chan_a_flip;
		}
		// update channel B if it needs it
		if (g_gi_chips[index]->chan_b_bytes_to_go <= 0)
		{
			g_gi_chips[index]->chan_b_bytes_to_go = g_gi_chips[index]->chan_b_bytes_per_switch
				+ g_gi_chips[index]->chan_b_bytes_to_go;
			g_gi_chips[index]->chan_b_flip = -g_gi_chips[index]->chan_b_flip;
		}
		// update channel C if it needs it
		if (g_gi_chips[index]->chan_c_bytes_to_go <= 0)
		{
			g_gi_chips[index]->chan_c_bytes_to_go = g_gi_chips[index]->chan_c_bytes_per_switch
				+ g_gi_chips[index]->chan_c_bytes_to_go;
			g_gi_chips[index]->chan_c_flip = -g_gi_chips[index]->chan_c_flip;
		}
		// update noise if it needs it
		if (g_gi_chips[index]->noise_bytes_to_go <= 0)
		{
			g_gi_chips[index]->noise_bytes_to_go = g_gi_chips[index]->noise_bytes_per_switch
				+ g_gi_chips[index]->noise_bytes_to_go;
			// the random number generator is a 17 bit shift register with the output as bit 0, and the input is 
			// not (bit 0 xor bit 3)
			g_gi_chips[index]->random_seed = (g_gi_chips[index]->random_seed >> 1)
				| ((~(g_gi_chips[index]->random_seed ^ (g_gi_chips[index]->random_seed >> 3)) & 0x01) << 16);
//			sprintf(s,"Random number %d", g_gi_chips[index]->random_number);
//			printline(s);

			if (g_gi_chips[index]->random_seed & 0x01)
			{
				g_gi_chips[index]->noise_flip = -g_gi_chips[index]->noise_flip;
			}
		}
		// update envelope if it needs it
		if (g_gi_chips[index]->envelope_bytes_to_go <= 0)
		{
			if (!g_gi_chips[index]->envelope_shape_cycle_cont && g_gi_chips[index]->envelope_cycle_complete)
			{
				g_gi_chips[index]->envelope_amplitude = 0; // always hold it low after a cycle if !cont
			}
			else if (g_gi_chips[index]->envelope_shape_cycle_hold && g_gi_chips[index]->envelope_cycle_complete)
			{               
				// don't do anything (hold it) if hold and the cycle is complete
				if (g_gi_chips[index]->envelope_shape_cycle_alt)
				{
					g_gi_chips[index]->envelope_amplitude = g_gi_chips[index]->envelope_shape_cycle_att?0:15;
				}
			}
			else if (g_gi_chips[index]->envelope_shape_cycle_alt && g_gi_chips[index]->envelope_cycle_complete)
			{
				g_gi_chips[index]->envelope_amplitude = (!g_gi_chips[index]->envelope_shape_cycle_att?
					g_gi_chips[index]->envelope_step:15 - g_gi_chips[index]->envelope_step);
			}
			else
			{
				g_gi_chips[index]->envelope_amplitude = (g_gi_chips[index]->envelope_shape_cycle_att?
					g_gi_chips[index]->envelope_step:15 - g_gi_chips[index]->envelope_step);
			}
			// update the volumes
			if (g_gi_chips[index]->chan_a_amplitude_mode) 
			{
				g_gi_chips[index]->chan_a_amplitude = g_gi_chips[index]->envelope_amplitude;
			}
			if (g_gi_chips[index]->chan_b_amplitude_mode)
			{
				g_gi_chips[index]->chan_b_amplitude = g_gi_chips[index]->envelope_amplitude;
			}
			if (g_gi_chips[index]->chan_c_amplitude_mode)
			{
				g_gi_chips[index]->chan_c_amplitude = g_gi_chips[index]->envelope_amplitude;
			}
			g_gi_chips[index]->envelope_bytes_to_go = g_gi_chips[index]->envelope_period 
				+ g_gi_chips[index]->envelope_bytes_to_go;            
			g_gi_chips[index]->envelope_step++;
            
			if (g_gi_chips[index]->envelope_step > 15)
			{
				g_gi_chips[index]->envelope_step = 0;
				if (g_gi_chips[index]->envelope_cycle_complete && g_gi_chips[index]->envelope_shape_cycle_alt
					&& g_gi_chips[index]->envelope_shape_cycle_cont && !g_gi_chips[index]->envelope_shape_cycle_hold)
				{
					g_gi_chips[index]->envelope_cycle_complete = false;
				}
				else
				{
					g_gi_chips[index]->envelope_cycle_complete = true;
				}
			}         
		}
	}
}

void gisound_shutdown(int index)
{
	delete g_gi_chips[index];
	g_gi_chips[index] = NULL;
}
