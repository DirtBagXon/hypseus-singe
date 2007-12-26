/*
 * fileparse.cpp
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

// fileparse.cpp
// by Matt Ownby

// some basic file-parsing functions which should be safe enough to prevent daphne from ever
// segfaulting, even if the file is totally corrupt

#include "fileparse.h"
#include "conout.h"

// copies into buf everything from the current position in the file until the end of a
//  line and positions the file stream at the beginning of the next line.
// returns how many bytes were copied
int read_line(mpo_io *io, string &buf)
{
	unsigned char ch = 0;
	MPO_BYTES_READ bytes_read = 0;

	buf = "";	// initalize it with an empty string

	// go until we reach EOF (or we reach the end of the line)
	while (!io->eof)
	{
		if (mpo_read(&ch, 1, &bytes_read, io))
		{
			if (bytes_read == 0) break;	// if we hit EOF, then break

			// if we have hit the end of the line
			if ((ch == 10) || (ch == 13))
			{
				// keep reading until we get passed the end of line chars
				while (((ch == 10) || (ch == 13)) && (bytes_read != 0))
				{
					mpo_read(&ch, 1, &bytes_read, io);
				}

				// if we still have more of the file to read in, then move back one character
				// so that we are at the beginning of the next line
				if (bytes_read != 0)
				{
					// if seek was successful
					if (mpo_seek(-1, MPO_SEEK_CUR, io) == true)
					{
					}
					else
					{
						printline("fileparse.cpp : mpo_seek function failed when it shouldn't have");
					}
				}

				break; // we're done
			}

			// if the character is part of the current line then add it
			else
			{
				buf = buf + (char) ch;
			}
		}
		else
		{
			printline("fileparse.cpp ERROR : mpo_read function failed");
			break;
		}
	}

	return buf.length();
}


// copies into buf everything from the current position in the file until the end of a
//  line and positions the file stream at the beginning of the next line.  Contents are also
//  null-terminated.
// Returns the # of bytes copied into the buf.
// buf_size is the total size of the buf to prevent buffer overflow
int read_line(mpo_io *io, char *buf, int buf_size)
{
	int index = 0;
	unsigned char ch = 0;
	MPO_BYTES_READ bytes_read = 0;

	// while we haven't overflowed the buffer
	// (we do -1 to leave space for null character at the end)
	while (index < (buf_size - 1))
	{
		if (mpo_read(&ch, 1, &bytes_read, io))
		{
			if (bytes_read == 0) break;	// if we hit EOF, then break

			// if we have hit the end of the line
			if ((ch == 10) || (ch == 13))
			{
				// keep reading until we get passed the end of line chars
				while (((ch == 10) || (ch == 13)) && (bytes_read != 0))
				{
					mpo_read(&ch, 1, &bytes_read, io);
				}

				// if we still have more of the file to read in, then move back one character
				// so that we are at the beginning of the next line
				if (bytes_read != 0)
				{
					// if seek was successful
					if (mpo_seek(-1, MPO_SEEK_CUR, io) == true)
					{
					}
					else
					{
						printline("fileparse.cpp : mpo_seek function failed when it shouldn't have");
					}
				}

				break; // we're done
			}

			// if the character is part of the current line
			else
			{
				buf[index++] = (unsigned char) ch;
			}
		}
		else
		{
			printline("fileparse.cpp ERROR : mpo_read function failed");
			break;
		}
	}
	buf[index] = 0;

	return (index + 1);	// + 1 to indicate # of bytes actually copied
}

int read_line(FILE *F, char *buf, int buf_size)
{
	int index = 0;
	int ch = 0;

	// while we haven't overflowed the buffer
	// (we do -1 to leave space for null character at the end)
	while (index < (buf_size - 1))
	{
		ch = fgetc(F);

		if (ch == EOF) break;	// if we hit the end of file, then break

		// if we have hit the end of the line
		if ((ch == 10) || (ch == 13))
		{
			// keep reading until we get passed the end of line chars
			while (((ch == 10) || (ch == 13)) && (ch != EOF))
			{
				ch = getc(F);
			}

			// if we still have more of the file to read in, then move back one character
			// so that we are at the beginning of the next line
			if (ch != EOF)
			{
				fseek(F, -1, SEEK_CUR);
			}

			break;
		}

		// if the character is part of the current line
		else
		{
			buf[index++] = (unsigned char) ch;
		}
	}
	buf[index] = 0;

	return (index + 1);	// + 1 to indicate # of bytes actually copied
}


const char *read_line(const char *pszInBuf, string &line)
{
	const char *result = NULL;
	unsigned char ch = 0;
	int idx = 0;

	line = "";	// initalize it with an empty string

	// go until we reach end of the buffer (or end of line)
	while (pszInBuf[idx] != 0)
	{
		ch = pszInBuf[idx];

		// if we have hit the end of the line
		if ((ch == 10) || (ch == 13))
		{
			// keep reading until we get passed the end of line chars
			while (((ch == 10) || (ch == 13)) && (ch != 0))
			{
				++idx;
				ch = pszInBuf[idx];
			}
			
			break;	// we got our line
		}

		// if the character is part of the current line then add it
		else
		{
			line = line + (char) ch;
		}
		++idx;
	}

	// if we aren't at the end of the buffer, return pointer to first character of next line
	if (pszInBuf[idx] != 0) result = pszInBuf + idx;
	
	return result;
}


// takes a pathname (relative or absolute) + a filename and removes the filename itself
// therefore it returns just the path (stored in 'path')
// returns false if there is an error (such as the file having no path)
bool get_path_of_file(string file_with_path, string &path)
{
	bool success = false;
	int index = file_with_path.length() - 1;	// start on last character

	// make sure the file_with_path is at least 2 characters long
	// because otherwise our proceeding calculations could cause a segfault
	if (index > 0)
	{
		// locate the preceeding / or \ character
		while ((index >= 0) && (file_with_path[index] != '/') && (file_with_path[index] != '\\'))
		{
			index--;
		}

		// if we found a leading / or \ character
		if (index >= 0)
		{
			path = file_with_path.substr(0, index+1);
			success = true;
		}
	}

	return success;
}

// Isolates a word within an arbitrary string (src_buf)
// 'word' will contain the word it found, 'remaining_line' will contain the rest of the line that has yet to be examined
// !!!NOTE : 'src_buf' is assumed to be a single line with no linefeed/carriage return characters!!!
// Returns 'true' if a word was found, or 'false' if a word could not be found (ie if the string was empty)
bool find_word(const char *src_buf, string &word, string &remaining_line)
{
	bool result = false;
	int index = 0;
	int start_index = 0;

	// find beginning of word, skip spaces or tabs
	while (my_is_whitespace(src_buf[index]))
	{
		index++;
	}

	// make sure we aren't at the end of the string already
	if (src_buf[index] != 0)
	{
		start_index = index;

		// now go to the end of the current word
		while ((!my_is_whitespace(src_buf[index])) && (src_buf[index] != 0))
		{
			index++;
		}

		// this section is a little tricky, so verify that it actually works :)
		word = src_buf;
		remaining_line = word.substr(index, word.length() - index);
		word = word.substr(start_index, index-start_index);
		result = true;
	}

	return result;
}

// Isolates a word within an arbitrary string (src_buf)
// 'word_begin' will point to the beginning of the word, and 'word_length' will be how long the word is
// !!!NOTE : 'src_buf' is assumed to be a single line with no linefeed/carriage return characters!!!
// Returns 'true' if a word was found, or 'false' if a word could not be found (ie if the string was empty)
bool find_word(const char *src_buf, const char **word_begin, int *word_length)
{
	bool result = false;
	int index = 0;

	*word_length = 0;
	*word_begin = NULL;

	// find beginning of word, skip spaces or tabs
	while (my_is_whitespace(src_buf[index]))
	{
		index++;
	}

	// make sure we aren't at the end of the string already
	if (src_buf[index] != 0)
	{
		*word_begin = &src_buf[index];	// point to beginning of word occurance

		// now go to the end of the current word
		while ((!my_is_whitespace(src_buf[index])) && (src_buf[index] != 0))
		{
			(*word_length)++;
			index++;
		}
		result = true;
	}

	return result;
}

// miniature version of isspace() because we don't want to consider linefeeds/carriage returns to be whitespace
bool my_is_whitespace(char ch)
{
	if ((ch == ' ') || (ch == '\t')) return true;
	else return false;
}
