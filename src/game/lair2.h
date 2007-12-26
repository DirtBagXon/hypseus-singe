/*
 * lair2.h
 *
 * Copyright (C) 2001 Paul Blagay
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

// lair2.h

#ifndef LAIR2_H
#define LAIR2_H

#include "game.h"

#define DL2_OVERLAY_W 320
#define DL2_OVERLAY_H 240

#define LAIR2_IRQ_PERIOD (1000.0/18.2)		// # of ms of IRQ period
#define LAIR2_CPU_HZ 10000000		// speed of cpu

// The # of coin slots that dragon's lair 2 has
#define NUM_COIN_SLOTS 2

// sounds
enum { S_DL2_WARBLE, S_DL2_ERROR, S_DL2_GOOD, S_DL2_BAD, S_DL2_TIC,
	   S_DL2_TOC, S_DL2_COIN1, S_DL2_COIN2, S_DL2_COIN3, S_DL2_COIN4,
	   S_DL2_WARN, S_SA91_TIC, S_SA91_TOC };

// COMPORT STUFF
#define DL2_IER     1       /*      Interrupt Enable        */
#define DL2_IIR     2       /*      Interrupt ID            */
#define DL2_LCR     3       /*      Line control            */
#define DL2_MCR     4       /*      Modem control           */
#define DL2_LSR     5       /*      Line Status             */
#define DL2_MSR     6       /*      Modem Status            */
#define DL2_CTS     0x10
#define DL2_DSR     0x20

// var stuff
enum {
	DL2_SOUND
};

// how many bytes our serial buffer can hold
// (there is no limit to this, but the real hardware probably wasn't expected to hold much)
#define DL2_BUF_SIZE 1024

class lair2 : public game
{
public:
	lair2();
	bool init();
	void do_irq(unsigned int);
	void cpu_mem_write(Uint32 addr, Uint8 value);
	Uint8 port_read(Uint16);
	void port_write(Uint16, Uint8);
	void input_enable(Uint8);
	void input_disable(Uint8);
	bool set_bank(unsigned char, unsigned char);
	void set_version(int);
	bool handle_cmdline_arg(const char *arg);
	void patch_roms();			
	void ldp_fill_buf();				// gets our ldp info and fills DL2 buffer
	void video_repaint();
	void EEPROM_9536_write(Uint8 value);

protected:
	Uint8 ldp_status;					// read val from LDP1000
    Uint8 banks[2];
	Uint16 EEPROM_9536[0x80];

private:
	unsigned char m_u8SerialBuf[DL2_BUF_SIZE];	// buffer of serial data received from laserdisc player
	unsigned int m_uSerialBufSize;	// how many bytes are currently in the buffer

	bool m_serial_int_enabled;	// whether serial port should generate interrupts

	bool m_real_1450;	// whether they have a real LDP-1450 or not
	bool m_sample_trigger;   // for triggering our samples

	// If we're going to try to write directly to memory to give the ROM the serial data,
	//  rather than generate serial IRQ's
	bool m_bSerialHack;

	// ID returned when soundchip is created
	int m_soundchip_id;

	// used only to prevent errors
	Uint8 m_port61_val;

	// holds a record of how many coins are queued up
	unsigned int m_uCoinCount[NUM_COIN_SLOTS];
};

// Space Ace rev A3
class ace91 : public lair2
{
public:
	ace91();
	void set_version(int);
};


#endif
