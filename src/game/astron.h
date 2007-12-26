/*
 * astron.h
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


//astron.h

#ifndef ASTRON_H
#define ASTRON_H

#include "game.h"

#define ASTRON_OVERLAY_W 256	// width of overlay
#define ASTRON_OVERLAY_H 256 // height of overlay

#define ASTRON_COLOR_COUNT 256

#define BASE_VID_MEM_ADDRESS 0xf000
#define BASE_SPRITE_MEM_ADDRESS 0xc000

// START modified Mame code
#define SPR_Y_TOP		0
#define SPR_Y_BOTTOM	1
#define SPR_X_LO		2
#define SPR_X_HI		3
#define SPR_SKIP_LO		4
#define SPR_SKIP_HI		5
#define SPR_GFXOFS_LO	6
#define SPR_GFXOFS_HI	7
// END modified Mame code

enum
{
	S_AB_PLAYER_SHIP, S_AB_PLAYER_FIRE, S_AB_ENEMY_FIRE, S_AB_ALARM1, S_AB_ALARM2, S_AB_ALARM3, S_AB_ALARM4
};

enum
{	
	S_GR_FIRE, S_GR_ENEMY_CANNON, S_GR_ENEMY_MINION, S_GR_ENEMY_ATTACK, S_GR_ALARM1, S_GR_ALARM2, S_GR_ALARM3, S_GR_ALARM4
};

class astron : public game
{
public:
	astron();
	void do_irq(unsigned int);		// does an IRQ tick
	void do_nmi();		// does an NMI tick
	Uint8 cpu_mem_read(Uint16 addr);			// memory read routine
	void cpu_mem_write(Uint16 addr, Uint8 value);		// memory write routine
	Uint8 port_read(Uint16 port);		// read from port
	void port_write(Uint16 port, Uint8 value);		// write to a port
	virtual void input_enable(Uint8);
	virtual void input_disable(Uint8);
	void video_repaint();	// function to repaint video
	void palette_calculate();
	bool set_bank(Uint8, Uint8);
	virtual void write_ldp(Uint8, Uint16);
	virtual Uint8 read_ldp(Uint16);
protected:
	int current_bank;
	void recalc_palette();
	void draw_sprite(int);
	Uint8 rombank[0x8000];	
	Uint8 character[0x1000];	
	Uint8 sprite[0x10000];	
	Uint8 bankprom[0x200];
	Uint8 miscprom[0x240];
	SDL_Color palette_lookup[4096];		// all possible color entries
	Uint8 m_transparent_color;	// which color is to be transparent
	bool palette_modified;		// has our palette been modified?
	bool compress_palette; // can we compress the palette into one 8 bit surface?
	bool used_sprite_color[256];
	Uint8 mapped_tile_color[256];
	Uint8 ldp_output_latch;	// holds data to be sent to the LDV1000
	Uint8 ldp_input_latch;	// holds data that was retrieved from the LDV1000

	Uint8 banks[4];				// astron's banks
		// bank 1 is switches
		// bank 2 is switches
		// bank 3 is dip switch 1
		// bank 4 is dip switch 2
};

class astronh : public astron
{
public:
	astronh();
	void astronh_nmi();		// clocks the 8251
	void do_nmi();		// does an NMI tick
	void write_ldp(Uint8, Uint16);
	Uint8 read_ldp(Uint16);
private:
	void write_8251_data(Uint8);
	Uint8 read_8251_data(void);
	void write_8251_control(Uint8);
	Uint8 read_8251_status(void);
	bool transmit_enable;
	bool recieve_enable;
	void clock_8251(void);
	bool rxrdy;
	bool txrdy;
};

class galaxy : public astronh
{
public:
	galaxy();
};

class galaxyp : public astron
{
public:
	galaxyp();
};

class blazer : public astron
{
public:
	blazer();
};

class cobraab : public astronh
{
public:
	cobraab();
	void input_enable(Uint8);
	void input_disable(Uint8);
	void patch_roms();
};

#endif
