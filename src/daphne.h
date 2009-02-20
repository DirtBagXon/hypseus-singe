/*
 * daphne.h
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

// daphne.h

#ifndef DAPHNE_H
#define DAPHNE_H

// global definitions ...

// The current version of DAPHNE will now be returned by this function alone.
// I was having problems with VS6's dependency detector (it wasn't detecting when I was changing
//  the version in the .h file).  Another advantage to having it here is not having to change
//  a header file.
const char *get_daphne_version();

unsigned char get_filename(char *s, unsigned char n);
void set_quitflag();
unsigned char get_quitflag();
bool change_dir(const char *pszNewDir);
void set_cur_dir(const char *exe_loc);
int main(int, char **);
void set_unix_signals();
void handle_unix_signals(int);
void set_serial_port(unsigned char i);
unsigned char get_serial_port();
void set_baud_rate(int i);
int get_baud_rate();
void set_search_offset(int i);
int get_search_offset();
void set_file_mask(char *mask);
char *get_file_mask();
unsigned char get_autostart();
unsigned char get_frame_modifier();
void set_frame_modifier(unsigned char value);
void set_scoreboard(unsigned char value);
unsigned char get_scoreboard();
void set_scoreboard_port(unsigned int value);
unsigned int get_scoreboard_port();
void reset_logfile(int argc, char **argv);
void set_scoreboard_text(char sb_text[]);

void set_idleexit(unsigned int value); // added by JFA for -idleexit
unsigned int get_idleexit(); // added by JFA for -idleexit
// added by JFA for -startsilent
void set_startsilent(unsigned char value);
unsigned char get_startsilent();
// end edit

#endif // DAPHNE_H

