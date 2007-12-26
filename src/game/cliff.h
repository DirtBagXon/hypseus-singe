/*
 * cliff.h
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

// cliff.h

#ifndef CLIFF_H
#define CLIFF_H

#include "game.h"
#include "../ldp-out/ldp.h"	// for FRAME_ARRAY_SIZE definition

#define CLIFF_CPU_HZ	4000000	// speed of cpu
#define CLIFF_NMI_PERIOD (1000.0/60.0)	// 60 Hz (TMS chip interrupt)
#define CLIFF_IRQ_PERIOD (1000.0/29.97)	// # of milliseconds per frame
#define CLIFF_BANK_COUNT 10

enum { S_C_CORRECT, S_C_WRONG, S_C_STARTUP };

// EASY WAY TO SET DIP SWITCHES

// BANK 4
// difficulty settings, AND these together to create difficulty (16 hardest)
#define DIP7_DIFFICULTY8	~8
#define DIP6_DIFFICULTY4	~4
#define DIP5_DIFFICULTY2	~2
#define DIP4_DIFFICULTY1	~1

// BANK 3
#define DIP12_SERVICE	~1
#define DIP13_SWITCHES	~2
#define DIP14_FREEPLAY	~4
#define DIP15_IMMORTALITY	~8
#define DIP16_DISKCHECK	~16
#define DIP17_ATTRACTAUDIOMUTED	~32
#define DIP18_SHORTSCENES	~64
#define DIP19_BUYIN		~128

// BANK 2
#define DIP20_LEFTCOIN1		~1
#define DIP21_LEFTCOIN2		~2
#define DIP22_UNKNOWN		~4	/* Bo Ayers has this set */
#define DIP23_UNKNOWN		~8	/* Bo ayers has this set */
#define DIP24_RIGHTCOIN1	~16
#define DIP25_RIGHTCOIN2	~32

// Coin settings work as follows:
// Settings      Credits   Coin
// OFF OFF          1       1
// ON  OFF          1       2
// OFF ON           1       3 
// ON  ON           1       4

// BANK 1
// Enabling both of these gives you 3 extra lives to what you normally get (3).  Hence 6 lives.
#define DIP28_EXTRALIFE		~1		// lives + 1
#define DIP29_2EXTRALIVES	~2		// lives + 2
#define DIP30_NOHANGINGSCENE	~4
#define DIP32_SCOREOVERLAY	~16		// score and lives over video
#define DIP33_DISPLAYHINTS	~32
#define DIP34_EXTRAHINT		~64		// hints + 1 (default # of death hints are 0)
#define DIP35_2EXTRAHINTS	~128	// hints + 2

////////////////////////////////////////////////

class cliff : public game
{
public:
	cliff();
	void reset();
	void do_irq(unsigned int);		// does an IRQ tick
	void do_nmi();		// does an NMI tick
#ifdef DEBUG
	void cpu_mem_write(Uint16 addr, Uint8 value);		// memory write routine
#endif
	Uint8 port_read(Uint16 port);		// read from port
	void port_write(Uint16 port, Uint8 value);		// write to a port
	void input_enable(Uint8);
	void input_disable(Uint8);
	bool set_bank(unsigned char, unsigned char);
	void palette_calculate();
	void video_repaint();	// function to repaint video
	void patch_roms();

protected:
	char m_frame_str[FRAME_ARRAY_SIZE];	// current frame of laserdisc in string form
	Uint16 m_frame_val;	// current frame of the laserdisc in numerical form (0 means busy/stopped)
	Uint8 m_banks[CLIFF_BANK_COUNT];

private:
	void cliff_do_blip();
	void cliff_set_service_mode(int);
	void cliff_set_test_mode(int);

	unsigned int m_blips; // holds blips for PR8210 commands (1 blip is a bit)
	int m_blips_count;	// position inside pr8210 buffer
	int m_banks_index;

	// used to make sure that a sound that is already playing doesn't get played again
	unsigned int m_uLastSoundIdx;
};

// alternate ROM set for Cliff Hanger
class cliffalt : public cliff
{
public:
	cliffalt();
};

// alternate ROM set for Cliff Hanger (Added by Buzz)
class cliffalt2 : public cliff
{
public:
	cliffalt2();
};

class gtg : public cliff
{
public:
	gtg();
	//bool load_roms();
	Uint8 cpu_mem_read(Uint16 addr); //has memory hack for disc side detection
	void set_preset(int val);  //gets disc side from command line
	void reset();

protected:
	int e1ba_accesscount;  //for tracking frame/chapter reads
 	
private:
	int disc_side;

};

#endif
