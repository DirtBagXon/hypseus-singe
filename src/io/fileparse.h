/*
 * fileparse.h
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

#include <stdio.h>
#include "my_stdio.h"
#include <string>

using namespace std;

int read_line(mpo_io *io, string &buf);
int read_line(FILE *F, char *buf, int buf_size);
int read_line(mpo_io *io, char *buf, int buf_size);

// reads line from a buffer (in_buf), returns the line in 'line'
// Returns a pointer to the first character in the next line, or NULL if there is no next line
const char *read_line(const char *in_buf, string &line);

bool get_path_of_file(string file_with_path, string &path);
bool find_word(const char *src_buf, string &word, string &remaining_line);
bool find_word(const char *src_buf, const char **word_begin, int *word_length);
bool my_is_whitespace(char ch);
