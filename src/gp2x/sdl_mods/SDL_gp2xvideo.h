/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id: $";
#endif

#ifndef _SDL_gp2xvideo_h
#define _SDL_gp2xvideo_h

#include "SDL_mouse.h"
#include "SDL_sysvideo.h"
#include "SDL_mutex.h"

#include "mmsp2_regs.h"

// Hidden "this" pointer for the video functions
#define _THIS	SDL_VideoDevice *this

// Allocate 5MB for the frame buffer (seems to be how gph have it)
//#define GP2X_VIDEO_MEM_SIZE ((5*1024*1024) - 4096)

// New memory allocation, VIDEO_MEM is now the entire upper 32MB with
//  the reserved parts marked internally as unusable.
//  OFFSET is from base of video memory to start of fbcon0
#define GP2X_VIDEO_MEM_SIZE (32*1024*1024)
#define GP2X_SCREEN_OFFSET 0
//0x1101000

// This is used in both gp2xvideo and gp2xyuv, so I put it here to make maintenance easier
#define GP2X_UPPER_MEM_START 0x2000000

// Number of native modes supported
#define SDL_NUMMODES 8

// TV control
#define IOCTL_CX25874_DISPLAY_MODE_SET	_IOW('v', 0x02, unsigned char)
#define IOCTL_CX25874_TV_MODE_POSITION	_IOW('v', 0x0A, unsigned char)

#define TV_POS_LEFT	0
#define TV_POS_RIGHT	1
#define TV_POS_UP	2
#define TV_POS_DOWN	3


#define CX25874_ID 	0x8A

#define DISPLAY_LCD     0x1
#define DISPLAY_MONITOR 0x2
#define DISPLAY_TV_NTSC 0x3
#define DISPLAY_TV_PAL 	0x4
#define DISPLAY_TV_GAME_NTSC 0x05


////
// Internal structure for allocating video memory
typedef struct video_bucket {
  struct video_bucket *prev, *next;
  char *base;
  unsigned int size;
  short used;
  short dirty;
} video_bucket;

////
// Internal structure for hardware cursor
typedef struct SDL_WMcursor {
  video_bucket *bucket;
  int dimension;
  unsigned short fgr, fb, bgr, bb, falpha, balpha;
} SDL_WMcursor;

////
// Private display data

typedef struct SDL_PrivateVideoData {
  int memory_fd, fbcon_fd, mouse_fd, keyboard_fd;
  int saved_keybd_mode;
  //  struct termios saved_kbd_termios;
  int x_offset, y_offset, ptr_offset;
  int w, h, pitch;
  int vsync_polarity;
  int phys_width, phys_height, phys_pitch, phys_ilace;
  int scale_x, scale_y;
  float xscale, yscale;
  SDL_mutex *hw_lock;
  unsigned short fastioclk, grpclk;
  unsigned short src_foreground, src_background;
  char *vmem;
  int buffer_showing;
  char *buffer_addr[2];
  unsigned short volatile *io;
  unsigned int volatile *fio;
  video_bucket video_mem;
  char *surface_mem;
  int memory_left;
  int memory_max;
  int allow_scratch_memory;
  SDL_WMcursor *visible_cursor;
  int cursor_px, cursor_py, cursor_vx, cursor_vy;
  SDL_Rect *SDL_modelist[SDL_NUMMODES+1];
  unsigned short stl_cntl, stl_mixmux, stl_alphal, stl_alphah;
  unsigned short stl_hsc, stl_vscl, stl_vsch, stl_hw;
  unsigned short stl_oadrl, stl_oadrh, stl_eadrl, stl_eadrh;
  unsigned short stl_regions[16]; // 1<=x<=4 of STLx_STX, _ENDX, _STY, _ENDY
  unsigned short mlc_ovlay_cntr;
} SDL_PrivateVideoData;

extern VideoBootStrap GP2X_bootstrap;

////
// utility functions
////

////
// convert virtual address to physical
static inline unsigned int GP2X_Phys(_THIS, char *virt)
{
  return (unsigned int)((long)virt - (long)(this->hidden->vmem) + 0x2000000);
}

////
// convert virtual address to physical (lower word)
static inline unsigned short GP2X_PhysL(_THIS, char *virt)
{
  return (unsigned short)(((long)virt - (long)(this->hidden->vmem) + 0x2000000) & 0xffff);
}

////
// convert virtual address to phyical (upper word)
static inline unsigned short GP2X_PhysH(_THIS, char *virt)
{
  return (unsigned short)(((long)virt - (long)(this->hidden->vmem) + 0x2000000) >> 16);
}

////
// mark surface has been used in HW accel
static inline void GP2X_AddBusySurface(SDL_Surface *surface)
{
  ((video_bucket *)surface->hwdata)->dirty = 1;
}

////
// test if surface has been used in HW accel
static inline int GP2X_IsSurfaceBusy(SDL_Surface *surface)
{
  return ((video_bucket *)surface->hwdata)->dirty;
}

////
// wait for blitter to finish with all busy surfaces
static inline void GP2X_WaitBusySurfaces(_THIS)
{
  video_bucket *bucket;

  for (bucket = &this->hidden->video_mem; bucket; bucket = bucket->next)
    bucket->dirty = 0;
  do {} while (this->hidden->fio[MESGSTATUS] & MESG_BUSY);
}

// Waits until vblank is active (doesn't necessarily wait for vblank to start)
static inline void GP2X_WaitVBlank(const SDL_PrivateVideoData *data)
{
	// wait for vblank to start, choose transition type by polarity
	if (data->vsync_polarity)
	  do {} while ((data->io[GPIOB_PINLVL] & GPIOB_VSYNC));
	else
	  do {} while (!(data->io[GPIOB_PINLVL] & GPIOB_VSYNC));
}

//#define GP2X_DEBUG 1
#endif // _SDL_gp2xvideo_h
