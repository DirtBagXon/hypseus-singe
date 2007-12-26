/*
 * tqsynth.h -- A bare-bones phoneme-based voice synthesizer based on
 *              "rsynth" (C) 1994 by Nick Ing-Simmons, which utilized
 *              the Klatt synthesizer (C) 1982 by Dennis H. Klatt.
 *              Rsynth is a "public domain" text-to-speech system assembled
 *              from a variety of sources.
 *
 * I found rsynth as "rsynth-2.0.tar.gz" at:
 *                svr-ftp.eng.cam.ac.uk:/pub/comp.speech/synthesis/ 
 *
 * Copyright (C) 2003 Garry Jordan
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
 *
 */

#include <SDL.h>
#include "sound.h"	// for sample_s definition

void tqsynth_init(int freq, Uint16 format, int channels, long base_F0);
bool audio_get_chunk(int num_samples, short *samples, sample_s *ptrSample);
bool tqsynth_phones_to_wave(char *phonemes, int len, sample_s *ptrSample);
void tqsynth_free_chunk(Uint8 *pu8Buf);
