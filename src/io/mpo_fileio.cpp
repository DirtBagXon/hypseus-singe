/*
 * mpo_fileio.cpp
 *
 * Copyright (C) 2005 Matthew P. Ownby
 *
 * This file is part of MPOLIB, a multi-purpose library
 *
 * MPOLIB is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPOLIB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// MPO's NOTE:
//  I may wish to use MPOLIB in a proprietary product some day.  Therefore,
//   the only way I can accept other people's changes to my code is if they
//   give me full ownership of those changes.

// mpo_fileio.cpp
// by Matt Ownby

// Replacement functions for the traditional fopen/fread/fclose/etc functions
// Main purpose: to support files larger than 4 gigs
// Secondary purpose: to not rely on MSCVRT dll in windows

#include "mpo_fileio.h"

#ifndef WIN32
#include <stdlib.h>	// for malloc
#endif

// if this doesn't crash, you're good to go!
// if anyone knows how to do this check at compile time, let me know!!!
void mpo_test()
{
	if (sizeof(MPO_UINT64) != 8)	// make sure this is really 64-bit
	{
		int i = 0;
		int b;
		
		b = 5 / i;	// force crash
	}
}

bool mpo_file_exists(const char *filename)
{
	bool result = false;
	mpo_io *io = NULL;

	io = mpo_open(filename, MPO_OPEN_READONLY);
	if (io)
	{
		mpo_close(io);
		result = true;
	}
	return result;
}

mpo_io *mpo_open(const char *filename, int flags)
{
	mpo_io *io = NULL;
	bool success = false;

	// dynamically allocate, will be freed by mpo_close
	io = (mpo_io *) malloc(sizeof(mpo_io));

#ifdef WIN32
	ZeroMemory(io, sizeof(mpo_io));
	io->handle = INVALID_HANDLE_VALUE;

	if (flags == MPO_OPEN_READONLY)
	{
		io->handle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
	}
	else if (flags == MPO_OPEN_READWRITE)
	{
		io->handle = CreateFile(filename, GENERIC_READ | GENERIC_WRITE,0, NULL,
			OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	else if (flags == MPO_OPEN_CREATE)
	{
		io->handle = CreateFile(filename, GENERIC_WRITE, 0, NULL,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	else if (flags == MPO_OPEN_APPEND)	// append
	{
		// will this seek to the end of the file automatically?
		io->handle = CreateFile(filename, GENERIC_WRITE, 0, NULL,
			OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (io->handle != INVALID_HANDLE_VALUE)
		{
			mpo_seek(0, MPO_SEEK_END, io);	// seek to the end of the file for appending
		}
	}
	else	// unknown value, error
	{
	}

	// if opening the file succeeded
	if (io->handle != INVALID_HANDLE_VALUE)
	{
		/*
		// GetFileSizeEx apparently only works for Win2k and WinXP so I reverted to GetFileSize for the older windows
		LARGE_INTEGER size;
		GetFileSizeEx(io->handle, &size);	// get the file size, we always want it..
		io->size = size.QuadPart;
		io->eof = false;
		result = true;
		*/

		DWORD hSize = 0;
		DWORD lSize = 0;
		lSize = GetFileSize(io->handle, &hSize);
		// if getting the file size succeeded
		if (lSize != INVALID_FILE_SIZE)
		{
			io->size = hSize;	// assign higher 32-bits initially
			io->size <<= 32;	// shift up 32-bits
			io->size |= lSize;	// and merge in lower 32-bit bits
			io->eof = false;

			// now try to get the file time
			FILETIME last_modified;
			if (GetFileTime(io->handle, NULL, NULL, &last_modified))
			{
				io->time_last_modified = last_modified.dwHighDateTime; // high part
				io->time_last_modified <<= 32;	// shift up 32
				io->time_last_modified |= last_modified.dwLowDateTime;	// low part
				success = true;
			}
			// else it failed for unknown reasons
		}
		// else getfilesize failed
	}
#else
	const char *mode = "rb";	// assume reading
	if (flags == MPO_OPEN_CREATE)
	{
		mode = "wb";
	}
	else if (flags == MPO_OPEN_READWRITE)
	{
		if (mpo_file_exists(filename)) mode = "rb+";	// read/write existing file
		else mode = "wb+";	// create file, open in read/write mode
	}
	else if (flags == MPO_OPEN_APPEND)
	{
		mode = "ab";
	}
	// else unknown... eror
	
	io->handle = MPO_FOPEN(filename, mode);
	if (io->handle)
	{
		struct stat teh_stats;	// to get last modified
		MPO_FSEEK(io->handle, 0, SEEK_END);	// go to end of file
		io->size = MPO_FTELL(io->handle);	// get position (total file size)
		MPO_FSEEK(io->handle, 0, SEEK_SET);	// go to beginning
		io->eof = false;
		if (fstat(fileno(io->handle), &teh_stats) == 0)
		{
			io->time_last_modified = teh_stats.st_mtime;
			success = true;
		}
		// else something went wrong (probably won't ever happen so we won't add error handling)
	}
#endif

	// if something went wrong
	if (!success)
	{
		free(io);
		io = NULL;
	}

	return io;
}

// returns true on success
// EOF is when bytes_read is 0, but true is returned
bool mpo_read (void *buf, size_t bytes_to_read,
			   MPO_BYTES_READ *bytes_read, mpo_io *io)
{
	bool result = false;
	MPO_BYTES_READ backup_bytes_read = 0;	// in case their 'bytes_read' is NULL

	// if they don't care what bytes_read is, then we'll just squelch it
	if (bytes_read == NULL)
	{
		bytes_read = &backup_bytes_read;
	}

#ifdef WIN32
	LPDWORD ptr = bytes_read;
	if (ReadFile(io->handle, buf, (DWORD) bytes_to_read, ptr, NULL) != 0)
	{
		result = true;
	}

	/*
	else
	{
			char s[160];
			DWORD i = GetLastError();
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, i, 0, s, sizeof(s), NULL);
			printline(s);
			tmp = "Error message # is " + g_numstr.ToStr((unsigned int) i);
			printline(tmp.c_str());
	}
	*/

#else
	*bytes_read = fread(buf, 1, bytes_to_read, io->handle);
	
	// if we read some bytes, or if we properly got to EOF
	if ((*bytes_read > 0) || ((*bytes_read == 0) || feof(io->handle) != 0))
	{
		result = true;
	}
#endif

	// if we hit EOF, set a flag for convenience
	if ((*bytes_read) != bytes_to_read)
	{
		io->eof = true;
	}

	return result;
}

// returns true on success
bool mpo_write (const void *buf, size_t bytes_to_write, unsigned int *bytes_written, mpo_io *io)
{
	bool result = false;

	unsigned int backup_bytes_written = 0;	// in case their 'bytes_written' is NULL

	// if they don't care what bytes_written is, then we'll just squelch it
	if (bytes_written == NULL)
	{
		bytes_written = &backup_bytes_written;
	}

#ifdef WIN32
	LPDWORD ptr = (LPDWORD) bytes_written;
	if (WriteFile(io->handle, buf, (DWORD) bytes_to_write, ptr, NULL) != 0)
	{
		result = true;
	}
#else
	*bytes_written = fwrite(buf, 1, bytes_to_write, io->handle);
	if (*bytes_written == bytes_to_write)
	{
		result = true;
	}
#endif
	return result;
}

// fseek replacement
// returns true if seek was successful
bool mpo_seek(MPO_INT64 offset, seek_type type, mpo_io *io)
{
	bool result = false;

#ifdef WIN32
	/*
	// This code (SetFilePointerEx) works but only for Win2k and WinXP so I have commented it out
	LARGE_INTEGER l;
	l.QuadPart = offset;
	LARGE_INTEGER pre_result;
	pre_result.QuadPart = -1;

	// if seek is successful
	if (SetFilePointerEx(io->handle, l, &pre_result, type) != 0)
	{
		// result should now contain the current position
	}
	else
	{
		pre_result.QuadPart = -1;
	}

	result = true;	// no decent error checking here
	*/

	LONG loffset = (LONG) offset;	// NOTE : assumes long is 32-bits
	LONG hoffset = (LONG) (offset >> 32);
	DWORD pre_result = 0;

	pre_result = SetFilePointer(io->handle, loffset, &hoffset, type);

	// if we potentially got an error ...
//	if (pre_result == INVALID_SET_FILE_POINTER)
	if (pre_result == -1)	// INVALID_SET_FILE_POINTER is -1 but some old visual studio 6's don't have this defined
	{
		result = false;
		pre_result = GetLastError();	// check to see if we really got an error
		if (pre_result == NO_ERROR)
		{
			result = true;
		}
	}

	// no error
	else
	{
		result = true;
	}

#else
	int pre_result = MPO_FSEEK(io->handle, offset, type);
	if (pre_result == 0) result = true;
#endif

	return result;
}

void mpo_close(mpo_io *io)
{
	if (io != NULL)
	{
#ifdef WIN32
		CloseHandle(io->handle);
#else
		fclose(io->handle);
#endif
		io->handle = 0;
		io->eof = false;
		io->size = 0;
		io->time_last_modified = 0;
		free(io);	// de-allocate
	}
	// else we cannot reference a NULL pointer
}

bool mpo_mkdir(const char *dirname)
{
	bool result = false;
#ifdef WIN32
	if (CreateDirectory(dirname, NULL) != 0) result = true;
#else
	// create directory with minimal permissions
	if (mkdir(dirname, 0700) == 0)
	{
		result = true;
	}
#endif
	return result;
}
