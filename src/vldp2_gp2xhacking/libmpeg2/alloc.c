/*
 * alloc.c
 * Copyright (C) 2000-2002 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

//#include <stdlib.h>
#include <mpo_lib.h>
#include <inttypes.h>

#include "mpeg2.h"
#include "mpeg2_internal.h"

#if defined(HAVE_MEMALIGN) && !defined(__cplusplus)
/* some systems have memalign() but no declaration for it */
//void * memalign (size_t align, size_t size);
#endif

void * (* mpeg2_malloc_hook) (int size, int reason) = NULL;
int (* mpeg2_free_hook) (void * buf) = NULL;

void * mpeg2_malloc (int size, int reason)
{
    char * buf;

	do_error("inside mpeg2_malloc");

	if (mpeg2_malloc_hook)
	{
		do_setvar(mpeg2_malloc_hook);
		do_error("mpeg2_malloc_hook is not 0, setting uCPU2Running to its value");

		// lockup since we can't go any further anyway
		for (;;);
	}

    if (mpeg2_malloc_hook) {
	buf = (char *) mpeg2_malloc_hook (size, reason);
	if (buf)
	    return buf;
    }

	do_error("about to call mpo memalign");

#if defined(HAVE_MEMALIGN) && !defined(__cplusplus) && !defined(DEBUG)
    return MPO_MEMALIGN (16, size);
#else
    buf = (char *) MPO_MALLOC (size + 15 + sizeof (void **)); we should never get here
    if (buf) {
	char * align_buf;

	align_buf = buf + 15 + sizeof (void **);
	align_buf -= (long)align_buf & 15;
	*(((void **)align_buf) - 1) = buf;
	return align_buf;
    }
    return NULL;
#endif
}

void mpeg2_free (void * buf)
{
    if (mpeg2_free_hook && mpeg2_free_hook (buf))
	return;

#if defined(HAVE_MEMALIGN) && !defined(__cplusplus) && !defined(DEBUG)
    MPO_FREE (buf);
#else
    MPO_FREE (*(((void **)buf) - 1));
#endif
}
