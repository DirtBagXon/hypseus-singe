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

/*
 * GP2X SDL video driver implementation,
 *  Base, dummy.
 *  Memory routines based on fbcon
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "SDL.h"
#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_mouse.h"
#include "SDL_sysvideo.h"
#include "SDL_pixels_c.h"
#include "SDL_events_c.h"

#include "SDL_gp2xvideo.h"
#include "SDL_gp2xevents_c.h"
#include "SDL_gp2xmouse_c.h"
#include "SDL_gp2xyuv_c.h"
#include "mmsp2_regs.h"

#define GP2XVID_DRIVER_NAME "GP2X"


// Initialization/Query functions
static int GP2X_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Rect **GP2X_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
static SDL_Surface *GP2X_SetVideoMode(_THIS, SDL_Surface *current,
				      int width, int height,
				      int bpp, Uint32 flags);
static int GP2X_SetColors(_THIS, int firstcolor, int ncolors,
			  SDL_Color *colors);
static void GP2X_VideoQuit(_THIS);

// Hardware surface functions
static void GP2X_SurfaceFree(_THIS, video_bucket *bucket);
static video_bucket *GP2X_SurfaceAllocate(_THIS, int size);

static int GP2X_InitHWSurfaces(_THIS /*, SDL_Surface *screen,
				       char *base, int size */ );
static void GP2X_FreeHWSurfaces(_THIS);
static int GP2X_AllocHWSurface(_THIS, SDL_Surface *surface);
static int GP2X_LockHWSurface(_THIS, SDL_Surface *surface);
static void GP2X_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void GP2X_FreeHWSurface(_THIS, SDL_Surface *surface);
static void GP2X_DummyBlit(_THIS);
static int GP2X_FlipHWSurface(_THIS, SDL_Surface *surface);
// etc.
static void GP2X_UpdateRects(_THIS, int numrects, SDL_Rect *rects);
static int GP2X_CheckHWBlit(_THIS, SDL_Surface *src, SDL_Surface *dst);
static int GP2X_FillHWRect(_THIS, SDL_Surface *surface,
			   SDL_Rect *area, Uint32 colour);
static int GP2X_HWAccelBlit(SDL_Surface *src, SDL_Rect *src_rect,
			    SDL_Surface *dst, SDL_Rect *dst_rect);
// cursor
static WMcursor *GP2X_CreateWMCursor(SDL_VideoDevice *video,
				     Uint8 *data, Uint8 *mask,
				     int w, int h,
				     int hot_x, int hot_y);
static void GP2X_FreeWMCursor(_THIS, WMcursor *cursor);
static int GP2X_ShowWMCursor(_THIS, WMcursor *cursor);
static void GP2X_WarpWMCursor(_THIS, Uint16 x, Uint16 y);
static void GP2X_MoveWMCursor(_THIS, int x, int y);

////////
// GP2X driver bootstrap functions
////////

////
// Are we available?
static int GP2X_Available(void)
{
  // Of course we are.
  return 1;
}

////
// Cleanup routine
static void GP2X_DeleteDevice(SDL_VideoDevice *device)
{
  SDL_PrivateVideoData *data = device->hidden;
  int i;
#ifdef GP2X_DEBUG
  fputs("SDL_GP2X: DeleteDevice\n", stderr);
#endif

  if (data->fio)
    munmap(data->fio, 0x100);
  if (data->io) {
    // Clear register bits we clobbered if they weren't on
    if (data->fastioclk == 0)
      data->io[SYSCLKENREG] &= ~(FASTIOCLK);
    if (data->grpclk == 0)
      data->io[VCLKENREG] &= ~(GRPCLK);
    // reset display hardware
    data->io[MLC_STL_CNTL]   = data->stl_cntl;
    data->io[MLC_STL_MIXMUX] = data->stl_mixmux;
    data->io[MLC_STL_ALPHAL] = data->stl_alphal;
    data->io[MLC_STL_ALPHAH] = data->stl_alphah;
    data->io[MLC_STL_HSC]    = data->stl_hsc;
    data->io[MLC_STL_VSCL]   = data->stl_vscl;
    data->io[MLC_STL_VSCH]   = data->stl_vsch;
    data->io[MLC_STL_HW]     = data->stl_hw;
    data->io[MLC_STL_OADRL]  = data->stl_oadrl;
    data->io[MLC_STL_OADRH]  = data->stl_oadrh;
    data->io[MLC_STL_EADRL]  = data->stl_eadrl;
    data->io[MLC_STL_EADRH]  = data->stl_eadrh;
    for (i = 0; i < 16; i++)
      data->io[MLC_STL1_STX + i] = data->stl_regions[i];
    data->io[MLC_OVLAY_CNTR] = data->mlc_ovlay_cntr;
    munmap(data->io, 0x10000);
  }
  if (data->vmem)
    munmap(data->vmem, GP2X_VIDEO_MEM_SIZE);

  if (data->fbcon_fd)
    close(data->fbcon_fd);

  if (data->memory_fd)
    close(data->memory_fd);
  free(device->hidden);
  free(device);
}

////
// Initalize driver
static SDL_VideoDevice *GP2X_CreateDevice(int devindex)
{
  SDL_VideoDevice *device;
#ifdef GP2X_DEBUG
  fputs("SDL_GP2X: CreateDevice\n", stderr);
#endif  

  /* Initialize all variables that we clean on shutdown */
  device = (SDL_VideoDevice *)malloc(sizeof(SDL_VideoDevice));
  if (device) {
    memset(device, 0, (sizeof *device));
    device->hidden = (struct SDL_PrivateVideoData *)
      malloc((sizeof *device->hidden));
  }
  if ((device == NULL) || (device->hidden == NULL)) {
    SDL_OutOfMemory();
    if (device) {
      free(device);
    }
    return 0;
  }
  memset(device->hidden, 0, (sizeof *device->hidden));

  device->hidden->mouse_fd = -1;
  device->hidden->keyboard_fd = -1;

  // Set the function pointers
  device->VideoInit = GP2X_VideoInit;
  device->ListModes = GP2X_ListModes;
  device->SetVideoMode = GP2X_SetVideoMode;
  device->CreateYUVOverlay = GP2X_CreateYUVOverlay;
  device->SetColors = GP2X_SetColors;
  device->UpdateRects = GP2X_UpdateRects;
  device->VideoQuit = GP2X_VideoQuit;
  device->AllocHWSurface = GP2X_AllocHWSurface;
  device->CheckHWBlit = GP2X_CheckHWBlit;
  device->FillHWRect = GP2X_FillHWRect;
  device->SetHWColorKey = NULL;
  device->SetHWAlpha = NULL;
  device->LockHWSurface = GP2X_LockHWSurface;
  device->UnlockHWSurface = GP2X_UnlockHWSurface;
  device->FlipHWSurface = GP2X_FlipHWSurface;
  device->FreeHWSurface = GP2X_FreeHWSurface;
  device->SetCaption = NULL;
  device->SetIcon = NULL;
  device->IconifyWindow = NULL;
  device->GrabInput = NULL;
  device->GetWMInfo = NULL;
  device->InitOSKeymap = GP2X_InitOSKeymap;
  device->PumpEvents = GP2X_PumpEvents;
  device->CreateWMCursor = GP2X_CreateWMCursor;
  device->FreeWMCursor = GP2X_FreeWMCursor;
  device->ShowWMCursor = GP2X_ShowWMCursor;
  device->WarpWMCursor = GP2X_WarpWMCursor;
  device->MoveWMCursor = GP2X_MoveWMCursor;
  device->free = GP2X_DeleteDevice;
  
  return device;
}

////
// Link info to SDL_video
VideoBootStrap GP2X_bootstrap = {
  GP2XVID_DRIVER_NAME, "SDL GP2X video driver",
  GP2X_Available, GP2X_CreateDevice
};

////
// Set up hardware
static int GP2X_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
  SDL_PrivateVideoData *data = this->hidden;
  int i;
#ifdef GP2X_DEBUG
  fputs("SDL_GP2X: VideoInit\n", stderr);
#endif

#ifndef DISABLE_THREADS
  // Create hardware surface lock mutex
  data->hw_lock = SDL_CreateMutex();
  if (data->hw_lock == NULL) {
    SDL_SetError("Unable to create lock mutex");
    GP2X_VideoQuit(this);
    return -1;
  }
#endif

  data->memory_fd = open("/dev/mem", O_RDWR, 0);
  if (data->memory_fd < 0) {
    SDL_SetError("Unable to open /dev/mem");
    return -1;
  }
  data->fbcon_fd = open("/dev/fb0", O_RDWR, 0);
  if (data->fbcon_fd < 0) {
    SDL_SetError("Unable to open /dev/fb0");
    return -1;
  }
  data->vmem = mmap(NULL, GP2X_VIDEO_MEM_SIZE, PROT_READ|PROT_WRITE,
		    MAP_SHARED, data->memory_fd, GP2X_UPPER_MEM_START);
  if (data->vmem == (char *)-1) {
    SDL_SetError("Unable to get video memory");
    data->vmem = NULL;
    GP2X_VideoQuit(this);
    return -1;
  }
  data->io = mmap(NULL, 0x10000, PROT_READ|PROT_WRITE,
		  MAP_SHARED, data->memory_fd, 0xc0000000);
  if (data->io == (unsigned short *)-1) {
    SDL_SetError("Unable to get hardware registers");
    data->io = NULL;
    GP2X_VideoQuit(this);
    return -1;
  }

  data->fio = mmap(NULL, 0x100, PROT_READ|PROT_WRITE,
		  MAP_SHARED, data->memory_fd, 0xe0020000);
  if (data->fio == (unsigned int *)-1) {
    SDL_SetError("Unable to get blitter registers");
    data->fio = NULL;
    GP2X_VideoQuit(this);
    return -1;
  }

  // Determine the screen depth (gp2x defaults to 16-bit depth)
  // we change this during the SDL_SetVideoMode implementation...
  vformat->BitsPerPixel = 16;
  vformat->BytesPerPixel = 2;
  vformat->Rmask = 0x1f00;
  vformat->Gmask = 0x07e0;
  vformat->Bmask = 0x001f;
  vformat->Amask = 0;

  this->info.wm_available = 0;
  this->info.hw_available = 1;
  this->info.video_mem = GP2X_VIDEO_MEM_SIZE / 1024;
  //  memset(data->vmem, GP2X_VIDEO_MEM_SIZE, 0);
  // Save hw register data that we clobber
  data->fastioclk = data->io[SYSCLKENREG] & FASTIOCLK;
  data->grpclk = data->io[VCLKENREG] & GRPCLK;
  // Need FastIO for blitter
  data->io[SYSCLKENREG] |= FASTIOCLK;
  // and enable graphics clock
  data->io[VCLKENREG] |= GRPCLK;

  // Save display registers so we can restore screen to original state
  data->stl_cntl =   data->io[MLC_STL_CNTL];
  data->stl_mixmux = data->io[MLC_STL_MIXMUX];
  data->stl_alphal = data->io[MLC_STL_ALPHAL];
  data->stl_alphah = data->io[MLC_STL_ALPHAH];
  data->stl_hsc =    data->io[MLC_STL_HSC];
  data->stl_vscl =   data->io[MLC_STL_VSCL];
  // HW bug - MLC_STL_VSCH returns VSCL instead
  data->stl_vsch =   0;//data->io[MLC_STL_VSCH];
  data->stl_hw =     data->io[MLC_STL_HW];
  data->stl_oadrl =  data->io[MLC_STL_OADRL];
  data->stl_oadrh =  data->io[MLC_STL_OADRH];
  data->stl_eadrl =  data->io[MLC_STL_EADRL];
  data->stl_eadrh =  data->io[MLC_STL_EADRH];
  // save the 4 region areas
  for (i = 0; i < 16; i++)
    data->stl_regions[i] = data->io[MLC_STL1_STX + i];
  data->mlc_ovlay_cntr = data->io[MLC_OVLAY_CNTR];

  // Save vsync polarity (since GPH change it in different firmwares)
  data->vsync_polarity = data->io[DPC_V_SYNC] & DPC_VSFLDPOL;
  // Check what video mode we're in (LCD, NTSC or PAL)
  data->phys_width = data->io[DPC_X_MAX] + 1;
  data->phys_height = data->io[DPC_Y_MAX] + 1;
  data->phys_ilace = (data->io[DPC_CNTL] & DPC_INTERLACE) ? 1 : 0;
#ifdef GP2X_DEBUG
  fprintf(stderr, "SDL_GP2X: Physical screen = %dx%d (ilace = %d, pol = %d)\n",
	  data->phys_width, data->phys_height, data->phys_ilace, data->vsync_polarity);
#endif
  for (i = 0; i < SDL_NUMMODES; i++) {
    data->SDL_modelist[i] = malloc(sizeof(SDL_Rect));
    data->SDL_modelist[i]->x = data->SDL_modelist[i]->y = 0;
  }
  data->SDL_modelist[0]->w =  320; data->SDL_modelist[0]->h = 200; // low-res
  data->SDL_modelist[1]->w =  320; data->SDL_modelist[1]->h = 240; // lo-res
  data->SDL_modelist[2]->w =  640; data->SDL_modelist[2]->h = 400; // vga-low
  data->SDL_modelist[3]->w =  640; data->SDL_modelist[3]->h = 480; // vga
  data->SDL_modelist[4]->w =  720; data->SDL_modelist[4]->h = 480; // TV NTSC
  data->SDL_modelist[5]->w =  720; data->SDL_modelist[5]->h = 576; // TV PAL
  data->SDL_modelist[6]->w =  800; data->SDL_modelist[6]->h = 600; // vga-med
  data->SDL_modelist[7]->w = 1024; data->SDL_modelist[7]->h = 768; // vga-high
  data->SDL_modelist[8] = NULL;

  this->info.blit_fill = 1;
  this->FillHWRect = GP2X_FillHWRect;
  this->info.blit_hw = 1;
  this->info.blit_hw_CC = 1;

  GP2X_InitHWSurfaces(this);
  // Enable mouse and keyboard support
  //  GP2X_OpenKeyboard(this);
  GP2X_OpenMouse(this);
  return 0;
}

////
// Return list of possible screen sizes for given mode
static SDL_Rect **GP2X_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
#ifdef GP2X_DEBUG
  fprintf(stderr, "SDL_GP2X: ListModes\n");
#endif
  // Only 8 & 16 bit modes. 4 & 24 are available, but tough.
  if ((format->BitsPerPixel != 8) && (format->BitsPerPixel != 16))
    return NULL;

  if (flags & SDL_FULLSCREEN)
    return this->hidden->SDL_modelist;
  else
    return (SDL_Rect **) -1;
}

////
// Set hw videomode
static SDL_Surface *GP2X_SetVideoMode(_THIS, SDL_Surface *current,
				      int width, int height,
				      int bpp, Uint32 flags)
{
  SDL_PrivateVideoData *data = this->hidden;
  video_bucket *screen_bucket;
  char *pixelbuffer;
#ifdef GP2X_DEBUG
  fprintf(stderr, "SDL_GP2X: Setting video mode %dx%d %d bpp, flags=%X\n",
	  width, height, bpp, flags);
#endif

  // Set up the new mode framebuffer, making sanity adjustments
  // 64 <= width <= 1024, multiples of 8 only
  width = (width + 7) & 0x7f8;
  if (width < 64) width = 64;
  if (width > 1024) width = 1024;

  // 64 <= height <= 768
  if (height < 64) height = 64;
  if (height > 768) height = 768;

  // 8 or 16 bpp. HW can handle 24, but limited support so not implemented
  bpp = (bpp <= 8) ? 8 : 16;
  
  // Allocate the new pixel format for the screen
  if (!SDL_ReallocFormat(current, bpp, 0, 0, 0, 0)) {
    SDL_SetError("Couldn't allocate new pixel format for requested mode");
    return NULL;
  }
  
  // Screen is always a HWSURFACE and FULLSCREEN
  current->flags = (flags & SDL_DOUBLEBUF) | SDL_FULLSCREEN |
    SDL_HWSURFACE | SDL_NOFRAME;
  if (bpp == 8) current->flags |= SDL_HWPALETTE;
  data->w = current->w = width;
  data->h = current->h = height;
  data->pitch = data->phys_pitch = current->pitch = width * (bpp / 8);
  if (data->phys_ilace && (width == 720))
    data->phys_pitch *= 2;
  this->screen = current;
  SDL_CursorQuit();
  GP2X_FreeHWSurfaces(this);
  GP2X_InitHWSurfaces(this);
  // sep screen from allocator: removed 1 line, added 2 lines
  //current->pixels = data->vmem + GP2X_SCREEN_OFFSET;
  screen_bucket = GP2X_SurfaceAllocate(this, height * data->pitch * ((flags & SDL_DOUBLEBUF) ? 2:1));
  if (!screen_bucket)
    return NULL;
  current->pixels = screen_bucket->base;

  data->x_offset = data->y_offset = 0;
  data->ptr_offset = 0;
  // gp2x holds x-scale as fixed-point, 1024 == 1:1
  data->scale_x = (1024 * width) / data->phys_width;
  // and y-scale is scale * pitch
  data->scale_y = (height * data->pitch) / data->phys_height;
  // xscale and yscale are set so that virtual_x * xscale = phys_x
  data->xscale = (float)data->phys_width / (float)width;
  data->yscale = (float)data->phys_height / (float)height;

  data->buffer_showing = 0;
  data->buffer_addr[0] = current->pixels;
  data->surface_mem = data->vmem; // + (height * data->pitch);
  data->memory_max = GP2X_VIDEO_MEM_SIZE - height * data->pitch;
  if (flags & SDL_DOUBLEBUF) {
    data->buffer_addr[1] = current->pixels += height * data->pitch;
    //    data->surface_mem += height * data->pitch;
    data->memory_max -= height * data->pitch;
  }
  // sep screen from allocator - added 1 line, removed 1 line
  current->hwdata = (struct private_hwdata *)&data->video_mem;
  //  GP2X_InitHWSurfaces(this, current, data->surface_mem, data->memory_max);

  // Load the registers
  data->io[MLC_STL_HSC] = data->scale_x;
  data->io[MLC_STL_VSCL] = data->scale_y & 0xffff;
  data->io[MLC_STL_VSCH] = data->scale_y >> 16;
  data->io[MLC_STL_HW] = data->phys_pitch;
  data->io[MLC_STL_CNTL] = (bpp==8 ? MLC_STL_BPP_8 : MLC_STL_BPP_16) |
    MLC_STL1ACT;
  data->io[MLC_STL_MIXMUX] = 0;
  data->io[MLC_STL_ALPHAL] = 255;
  data->io[MLC_STL_ALPHAH] = 255;
  data->io[MLC_OVLAY_CNTR] |= DISP_STL1EN;

  pixelbuffer = current->pixels;
  if (data->phys_ilace) {
    data->io[MLC_STL_OADRL] = GP2X_PhysL(this, pixelbuffer);
    data->io[MLC_STL_OADRH] = GP2X_PhysH(this, pixelbuffer);
    if (data->w == 720) pixelbuffer += data->pitch;
  }
  data->io[MLC_STL_EADRL] = GP2X_PhysL(this, pixelbuffer);
  data->io[MLC_STL_EADRH] = GP2X_PhysH(this, pixelbuffer);

  return current;
}

////
// Initialize HW surface list
//static int GP2X_InitHWSurfaces(_THIS, SDL_Surface *screen, char *base, int size)
static int GP2X_InitHWSurfaces(_THIS)
{
  video_bucket *bucket, *first_bucket, *second_bucket, *third_bucket;
  int cursor_state;
  char *base = this->hidden->vmem;
#ifdef GP2X_DEBUG
  fprintf(stderr, "SDL_GP2X: InitHWSurfaces\n");
#endif

  first_bucket = (video_bucket *)malloc(sizeof(*bucket));
  second_bucket = (video_bucket *)malloc(sizeof(*bucket));
  if ((first_bucket == NULL) || (second_bucket == NULL)) {
    SDL_OutOfMemory();
    return -1;
  }

  // ***HACK*** whether gfx memory has 16MB scratch available
#ifdef GP2X_DEBUG
  fprintf(stderr, "SDL_GP2X: InitHWSurfaces scratch = %d\n", this->hidden->allow_scratch_memory);
#endif
  if (this->hidden->allow_scratch_memory) {
    third_bucket = (video_bucket *)malloc(sizeof(*bucket));
    if (third_bucket == NULL) {
      SDL_OutOfMemory();
      return -1;
    }
    first_bucket->next = second_bucket;
    first_bucket->prev = &this->hidden->video_mem;
    first_bucket->used = 0;
    first_bucket->dirty = 0;
    first_bucket->base = base;
    first_bucket->size = 16*1024*1024;

    second_bucket->next = third_bucket;
    second_bucket->prev = first_bucket;
    second_bucket->used = 2;
    second_bucket->dirty = 0;
    second_bucket->base = base + 16*1024*1024;
    second_bucket->size = 1024*1024;

    third_bucket->next = NULL;
    third_bucket->prev = second_bucket;
    third_bucket->used = 0;
    third_bucket->dirty = 0;
    third_bucket->base = base + 17*1024*1024;
    third_bucket->size = 5*1024*1024;
  } else {
    first_bucket->next = second_bucket;
    first_bucket->prev = &this->hidden->video_mem;
    first_bucket->used = 2;
    first_bucket->dirty = 0;
    first_bucket->base = base;
    first_bucket->size = 17*1024*1024;

    second_bucket->next = NULL;
    second_bucket->prev = first_bucket;
    second_bucket->used = 0;
    second_bucket->dirty = 0;
    second_bucket->base = base + 17*1024*1024;
    second_bucket->size = 5*1024*1024;
  }

  this->hidden->video_mem.next = first_bucket;
  this->hidden->video_mem.prev = NULL;
  this->hidden->video_mem.used = 1;
  this->hidden->video_mem.dirty = 0;
  this->hidden->video_mem.base = base; // screen->pixels;
  //  this->hidden->video_mem.size = (unsigned int)((long)base - (long)screen->pixels);
  // set these three variables to amount of memory free
  this->hidden->video_mem.size =
    this->hidden->memory_left =
    this->hidden->memory_max =
    (this->hidden->allow_scratch_memory ? 21:5) * 1024*1024;

  ////
  //  screen->hwdata = (struct private_hwdata *)&this->hidden->video_mem;

  SDL_CursorInit(1);

  return 0;
}

////
// Insert invalid addresses into surface list
static int GP2X_insert_invalid_memory(_THIS, char *block_start, int block_size)
{
  video_bucket *bucket = &this->hidden->video_mem;
  char *block_end = block_start + block_size;

  // Find bucket which encloses invalid area
  
  while (((bucket = bucket->next) != NULL) &&
	 (bucket->base + bucket->size <= block_start)) {
    if (bucket->base > block_start) {
      bucket = NULL;
      break;
    }
  }
  if (bucket == NULL) {
#ifdef GP2X_DEBUG
    fputs("GP2X_insert_invalid_memory: block not in bucket list\n", stderr);
#endif
    return -1;
  }
}

////
// Free all surfaces
static void GP2X_FreeHWSurfaces(_THIS)
{
  video_bucket *curr, *next;
#ifdef GP2X_DEBUG
  fprintf(stderr, "SDL_GP2X: FreeHWSurfaces\n");
#endif

  next = this->hidden->video_mem.next;
  while (next) {
#ifdef GP2X_DEBUG
    fprintf(stderr, "SDL_GP2X: Freeing bucket %p (size %d)\n", next, next->size);
#endif
    curr = next;
    next = curr->next;
    free(curr);
  }
  this->hidden->video_mem.next = NULL;
}

//// SURFACE MEMORY MANAGER
// Allocate memory from free pool
static video_bucket *GP2X_SurfaceAllocate(_THIS, int size)
{
  int left_over;
  video_bucket *bucket;
  SDL_PrivateVideoData *data = this->hidden;
#ifdef GP2X_DEBUG
  fprintf(stderr, "SDL_GP2X: SurfaceManager allocating %d bytes\n", size);
#endif

  if (size > data->memory_left) {
    SDL_SetError("Not enough video memory");
    return NULL;
  }

  for (bucket = &data->video_mem; bucket; bucket = bucket->next)
    if (!bucket->used && (size <= bucket->size))
      break;
  if (!bucket) {
    SDL_SetError("Video memory too fragmented");
    return NULL;
  }

  left_over = bucket->size - size;
  if (left_over) {
    video_bucket *new_bucket;
    new_bucket = (video_bucket *)malloc(sizeof(*new_bucket));
    if (!new_bucket) {
      SDL_OutOfMemory();
      return NULL;
    }
#ifdef GP2X_DEBUG
    fprintf(stderr, "SDL_GP2X: SurfaceManager adding new free bucket of %d bytes @ %p\n", left_over, new_bucket);
#endif
    new_bucket->prev = bucket;
    new_bucket->used = 0;
    new_bucket->dirty = 0;
    new_bucket->size = left_over;
    new_bucket->base = bucket->base + size;
    new_bucket->next = bucket->next;
    if (bucket->next)
      bucket->next->prev = new_bucket;
    bucket->next = new_bucket;
  }
  bucket->used = 1;
  bucket->size = size;
  bucket->dirty = 0;
  data->memory_left -= size;

#ifdef GP2X_DEBUG
  fprintf(stderr, "SDL_GP2X: SurfaceManager allocated %d bytes at %p\n", size, bucket->base);
#endif
  return bucket;
}

////
// Return memory to free pool
static void GP2X_SurfaceFree(_THIS, video_bucket *bucket)
{
  video_bucket *wanted;
  SDL_PrivateVideoData *data = this->hidden;
#ifdef GP2X_DEBUG
  fprintf(stderr, "SDL_GP2X: SurfaceManager freeing %d bytes @ %p from bucket %p\n", bucket->size, bucket->base, bucket);
#endif

  if (bucket->used == 1) {
    data->memory_left += bucket->size;
    bucket->used = 0;

    if (bucket->next && !bucket->next->used) {
#ifdef GP2X_DEBUG
      fprintf(stderr, "SDL_GP2X: merging with next bucket (%p) making %d bytes\n",
	      bucket->next, bucket->size + bucket->next->size);
#endif
      wanted = bucket->next;
      bucket->size += bucket->next->size;
      bucket->next = bucket->next->next;
      if (bucket->next)
	bucket->next->prev = bucket;
      free(wanted);
    }

    if (bucket->prev && !bucket->prev->used) {
#ifdef GP2X_DEBUG
      fprintf(stderr, "SDL_GP2X: merging with previous bucket (%p) making %d bytes\n",
	      bucket->prev, bucket->size + bucket->prev->size);
#endif
      wanted = bucket->prev;
      wanted->size += bucket->size;
      wanted->next = bucket->next;
      if (bucket->next)
	bucket->next->prev = wanted;
      free(bucket);
    }
  }
}

////
// Allocate a surface from video memory
static int GP2X_AllocHWSurface(_THIS, SDL_Surface *surface)
{
  int w, h, pitch, size;
  video_bucket *gfx_memory;
#ifdef GP2X_DEBUG
  fprintf(stderr, "SDL_GP2X: AllocHWSurface %p\n", surface);
#endif
  h = surface->h;
  w = surface->w;
  pitch = ((w * surface->format->BytesPerPixel) + 3) & ~3; // 32-bit align
  size = h * pitch;
  gfx_memory = GP2X_SurfaceAllocate(this, size);
  if (gfx_memory == NULL)
    return -1;

  surface->hwdata = (struct private_hwdata *)gfx_memory;
  surface->pixels = gfx_memory->base;
  surface->flags |= SDL_HWSURFACE;
  fputs("SDL_GP2X: Allocated\n", stderr);
  return 0;
}

////
// Free a surface back to video memry
static void GP2X_FreeHWSurface(_THIS, SDL_Surface *surface)
{
  video_bucket *bucket;
#ifdef GP2X_DEBUG
  fprintf(stderr, "SDL_GP2X: FreeHWSurface %p\n", surface);
#endif
  /*  
  for (bucket = &data->video_mem; bucket; bucket = bucket->next)
    if (bucket == (video_bucket *)surface->hwdata)
      break;
  */
  bucket = (video_bucket *)surface->hwdata;
  GP2X_SurfaceFree(this, bucket);
  surface->pixels = NULL;
  surface->hwdata = NULL;
}

////
// Mark surface as unavailable for HW acceleration
static int GP2X_LockHWSurface(_THIS, SDL_Surface *surface)
{
  if (surface == this->screen)
    SDL_mutexP(this->hidden->hw_lock);

  if (GP2X_IsSurfaceBusy(surface)) {
    GP2X_DummyBlit(this);
    GP2X_WaitBusySurfaces(this);
  }
  return 0;
}

////
// Hardware can use the surface now
static void GP2X_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
  if (surface == this->screen)
    SDL_mutexV(this->hidden->hw_lock);
}

////
// Dummy blit to flush blitter cache
static void GP2X_DummyBlit(_THIS)
{
  SDL_PrivateVideoData *data = this->hidden;
#ifdef GP2X_DEBUG
  //  fputs("SDL_GP2X: DummyBlit\n", stderr);
#endif
  do {} while (data->fio[MESGSTATUS] & MESG_BUSY);
  data->fio[MESGDSTCTRL] = MESG_DSTENB | MESG_DSTBPP_16;
  data->fio[MESGDSTADDR] = 0x3101000;
  data->fio[MESGDSTSTRIDE] = 4;
  data->fio[MESGSRCCTRL] = MESG_SRCBPP_16 | MESG_INVIDEO;
  data->fio[MESGPATCTRL] = MESG_PATENB | MESG_PATBPP_1;
  data->fio[MESGFORCOLOR] = ~0;
  data->fio[MESGBACKCOLOR] = ~0;
  data->fio[MESGSIZE] = (1 << MESG_HEIGHT) | 32;
  data->fio[MESGCTRL] = MESG_FFCLR | MESG_XDIR_POS | MESG_YDIR_POS | 0xAA;
  asm volatile ("":::"memory");
  data->fio[MESGSTATUS] = MESG_BUSY;
  do {} while (data->fio[MESGSTATUS] & MESG_BUSY);
}

////
// Flip between double-buffer pages
//  - added: moved setting scaler in here too
static int GP2X_FlipHWSurface(_THIS, SDL_Surface *surface)
{
  SDL_PrivateVideoData *data = this->hidden;
  char *pixeldata;
#ifdef GP2X_DEBUG
  //  fprintf(stderr, "SDL_GP2X: Flip %p\n", surface);
#endif
  // make sure the blitter has finished
  GP2X_WaitBusySurfaces(this);
  GP2X_DummyBlit(this);
  do {} while (data->fio[MESGSTATUS] & MESG_BUSY);

  // MPO : waiting for vblank can be used by the YUV stuff too, so I moved it into a separate function
  GP2X_WaitVBlank(data);

  // Wait to be on even field (non-interlaced always returns 0)
  //  do {} while (data->io[SC_STATUS] & SC_DISP_FIELD);

  // set write address to be the page currently showing
  surface->pixels = data->buffer_addr[data->buffer_showing];
  // Flip buffers if need be
  if (surface->flags & SDL_DOUBLEBUF)
    data->buffer_showing = 1 - data->buffer_showing;

  pixeldata = data->buffer_addr[data->buffer_showing] + data->ptr_offset;
  if (data->phys_ilace) {
    data->io[MLC_STL_OADRL] = GP2X_PhysL(this, pixeldata);
    data->io[MLC_STL_OADRH] = GP2X_PhysH(this, pixeldata);
    if (data->w == 720) pixeldata += data->pitch;
  }
  data->io[MLC_STL_EADRL] = GP2X_PhysL(this, pixeldata);
  data->io[MLC_STL_EADRH] = GP2X_PhysH(this, pixeldata);

  data->io[MLC_STL_HSC] = data->scale_x;
  data->io[MLC_STL_VSCL] = data->scale_y & 0xffff;
  data->io[MLC_STL_VSCH] = data->scale_y >> 16;

  // Wait for vblank to end (to prevent 2 close page flips in one frame)
  //  while (!(data->io[GPIOB_PINLVL] & GPIOB_VSYNC));
  return 0;
}

////
//
static void GP2X_UpdateRects(_THIS, int numrects, SDL_Rect *rects)
{
  // We're writing directly to video memory
}

////
// Set HW palette (8-bit only)
static int GP2X_SetColors(_THIS, int firstcolour, int ncolours,
			  SDL_Color *colours)
{
  unsigned short volatile *memregs = this->hidden->io;
  int i;
#ifdef GP2X_DEBUG
  //  fprintf(stderr, "SDL_GP2X: Setting %d colours, starting with %d\n",
  //  	  ncolours, firstcolour);
#endif
  memregs[MLC_STL_PALLT_A] = firstcolour;
  asm volatile ("":::"memory");
  for (i = 0; i < ncolours; i++) {
    memregs[MLC_STL_PALLT_D] = ((int)colours[i].g << 8) + colours[i].b;
    asm volatile ("":::"memory");
    memregs[MLC_STL_PALLT_D] = colours[i].r;
    asm volatile ("":::"memory");
  }
  return 0;
}

////
// Note:  If we are terminated, this could be called in the middle of
//        another SDL video routine -- notably UpdateRects.
static void GP2X_VideoQuit(_THIS)
{
  SDL_PrivateVideoData *data = this->hidden;
  int i;
#ifdef GP2X_DEBUG
  fputs("SDL_GP2X: VideoQuit\n", stderr);
#endif

  if (data->hw_lock) {
    SDL_DestroyMutex(data->hw_lock);
    data->hw_lock = NULL;
  }

  for (i = 0; i < SDL_NUMMODES; i++)
    if (data->SDL_modelist[i]) {
      free(data->SDL_modelist[i]);
      data->SDL_modelist[i] = NULL;
    }

  GP2X_FreeHWSurfaces(this);
}


////
// Check if blit between surfaces can be accelerated
static int GP2X_CheckHWBlit(_THIS, SDL_Surface *src, SDL_Surface *dst)
{
#ifdef GP2X_DEBUG
  //  fprintf(stderr, "SDL_GP2X: Checking HW accel of %p to %p... ", src, dst);
#endif
  // dst has to be HW to accelerate blits
  // can't accelerate alpha blits
  if ((dst->flags & SDL_HWSURFACE) && !(src->flags & SDL_SRCALPHA)) {
    src->flags |= SDL_HWACCEL;
    src->map->hw_blit = GP2X_HWAccelBlit;
#ifdef GP2X_DEBUG
    // fputs("Okay\n", stderr);
#endif
    return -1;
  } else {
    src->flags &= ~SDL_HWACCEL;
#ifdef GP2X_DEBUG
    // fputs("Nope\n", stderr);
#endif
    return 0;
  }
}

////
// Hardware accelerated fill
static int GP2X_FillHWRect(_THIS, SDL_Surface *surface,
			   SDL_Rect *area, Uint32 colour)
{
  Uint32 dstctrl, dest;
  SDL_PrivateVideoData *data = this->hidden;
#ifdef GP2X_DEBUG
  /*
  fprintf(stderr, "SDL_GP2X: FillHWRect %p (%d,%d)x(%d,%d) in %d\n",
      	  surface, area->x, area->y, area->w, area->h, colour);
  */
#endif

  if (surface == this->screen)
    SDL_mutexP(data->hw_lock);

  switch (surface->format->BitsPerPixel) {
  case 8:
    dstctrl = MESG_DSTBPP_8 | ((area->x & 0x03) << 3);
    dest = GP2X_Phys(this, surface->pixels) +
      (area->y * surface->pitch) + (area->x);
    break;
  case 16:
    dstctrl = MESG_DSTBPP_16 | ((area->x & 0x01) << 4);
    dest = GP2X_Phys(this, surface->pixels) +
      (area->y * surface->pitch) + (area->x << 1);
    break;
  default:
    SDL_SetError("SDL: GP2X can't hardware FillRect to surface");
    return -1;
    break;
  }
  do {} while (data->fio[MESGSTATUS] & MESG_BUSY);
  data->fio[MESGDSTCTRL] = dstctrl;
  data->fio[MESGDSTADDR] = dest & ~3;
  data->fio[MESGDSTSTRIDE] = surface->pitch;
  data->fio[MESGSRCCTRL] = 0;
  data->fio[MESGPATCTRL] = MESG_PATENB | MESG_PATBPP_1;
  data->fio[MESGFORCOLOR] = colour;
  data->fio[MESGBACKCOLOR] = colour;
  data->fio[MESGSIZE] = (area->h << MESG_HEIGHT) | area->w;
  data->fio[MESGCTRL] = MESG_FFCLR | MESG_XDIR_POS | MESG_YDIR_POS | MESG_ROP_PAT;
  asm volatile ("":::"memory");
  data->fio[MESGSTATUS] = MESG_BUSY;

  GP2X_AddBusySurface(surface);

  if (surface == this->screen)
    SDL_mutexV(data->hw_lock);
  return 0;
}

////
// Accelerated blit, 1->8, 1->16, 8->8, 16->16
static int GP2X_HWAccelBlit(SDL_Surface *src, SDL_Rect *src_rect,
			    SDL_Surface *dst, SDL_Rect *dst_rect)
{
  SDL_VideoDevice *this = current_video;
  SDL_PrivateVideoData *data = this->hidden;
  int w, h, src_x, src_y, src_stride, dst_stride, dst_x, dst_y;
  Uint32 ctrl, src_start, dst_start, src_ctrl, dst_ctrl;
  Uint32 *read_addr;
#ifdef GP2X_DEBUG
  /*
  fprintf(stderr, "SDL_GP2X: HWBlit src:%p (%d,%d)x(%d,%d) -> %p (%d,%d)\n",
    	  src, src_rect->x, src_rect->y, src_rect->w, src_rect->h,
    	  dst, dst_rect->x, dst_rect->y);
  */
#endif

  if (dst == this->screen)
    SDL_mutexP(data->hw_lock);

  src_x = src_rect->x;
  src_y = src_rect->y;
  dst_x = dst_rect->x;
  dst_y = dst_rect->y;
  w = src_rect->w;
  h = src_rect->h;
  src_stride = src->pitch;
  dst_stride = dst->pitch;

  // set blitter control with ROP and colourkey
  ctrl = MESG_ROP_COPY | MESG_FFCLR;
  if (src->flags & SDL_SRCCOLORKEY)
    ctrl |= MESG_TRANSPEN | (src->format->colorkey << MESG_TRANSPCOLOR);

  // In the case where src == dst, reverse blit direction if need be
  //  to cope with potential overlap.
  if (src != dst)
    ctrl |= MESG_XDIR_POS | MESG_YDIR_POS;
  else {
    // if src rightof dst, blit left->right else right->left
    if (src_x >= dst_x)
      ctrl |= MESG_XDIR_POS;
    else {
      src_x += w - 1;
      dst_x += w - 1;
    }
    // likewise, if src below dst blit top->bottom else bottom->top
    if (src_y >= dst_y)
      ctrl |= MESG_YDIR_POS;
    else {
      src_y += h - 1;
      dst_y += h - 1;
      src_stride = -src_stride;
      dst_stride = -dst_stride;
    }
  }

  if (dst->format->BitsPerPixel == 8) {
    dst_start = GP2X_Phys(this, dst->pixels) + (dst_y * dst->pitch) + dst_x;
    dst_ctrl = MESG_DSTBPP_8 | (dst_start & 0x03) << 3;
  } else {
    dst_start = GP2X_Phys(this, dst->pixels) +(dst_y*dst->pitch) +(dst_x<<1);
    dst_ctrl = MESG_DSTBPP_16 | (dst_start & 0x02) << 3;
  }
  do {} while (data->fio[MESGSTATUS] & MESG_BUSY);
  data->fio[MESGDSTCTRL] = dst_ctrl;
  data->fio[MESGDSTADDR] = dst_start & ~3;
  data->fio[MESGDSTSTRIDE] = dst_stride;
  data->fio[MESGFORCOLOR] = data->src_foreground;
  data->fio[MESGBACKCOLOR] = data->src_background;
  data->fio[MESGPATCTRL] = 0;
  data->fio[MESGSIZE] = (h << MESG_HEIGHT) | w;
  data->fio[MESGCTRL] = ctrl;

  ////// STILL TO CHECK SW->HW BLIT & 1bpp BLIT
  if (src->flags & SDL_HWSURFACE) {
    // src HW surface needs mapping from virtual -> physical
    switch (src->format->BitsPerPixel) {
    case 1:
      src_start = GP2X_Phys(this, src->pixels) +(src_y*src->pitch) +(src_x>>3);
      src_ctrl = MESG_SRCBPP_1 | (src_x & 0x1f);
      break;
    case 8:
      src_start = GP2X_Phys(this, src->pixels) + (src_y * src->pitch) + src_x;
      src_ctrl = MESG_SRCBPP_8 | (src_start & 0x03) << 3;
      break;
    case 16:
      src_start = GP2X_Phys(this, src->pixels) +(src_y*src->pitch) +(src_x<<1);
      src_ctrl = MESG_SRCBPP_16 | (src_start & 0x02) << 3;
      break;
    default:
      SDL_SetError("Invalid bit depth for GP2X_HWBlit");
      return -1;
    }
    data->fio[MESGSRCCTRL] = src_ctrl | MESG_SRCENB | MESG_INVIDEO;
    data->fio[MESGSRCADDR] = src_start & ~3;
    data->fio[MESGSRCSTRIDE] = src_stride;
    asm volatile ("":::"memory");
    data->fio[MESGSTATUS] = MESG_BUSY;
  } else {
    // src SW surface needs CPU to pump blitter
    int src_int_width, frac;
    switch (src->format->BitsPerPixel) {
    case 1:
      src_start = (Uint32)src->pixels + (src_y * src->pitch) + (src_x >> 3);
      frac = src_x & 0x1f;
      src_ctrl = MESG_SRCENB | MESG_SRCBPP_1 | frac;
      src_int_width = (frac + w + 31) / 32;
      break;
    case 8:
      src_start = (Uint32)src->pixels + (src_y * src->pitch) + src_x;
      frac = (src_start & 0x03) << 3;
      src_ctrl = MESG_SRCENB | MESG_SRCBPP_8 | frac;
      src_int_width = (frac + w*8 + 31) / 32;
      break;
    case 16:
      src_start = (Uint32)src->pixels + (src_y * src->pitch) + (src_x << 1);
      frac = (src_start & 0x02) << 3;
      src_ctrl = MESG_SRCENB | MESG_SRCBPP_16 | frac;
      src_int_width = (frac + w*16 + 31) / 32;
      break;
    default:
      SDL_SetError("Invalid bit depth for GP2X_HWBlit");
      return -1;
    }
    data->fio[MESGSRCCTRL] = src_ctrl;
    asm volatile ("":::"memory");
    data->fio[MESGSTATUS] = MESG_BUSY;

    while (--h) {
      int i = src_int_width;
      read_addr = (Uint32 *)(src_start & ~3);
      src_start += src_stride;
      if (ctrl & MESG_XDIR_POS)
	while (--i)
	  data->fio[MESGFIFO] = 0xff; //*read_addr++;
      else
	while (--i)
	  data->fio[MESGFIFO] = 0x80; //*read_addr--;
    }
  }

  GP2X_AddBusySurface(src);
  GP2X_AddBusySurface(dst);

  if (dst == this->screen)
    SDL_mutexV(data->hw_lock);

  return 0;
}

////
// HW cursor support

// Support routine to fill cursor data
static void fill_cursor_data(Uint16 *cursor, int w, int h, int size,
			     int skip, Uint8 *data, Uint8 *mask)
{
  Uint16 *cursor_addr = cursor;
  Uint16 *cursor_end  = cursor + size;
  int x, y, datab, maskb, pixel, i, dimension;
  //############
  //######
  //###### HW LACE NEEDS FIXING
  //######
  //############
  dimension = (size == 256 ? 32 : 64);
  if (skip) skip = dimension << 4;
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x += 8) {
      datab = *data++;
      maskb = *mask++;
      pixel = 0;
      for (i = 8; i; i--) {
	pixel <<= 2;
	if (!(maskb & 0x01))
	  pixel |= 0x02;
	else if (!(datab & 0x01))
	  pixel |= 0x01;
	maskb >>= 1;
	datab >>= 1;
      }
      *cursor_addr++ = pixel;
    }
    while (x < dimension) {
      *cursor_addr++ = 0xAAAA;
      x += 8;
    }
    if (skip) {
      cursor_addr += skip;
      data += 4;
      mask += 4;
      y++;
    }
  }
  while(cursor_addr < cursor_end)
    *cursor_addr++ = 0xAAAA;
}

// Create cursor in HW format
static WMcursor *GP2X_CreateWMCursor(SDL_VideoDevice *video,
				     Uint8 *data, Uint8 *mask,
				     int w, int h,
				     int hot_x, int hot_y)
{
  SDL_PrivateVideoData *pvd = video->hidden;
  int cursor_size, cursor_dimension, x, y, i;
  Uint16 *cursor_addr, *cursor_end;
  Uint16 pixel;
  Uint8 datab, maskb;
  SDL_WMcursor *cursor;
#ifdef GP2X_DEBUG
  fprintf(stderr, "SDL_GP2X: Creating cursor %dx%d\n", w, h);
#endif

  // HW only supports 32x32 or 64x64. Pick smallest possible or crop
  cursor_dimension = ((w < h) ? h : w) <= 32 ? 32 : 64;
  cursor_size = cursor_dimension * cursor_dimension / 4;
  if (!(cursor = (SDL_WMcursor*)malloc(sizeof *cursor))) {
    SDL_OutOfMemory();
    return NULL;
  }
#ifdef GP2X_DEBUG
  fprintf(stderr, "SDL_GP2X: Allocated WMcursor @ %p (%d)\n",
	  cursor, cursor_dimension);
#endif

  if (!(cursor->bucket = GP2X_SurfaceAllocate(video, cursor_size))) {
    free(cursor);
    return NULL;
  }

  cursor->dimension = cursor_dimension;
  cursor->fgr = 0xffff;
  cursor->fb = 0xff;
  cursor->bgr = 0x0000;
  cursor->bb = 0x00;
  cursor->falpha = 0xf;
  cursor->balpha = 0xf;

  cursor_addr = (Uint16*)cursor->bucket->base;
  if (pvd->phys_ilace) {
    fill_cursor_data(cursor_addr, w, h, cursor_size, 1, data, mask);
    fill_cursor_data(cursor_addr + (cursor_size << 1), w, h, cursor_size, 1, data + (cursor_dimension << 4), mask + (cursor_dimension << 4));
  } else {
    fill_cursor_data(cursor_addr, w, h, cursor_size, 0, data, mask);
  }
  return (WMcursor*)cursor;
}

////
// Free the cursor memory
static void GP2X_FreeWMCursor(_THIS, WMcursor *wmcursor)
{
  SDL_WMcursor *cursor = (SDL_WMcursor*)wmcursor;
#ifdef GP2X_DEBUG
  fprintf(stderr, "SDL_GP2X: Freeing cursor %p\n", cursor);
#endif
  if (cursor->bucket)
    GP2X_SurfaceFree(this, cursor->bucket);
  free(cursor);
}

////
// Change HW cursor to passed, NULL to turn cursor off
static int GP2X_ShowWMCursor(_THIS, WMcursor *wmcursor)
{
  SDL_PrivateVideoData *data = this->hidden;
  unsigned short volatile *io = data->io;
  SDL_WMcursor *cursor = (SDL_WMcursor*)wmcursor;

  data->visible_cursor = cursor;
  if (cursor) {
    io[MLC_HWC_OADRL] = io[MLC_HWC_EADRL] =
      GP2X_PhysL(this, cursor->bucket->base);
    io[MLC_HWC_OADRH] = io[MLC_HWC_EADRH] =
      GP2X_PhysH(this, cursor->bucket->base);
    io[MLC_HWC_FGR] = cursor->fgr;
    io[MLC_HWC_FB] = cursor->fb;
    io[MLC_HWC_BGR] = cursor->bgr;
    io[MLC_HWC_BB] = cursor->bb;
    io[MLC_HWC_CNTL] = (cursor->falpha << 12) |
      (cursor->balpha << 8) |
      cursor->dimension;
    io[MLC_OVLAY_CNTR] |= DISP_CURSOR;
    return -1;
  } else
    io[MLC_OVLAY_CNTR] &= ~DISP_CURSOR;
  return 0;
}

////
// Set colour & alpha of a cursor (alpha is 0-15)
void SDL_GP2X_SetCursorColour(SDL_Cursor *scursor,
			      int bred, int bgreen, int bblue, int balpha,
			      int fred, int fgreen, int fblue, int falpha)
{
  SDL_PrivateVideoData *data = current_video->hidden;
  SDL_WMcursor *cursor = (SDL_WMcursor*)scursor->wm_cursor;

  cursor->fgr = ((fgreen & 0xFF) << 8) | (fred & 0xFF);
  cursor->fb = fblue & 0xFF;
  cursor->bgr = ((bgreen &0xFF) << 8) | (bred & 0xFF);
  cursor->bb = bblue & 0xFF;
  cursor->falpha = falpha & 0x0F;
  cursor->balpha = balpha & 0x0F;

  if (cursor == data->visible_cursor) {
    data->io[MLC_HWC_FGR] = cursor->fgr;
    data->io[MLC_HWC_FB] =  cursor->fb;
    data->io[MLC_HWC_BGR] = cursor->bgr;
    data->io[MLC_HWC_BB] =  cursor->bb;
    data->io[MLC_HWC_CNTL] = (cursor->falpha << 12) |
      (cursor->balpha << 8) |
      cursor->dimension;
  }
}


////
// Move the cursor to specified (physical) coordinate
static void GP2X_WarpWMCursor(_THIS, Uint16 x, Uint16 y)
{
  SDL_PrivateVideoData *data = this->hidden;

  data->cursor_px = x;
  data->cursor_py = y;
  //  data->io[MLC_HWC_STX] = x;
  //  data->io[MLC_HWC_STY] = y;
  SDL_PrivateMouseMotion(0, 0, x, y);
}

////
// Move the cursor to (virtual) coordinate
static void GP2X_MoveWMCursor(_THIS, int x, int y)
{
  SDL_PrivateVideoData *data = this->hidden;

  data->cursor_vx = x;
  data->cursor_vy = y;
  // convert virtual coordinate into physical
  x -= data->x_offset;
  x *= data->xscale;
  y -= data->y_offset;
  y *= data->yscale;
  data->cursor_px = x;
  data->cursor_py = y;
  data->io[MLC_HWC_STX] = x;
  data->io[MLC_HWC_STY] = y;
}


////////
// GP2X specific functions -

////
// Set foreground & background colours for 1bpp blits
void SDL_GP2X_SetMonoColours(int background, int foreground)
{
  if (current_video) {
    current_video->hidden->src_foreground = foreground;
    current_video->hidden->src_background = background;
  }
}

////
// Enquire physical screen size - for detecting LCD / TV
//  Returns 0: Progressive
//          1: Interlaced
int SDL_GP2X_GetPhysicalScreenSize(SDL_Rect *size)
{
  if (current_video) {
    SDL_PrivateVideoData *data = current_video->hidden;
    size->w = data->phys_width;
    size->h = data->phys_height;
    return data->phys_ilace;
  }
  return -1;
}

////
// Dynamic screen scaling
void SDL_GP2X_Display(SDL_Rect *area)
{
  SDL_PrivateVideoData *data = current_video->hidden;
  int sc_x, sc_y;

  // If top-left is out of bounds then correct it
  if (area->x < 0)
    area->x = 0;
  if (area->x > (data->w - 8))
    area->x = data->w - 8;
  if (area->y < 0)
    area->y = 0;
  if (area->y > (data->h - 8))
    area->y = data->h - 8;
  // if requested area is wider than screen, reduce width
  if (data->w < (area->x + area->w))
    area->w = data->w - area->x;
  // if requested area is taller than screen, reduce height
  if (data->h < (area->y + area->h))
    area->h = data->h - area->y;

  data->xscale = (float)data->phys_width / (float)area->w;
  data->yscale = (float)data->phys_height / (float)area->h;
  sc_x = (1024 * area->w) / data->phys_width;
  sc_y = (area->h * data->pitch) / data->phys_height;
  // Evil hacky thing. Scaler only works if horiz needs scaling.
  // If requested scale only needs to scale in vertical, fudge horiz
  if ((sc_x == 1024) && (area->h != data->phys_height))
    sc_x++;

  data->scale_x = sc_x;
  data->scale_y = sc_y;
  data->x_offset = area->x;
  data->y_offset = area->y;
  data->ptr_offset = ((area->y * data->pitch) +
		      (area->x *current_video->info.vfmt->BytesPerPixel)) & ~3;

  // Apply immediately if we're not double-buffered
  if (!(current_video->screen->flags & SDL_DOUBLEBUF)) {
    char *pixeldata = data->buffer_addr[data->buffer_showing]+data->ptr_offset;
    if (data->phys_ilace) {
      data->io[MLC_STL_OADRL] = GP2X_PhysL(current_video, pixeldata);
      data->io[MLC_STL_OADRH] = GP2X_PhysH(current_video, pixeldata);
      if (data->w == 720) pixeldata += data->pitch;
    }
    data->io[MLC_STL_EADRL] = GP2X_PhysL(current_video, pixeldata);
    data->io[MLC_STL_EADRH] = GP2X_PhysH(current_video, pixeldata);
    data->io[MLC_STL_HSC] = data->scale_x;
    data->io[MLC_STL_VSCL] = data->scale_y & 0xffff;
    data->io[MLC_STL_VSCH] = data->scale_y >> 16;
  }
}

////
// window region routines - 
//
// Set region area.
//   region = which hw region 1-4
//   area   = coords of area
void SDL_GP2X_DefineRegion(int region, SDL_Rect *area)
{
  if ((region >= 1) && (region <= 4) && (area)) {
    SDL_PrivateVideoData *data = current_video->hidden;
    unsigned short volatile *region_reg;
    
    region_reg = &data->io[MLC_STL1_STX + (region - 1) * 4];
    *region_reg++ = area->x;
    *region_reg++ = area->x + area->w - 1;
    *region_reg++ = area->y;
    *region_reg++ = area->y + area->h - 1;
  }
}

// (De)activate region
void SDL_GP2X_ActivateRegion(int region, int activate)
{
  if ((region >= 1) && (region <= 5)) {
    SDL_PrivateVideoData *data = current_video->hidden;
    int stl_region_bit = 1 >> ((region - 1) * 2);
    int ovlay_region_bit = 1 >> (region + 1);

    if (activate) {
      data->io[MLC_STL_CNTL] |= stl_region_bit;
      data->io[MLC_OVLAY_CNTR] |= ovlay_region_bit;
    } else {
      data->io[MLC_STL_CNTL] &= ~stl_region_bit;
      data->io[MLC_OVLAY_CNTR] &= ~ovlay_region_bit;
    }
  }
}

// Allow a smaller screen than the display, without scaling
void SDL_GP2X_MiniDisplay(int x, int y)
{
  SDL_PrivateVideoData *data = current_video->hidden;
  SDL_Rect mini_region;

  // Set scaler back to 1:1
  data->scale_x = 1024;
  data->scale_y = data->phys_pitch;
  data->xscale = data->yscale = 1.0;
  // offsets needed to start screen at (x,y)
  data->x_offset = -x;
  data->y_offset = -y;
  data->ptr_offset = -((y * data->pitch) +
		       (x *current_video->info.vfmt->BytesPerPixel)) & ~3;
  // Apply immediately if we're not double-buffered
  if (!(current_video->screen->flags & SDL_DOUBLEBUF)) {
    char *pixeldata = data->buffer_addr[data->buffer_showing]+data->ptr_offset;
    if (data->phys_ilace) {
      data->io[MLC_STL_OADRL] = GP2X_PhysL(current_video, pixeldata);
      data->io[MLC_STL_OADRH] = GP2X_PhysH(current_video, pixeldata);
      if (data->w == 720) pixeldata += data->pitch;
    }
    data->io[MLC_STL_EADRL] = GP2X_PhysL(current_video, pixeldata);
    data->io[MLC_STL_EADRH] = GP2X_PhysH(current_video, pixeldata);
    data->io[MLC_STL_HSC] = 1024;
    data->io[MLC_STL_VSCL] = data->scale_y & 0xffff;
    data->io[MLC_STL_VSCH] = data->scale_y >> 16;
  }

  mini_region.x = x;
  mini_region.y = y;
  mini_region.w = data->w;
  mini_region.h = data->h;
  SDL_GP2X_DefineRegion(1, &mini_region);
}

// Lets the user wait for the blitter to finish
void SDL_GP2X_WaitForBlitter()
{
  GP2X_WaitBusySurfaces(current_video);
}

static int tv_device = 0;

// Switch TV mode on/off, and set position & offsets
int SDL_GP2X_TV(int state)
{
  if (state == 0) {   // Turn TV off
    if (tv_device) {  // close device to return to LCD
      fputs("Closing CX25874\n", stderr);
      close(tv_device);
      tv_device = 0;
    }
    return 0;
  }

  // Turn TV on
  if (!tv_device) {  // open device to enable TV
    fputs("Opening CX25874\n", stderr);
    tv_device = open("/dev/cx25874", O_RDWR, 0);
    if (!tv_device) {
      SDL_SetError("Failed to open TV device.");
      return 0;
    }
  }
  return 1;
}

int SDL_GP2X_TVMode(int mode)
{
  // No device open, or mode is out of range
  if ((!tv_device) || (mode < 1) || (mode > 5))
    return -1;
  fprintf(stderr, "Switching tv mode to %d\n", mode);
  ioctl(tv_device, IOCTL_CX25874_DISPLAY_MODE_SET, mode);
  return 0;
}

void SDL_GP2X_TVAdjust(int direction)
{
  if ((!tv_device) || (direction < 0) || (direction > 3))
    return;

  fprintf(stderr, "Moving TV by %d\n", direction);
  ioctl(tv_device, IOCTL_CX25874_TV_MODE_POSITION, direction);
}

////
// Mark gfx memory as allowed
void SDL_GP2X_AllowGfxMemory(char *start, int size)
{
  SDL_PrivateVideoData *data = current_video->hidden;
  char *end = start + size;
  char *block = GP2X_UPPER_MEM_START;  // Start of upper memory

  data->allow_scratch_memory = 1;
}
  

void SDL_GP2X_DenyGfxMemory(char *start, int size)
{
  SDL_PrivateVideoData *data = current_video->hidden;
  data->allow_scratch_memory = 0;
}

// MPO : small, useful replacement for SDL_GetTicks
// (video subsystem _must_ be initialized for this to work!)
// Returns a value that can be converted to relative milliseconds by
//  dividing by 7372.8 (or 7373 for better speed)
// This timer value wraps around every 10 minutes (approximately)
unsigned int SDL_GP2X_GetMiniTicks()
{
	SDL_PrivateVideoData *data = current_video->hidden;
	return ((unsigned int *) data->io)[0xA00 >> 2];
}
