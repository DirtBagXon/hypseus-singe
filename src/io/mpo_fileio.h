/*
 * mpo_fileio.h
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

// mpo.h
// by Matt Ownby

#ifndef MPO_FILEIO_H
#define MPO_FILEIO_H

#ifdef WIN32
#include <windows.h>
#else
#include <stdio.h>
#include <sys/stat.h>	// for fstat
#endif

#include "numstr.h"	// for MPO_UINT64 definition

#ifdef WIN32
#define MPO_HANDLE HANDLE
#define MPO_BYTES_READ DWORD
#else
#define MPO_HANDLE FILE *
#define MPO_BYTES_READ size_t

#ifdef LINUX
#define MPO_FOPEN fopen64
#define MPO_FSEEK fseeko64
#define MPO_FTELL ftello64
#endif

#ifdef MAC_OSX
#define MPO_FOPEN fopen
#define MPO_FSEEK fseeko
#define MPO_FTELL ftello
#endif

#endif

struct mpo_io
{
	MPO_HANDLE handle;	// handle of the file (keep this at the beginning of the struct to make it easy to statically initialize)
	MPO_UINT64 size;	// the size of the file

	// A unit of time representing when this file was last modified.
	// It will mean something different in win32 and linux and thus is only useful for
	// comparing against other files to see which one was modified most recently.
	MPO_UINT64 time_last_modified;

	bool eof;	// whether we have reached the End-Of-File
};

// ways that file can be opened
enum
{
	MPO_OPEN_READONLY,	// opens pre-existing file in read-only mode
	MPO_OPEN_READWRITE,	// opens pre-existing file in read/write mode, or creates file if it does not exist
	MPO_OPEN_CREATE,	// creates/overwrites file, for writing purposes
	MPO_OPEN_APPEND,	// creates/appends file, for writing purposes
};

typedef enum
{
#ifdef WIN32
		MPO_SEEK_SET = FILE_BEGIN,
		MPO_SEEK_CUR = FILE_CURRENT,
		MPO_SEEK_END = FILE_END

#else
		MPO_SEEK_SET = SEEK_SET,
		MPO_SEEK_CUR = SEEK_CUR,
		MPO_SEEK_END = SEEK_END
#endif
} seek_type;

void mpo_test();
bool mpo_file_exists(const char *filename);

// returns a pointer to an mpo_io structure if successful, NULL if unsuccessful
mpo_io *mpo_open(const char *filename, int flags);

bool mpo_read (void *buf, size_t bytes_to_read, MPO_BYTES_READ *bytes_read, mpo_io *io);
bool mpo_write (const void *buf, size_t bytes_to_write, unsigned int *bytes_written, mpo_io *io);
bool mpo_seek(MPO_INT64 offset, seek_type type, mpo_io *io);
void mpo_close(mpo_io *io);

// Attempts to create a directory (with permissions such that only the user can access that dir)
// 'dirname' is the name of the directory to create, returns on true on success
bool mpo_mkdir(const char *dirname);
#endif // MPO_FILEIO_H
