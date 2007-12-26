/*
    SDL - Simple DirectMedia Layer - SDL_gp2xyuv.c extension
    Copyright (C) 2007 Matt Ownby

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
*/

/* This is the GP2X implementation of YUV video overlays */

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>	// for open
#include <sys/mman.h>	// for mmap
#include <unistd.h>	// for close

#ifdef GP2X_DEBUG
	#include <assert.h>	// for self testing ...
#endif // GP2X_DEBUG

#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_gp2xyuv_c.h"
#include "../SDL_yuvfuncs.h"
#include "mmsp2_regs.h"
//#include "arm_colorconv.h"

/* The functions used to manipulate software video overlays */
static struct private_yuvhwfuncs gp2x_yuvfuncs =
{
	GP2X_LockYUVOverlay,
	GP2X_UnlockYUVOverlay,
	GP2X_DisplayYUVOverlay,
	GP2X_FreeYUVOverlay
};

struct private_yuvhwdata
{
	Uint8 *YUVBuf[2];	// we will have two buffers, a front and back (double buffered)
	unsigned int GP2XAddr[2];	// the yuv buffer address from the gp2x's perspective
	unsigned int uBackBuf;	// which buffer is currently the back buffer (0 or 1)
	unsigned int uBufSize;	// how big the YUVBuf buffers are (in bytes)
	SDL_Rect rctLast;	// the last rectangle we used for displaying (so we know when to rescale/change coordinates)
	Uint16 u16PixelWidth;	// value stored for MLC_VLA_TP_PXW register

	/* These are just so we don't have to allocate them separately */
	Uint16 pitches[3];
	Uint8 *planes[3];
};

// These are the registers (currently) modified by this code;
// They are in this array so we can restore them to their previous state after the overlay is destroyed.
const unsigned int g_pTouchedRegs[] =
{
	MLC_OVLAY_CNTR,
	0x2882 >> 1,
	0x2884 >> 1,
	0x2886 >> 1,
	0x288A >> 1,
	0x288C >> 1,
	0x2892 >> 1,
	0x2896 >> 1,
	0x2898 >> 1,
	0x289A >> 1,
	0x289C >> 1,
	0x289E >> 1,
	0x28A0 >> 1,
	0x28A2 >> 1,
	0x28A4 >> 1,
	0x28A6 >> 1,
	0x28C0 >> 1,
	0x28C2 >> 1
};

const unsigned int TOUCHEDREGS_COUNT = sizeof(g_pTouchedRegs) / sizeof(unsigned int);

// where we save the regs (compiler wouldn't let me use TOUCHEDREGS_COUNT in array)
static unsigned short g_pu16SavedRegs[sizeof(g_pTouchedRegs) / sizeof(unsigned int)];

#ifdef GP2X_DEBUG
	// to test whether saving and restoring the regs is working
	void *g_pRegTester = NULL;
#endif // GP2X_DEBUG

// saves or restores registers that are used by the YUV code depending on 'iSave'
static void GP2X_SaveRestoreRegs(_THIS, int iSave)
{
	SDL_PrivateVideoData *data = this->hidden;
	unsigned int u = 0;

	for (u = 0; u < TOUCHEDREGS_COUNT; u++)
	{
		// if we're saving the registers
		if (iSave)
		{
			g_pu16SavedRegs[u] = data->io[g_pTouchedRegs[u]];
		}
		// else we're restoring the registers
		else
		{
			data->io[g_pTouchedRegs[u]] = g_pu16SavedRegs[u];
		}
	}
}

static int GP2X_SetupYUV(_THIS, int width, int height, Uint32 format, struct private_yuvhwdata *hwdata)
{
	int iSuccess = 0;
	SDL_PrivateVideoData *data = this->hidden;
	unsigned short volatile *p16GP2XRegs = data->io;

	hwdata->uBufSize = width * height * 2;	// YUY2 is (2 * width) bytes per line

	// initialize to values that will never be used so that we force a rescale the first time GP2X_DisplayYUVOverlay is called
	hwdata->rctLast.h = hwdata->rctLast.w = 0;
	hwdata->rctLast.x = hwdata->rctLast.y = -1;

	hwdata->uBackBuf = 0;	// arbitrary starting point (can be 0 or 1)

	// We're using GP2X_UPPER_MEM_START because that's where vmem is mapped in SDL_gp2xvideo.c
	hwdata->GP2XAddr[0] = GP2X_UPPER_MEM_START;	// this is where vmem is mapped from the gp2x's pov
	hwdata->GP2XAddr[1] = GP2X_UPPER_MEM_START + hwdata->uBufSize;

	//map memory for our cursor + 4 YUV regions (double buffered each one)
	hwdata->YUVBuf[0] = (Uint8 *) data->vmem;
	hwdata->YUVBuf[1] = ((Uint8 *) data->vmem) + hwdata->uBufSize;

	// tests have shown that this value should be the width of the overlay, not the width of the display (ie not always 320)
	hwdata->u16PixelWidth = width;

	// Set MLC_VLA_TP_PXW
	p16GP2XRegs[0x2892>>1] = hwdata->u16PixelWidth;

	// MLC_YUV_CNTL: set YUV source to external memory for regions A and B, turn off "stepping"
	p16GP2XRegs[0x2884>>1]=0;

	// disable all RGB Windows, and YUV region B
	p16GP2XRegs[MLC_OVLAY_CNTR] &= ~(DISP_STL5EN|DISP_STL4EN|DISP_STL3EN|DISP_STL2EN|DISP_STL1EN|DISP_VLBON);

	// enable YUV region A
	p16GP2XRegs[MLC_OVLAY_CNTR] |= DISP_VLAON;

	// disable bottom regions for A & B, and turn off all mirroring
	p16GP2XRegs[0x2882>>1] &= 0xFC00;

	// Region B is disabled so we (hopefully) don't need to set anything for it

	// There currently is no way for this function to fail, but I'm leaving this in in case a future way presents itself
	iSuccess = 1;

	return iSuccess;
}

SDL_Overlay *GP2X_CreateYUVOverlay(_THIS, int width, int height, Uint32 format, SDL_Surface *display)
{
	SDL_Overlay *overlay = NULL;
	struct private_yuvhwdata *hwdata;

	// we ONLY support YUY2 because that is the native gp2x YUV format
	if (format != SDL_YUY2_OVERLAY)
	{
		printf("SDL_GP2X: Unsupported YUV format for hardware acceleration. Falling back to software.\n");
		return NULL;
	}

	/* Create the overlay structure */
	overlay = (SDL_Overlay *)malloc(sizeof *overlay);
	if ( overlay == NULL ) {
		SDL_OutOfMemory();
		return(NULL);
	}
	memset(overlay, 0, (sizeof *overlay));

	/* Fill in the basic members */
	overlay->format = format;
	overlay->w = width;
	overlay->h = height;

	/* Set up the YUV surface function structure */
	overlay->hwfuncs = &gp2x_yuvfuncs;

	// save our registers
	GP2X_SaveRestoreRegs(this, 1);

#ifdef GP2X_DEBUG
	// this is a self-test precaution
	{
		SDL_PrivateVideoData *data = this->hidden;
		unsigned short volatile *p16GP2XRegs = data->io;
		g_pRegTester = malloc(0x50);
		memcpy(g_pRegTester, (const void *) &p16GP2XRegs[0x2880 >> 1], 0x50);	// store a big chunk of register info so we can compare it later
	}
#endif // GP2X_DEBUG

	/* Create the pixel data and lookup tables */
	hwdata = (struct private_yuvhwdata *)malloc(sizeof *hwdata);
	overlay->hwdata = hwdata;
	if ( hwdata == NULL )
	{
		SDL_OutOfMemory();
		SDL_FreeYUVOverlay(overlay);
		return(NULL);
	}

	// if YUV setup failed
	if (!GP2X_SetupYUV(this, width, height, format, hwdata))
	{
		SDL_FreeYUVOverlay(overlay);
		return(NULL);
	}

	overlay->hw_overlay = 1;

	/* Set up the plane pointers */
	overlay->pitches = hwdata->pitches;
	overlay->pixels = hwdata->planes;
	overlay->planes = 1;	// since we only support YUY2, there is always only 1 plane

	// the pitch will always be the overlay width * 2, because we only support YUY2
	overlay->pitches[0] = overlay->w * 2;

	// start on the correct back buffer
	overlay->pixels[0] = hwdata->YUVBuf[hwdata->uBackBuf];

	/* We're all done.. */
	printf("SDL_GP2X: Created YUY2 overlay\n");
	return(overlay);
}

int GP2X_LockYUVOverlay(_THIS, SDL_Overlay *overlay)
{
	return(0);
}

void GP2X_UnlockYUVOverlay(_THIS, SDL_Overlay *overlay)
{
}

int GP2X_DisplayYUVOverlay(_THIS, SDL_Overlay *overlay, SDL_Rect *dstrect)
{
	struct private_yuvhwdata *hwdata = overlay->hwdata;
	unsigned int uAddress = (unsigned int) hwdata->GP2XAddr[hwdata->uBackBuf];
	SDL_PrivateVideoData *data = this->hidden;
	unsigned short volatile *p16GP2XRegs = data->io;

	// check to see if we are using a different rectangle
	// (This should be a faster way than doing a bunch of conditionals and comparisons)
	if (((dstrect->w ^ hwdata->rctLast.w) | (dstrect->h ^ hwdata->rctLast.h) | (dstrect->x ^ hwdata->rctLast.x) |
		(dstrect->y ^ hwdata->rctLast.y)) == 0)
	{
		// if we get here, it means that the rectangle size has not changed, so we can flip after vblank
		GP2X_WaitVBlank(data);
	}
	// else we are using a different rectangle
	else
	{
		Uint16 u16HScaleVal = (unsigned short)((1024 * overlay->w) / dstrect->w);
		unsigned int uVScaleVal = (unsigned int) ((hwdata->u16PixelWidth * overlay->h) / dstrect->h);

		// the boundary of the window on the right and bottom sides
		// (-1 because 0 is the first pixel)
		Sint16 s16Right, s16Bottom;
		Sint16 x = dstrect->x;
		Sint16 y = dstrect->y;

#ifdef GP2X_DEBUG
		// warn the user if this extra math is being done every frame
		printf("SDL_GP2X: GP2X YUV Overlay is being resized. This usually should NOT happen every frame!\n");
#endif // GP2X_DEV

		// gp2x doesn't seem to handle negative coordinates
		if (x < 0) x = 0;
		if (y < 0) y = 0;

		s16Right = x + (dstrect->w - 1);
		s16Bottom = y + (dstrect->h - 1);

		// gp2x doesn't seem like the coordinates going outside the physical boundaries
		if (s16Right > 319)	s16Right = 319;
		if (s16Bottom > 239) s16Bottom = 239;

		// now that we've done all the calculations we can before changing registers, wait for vblank
		GP2X_WaitVBlank(data);

		// we need to rescale now ...
		// MLC_YUVA_TP_HSC (horizontal scale factor of Region A, top)
		p16GP2XRegs[0x2886>>1] = u16HScaleVal;

		// MLC_YUVA_TP_VSC[L/H] (vertical scale factor of Region A, top)
		p16GP2XRegs[0x288A>>1] = (unsigned short) (uVScaleVal & 0xFFFF);
		p16GP2XRegs[0x288C>>1] = (unsigned short) (uVScaleVal >> 16);	// this will usually be 0

		// NOW SET COORDINATES

		// MLC_YUVA_STX (X start coordinate of region A)
		p16GP2XRegs[0x2896>>1]=x;

		// MLC_YUVA_ENDX (X stop coordinate of region A)
		p16GP2XRegs[0x2898>>1]=s16Right;

		// MLC_YUV_TP_STY (Y start coordinate of region A top)
		p16GP2XRegs[0x289A>>1]=y;

		// MLC_YUV_TP_ENDY (Y stop coordinate of region A top, and the start of the bottom)
		p16GP2XRegs[0x289C>>1] = s16Bottom;

		// MLC_YUV_BT_ENDY (Y stop coordinate of region A bottom)
		p16GP2XRegs[0x289E>>1] = s16Bottom;

		// copy new rectangle over to rctLast so that we don't do this stuff on every single frame
		hwdata->rctLast = *dstrect;
	}

	// NOW FLIP THE ACTUAL BUFFER

	// NOTE : I am flipping the odd and even fields here because that's how rlyeh did it, but I am not sure
	//  whether both of these need to be flipped.

	// region A, odd fields
	p16GP2XRegs[0x28A0>>1] = (uAddress & 0xFFFF);
	p16GP2XRegs[0x28A2>>1] = (uAddress >> 16);

	// region A, even fields
	p16GP2XRegs[0x28A4>>1] = (uAddress & 0xFFFF);
	p16GP2XRegs[0x28A6>>1] = (uAddress >> 16);

	// change back buffer to the other buffer (can be 0 or 1, so XOR works nicely)
	hwdata->uBackBuf ^= 1;

	// update the pixel pointer to new back buffer
	overlay->pixels[0] = hwdata->YUVBuf[hwdata->uBackBuf];

	return(0);
}

void GP2X_FreeYUVOverlay(_THIS, SDL_Overlay *overlay)
{
	struct private_yuvhwdata *hwdata;
	unsigned short volatile *p16GP2XRegs = this->hidden->io;

	hwdata = overlay->hwdata;

	if ( hwdata )
	{
		free(hwdata);
	}

	// restore our registers
	GP2X_SaveRestoreRegs(this, 0);

#ifdef GP2X_DEBUG
	// this is a self-test precaution

	// make sure that the registers were restored properly
	assert (memcmp(g_pRegTester, (const void *) &p16GP2XRegs[0x2880 >> 1], 0x50) == 0);

	free(g_pRegTester);
#endif // GP2X_DEBUG

}


