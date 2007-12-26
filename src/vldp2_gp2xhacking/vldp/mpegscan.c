
/*
 * mpegscan.c
 *
 * Copyright (C) 2001 Matt Ownby
 *
 * This file is part of VLDP, a virtual laserdisc player.
 *
 * VLDP is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * VLDP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

///////////////////////////

/*

Some mpeg header information that may be useful at some point.

---------------------------
PICTURE_CODING_EXT DETAILS:
---------------------------

Sample mpeg2 byte sequence: 00 00 01 B5 83 4F F3 59 80

the 8 in 0x83 = picture_coding_ext (in header.c)

the 3 in 0xF3 means FRAME_PICTURE (progressive, not a field)

if the high bit is set (for the byte that is 0x80) then it means the
picture is a progressive frame (I think!)

---------------------
SEQUENCE_EXT DETAILS:
---------------------
Sample mpeg2 byte sequence: 00 00 01 B5 14 8A 00 01 00 00

1 in 0x14 = sequence_ext (in header.c)
bit 3 in 8A set means progressive frame, clear means non-progressive (I
think!)
bits 1 and 2: 00 = invalid, 01 (2) = 4:2:0, 10 (4) = 4:2:2

---------------------
SEQUENCE_DISPLAY_EXT DETAILS:
---------------------

Sample mpeg2 byte sequence: 00 00 01 B5 25 06 06 06 0B 42 0F 00

2 in 0x25 = sequence_display_ext (in header.c)

*/

#include <stdio.h>
#include <stdlib.h>	// for malloc
#include "mpegscan.h"

unsigned char g_last_three[3] = { 0 };		// the last 3 bytes read
unsigned int g_last_three_loc[3] = { 0 };	// the position of the last 3 bytes read

int g_last_three_pos = 0;
int g_iframe_count = 0;
int g_pframe_count = 0;
int g_bframe_count = 0;
int g_gop_count = 0;	// group of picture count
int g_curframe = 0;
unsigned int g_goppos = 0;
unsigned int g_filepos = 0;	// where we are in the file
unsigned int g_frame_type = 0;	// I, P, B frame, etc
unsigned int g_last_header_pos = 0;	// the position of the last header we've parsed

int g_fields_detected = 0;	// whether the stream uses fields
int g_frames_detected = 0;	// whether the stream uses frames (these are both here to detect errors)

enum { IN_NOTHING, IN_PIC, IN_PIC_EXT };
int g_status = 0;	// whether we are in a special area (inside a picture header, for example)
int g_rel_pos = 0;	// which byte of the special area we are in (relative position)

// 1 = sequence_ext, 2 = sequence_display_ext, 8 = picture_coding_ext
unsigned char g_ext_type = 0;

/////////////////////////////////////////////////

// resets all state variables to their initial values.  This needs to be called every time an mpeg is parsed
void init_mpegscan()
{
	int i = 0;
	
	// clear arrays
	for (i = 0; i < 3; i++)
	{
		g_last_three[i] = 0;
		g_last_three_loc[i] = 0;
	}
	g_last_three_pos = 0;
	g_iframe_count = 0;
	g_pframe_count = 0;
	g_bframe_count = 0;
	g_gop_count = 0;	// group of picture count
	g_curframe = 0;
	g_goppos = 0;
	g_filepos = 0;	// where we are in the file
	g_frame_type = 0;
	g_last_header_pos = 0;
	g_status = 0;
	g_rel_pos = 0;
	g_fields_detected = 0;	// assume we don't use fields
	g_frames_detected = 0;	// " " " frames
	g_ext_type = 0;
}

/*
int get_next_byte(FILE *F)
{
	g_filepos++;
	g_rel_pos++;
	return getc(F);
}
*/

// skip ahead number of bytes in the file stream
/*
void skip_bytes (FILE *F, long bytes_to_skip)
{
	fseek(F, bytes_to_skip, SEEK_CUR);
	g_filepos += bytes_to_skip;
}
*/

// return the last 3 bytes that have been read, storing them in a, b, and c
// a is the most recent byte, c is the oldest
// for example, a header of 0 0 1 would be a = 1, b = 0, c = 0
// header is the position of the 'c' byte (oldest byte)
void get_last_three(unsigned char *a, unsigned char *b, unsigned char *c, unsigned int *header)
{

	int count = 0;
	unsigned char result[3] = { 0 };
	int pos = g_last_three_pos;

	while (count < 3)
	{
		pos--;
		if (pos < 0)
		{
			pos = 2;
		}

		result[count] = g_last_three[pos];
		*header = g_last_three_loc[pos];	// this works out so that the oldest value is the final value
		count++;
	}

	*a = result[0];	// newest
	*b = result[1];
	*c = result[2];	// oldest

}


// updates the last 3 values that we've read, replacing the oldest one with 'val'
// pos is the position of the 'val' in the file
void add_to_last_three(unsigned char val, unsigned int pos)
{
	g_last_three[g_last_three_pos] = val;
	g_last_three_loc[g_last_three_pos] = pos;
	g_last_three_pos++;
	if (g_last_three_pos > 2)
	{
		g_last_three_pos = 0;
	}
}

// parses from file stream, length # of bytes
// writes results to the open datafile
// returns stat codes
int parse_video_stream(FILE *datafile, unsigned int length)
{
	int result = P_IN_PROGRESS;
	int ch = 0;
	unsigned int start_pos = g_filepos;
	const int minus_one = -1;
	unsigned int buf_index = 0;
	size_t bytes_read = 0;
	unsigned char *buf = (unsigned char *) malloc(length);	// allocate a chunk of memory to read in file

	if (!buf)
	{
		return P_ERROR;
	}

	bytes_read = io_read(buf, length);	// read in a chunk

	// parse this chunk of video
	while (g_filepos  - start_pos < length)
	{
		// this should always be true unless we hit EOF
		if (buf_index < bytes_read)
		{
			g_filepos++;
			g_rel_pos++;
			ch = buf[buf_index++];
		}
		// if we've hit EOF
		else
		{
			// if we're certain we're using fields
			if ((g_fields_detected) && (!g_frames_detected))
			{
				result = P_FINISHED_FIELDS;
			}
			// else if we're certain we're not using fields
			// (for mpeg1 g_fields_detected and g_frames_detected will both be 0)
			else if (!g_fields_detected)
			{
				result = P_FINISHED_FRAMES;
			}
			// else we can't determine what's going on, so do an error to be safe
			else
			{
				result = P_ERROR;
			}
			break; // end loop
		}

		// We must make sure we only process 1 byte per iteration of this loop so that we don't
		// exceed the maximum length.

		// if we are in the middle of a frame header
		if (g_status == IN_PIC)
		{
			// if we need the first byte following a frame header
			if (g_rel_pos == 0)
			{
				g_frame_type = ch << 8;
			}
			// else if we need the second byte following a frame header
			else if (g_rel_pos == 1)
			{
				g_frame_type = g_frame_type | ch;
				g_frame_type = (g_frame_type >> 3) & 3;	// isolate frame type

				// examine which type of frame we've found
				switch (g_frame_type)
				{
				case 1:		// I frame
					g_iframe_count++;
					fwrite(&g_last_header_pos, sizeof(g_last_header_pos), 1, datafile);	// actual beginning of I frame
#ifdef VLDP_DEBUG
//					fprintf(stderr, "Found an I frame at %x\n", g_last_header_pos);
#endif
					break;
				default:	// if it's not an I frame, just write -1
					fwrite(&minus_one, sizeof(minus_one), 1, datafile);
#ifdef VLDP_DEBUG
//					fprintf(stderr, "Found a non I frame at %x\n", g_last_header_pos);
#endif
					break;
				}
				g_status = IN_NOTHING;	// we got what we came for, now get it :)
			} // end if we are on the second byte of the picture
		} // end if we're in a frame header
		
		// if we're in a picture header extension ...
		else if (g_status == IN_PIC_EXT)
		{
			// if we're about to get the EXT type
			if (g_rel_pos == 0)
			{
				g_ext_type = ch >> 4;
			}

			// UPDATE : It seems that this information is just a hint of whether the mpeg is interlaced or progressive, and
			//  may be wrong, so we cannot rely on this information.
			/*
			// if we have ext type 1 (sequence_ext) then we can see if it's frames or fields
			else if ((g_rel_pos == 1) && (g_ext_type == 1))
			{
				// are we progressive?
				if (ch & 8)
				{
					g_frames_detected = 1;
				}
				// else we're using fields
				else
				{
					g_fields_detected = 1;
				}
				g_status = IN_NOTHING;
			}
			*/

			// this is where we either find out if we're using fields/frames or eject
			else if (g_rel_pos >= 2)
			{
				// if we have ext type 8, then we can see if this uses frames or fields
				if ((g_rel_pos == 2) && (g_ext_type == 8))
				{
					unsigned char u8Val = ch & 3;

					// we need to detect whether the stream uses fields so we can adjust our searches accordingly
					// 1 is the code for TOP FIELD, 2 is the code for BOTTOM_FIELD
					if ((u8Val == 1) || (u8Val == 2))
					{
						g_fields_detected = 1;
					}

					// 3 is code for a full image
					else if (u8Val == 3)
					{
						g_frames_detected = 1;
					}
				} // end if ext type is 8 ...
				// else other ext type which we ignore ...

				// when we get this far, we are done parsing EXT ...
				g_status = IN_NOTHING;
			}

		}

		// if we are in nothing, looking for a new header
		else
		{
			unsigned char header[3] = { 0 };

			get_last_three(&header[0], &header[1], &header[2], &g_last_header_pos);

			// if we're at a place where a header is
			if ((header[2] == 0) && (header[1] == 0) && (header[0] == 1))
			{

				// see what type of header this is
				switch (ch)
				{
				case 0:	// video frame
					g_curframe++;	// advance frame pointer
					g_rel_pos = -1;	// this gets incremented to 0 before we check, and I wanted 0 to mean 1st byte
					g_status = IN_PIC;
					break;
				case 0xB3:	// sequence header
					break;
				case 0xB5:	// extension header
					g_rel_pos = -1;
					g_status = IN_PIC_EXT;
					break;
				case 0xB8:	// Group of Picture
					g_goppos = g_last_header_pos;
					g_gop_count++;
#ifdef VLDP_DEBUG
					fprintf(stderr, "Got GOP at %x\n", g_goppos);
#endif
					break;
				default:
					break;
				} // end switch
			} // end if we found a header
		} // end if we are looking for a new header

		add_to_last_three((unsigned char) ch, g_filepos - 1);

	} // end while we're not done with video stream

	free(buf);	// de-allocate buffer

	return result;

}
