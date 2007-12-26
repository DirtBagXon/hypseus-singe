
// mpegscan.cpp
// by Matt Ownby

// part of the DAPHNE emulator project

///////////////////////////

#include <stdio.h>
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

/////////////////////////////////////////////////

int get_next_byte(FILE *F)
{
	g_filepos++;
	return getc(F);
}

// skip ahead number of bytes in the file stream
void skip_bytes (FILE *F, long bytes_to_skip)
{
	fseek(F, bytes_to_skip, SEEK_CUR);
	g_filepos += bytes_to_skip;
}

// return the last 3 bytes that have been read, storing them in a, b, and c
// a is the most recent byte, c is the oldest
// for example, a header of 0 0 1 would be a = 1, b = 0, c = 0
// header is the position of the 'c' byte (oldest byte)
void get_last_three(unsigned char &a, unsigned char &b, unsigned char &c, unsigned int &header)
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
		header = g_last_three_loc[pos];	// this works out so that the oldest value is the final value
		count++;
	}

	a = result[0];	// newest
	b = result[1];
	c = result[2];	// oldest

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
// returns true if no errors, false on error
bool parse_video_stream(FILE *F, FILE *datafile, unsigned int length)
{

	bool result = true;
	int ch = 0;
	unsigned int start_pos = g_filepos;
	static unsigned int frame_type = 0;
	static bool frame_need_byte_one = false, frame_need_byte_two = false;
	static unsigned int last_header_pos = 0;	// the position of the last header we've parsed

	// parse this chunk of video
	while (g_filepos  - start_pos < length)
	{
		ch = get_next_byte(F);

		if (ch == EOF)
		{
			break;
		}

		// We must make sure we only process 1 byte per iteration of this loop so that we don't
		// exceed the maximum length.  Therefore, I've created a few booleans to make it possible
		// so we only need to process 1 byte per iteration.  The booleans are frame_need_byte_one
		// and frame_need_byte_two

		// if we need the first byte following a frame header
		if (frame_need_byte_one)
		{
			frame_type = ch << 8;
			frame_need_byte_one = false;
			frame_need_byte_two = true;
		}
		// else if we need the second byte following a frame header
		else if (frame_need_byte_two)
		{
			frame_type = frame_type | ch;

			frame_type = (frame_type >> 3) & 3;	// isolate frame type

			switch (frame_type)
			{
			case 1:		// I frame
				g_iframe_count++;
				fwrite(&g_goppos, sizeof(g_goppos), 1, datafile);
				// with I frames we store the Group of Picture instead because that's what
				// SMPEG parses first
				break;
			case 2:	// P frame
				g_pframe_count++;
				fwrite(&last_header_pos, sizeof(last_header_pos), 1, datafile);
				break;
			case 3:	// B frame
				g_bframe_count++;
				fwrite(&last_header_pos, sizeof(last_header_pos), 1, datafile);
				break;
			default:
				result = false;
				break;
			}

			frame_need_byte_two = false;
		}

		// if we are looking for a new header
		else
		{
			unsigned char header[3] = { 0 };

			get_last_three(header[0], header[1], header[2], last_header_pos);

			// if we're at a place where a header is
			if ((header[2] == 0) && (header[1] == 0) && (header[0] == 1))
			{

				// see what type of header this is
				switch (ch)
				{
				case 0xB3:	// sequence header
					break;
				case 0:	// video frame

					/*
					if (g_curframe % 100 == 0)
					{
						cout << "Found frame " << g_curframe << " at " << last_header_pos << endl;
						cout << "Last GOP started at " << g_goppos << endl;
					}
					*/

					g_curframe++;	// advance frame pointer
					frame_need_byte_one = true;
					break;
				case 0xB8:	// Group of Picture
					g_goppos = last_header_pos;
					g_gop_count++;
					break;
				default:
					break;
				} // end switch
			} // end if we found a header
		} // end if we are looking for a new header

		add_to_last_three((unsigned char) ch, g_filepos - 1);

	} // end while we're not done with video stream

	return result;

}

// parses a chunk from the _OPEN_ file F
// writes results to the _OPEN_ datafile
// returns how many bytes of the file have been parsed so far
// status returns whether we got an error, whether we're finished, or whether we're still going
// (see header file for enum values)

// The reason this only parses in chunks instead of the entire file is so the calling function
//  can keep the user advised of how fast the process is taking, since it can take a few minutes
//  on some larger mpegs.
int parse_system_stream(FILE *F, FILE *datafile, int &status)
{

	status = P_IN_PROGRESS;
	int header[4];

	header[0] = get_next_byte(F);
	header[1] = get_next_byte(F);
	header[2] = get_next_byte(F);
	header[3] = get_next_byte(F);

	// if we've hit the end of the file
	if ((header[0] == EOF) || (header[1] == EOF) || (header[2] == EOF))
	{
		status = P_FINISHED;
	}
	// if we're not to the end of the file yet
	else
	{
		// if we found a header
		if ((header[0] == 0) && (header[1] == 0) && (header[2] == 1))
		{
			unsigned char stream_type = 0;
			unsigned int offset_pos = 0;
			unsigned int length = 0;
			unsigned char high = 0, low = 0;

			switch(header[3])
			{
			case 0xBA: // packet header
				stream_type = (unsigned char) get_next_byte(F);

				// if it's an mpeg2 stream
				if ((stream_type & 0xC0) == 0x40)
				{
					unsigned char skip_extra = 0;
					skip_bytes(F, 8);	// get to a position to read the 13th byte of the PACK
					skip_extra = (unsigned char) get_next_byte(F);
					skip_bytes(F, 1 + (skip_extra & 7));	// skip 10 bytes total + whatever's in the 13th position
				}

				// if it's an mpeg1 stream then the header is 12 bytes, so skip 8 more in addition to the 4 we've read
				else if ((stream_type & 0xF0) == 0x20)
				{
					skip_bytes(F, 7);
				}

				else
				{
					status = P_ERROR;
				}
				break;
			case 0xE0:	// video stream
				high = (unsigned char) get_next_byte(F);	// header[4]
				low = (unsigned char) get_next_byte(F);	// header[5]
				length = (high << 8) | low;	// how long this 0xE0 packet lasts
				offset_pos = g_filepos;	// save current position so we can adjust the length
				stream_type = (unsigned char) get_next_byte(F);	// header[6]

				// if it's an mpeg2
				if ((stream_type & 0xC0) == 0x80)
				{
					get_next_byte(F);	// header[7]
					unsigned char skip_extra = (unsigned char) get_next_byte(F);	// get header[8]
					skip_bytes(F, 1 + skip_extra);	// skip to header[9] + whatever's in header[8]
					// we should now be out of the 0xE0 header and ready to parse video
				}

				else
				{
					// strip off any extra padding
					while (stream_type == 0xFF)
					{
						stream_type = (unsigned char) get_next_byte(F);
					}

					if ((stream_type & 0xC0) == 0x40)
					{
						get_next_byte(F);
						stream_type = (unsigned char) get_next_byte(F);	// skip 2 bytes and get value
					}

					unsigned char skip_extra = (unsigned char) (stream_type >> 4);

					// here we calculate how many remaining bytes there are in the header
					// so we can skip them

					if (skip_extra == 0)
					{
						// don't skip anything
					}

					else if (skip_extra == 2)
					{
						skip_bytes(F, 4);
					}

					else if (skip_extra == 3)
					{
						skip_bytes(F, 9);
					}

					else
					{
						// error, unknown size of header
						status = P_ERROR;
					}

					// parse the video stream and check for an error
					if (!parse_video_stream(F, datafile, length - (g_filepos - offset_pos)))
					{
						status = P_ERROR;
					}

				}
				break;
			default:
				// if header is too small to be part of the system stream, give an error and abort
				if (header[3] < 0xB9)
				{
					status = P_ERROR;
				}

				// else it's a packet that we simply don't care about, so we skip it
				else
				{
					unsigned char high = (unsigned char) get_next_byte(F);
					unsigned char low = (unsigned char) get_next_byte(F);
					skip_bytes(F, (high << 8) | low);	// skip entire block which is this many bytes big
					// could be an audio stream for example
				}
				break;
			}
		}
	} // end if we're not at the end of the file yet

	return g_filepos;

}



// if we want to compile this as a standalone application instead of integrating it into another app
#ifdef STANDALONE

int main()
{
	char filename[81];
	FILE *F = NULL;
	int status = 0;

	cout << "Enter filename: ";
	cin << filename;

	F = fopen(filename, "rb");
	G = fopen("deleteme.dat", "wb");

	if (F && G)
	{
		do
		{
			pos = parse_system_stream(F, G, status);
		} while (status == P_IN_PROGRESS);

		fclose(F);
		fclose(G);
	}

	return 0;

}

#endif