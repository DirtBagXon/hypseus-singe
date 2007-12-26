/*
 * video_out_null.c
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

#include <stdlib.h>
#include <inttypes.h>

#include "video_out.h"
#include "convert.h"

// start MPO

#include <string.h>	// for memset
#include "../vldp/vldp_common.h"	// to get access to g_in_info struct and the yuv_buf struct
#include "../vldp/vldp_internal.h"	// for access to s_ variables from vldp_internal

#define YUV_BUF_COUNT 3	// libmpeg2 needs 3 buffers to do its thing ...
struct yuv_buf g_yuv_buf[YUV_BUF_COUNT];

////

static void null_draw_frame (vo_instance_t *instance, uint8_t * const * buf, void *id)
{
    Sint32 correct_elapsed_ms = 0;	// we want this signed since we compare against actual_elapsed_ms
    Sint32 actual_elapsed_ms = 0;	// we want this signed because it could be negative
	unsigned int uStallFrames = 0;	// how many frames we have to stall during the loop (for multi-speed playback)

	// if we don't need to skip any frames
	if (!(s_frames_to_skip | s_skip_all))
	{		
		// loop once, or more than once if we are paused
		do
		{
			VLDP_BOOL bFrameNotShownDueToCmd = VLDP_FALSE;

#ifndef VLDP_BENCHMARK
			// PERFORMANCE WARNING:
			//  We need to use 64-bit math here because otherwise, we will overflow a little after 2 minutes,
			//   using 32-bit math.
			// If you want to assume that you will never be playing video longer than 2 minutes, then you can change this back
			//  to 32-bit integer math.
			// Also, you can use floating point math here, but some CPU's (gp2x) don't have floating point units, which drastically
			//  hurts performance.  On a fast modern PC, you probably won't notice a difference either way.
			Sint64 s64Ms = s_uFramesShownSinceTimer;
			s64Ms = (s64Ms * 1000000) / g_out_info.uFpks;

			// compute how much time ought to have elapsed based on our frame count
			correct_elapsed_ms = (Sint32) (s64Ms) +
				// add on any extra delay that has been requested (simulated seek delay)
				s_extra_delay_ms;
			actual_elapsed_ms = g_in_info->uMsTimer - s_timer;

			// the extra delay should only be 'used' once, so for safety reasons we reset
			// it here, where we can guarantee that it only will be used once.
			s_extra_delay_ms = 0;

			// if we are caught up enough that we don't need to skip any frames, then display the frame
			if (actual_elapsed_ms < (correct_elapsed_ms + g_out_info.u2milDivFpks))
			{
#endif
				// this is the potentially expensive callback that gets the hardware overlay
				// ready to be displayed, so we do this before we sleep
				// NOTE : if this callback fails, we don't want to display the frame due to double buffering considerations
				if (g_in_info->prepare_frame(&g_yuv_buf[(int) id]))
				{
#ifndef VLDP_BENCHMARK
				
					// stall if we are playing too quickly and if we don't have a command waiting for us
					while (((Sint32) (g_in_info->uMsTimer - s_timer) < correct_elapsed_ms)
						&& (!bFrameNotShownDueToCmd))
					{
						// IMPORTANT: this delay should come before the check for ivldp_got_new_command,
						//  so that if we get a new command, we exit the loop immediately without
						//  delaying, so that we don't have to check a second time for a new command.
	    				SDL_Delay(1);	// note, if this is set to 0, we don't get commands as quickly

						// Breaking when getting a new commend before our frame has expired
						//  will shorten 1 frame's length.  However, it could speed skips up,
						//  so I am leaving it in.
						// Also, if we get a new command, uMsTimer may not advance until
						//  we acknowledge the new command.
						if (ivldp_got_new_command())
						{
							// strip off count and examine command
							switch(g_req_cmdORcount & 0xF0)
							{
							case VLDP_REQ_PAUSE:
							case VLDP_REQ_STEP_FORWARD:
								ivldp_respond_req_pause_or_step();
								break;
							case VLDP_REQ_SPEEDCHANGE:
								ivldp_respond_req_speedchange();
								break;
							case VLDP_REQ_NONE:
								break;

								// Anything else, we will not show the next frame and will
								//  immediately exit this loop in order to handle the command
								//  elsewhere.
							default:
								bFrameNotShownDueToCmd = VLDP_TRUE;
								break;
							}
						}
					}

					// If a command comes in at the last second,
					//  we don't want to render the next frame that we were going to because it could cause overrun
					//  so we only display the frame if we haven't received a command
					if (!bFrameNotShownDueToCmd)
					{
#endif
						// draw the frame
						// we are using the pointer 'id' as an index, kind of risky, but convenient :)
						g_in_info->display_frame(&g_yuv_buf[(int) id]);
#ifndef VLDP_BENCHMARK
					} // end if we didn't get a new command to interrupt the frame being displayed
#endif
				} // end if the frame was prepared properly
#ifndef VLDP_BENCHMARK
				// else maybe we couldn't get a lock on the buffer fast enough, so we'll have to wait ...

			} // end if we don't drop any frames

			/*
			// else we're too far beyond so we're gonna have to drop some this frame to catch up (doh!)
			else
			{
				fprintf(stderr, "NOTE : dropped frame %u!  Expected %u but got %u\n",
					g_out_info.current_frame, correct_elapsed_ms, actual_elapsed_ms);
			}
			*/

#endif
			// if the frame was either displayed or dropped (due to lag) ...
			if (!bFrameNotShownDueToCmd)
			{
				// now that the frame has either been displayed or dropped, we can update the counter to reflect
				// NOTE : this should come before play_handler() is called, because if we become paused,
				//  we change the value of s_uFramesShownSinceTimer.
				++s_uFramesShownSinceTimer;
			}

			// if the frame is to be paused, then stall
			if (s_paused)
			{
				paused_handler();
			}
			// else if we are supposed to be playing
			else
			{
				play_handler();
				
				// We only want to advance the current frame if we didn't receive a pause request in the play handler.
				// NOTE : this should come after the handlers in case the state of s_paused changes
				if (!s_paused)
				{
					// If we aren't going to stall on any frames ...
					// NOTE : I think this should be toward the outside of this loop, because
					//  if we are skipping while playing at a slower speed, I would think that
					//  the skip should be slower too.  I could be wrong though ...
					// We DO want to make sure that if we are stalling, that the current frame
					//  is not incremented.
					if (uStallFrames == 0)
					{
						// if we have no pending frame, then the frame we've just displayed is the proceeding frame
						if (s_uPendingSkipFrame == 0)
						{
							// only advance frame # if we've displayed/dropped a frame
							if (!bFrameNotShownDueToCmd)
							{
								++g_out_info.current_frame;

								// if we have to stall one or more frames per frame (multi-speed playback)
								if (s_stall_per_frame > 0)
								{
									uStallFrames = s_stall_per_frame;
								}

								// if we are skipping one or more frames per frame that is displayed
								// (multi-speed playback)
								if (s_skip_per_frame > 0)
								{
									s_frames_to_skip = s_frames_to_skip_with_inc = s_skip_per_frame;
								}
								// else don't adjust the frameskip variables
							}
							// else don't advance the frame number
						}
						// else we just skipped, so update current frame to reflect that ...
						else
						{
							g_out_info.current_frame = s_uPendingSkipFrame;
							s_uPendingSkipFrame = 0;
						}
					} // end if we aren't going to stall on the currently rendered frame
					// else don't increment the frame, but decrement the uStallFrames counter
					else
					{
						--uStallFrames;
					}
				}
			}

		} while ((s_paused || uStallFrames > 0) && !s_skip_all && !s_step_forward);
		// loop while we are paused OR while we are stalling so video overlay gets redrawn

		s_step_forward = 0;	// clear this in case it was set (since we have now stepped forward)

	} // end if we don't have frames to skip
	
	// if we have frames to skip
	else
	{		
		// we could skip frames for another reason
		if (s_frames_to_skip > 0)
		{
			--s_frames_to_skip;	// we've skipped a frame, so decrease the count

			// if we need to also increase the frame number (multi-speed playback)
			if (s_frames_to_skip_with_inc > 0)
			{
				--s_frames_to_skip_with_inc;
				++g_out_info.current_frame;
			}
		}
	}

#ifdef VLDP_DEBUG
	// if we're dropping all frames, log it
	if (s_skip_all)
	{
		s_uSkipAllCount++;
	}
#endif // VLDP_DEBUG

#ifndef VLDP_BENCHMARK
	// WARNING : by putting this delay here, we are slowing down seek speed (s_skip_all and s_frames_to_skip!!!)
	// Maybe it would be better to remove this???  What would be impacted?  Why did I put this here in the first place?
	// My comment wasn't very informative :)

	// UPDATE : this SDL_Delay seems harmful to me and doesn't seem to help, so I've commented it out to see if anything
	//  bad happens as a result :)
//	SDL_Delay(0);
#endif
	
	// end MATT
}

static void null_setup_fbuf (vo_instance_t * _instance,
			    uint8_t ** buf, void ** id)
{
	static buffer_index = 0;
	*id = (int *) buffer_index;	// THIS IS A LITTLE TRICKY
	// We are setting an integer value to a pointer ...
	// Because it is convenient to let the pointer hold the value of this integer for us
	// Hopefully it doesn't cause any trouble later ;)

    buf[0] = g_yuv_buf[buffer_index].Y;
    buf[1] = g_yuv_buf[buffer_index].U;
    buf[2] = g_yuv_buf[buffer_index].V;
    
    buffer_index++;

	// we are operating under a (safe?) assumption that this function is called in sets of YUV_BUF_COUNT
	// so it should be safe to wraparound ... if our assumption is wrong, major havoc will ensure :)
	if (buffer_index >= YUV_BUF_COUNT)
	{
		buffer_index = 0;
	}

}

static int null_setup (vo_instance_t * instance, int width, int height,
		       vo_setup_result_t * result)
{
	int i = 0;

	// UPDATE : I believe these functions are no longer necessary because we do them in
	// idle_handler_open() now instead.
	/*	
	g_in_info->report_mpeg_dimensions(width, height);	// alert parent-thread
	g_out_info.w = width;
	g_out_info.h = height;
	*/

	for (i = 0; i < YUV_BUF_COUNT; i++)
	{
		// allocate buffer according to size of picture	
		// We do not re-allocate these buffers if they have already been previous allocated, as a safety measure
		g_yuv_buf[i].Y_size = width * height;
		g_yuv_buf[i].UV_size = g_yuv_buf[i].Y_size >> 2;
		if (!g_yuv_buf[i].Y) g_yuv_buf[i].Y = malloc(g_yuv_buf[i].Y_size);
		if (!g_yuv_buf[i].U) g_yuv_buf[i].U = malloc(g_yuv_buf[i].UV_size);
		if (!g_yuv_buf[i].V) g_yuv_buf[i].V = malloc(g_yuv_buf[i].UV_size);
	}
	
    result->convert = NULL;
    return 0;
}

static void null_close (vo_instance_t *instance)
{
	int i = 0;
	
	for (i = 0; i < YUV_BUF_COUNT; i++)
	{
		// NOTE : it's ok to call free(NULL) so we do not need to do safety checking here!
		free(g_yuv_buf[i].Y);
		g_yuv_buf[i].Y = NULL;
		free(g_yuv_buf[i].U);
		g_yuv_buf[i].U = NULL;
		free(g_yuv_buf[i].V);
		g_yuv_buf[i].V = NULL;
	}
}

vo_instance_t * vo_null_open ()
{
    vo_instance_t * instance;

    instance = (vo_instance_t *) malloc (sizeof (vo_instance_t));
    if (instance == NULL)
	return NULL;

    instance->setup = null_setup;	// MPO
    instance->setup_fbuf = null_setup_fbuf;	// MPO
    instance->set_fbuf = NULL;
    instance->start_fbuf = NULL;
    instance->draw = null_draw_frame;
    instance->discard = NULL;
    instance->close = null_close;	// MPO
	memset(g_yuv_buf, 0, sizeof(g_yuv_buf));	// MPO, fill this struct with 0's so that we can track whether mem has been allocated or not

    return instance;
}

// end MPO
