/*
 * badlands.h
 *
 * Copyright (C) 2001-2005 Mark Broadhead
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

// badlands.h
// by Mark Broadhead

#include "game.h"

#define BADLANDS_CPU_HZ 14318180

#define BADLANDS_OVERLAY_W 320	// width of overlay
#define BADLANDS_OVERLAY_H 256 // height of overlay

#define BADLANDS_COLOR_COUNT 16

#define BADLANDS_GAMMA 1.0	// don't adjust colors 

enum
{
	S_BL_SHOT
};

class badlands : public game
{
public:
	badlands();
	void do_nmi();		// does an NMI tick
	void do_irq(unsigned int);		// does an IRQ/FIRQ tick
	Uint8 cpu_mem_read(Uint16 addr);			// memory read routine
	void cpu_mem_write(Uint16 addr, Uint8 value);		// memory write routine
	void reset();
	void set_preset(int);
	void input_enable(Uint8);
	void input_disable(Uint8);
	bool set_bank(unsigned char, unsigned char);
	void palette_calculate();
	void video_repaint();	// function to repaint video

protected:
	void update_shoot_led(Uint8);
	Uint8 charx_offset;
	Uint8 chary_offset;
	Uint16 char_base;
	Uint8 m_soundchip_id;
   bool shoot_led_overlay;
	bool shoot_led_numlock;
	bool shoot_led;
	bool firq_on;
	bool irq_on;
	bool nmi_on;
	Uint8 character[0x2000];		
	Uint8 color_prom[0x20];
	Uint8 banks[3];				// badlands's banks
		// bank 1 is switches
		// bank 2 is dip switch 1
		// bank 3 is dip switch 2
};

class badlandp : public badlands
{
public:
   badlandp();
	Uint8 cpu_mem_read(Uint16 addr);			// memory read routine
	void cpu_mem_write(Uint16 addr, Uint8 value);		// memory write routine
};

