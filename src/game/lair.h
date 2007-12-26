/*
 * lair.h
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

// lair.h

#ifndef LAIR_H
#define LAIR_H

#include "game.h"
#include "../scoreboard/scoreboard_collection.h"

#define LAIR_IRQ_PERIOD 32.768	// # of ms of IRQ period
#define LAIR_CPU_HZ 4000000		// speed of cpu

enum { S_DL_CREDIT, S_DL_ACCEPT, S_DL_BUZZ };

class lair : public game
{
public:
	lair();
	bool init();
	void shutdown();
	void do_irq(unsigned int);
	void do_nmi();
	Uint8 cpu_mem_read(Uint16 addr);
	void cpu_mem_write(Uint16 addr, Uint8 value);
	void input_enable(Uint8);
	void input_disable(Uint8);
	void OnVblank();
	void OnLDV1000LineChange(bool bIsStatus, bool bIsEnabled);
	void video_repaint();
    void init_overlay_scoreboard();
	void palette_calculate();
	void set_preset(int);
	bool set_bank(unsigned char, unsigned char);
	void set_version(int);

	// what follows are functions specific to this class
	Uint8 read_C010();
	void patch_roms();

protected:
   Uint8 m_soundchip_id;
   Uint8 m_soundchip_address_latch;
	Uint64 m_status_strobe_timer;			// indicates when the last status strobe occurred
	unsigned char m_switchA;			// dip switch bank A (enabled low)
	unsigned char m_switchB;			// dip switch bank B (enabled low)
	unsigned char m_joyskill_val;		// joystick and space ace skill (enabled low)
	unsigned char m_misc_val;			// player1/2 buttons, coin insertion, LD-V1000 strobe (enabled low)

	bool m_uses_pr7820;		// enables emulation of an older PR-7820 player instead of an LD-V1000
	bool m_leds_cleared; 		// minor hack for real SA annunciator boards

	// whether scoreboard is currently visible or not
	bool m_bScoreboardVisibility;

	// whether we're using the annunciator board for Space Ace
	bool m_bUseAnnunciator;

	// the scoreboard instance that we interract with
	IScoreboard *m_pScoreboard;
};

// dragon's lair enhanced roms v1.1
class dle11 : public lair
{
public:
	dle11();
	void patch_roms();
};

// dragon's lair enhanced roms v2.0
class dle2 : public lair
{
public:
	dle2();
	void patch_roms();
	void set_version(int);
};

// Space Ace rev A3
class ace : public lair
{
public:
	ace();
	void set_version(int);
	void patch_roms();
	bool handle_cmdline_arg(const char *arg);
};

// Space Ace enhanced
// (I inherit from the ace class so we can use the annunciator)
class sae : public ace
{
public:
	sae();
	void patch_roms();
};

// old DL revisions w/PR-7820 player
class lairalt : public lair
{
public:
	lairalt();
	void set_preset(int);
	void set_version(int);
};

#endif
