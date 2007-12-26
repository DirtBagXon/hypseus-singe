/*
 * ssi263.h
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

// ssi263.h
// by Matt Ownby
// Speech synthesis via rsysnth by Garry Jordan

//#define SSI_REG_DEBUG
//#define SSI_DEBUG

// Buffer set aside in game RAM to hold SSI-263 speech text.
#define SSI_PHRASE_BUF_LEN 256
	
void ssi263_reg0(unsigned char value, Uint8 *irq_status);
void ssi263_reg1(unsigned char value);
void ssi263_reg2(unsigned char value);
void ssi263_reg3(unsigned char value);
void ssi263_reg4(unsigned char value);
bool ssi263_init(bool init_speech);
void ssi263_finished_callback(Uint8 *pu8Buf, unsigned int uSlot);
