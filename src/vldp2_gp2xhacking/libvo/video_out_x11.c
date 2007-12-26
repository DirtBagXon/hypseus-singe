/*
 * video_out_x11.c
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

#ifdef LIBVO_X11

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <inttypes.h>
/* since it doesn't seem to be defined on some platforms */
int XShmGetEventBase (Display *);

#ifdef LIBVO_XV
#include <string.h>	/* strcmp */
#include <X11/extensions/Xvlib.h>
#define FOURCC_YV12 0x32315659
#endif

#include "video_out.h"
#include "convert.h"

typedef struct {
    void * data;
    int wait_completion;
    XImage * ximage;
#ifdef LIBVO_XV
    XvImage * xvimage;
#endif
} x11_frame_t;

typedef struct {
    vo_instance_t vo;
    x11_frame_t frame[3];
    int index;
    int width;
    int height;
    Display * display;
    Window window;
    GC gc;
    XVisualInfo vinfo;
    XShmSegmentInfo shminfo;
    int completion_type;
#ifdef LIBVO_XV
    XvPortID port;
#endif
} x11_instance_t;

static int open_display (x11_instance_t * instance)
{
    int major;
    int minor;
    Bool pixmaps;
    XVisualInfo visualTemplate;
    XVisualInfo * XvisualInfoTable;
    XVisualInfo * XvisualInfo;
    int number;
    int i;
    XSetWindowAttributes attr;
    XGCValues gcValues;

    instance->display = XOpenDisplay (NULL);
    if (! (instance->display)) {
	fprintf (stderr, "Can not open display\n");
	return 1;
    }

    if ((XShmQueryVersion (instance->display, &major, &minor,
			   &pixmaps) == 0) ||
	(major < 1) || ((major == 1) && (minor < 1))) {
	fprintf (stderr, "No xshm extension\n");
	return 1;
    }

    instance->completion_type =
	XShmGetEventBase (instance->display) + ShmCompletion;

    /* list truecolor visuals for the default screen */
#ifdef __cplusplus
    visualTemplate.c_class = TrueColor;
#else
    visualTemplate.class = TrueColor;
#endif
    visualTemplate.screen = DefaultScreen (instance->display);
    XvisualInfoTable = XGetVisualInfo (instance->display,
				       VisualScreenMask | VisualClassMask,
				       &visualTemplate, &number);
    if (XvisualInfoTable == NULL) {
	fprintf (stderr, "No truecolor visual\n");
	return 1;
    }

    /* find the visual with the highest depth */
    XvisualInfo = XvisualInfoTable;
    for (i = 1; i < number; i++)
	if (XvisualInfoTable[i].depth > XvisualInfo->depth)
	    XvisualInfo = XvisualInfoTable + i;

    instance->vinfo = *XvisualInfo;
    XFree (XvisualInfoTable);

    attr.background_pixmap = None;
    attr.backing_store = NotUseful;
    attr.border_pixel = 0;
    attr.event_mask = 0;
    /* fucking sun blows me - you have to create a colormap there... */
    attr.colormap = XCreateColormap (instance->display,
				     RootWindow (instance->display,
						 instance->vinfo.screen),
				     instance->vinfo.visual, AllocNone);
    instance->window =
	XCreateWindow (instance->display,
		       DefaultRootWindow (instance->display),
		       0 /* x */, 0 /* y */, instance->width, instance->height,
		       0 /* border_width */, instance->vinfo.depth,
		       InputOutput, instance->vinfo.visual,
		       (CWBackPixmap | CWBackingStore | CWBorderPixel |
			CWEventMask | CWColormap), &attr);

    instance->gc = XCreateGC (instance->display, instance->window, 0,
			      &gcValues);

    return 0;
}

static int shmerror = 0;

static int handle_error (Display * display, XErrorEvent * error)
{
    shmerror = 1;
    return 0;
}

static void * create_shm (x11_instance_t * instance, int size)
{
    instance->shminfo.shmid = shmget (IPC_PRIVATE, size, IPC_CREAT | 0777);
    if (instance->shminfo.shmid == -1)
	goto error;

    instance->shminfo.shmaddr = (char *) shmat (instance->shminfo.shmid, 0, 0);
    if (instance->shminfo.shmaddr == (char *)-1)
	goto error;

    /* on linux the IPC_RMID only kicks off once everyone detaches the shm */
    /* doing this early avoids shm leaks when we are interrupted. */
    /* this would break the solaris port though :-/ */
    /* shmctl (instance->shminfo.shmid, IPC_RMID, 0); */

    /* XShmAttach fails on remote displays, so we have to catch this event */

    XSync (instance->display, False);
    XSetErrorHandler (handle_error);

    instance->shminfo.readOnly = True;
    if (! (XShmAttach (instance->display, &(instance->shminfo))))
	shmerror = 1;

    XSync (instance->display, False);
    XSetErrorHandler (NULL);
    if (shmerror) {
    error:
	fprintf (stderr, "cannot create shared memory\n");
	return NULL;
    }

    return instance->shminfo.shmaddr;
}

static void destroy_shm (x11_instance_t * instance)
{
    XShmDetach (instance->display, &(instance->shminfo));
    shmdt (instance->shminfo.shmaddr);
    shmctl (instance->shminfo.shmid, IPC_RMID, 0);
}

static void x11_event (x11_instance_t * instance)	/* XXXXXXXXXXX */
{
    XEvent event;
    char * addr;
    int i;

    XNextEvent (instance->display, &event);
    if (event.type == instance->completion_type) {
	addr = (instance->shminfo.shmaddr +
		((XShmCompletionEvent *)&event)->offset);
	for (i = 0; i < 3; i++)
	    if (addr == instance->frame[i].data)
		instance->frame[i].wait_completion = 0;
    }
}

static void x11_start_fbuf (vo_instance_t * _instance,
			    uint8_t * const * buf, void * id)
{
    x11_instance_t * instance = (x11_instance_t *) _instance;
    x11_frame_t * frame = (x11_frame_t *) id;

    while (frame->wait_completion)
	x11_event (instance);
}

static void x11_setup_fbuf (vo_instance_t * _instance,
			    uint8_t ** buf, void ** id)
{
    x11_instance_t * instance = (x11_instance_t *) _instance;

    buf[0] = (uint8_t *) instance->frame[instance->index].data;
    buf[1] = buf[2] = NULL;
    *id = instance->frame + instance->index++;
}

static void x11_draw_frame (vo_instance_t * _instance,
			    uint8_t * const * buf, void * id)
{
    x11_frame_t * frame;
    x11_instance_t * instance;

    frame = (x11_frame_t *) id;
    instance = (x11_instance_t *) _instance;

    XShmPutImage (instance->display, instance->window, instance->gc,
		  frame->ximage, 0, 0, 0, 0, instance->width, instance->height,
		  True);
    XFlush (instance->display);
    frame->wait_completion = 1;
}

static int x11_alloc_frames (x11_instance_t * instance)
{
    int size;
    char * alloc;
    int i;

    size = 0;
    alloc = NULL;
    for (i = 0; i < 3; i++) {
	instance->frame[i].wait_completion = 0;
	instance->frame[i].ximage =
	    XShmCreateImage (instance->display, instance->vinfo.visual,
			     instance->vinfo.depth, ZPixmap, NULL /* data */,
			     &(instance->shminfo),
			     instance->width, instance->height);
	if (instance->frame[i].ximage == NULL) {
	    fprintf (stderr, "Cannot create ximage\n");
	    return 1;
	} else if (i == 0) {
	    size = (instance->frame[0].ximage->bytes_per_line *
		    instance->frame[0].ximage->height);
	    alloc = (char *) create_shm (instance, 3 * size);
	    if (alloc == NULL)
		return 1;
	} else if (size != (instance->frame[0].ximage->bytes_per_line *
			    instance->frame[0].ximage->height)) {
	    fprintf (stderr, "unexpected ximage data size\n");
	    return 1;
	}

	instance->frame[i].data = instance->frame[i].ximage->data = alloc;
	alloc += size;
    }
    instance->index = 0;
    return 0;
}

static void x11_close (vo_instance_t * _instance)
{
    x11_instance_t * instance = (x11_instance_t *) _instance;
    int i;

    for (i = 0; i < 3; i++) {
	while (instance->frame[i].wait_completion)
	    x11_event (instance);
	XDestroyImage (instance->frame[i].ximage);
    }
    destroy_shm (instance);
    XFreeGC (instance->display, instance->gc);
    XDestroyWindow (instance->display, instance->window);
    XCloseDisplay (instance->display);
}

#ifdef LIBVO_XV
static void xv_setup_fbuf (vo_instance_t * _instance,
			   uint8_t ** buf, void ** id)
{
    x11_instance_t * instance = (x11_instance_t *) _instance;
    uint8_t * data;

    data = (uint8_t *) instance->frame[instance->index].xvimage->data;
    buf[0] = data + instance->frame[instance->index].xvimage->offsets[0];
    buf[1] = data + instance->frame[instance->index].xvimage->offsets[2];
    buf[2] = data + instance->frame[instance->index].xvimage->offsets[1];
    *id = instance->frame + instance->index++;
}

static void xv_draw_frame (vo_instance_t * _instance,
			   uint8_t * const * buf, void * id)
{
    x11_frame_t * frame = (x11_frame_t *) id;
    x11_instance_t * instance = (x11_instance_t *) _instance;

    XvShmPutImage (instance->display, instance->port, instance->window,
		   instance->gc, frame->xvimage, 0, 0,
		   instance->width, instance->height, 0, 0,
		   instance->width, instance->height, True);
    XFlush (instance->display);
    frame->wait_completion = 1;
}

static int xv_check_yv12 (x11_instance_t * instance, XvPortID port)
{
    XvImageFormatValues * formatValues;
    int formats;
    int i;

    formatValues = XvListImageFormats (instance->display, port, &formats);
    for (i = 0; i < formats; i++)
	if ((formatValues[i].id == FOURCC_YV12) &&
	    (! (strcmp (formatValues[i].guid, "YV12")))) {
	    XFree (formatValues);
	    return 0;
	}
    XFree (formatValues);
    return 1;
}

static int xv_check_extension (x11_instance_t * instance)
{
    unsigned int version;
    unsigned int release;
    unsigned int dummy;
    unsigned int adaptors;
    unsigned int i;
    unsigned long j;
    XvAdaptorInfo * adaptorInfo;

    if ((XvQueryExtension (instance->display, &version, &release,
			   &dummy, &dummy, &dummy) != Success) ||
	(version < 2) || ((version == 2) && (release < 2))) {
	fprintf (stderr, "No xv extension\n");
	return 1;
    }

    XvQueryAdaptors (instance->display, instance->window, &adaptors,
		     &adaptorInfo);

    for (i = 0; i < adaptors; i++)
	if (adaptorInfo[i].type & XvImageMask)
	    for (j = 0; j < adaptorInfo[i].num_ports; j++)
		if ((! (xv_check_yv12 (instance,
				       adaptorInfo[i].base_id + j))) &&
		    (XvGrabPort (instance->display, adaptorInfo[i].base_id + j,
				 0) == Success)) {
		    instance->port = adaptorInfo[i].base_id + j;
		    XvFreeAdaptorInfo (adaptorInfo);
		    return 0;
		}

    XvFreeAdaptorInfo (adaptorInfo);
    fprintf (stderr, "Cannot find xv port\n");
    return 1;
}

static int xv_alloc_frames (x11_instance_t * instance)
{
    int size;
    char * alloc;
    int i;

    size = instance->width * instance->height / 4;
    alloc = (char *) create_shm (instance, 18 * size);
    if (alloc == NULL)
	return 1;

    for (i = 0; i < 3; i++) {
	instance->frame[i].wait_completion = 0;
	instance->frame[i].xvimage =
	    XvShmCreateImage (instance->display, instance->port, FOURCC_YV12,
			      alloc, instance->width, instance->height,
			      &(instance->shminfo));
	if ((instance->frame[i].xvimage == NULL) ||
	    (instance->frame[i].xvimage->data_size != 6 * size)) { /* FIXME */
	    fprintf (stderr, "Cannot create xvimage\n");
	    return 1;
	}
	instance->frame[i].data = alloc;
	alloc += 6 * size;
    }

    return 0;
}

static void xv_close (vo_instance_t * _instance)
{
    x11_instance_t * instance = (x11_instance_t *) _instance;
    int i;

    for (i = 0; i < 3; i++) {
	while (instance->frame[i].wait_completion)
	    x11_event (instance);
	XFree (instance->frame[i].xvimage);
    }
    destroy_shm (instance);
    XvUngrabPort (instance->display, instance->port, 0);
    XFreeGC (instance->display, instance->gc);
    XDestroyWindow (instance->display, instance->window);
    XCloseDisplay (instance->display);
}
#endif

static int common_setup (x11_instance_t * instance, int width, int height,
			 vo_setup_result_t * result, int xv)
{
    instance->vo.set_fbuf = NULL;
    instance->vo.discard = NULL;
    instance->vo.start_fbuf = x11_start_fbuf;
    instance->width = width;
    instance->height = height;

    if (open_display (instance))
	return 1;

#ifdef LIBVO_XV
    if (xv && (! (xv_check_extension (instance)))) {
	if (xv_alloc_frames (instance))
	    return 1;
	instance->vo.setup_fbuf = xv_setup_fbuf;
	instance->vo.draw = xv_draw_frame;
	instance->vo.close = xv_close;
	result->convert = NULL;
    } else
#endif
    {
	if (x11_alloc_frames (instance))
	    return 1;
	instance->vo.setup_fbuf = x11_setup_fbuf;
	instance->vo.draw = x11_draw_frame;
	instance->vo.close = x11_close;

#ifdef WORDS_BIGENDIAN
	if (instance->frame[0].ximage->byte_order != MSBFirst) {
	    fprintf (stderr, "No support for non-native byte order\n");
	    return 1;
	}
#else
	if (instance->frame[0].ximage->byte_order != LSBFirst) {
	    fprintf (stderr, "No support for non-native byte order\n");
	    return 1;
	}
#endif

	/*
	 * depth in X11 terminology land is the number of bits used to
	 * actually represent the colour.
	 *
	 * bpp in X11 land means how many bits in the frame buffer per
	 * pixel.
	 *
	 * ex. 15 bit color is 15 bit depth and 16 bpp. Also 24 bit
	 *     color is 24 bit depth, but can be 24 bpp or 32 bpp.
	 *
	 * If we have blue in the lowest bit then "obviously" RGB
	 * (the guy who wrote this convention never heard of endianness ?)
	 */

	result->convert =
	    convert_rgb (((instance->frame[0].ximage->blue_mask & 1) ?
			  CONVERT_RGB : CONVERT_BGR),
			 ((instance->vinfo.depth == 24) ?
			  instance->frame[0].ximage->bits_per_pixel :
			  instance->vinfo.depth));
    }

    XMapWindow (instance->display, instance->window);

    return 0;
}

static int x11_setup (vo_instance_t * instance, int width, int height,
		      vo_setup_result_t * result)
{
    return common_setup ((x11_instance_t *)instance, width, height, result, 0);
}

vo_instance_t * vo_x11_open (void)
{
    x11_instance_t * instance;

    instance = (x11_instance_t *) malloc (sizeof (x11_instance_t));
    if (instance == NULL)
	return NULL;

    instance->vo.setup = x11_setup;
    return (vo_instance_t *) instance;
}

#ifdef LIBVO_XV
static int xv_setup (vo_instance_t * instance, int width, int height,
		     vo_setup_result_t * result)
{
    return common_setup ((x11_instance_t *)instance, width, height, result, 1);
}

vo_instance_t * vo_xv_open (void)
{
    x11_instance_t * instance;

    instance = (x11_instance_t *) malloc (sizeof (x11_instance_t));
    if (instance == NULL)
	return NULL;

    instance->vo.setup = xv_setup;
    return (vo_instance_t *) instance;
}
#endif
#endif
