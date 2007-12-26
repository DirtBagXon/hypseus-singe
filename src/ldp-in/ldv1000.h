/*
 * ldv1000.h
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


// LDV1000.H

#ifndef LDV1000_H
#define LDV1000_H

unsigned char read_ldv1000();
void write_ldv1000 (unsigned char value);
void pre_display_disable();
void pre_display_enable();
void ldv1000_enable_instant_seeking();
void clear(void);
Uint16 get_buffered_frame(void);
void ldv1000_add_digit(char);
void pre_audio1();
void pre_audio2();

void ldv1000_report_vsync();
bool ldv1000_is_vsync_active();
bool ldv1000_is_status_strobe_active();
bool ldv1000_is_command_strobe_active();

void reset_ldv1000();

// so cobraconv can change this vlaue :)
// (this must be called _before_ reset_ldv1000!)
void ldv1000_set_seconds_per_search(double d);

// for the cpu debugger's benefit
void print_ldv1000_info();

#endif
