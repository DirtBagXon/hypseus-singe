/*
 * thayers.h
 *
 * Copyright (C) 2001 Mark Broadhead
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

#ifndef THAYERS_H
#define THAYERS_H

#include <SDL.h>
#include "game.h"
#include "../scoreboard/scoreboard_collection.h"

#define THAYERS_CPU_HZ	4000000	// speed of cpu

class thayers : public game
{
public:
	thayers();
	bool init();
	void shutdown();
	void set_version(int);
	void do_irq(unsigned int which);
	void do_nmi();		// dummy function to generate timer IRQ
	void thayers_irq();	
	Uint8 cpu_mem_read(Uint16 addr);			// memory read routine
	void cpu_mem_write(Uint16 addr, Uint8 value);		// memory write routine
	Uint8 port_read(Uint16 port);		// read from port
	void port_write(Uint16 port, Uint8 value);		// write to a port
	void process_keydown(SDLKey);
	void process_keyup(SDLKey);
	bool set_bank(unsigned char, unsigned char);
	void palette_calculate();
	void video_repaint();
    void init_overlay_scoreboard();
	void write_scoreboard(Uint8, Uint8, int); // function to decode scoreboard data

	// COP421 interface functions
	void thayers_write_d_port(unsigned char); // Write to D port
	void thayers_write_l_port(unsigned char); // Write L port
	unsigned char thayers_read_l_port(void); // Read L port
	void thayers_write_g_port(unsigned char);
	unsigned char thayers_read_g_port(void); // Read to G I/O port
	void thayers_write_so_bit(unsigned char); // Write to SO
	unsigned char thayers_read_si_bit(void); // Read to SI

    // To turn off speech synthesis (only called from cmdline.cpp)
    void no_speech();

    // Called by ssi263.cpp whenever it has something to say <g>.
    void show_speech_subtitle();

protected:
//	void string_draw(char*, int, int);
	Uint8 coprom[0x400];
	bool key_press;
	Uint8 cop_read_latch;
	Uint8 cop_write_latch;
	Uint8 cop_g_read_latch;
	Uint8 cop_g_write_latch;
	Uint8 m_irq_status;
	Uint8 banks[4];				// thayers's banks
	// bank 1 is Dip Bank A
	// bank 2 is bits 0-3 is Dip Bank B, 4 and 5 Coin 1 and 2, 6 and 7 laserdisc ready	

private:
    // Overlay text control stuff.
    bool m_use_overlay_scoreboard;
    bool m_show_speech_subtitle;
	int m_message_timer;

    // Text-to-speech related vars/methods.
    bool m_use_speech;
    void speech_buffer_cleanup(char *src, char *dst, int len);

	// pointer to our scoreboard interface
	IScoreboard *m_pScoreboard;

	// whether overlay scoreboard is visible or not
	bool m_bScoreboardVisibility;
};

#endif  // THAYERS_H
