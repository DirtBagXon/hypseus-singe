/*
 * conout.h
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

// Conout.h header
// By Matt Ownby

#ifndef CONOUT_H
#define CONOUT_H

#define LOGNAME "daphne_log.txt"	// name of our logfile if we are using one

void outstr(const char *s);
void outchr(const char ch);
void printline(const char *s);
void newline();
void con_flush();
void noflood_printline(char *s);
void safe_itoa(int num, char *a, int sizeof_a);

// enables/disables log from being written to disk
void set_log_enabled(bool val);

void addlog(const char *s);

#endif
