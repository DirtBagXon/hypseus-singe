/*
 * laireuro.h
 *
 * Copyright (C) 2001-2007 Mark Broadhead
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

#ifndef LAIREURO_H
#define LAIREURO_H

#include "game.h"

#define LAIREURO_CPU_HZ	3579545	// speed of cpu - from schematics

#define LAIREURO_OVERLAY_W 360	// width of overlay
#define LAIREURO_OVERLAY_H 288 // height of overlay
#define LAIREURO_COLOR_COUNT 8 // 8 colors total

// we need our own callback since laireuro uses IM2
Sint32 laireuro_irq_callback(int);

// CTC Stuff
void ctc_init(double, double, double, double, double);
void ctc_write(Uint8, Uint8);
Uint8 ctc_read(Uint8);
void dart_write(bool b, bool command, Uint8 data);
void ctc_update_period(Uint8 channel);

#define COUNTER true
#define TIMER false

struct ctc_channel
{
	double trig; // period of the trigger input
	Uint8 control;
	Uint8 time_const;
	bool load_const;
	bool time;
	bool time_trig;
	bool clk_trig_section;
	Uint16 prescaler;
	bool mode;
	bool interrupt;
};

struct ctc_chip
{
	Uint8 int_vector;
	ctc_channel channels[4];
	double clock; // period of the clock
};

struct dart_chip
{
	Uint8 next_reg;
	Uint8 int_vector;
	bool transmit_int;
	bool ext_int;
};


class laireuro : public game
{
public:
	laireuro();
	void do_irq(Uint32);
	void do_nmi();
	Uint8 cpu_mem_read(Uint16);
	void cpu_mem_write(Uint16, Uint8);
	Uint8 port_read(Uint16);
	void port_write(Uint16, Uint8);
	void input_enable(Uint8);
	void input_disable(Uint8);
	void palette_calculate();
	void video_repaint();
	void set_version(int);
	bool set_bank(Uint8, Uint8);

protected:
	Uint8 m_wt_misc;
	Uint8 m_character[0x2000];	
	SDL_Color m_colors[LAIREURO_COLOR_COUNT];		
	Uint8 m_banks[4];				
};

class aceeuro : public laireuro
{
public:
	aceeuro();
};

#endif
