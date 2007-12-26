/*
 * interstellar.h
 *
 * Copyright (C) 2002-2005 Mark Broadhead
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


//interstellar.h

#ifndef INTERSTELLAR_H
#define INTERSTELLAR_H

#include "game.h"

#define INTERSTELLAR_OVERLAY_W 256	// width of overlay
#define INTERSTELLAR_OVERLAY_H 256	// height of overlay

#define INTERSTELLAR_COLOR_COUNT 256

#define INTERSTELLAR_CPU_SPEED 3072000

class interstellar : public game
{
public:
	interstellar();
	void do_irq(unsigned int);		// does an IRQ tick
	void do_nmi();		// does an NMI tick
	Uint8 cpu_mem_read(Uint16 addr);			// memory read routine
	void cpu_mem_write(Uint16 addr, Uint8 value);		// memory write routine
	Uint8 port_read(Uint16 port);		// read from port
	void port_write(Uint16 port, Uint8 value);		// write to a port
	void input_enable(Uint8);
	void input_disable(Uint8);
	void palette_calculate();
	void video_repaint();	// function to repaint video
	bool set_bank(Uint8, Uint8);

private:	
	bool m_cpu0_nmi_enable;
	bool m_cpu1_nmi_enable;
	bool m_cpu2_nmi_enable;
	SDL_Color m_background_color;
	Uint8 m_soundchip1_id;
	Uint8 m_soundchip2_id;
   Uint8 character[0x6000];
	Uint8 color_prom[0x300];
	Uint8 banks[3];
	Uint8 m_cpumem2[0x10000]; // memory space for the second z80
	Uint8 m_cpumem3[0x10000]; // memory space for the third z80
	Uint8 cpu_latch1;
	Uint8 cpu_latch2;
	Uint8 sound_latch;
	bool sound_data;
	void draw_8x8(int character_number, int xcoord, int ycoord, int xflip, int yflip, int palette);
	void draw_16x16(int character_number, int xcoord, int ycoord, int xflip, int yflip, int palette);

};

#endif

