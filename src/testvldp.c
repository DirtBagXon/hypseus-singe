#include "vldp2/vldp/vldp.h"	// VLDP stuff
#include "io/dll.h"	// for DLL stuff
#include "video/yuv2rgb.h"	// to bypass SDL's yuv overlay and do it ourselves ...

#ifndef WIN32
#include <unistd.h>	// for chdir
#include <string.h>
#include <stdio.h>	// for stderr
#include <stdlib.h>	// for srand
#endif

#define MPO_MALLOC(bytes) malloc(bytes)

// only frees memory if it hasn't already been freed
#define MPO_FREE(ptr) if (ptr != 0) { free(ptr); ptr = 0; }

const unsigned int uVidWidth = 320;
const unsigned int uVidHeight = 240;

typedef const struct vldp_out_info *(*initproc)(const struct vldp_in_info *in_info);
initproc pvldp_init;	// pointer to the init proc ...

// pointer to all functions the VLDP exposes to us ...
const struct vldp_out_info *g_vldp_info = NULL;

// info that we provide to the VLDP DLL
struct vldp_in_info g_local_info;

unsigned int g_uFrameCount = 0;	// to measure framerate

unsigned int g_uQuitFlag = 0;

SDL_Surface *g_screen = NULL;

#ifdef USE_OVERLAY
SDL_Rect *g_screen_clip_rect = NULL;	// used a lot, we only want to calulcate once
SDL_Overlay *g_hw_overlay = NULL;
#endif // USE_OVERLAY

Uint8 *g_line_buf = NULL;	// temp sys RAM for doing calculations so we can do fastest copies to slow video RAM
Uint8 *g_line_buf2 = NULL;	// 2nd buf
Uint8 *g_line_buf3 = NULL;	// 3rd buf

void printerror(const char *cpszErrMsg)
{
	puts(cpszErrMsg);
}

// This function converts the YV12-formatted 'src' to a YUY2-formatted overlay (which Xbox-Daphne may be using)
// copies the contents of src into dst
// assumes destination overlay is locked and *IMPORTANT* assumes src and dst are the same resolution
void buf2overlay_YUY2(SDL_Overlay *dst, struct yuv_buf *src)
{
	unsigned int channel0_pitch = dst->pitches[0];
	Uint8 *dst_ptr = (Uint8 *) dst->pixels[0];			
	Uint8 *Y = (Uint8 *) src->Y;
	Uint8 *Y2 = ((Uint8 *) src->Y) + dst->w;
	Uint8 *U = (Uint8 *) src->U;
	Uint8 *V = (Uint8 *) src->V;
	int col, row;

	// do 2 rows at a time
	for (row = 0; row < (dst->h >> 1); row++)
	{
		// do 4 bytes at a time, but only iterate for w_half
		for (col = 0; col < (dst->w << 1); col += 4)
		{
			unsigned int Y_chunk = *((Uint16 *) Y);
			unsigned int Y2_chunk = *((Uint16 *) Y2);
			unsigned int V_chunk = *V;
			unsigned int U_chunk = *U;
			
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
			
			//Little-Endian (PC)
			*((Uint32 *) (g_line_buf + col)) = (Y_chunk & 0xFF) | (U_chunk << 8) |
				((Y_chunk & 0xFF00) << 8) | (V_chunk << 24);
			*((Uint32 *) (g_line_buf2 + col)) = (Y2_chunk & 0xFF) | (U_chunk << 8) |
				((Y2_chunk & 0xFF00) << 8) | (V_chunk << 24);
			
#else
			
			//Big-Endian (Mac)
			*((Uint32 *) (g_line_buf + col)) = ((Y_chunk & 0xFF00) << 16) | ((U_chunk) << 16) |
				((Y_chunk & 0xFF) << 8) | (V_chunk);
			*((Uint32 *) (g_line_buf2 + col)) = ((Y2_chunk & 0xFF00) << 16) | ((U_chunk) << 16) |
				((Y2_chunk & 0xFF) << 8) | (V_chunk);	
			
#endif
			
			Y += 2;
			Y2 += 2;
			U++;
			V++;
		}
		
		memcpy(dst_ptr, g_line_buf, (dst->w << 1));
		memcpy(dst_ptr + channel0_pitch, g_line_buf2, (dst->w << 1));
		
		dst_ptr += (channel0_pitch << 1);	// we've done 2 rows, so skip a row
		Y += dst->w;	// we've done 2 vertical Y pixels, so skip a row
		Y2 += dst->w;
	}
}

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define MAKE16(R,G,B) (((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3))
//#define MAKE16(R,G,B) (((0xF8) << 8) | ((0xFC) << 3) | ((0 & 0xF8) >> 3))
#else
#endif

int prepare_frame_callback(struct yuv_buf *src)
{
	VLDP_BOOL result = VLDP_TRUE;

#ifdef SHOW_FRAMES
#ifdef USE_OVERLAY
	// if locking the video overlay is successful
	if (SDL_LockYUVOverlay(g_hw_overlay) == 0)
	{
		buf2overlay_YUY2(g_hw_overlay, src);
		SDL_UnlockYUVOverlay(g_hw_overlay);
	}
	else result = VLDP_FALSE;
#else
	if (SDL_LockSurface(g_screen) == 0)
	{
		unsigned int uPitch = g_screen->pitch;
		Uint8 *dst_ptr = (Uint8 *) g_screen->pixels;
		unsigned int uW = uVidWidth;
		unsigned int uH = uVidHeight;
		Uint8 *Y = (Uint8 *) src->Y;
		Uint8 *Y2 = ((Uint8 *) src->Y) + uVidWidth;	// Y's width is assumed to be the same size as output screen
		Uint8 *U = (Uint8 *) src->U;
		Uint8 *V = (Uint8 *) src->V;
		int col, row;

		// do 2 rows at a time
		for (row = 0; row < (uH >> 1); row++)
		{
			// do a 2x2 block at a time (2 pixels = 4 bytes)
			for (col = 0; col < (uW << 1); col += 4)
			{
				// read as int32's for faster memory efficiency
				unsigned int Y_chunk32 = *((Uint32 *) Y);
				unsigned int Y2_chunk32 = *((Uint32 *) Y2);
				unsigned int V_chunk32 = *((Uint32 *) V);
				unsigned int U_chunk32 = *((Uint32 *) U);

				// encourage these to be registers
				register unsigned int uR, uG, uB, uR2, uG2, uB2;

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
				//Little-Endian (x86)

/*
				// 0x0
				YUV2RGB(Y_chunk32 & 0xFF, U_chunk32 & 0xFF, V_chunk32 & 0xFF, uR, uG, uB);
				// 1x0
				YUV2RGB((Y_chunk32 >> 8) & 0xFF, U_chunk32 & 0xFF, V_chunk32 & 0xFF, uR2, uG2, uB2);
				*((Uint32 *) (g_line_buf + col)) = MAKE16(uR, uG, uB) | (MAKE16(uR2, uG2, uB2) << 16);

				// 0x1
				YUV2RGB(Y2_chunk32 & 0xFF, U_chunk32 & 0xFF, V_chunk32 & 0xFF, uR, uG, uB);
				// 1x1
				YUV2RGB((Y2_chunk32 >> 8) & 0xFF, U_chunk32 & 0xFF, V_chunk32 & 0xFF, uR2, uG2, uB2);
				*((Uint32 *) (g_line_buf2 + col)) = MAKE16(uR, uG, uB) | (MAKE16(uR2, uG2, uB2) << 16);
*/

				// black and white, just a test to see if our logic thus far is good
				uR = Y_chunk32 & 0xFF;
				uR2 = (Y_chunk32 >> 8) & 0xFF;
				*((Uint32 *) (g_line_buf + col)) = MAKE16(uR, uR, uR) | (MAKE16(uR2, uR2, uR2) << 16);
				uR = Y2_chunk32 & 0xFF;
				uR2 = (Y2_chunk32 >> 8) & 0xFF;
				*((Uint32 *) (g_line_buf2 + col)) = MAKE16(uR, uR, uR) | (MAKE16(uR2, uR2, uR2) << 16);
#else
#endif // LIL ENDIAN
				
				Y += 2;
				Y2 += 2;
				U++;
				V++;
			}
			
			memcpy(dst_ptr, g_line_buf, uW << 1);
			memcpy(dst_ptr + uPitch, g_line_buf2, uW << 1);
			
			dst_ptr += (uPitch << 1);	// we've done 2 rows, so skip a row
			Y += uW;	// we've done 2 vertical Y pixels, so skip a row
			Y2 += uW;
		}
		SDL_UnlockSurface(g_screen);
	}
/*
	else
	{
		printerror("Can't lock surface!");
	}
*/

#endif // USE_OVERLAY
#endif // SHOW FRAMES

	return result;
}

struct sdl_gp2x_yuvhwdata
{
	// the first few fields in this struct
	Uint8 *YUVBuf[2];
	unsigned int GP2XAddr[2];
	unsigned int uBackBuf;
};

int prepare_yuy2_frame_callback(struct yuy2_buf *src)
{
	VLDP_BOOL result = VLDP_TRUE;

#ifdef SHOW_FRAMES
#ifdef USE_OVERLAY
	// if locking the video overlay is successful
	if (SDL_LockYUVOverlay(g_hw_overlay) == 0)
	{
#ifdef GP2X
		struct sdl_gp2x_yuvhwdata *pData = (struct sdl_gp2x_yuvhwdata *) g_hw_overlay->hwdata;
		pData->GP2XAddr[pData->uBackBuf] = src->uPhysAddress;	// redirect sdl's buffer to the buffer that already has the image
#endif // GP2X
		SDL_UnlockYUVOverlay(g_hw_overlay);
	}
	else result = VLDP_FALSE;
#endif // USE_OVERLAY
#endif // SHOW FRAMES

	return result;
}

void display_frame_callback(struct yuv_buf *buf)
{
#ifdef SHOW_FRAMES
#ifdef USE_OVERLAY
	SDL_DisplayYUVOverlay(g_hw_overlay, g_screen_clip_rect);
#else
	SDL_Flip(g_screen);	// display it!
#endif // USE_OVERLAY
#endif // SHOW_FRAMES

	g_uFrameCount++;
}

void display_yuy2_frame_callback()
{
#ifdef SHOW_FRAMES
#ifdef USE_OVERLAY
	SDL_DisplayYUVOverlay(g_hw_overlay, g_screen_clip_rect);
#endif
#endif
	g_uFrameCount++;
}

void report_parse_progress_callback(double percent_complete_01)
{
}

#include <signal.h>

void report_mpeg_dimensions_callback(int width, int height)
{
#ifdef SHOW_FRAMES
#ifdef USE_OVERLAY
	g_hw_overlay = SDL_CreateYUVOverlay (width, height, SDL_YUY2_OVERLAY, g_screen);
	printf("HW Overlay: %u\n", g_hw_overlay->hw_overlay);

	// if we're not using hardware, then abort this "test"
	if (g_hw_overlay->hw_overlay == 0)
	{
		printf("Since we have no accel, we're shutting down ...\n");
		raise(SIGTRAP);
		g_uQuitFlag = 1;
	}

	g_screen_clip_rect = &g_screen->clip_rect;	// used a lot, we only want to calculate it once
#else
	if ((width != uVidWidth) || (height != uVidHeight))
	{
		printf("mpeg is the wrong dimensions (%d x %d)\n", width, height);
		g_uQuitFlag = 1;
	}
#endif // USE_OVERLAY

	// yuy2 needs twice as much space across for lines
	g_line_buf = MPO_MALLOC(width * 2);
	g_line_buf2 = MPO_MALLOC(width * 2);
	g_line_buf3 = MPO_MALLOC(width * 2);

#endif // SHOW_FRAMES
}

void blank_overlay_callback()
{
}

unsigned int get_ticks()
{
	return SDL_GetTicks();
}

void start_test(const char *cpszMpgName)
{
	g_uFrameCount = 0;
	if (g_vldp_info->open_and_block(cpszMpgName))
	{
		unsigned int uStartMs = get_ticks();
		g_local_info.uMsTimer = uStartMs;
		if (g_vldp_info->play(uStartMs))
		{
			unsigned int uEndMs, uDiffMs;
			// wait for entire mpeg to play ...
			while ((g_vldp_info->status == STAT_PLAYING) && (!g_uQuitFlag) && (g_uFrameCount < 720))
			{
				g_local_info.uMsTimer = get_ticks();
				usleep(1000);
//				SDL_Delay(1);
			}
			uEndMs = get_ticks();
			uDiffMs = uEndMs - uStartMs;
			printf("%u frames played in %u ms (%u fps)\n", g_uFrameCount,
				uDiffMs, (g_uFrameCount * 1000) / (uDiffMs));
		}
		else printerror("play() failed");
	}
	else printerror("Could not open video file");
}

void set_cur_dir(const char *exe_loc)
{
	int index = strlen(exe_loc) - 1;	// start on last character
	char path[512];

	// locate the preceeding / or \ character
	while ((index >= 0) && (exe_loc[index] != '/') && (exe_loc[index] != '\\'))
	{
		index--;
	}

	// if we found a leading / or \ character
	if (index >= 0)
	{
		strncpy(path, exe_loc, index);
		path[index] = 0;
		chdir(path);
	}
}

int main(int argc, char **argv)
{
//	DLL_INSTANCE dll_instance = NULL;

	set_cur_dir(argv[0]);
#ifdef SHOW_FRAMES
	SDL_Init(SDL_INIT_VIDEO);
	SDL_ShowCursor(SDL_DISABLE);	// always hide mouse for gp2x
	g_screen = SDL_SetVideoMode(uVidWidth, uVidHeight, 0, SDL_HWSURFACE);
	printf("Video mode set to %d bpp\n", g_screen->format->BitsPerPixel);

	// just a test ...
	printf("Making dummy overlay\n");
	SDL_Overlay *pDummy = SDL_CreateYUVOverlay(320, 240, SDL_YUY2_OVERLAY, g_screen);
	if (pDummy)
	{
		printf("Hw accel of dummy is %u\n", pDummy->hw_overlay);
		SDL_FreeYUVOverlay(pDummy);
	}

#endif

#ifndef STATIC_VLDP
	dll_instance = M_LOAD_LIB(vldp2);	// load VLDP2.DLL or libvldp2.so

	// If the handle is valid, try to get the function address. 
	if (dll_instance)
	{
		pvldp_init = (initproc) M_GET_SYM(dll_instance, "vldp_init");
#else
		pvldp_init = (initproc) &vldp_init;
#endif // STATIC_VLDP
		
		// if all functions were found, then return success
		if (pvldp_init)
		{
			g_local_info.prepare_frame = prepare_frame_callback;
			g_local_info.prepare_yuy2_frame = prepare_yuy2_frame_callback;
			g_local_info.display_frame = display_frame_callback;
			g_local_info.display_yuy2_frame = display_yuy2_frame_callback;
			g_local_info.report_parse_progress = report_parse_progress_callback;
			g_local_info.report_mpeg_dimensions = report_mpeg_dimensions_callback;
			g_local_info.render_blank_frame = blank_overlay_callback;
			g_local_info.blank_during_searches = 0;
			g_local_info.blank_during_skips = 0;
			g_local_info.GetTicksFunc = get_ticks;
			g_vldp_info = pvldp_init(&g_local_info);
			
			// if initialization succeeded
			if (g_vldp_info != NULL)
			{
				start_test(argv[1]);
				g_vldp_info->shutdown();
			}
			else printerror("vldp_init() failed");
		}
		else
		{
			printerror("VLDP LOAD ERROR : vldp_init could not be loaded");
		}
#ifndef STATIC_VLDP
		M_FREE_LIB(dll_instance);
	}
	else
	{
		printerror("ERROR: could not open the VLDP2 dynamic library (file not found maybe?)");
	}
#endif // STATIC_VLDP

#ifdef SHOW_FRAMES

#ifdef USE_OVERLAY
	if (g_hw_overlay)
	{
		SDL_FreeYUVOverlay(g_hw_overlay);
	}
	g_hw_overlay = NULL;
#endif // USE_OVERLAY

	MPO_FREE(g_line_buf);
	MPO_FREE(g_line_buf2);
	MPO_FREE(g_line_buf3);

	SDL_Quit();

#endif // SHOW_FRAMES
	
	return 0;
}

