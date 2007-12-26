/*
 * vp931.h
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


// VP931.H

#ifndef VP931_H
#define VP931_H

unsigned char read_vp931();
void write_vp931(unsigned char u8Value);

// DAV is active low
bool vp931_is_dav_active();

// DAK is active high
bool vp931_is_dak_active();

// OPRT is apparently always high, but I don't know if it is considered active high or not
// For now I will assume it is active high
bool vp931_is_oprt_active();

// active low
void vp931_change_write_line(bool bActive);

// active low
void vp931_change_read_line(bool bActive);

// active low
void vp931_change_reset_line(bool bActive);

// should be called by game driver for every vsync
void vp931_report_vsync();

void reset_vp931();

// CPU event callback, used internally
void vp931_event_callback(void *dontCare);

#endif
