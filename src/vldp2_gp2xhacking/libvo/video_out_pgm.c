/*
 * video_out_pgm.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "video_out.h"

typedef struct {
    vo_instance_t vo;
    int framenum;
    int width;
    int height;
    char header[1024];
    char filename[128];
} pgm_instance_t;

static void internal_draw_frame (pgm_instance_t * instance,
				 FILE * file, uint8_t * const * buf)
{
    int i;

    fwrite (instance->header, strlen (instance->header), 1, file);
    fwrite (buf[0], instance->width, instance->height, file);
    for (i = 0; i < instance->height >> 1; i++) {
	fwrite (buf[1] + i * (instance->width >> 1), instance->width >> 1, 1,
		file);
	fwrite (buf[2] + i * (instance->width >> 1), instance->width >> 1, 1,
		file);
    }
}

static void pgm_draw_frame (vo_instance_t * _instance,
			    uint8_t * const * buf, void * id)
{
    pgm_instance_t * instance;
    FILE * file;

    instance = (pgm_instance_t *) _instance;
    sprintf (instance->filename, "%d.pgm", instance->framenum++);
    file = fopen (instance->filename, "wb");
    if (!file)
	return;
    internal_draw_frame (instance, file, buf);
    fclose (file);
}

static int pgm_setup (vo_instance_t * _instance, int width, int height,
		      vo_setup_result_t * result)
{
    pgm_instance_t * instance;

    instance = (pgm_instance_t *) _instance;

    instance->width = width;
    instance->height = height;
    sprintf (instance->header, "P5\n%d %d\n255\n", width, height * 3 / 2);
    result->convert = NULL;
    return 0;
}

static vo_instance_t * internal_open (void draw (vo_instance_t *,
						 uint8_t * const *, void *))
{
    pgm_instance_t * instance;

    instance = (pgm_instance_t *) malloc (sizeof (pgm_instance_t));
    if (instance == NULL)
        return NULL;

    instance->vo.setup = pgm_setup;
    instance->vo.setup_fbuf = NULL;
    instance->vo.set_fbuf = NULL;
    instance->vo.start_fbuf = NULL;
    instance->vo.draw = draw;
    instance->vo.discard = NULL;
    instance->vo.close = NULL;
    instance->framenum = 0;

    return (vo_instance_t *) instance;
}

vo_instance_t * vo_pgm_open (void)
{
    return internal_open (pgm_draw_frame);
}

static void pgmpipe_draw_frame (vo_instance_t * _instance,
				uint8_t * const * buf, void * id)
{
    pgm_instance_t * instance;

    instance = (pgm_instance_t *) _instance;
    internal_draw_frame (instance, stdout, buf);
}

vo_instance_t * vo_pgmpipe_open (void)
{
    return internal_open (pgmpipe_draw_frame);
}

static void md5_draw_frame (vo_instance_t * _instance,
			    uint8_t * const * buf, void * id)
{
    pgm_instance_t * instance;
    char command[100];

    instance = (pgm_instance_t *) _instance;
    pgm_draw_frame (_instance, buf, id);
    sprintf (command, "md5sum -b %s", instance->filename);
    system (command);
    remove (instance->filename);
}

vo_instance_t * vo_md5_open (void)
{
    return internal_open (md5_draw_frame);
}
