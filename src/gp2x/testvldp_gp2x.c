#include "vldp2/vldp/vldp.h"	// VLDP stuff
#include "io/dll.h"	// for DLL stuff
#include "video/yuv2rgb.h"	// to bypass SDL's yuv overlay and do it ourselves ...

#ifdef GP2X
#include "minimal.h"	// using minimal library for speed
#endif

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

Uint8 *g_line_buf = NULL;	// temp sys RAM for doing calculations so we can do fastest copies to slow video RAM
Uint8 *g_line_buf2 = NULL;	// 2nd buf
Uint8 *g_line_buf3 = NULL;	// 3rd buf

void printerror(const char *cpszErrMsg)
{
	puts(cpszErrMsg);
}

// This function converts the YV12-formatted 'src' to a YUY2-formatted GP2X memory (using rlyeh's minimal library)
// *IMPORTANT* assumes src and dst memory are the same resolution
void buf2overlay_YUY2_GP2X(struct yuv_buf *src)
{
	unsigned int channel0_pitch = uVidWidth << 1;	// YUY2 has 2 bytes per pixel
	Uint8 *dst_ptr = (Uint8 *) gp2x_video_YUV[0].screen32;
	Uint8 *Y = (Uint8 *) src->Y;
	Uint8 *Y2 = ((Uint8 *) src->Y) + uVidWidth;
	Uint8 *U = (Uint8 *) src->U;
	Uint8 *V = (Uint8 *) src->V;
	int col, row;

	// do 2 rows at a time
	for (row = 0; row < (uVidHeight >> 1); row++)
	{
		// do 4 bytes at a time, but only iterate for w_half
		for (col = 0; col < (uVidWidth << 1); col += 4)
		{
			unsigned int Y_chunk = *((Uint16 *) Y);
			unsigned int Y2_chunk = *((Uint16 *) Y2);
			unsigned int V_chunk = *V;
			unsigned int U_chunk = *U;
			
			//Little-Endian (PC)
			*((Uint32 *) (g_line_buf + col)) = (Y_chunk & 0xFF) | (U_chunk << 8) |
				((Y_chunk & 0xFF00) << 8) | (V_chunk << 24);
			*((Uint32 *) (g_line_buf2 + col)) = (Y2_chunk & 0xFF) | (U_chunk << 8) |
				((Y2_chunk & 0xFF00) << 8) | (V_chunk << 24);

			Y += 2;
			Y2 += 2;
			U++;
			V++;
		}
		
		memcpy(dst_ptr, g_line_buf, (uVidWidth << 1));
		memcpy(dst_ptr + channel0_pitch, g_line_buf2, (uVidWidth << 1));
		
		dst_ptr += (channel0_pitch << 1);	// we've done 2 rows, so skip a row
		Y += uVidWidth;	// we've done 2 vertical Y pixels, so skip a row
		Y2 += uVidWidth;
	}
}

int prepare_frame_callback(struct yuv_buf *src)
{
	VLDP_BOOL result = VLDP_TRUE;

	buf2overlay_YUY2_GP2X(src);

	return result;
}

void display_frame_callback(struct yuv_buf *buf)
{
	gp2x_video_YUV_flip(0);	// flip the region we just painted
	g_uFrameCount++;
}

void report_parse_progress_callback(double percent_complete_01)
{
}

void report_mpeg_dimensions_callback(int width, int height)
{
#ifdef SHOW_FRAMES
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
	return gp2x_timer_read();
}

void start_test(const char *cpszMpgName)
{
	const unsigned int FRAMES2PLAY = 900;

	g_uFrameCount = 0;
	if (g_vldp_info->open_and_block(cpszMpgName))
	{
		unsigned int uStartMs = get_ticks();
		g_local_info.uMsTimer = uStartMs;

		if (g_vldp_info->play(uStartMs))
		{
			unsigned int uEndMs, uDiffMs;

			// wait for entire mpeg to play ...
			while ((g_vldp_info->status == STAT_PLAYING) && (!g_uQuitFlag) && (g_uFrameCount < FRAMES2PLAY))
			{
				g_local_info.uMsTimer = get_ticks();
				usleep(0);
			}
			uEndMs = get_ticks();
			uDiffMs = uEndMs - uStartMs;
			printf("%u frames played in %u ms (%f fps)\n", g_uFrameCount,
				uDiffMs, (g_uFrameCount * 1000.0) / (uDiffMs));
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

// dummy function for minimal lib's benefit
void gp2x_sound_frame(void *blah, void *buff, int samples) { }

int main(int argc, char **argv)
{
	set_cur_dir(argv[0]);
#ifdef SHOW_FRAMES
	gp2x_init(
		1000,	// timer based on milliseconds
		15,	// bits per pixel
		11025,	// audio freq
		16,	// bits per sample
		1,	// stereo sound
		60,	// unused
		1);	// solid font (unused)

	gp2x_video_YUV_setparts(0,-1,-1,-1,319,239);
	gp2x_video_RGB_setwindows(0x10,0x10,0x10,0x10,319,239);
#endif
	pvldp_init = (initproc) &vldp_init;
		
	// if all functions were found, then return success
	if (pvldp_init)
	{
			g_local_info.prepare_frame = prepare_frame_callback;
			g_local_info.display_frame = display_frame_callback;
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

#ifdef SHOW_FRAMES

	MPO_FREE(g_line_buf);
	MPO_FREE(g_line_buf2);
	MPO_FREE(g_line_buf3);

#endif // SHOW_FRAMES
	
	return 0;
}
