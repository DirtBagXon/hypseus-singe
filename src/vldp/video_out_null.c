/*
 * video_out_null.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
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

#include <stdlib.h>
#include <inttypes.h>

#include <mpeg2.h>
#include "video_out.h"
#include <mpeg2convert.h>

#include <string.h>
#include "vldp.h"
#include "vldp_common.h"
#include "vldp_internal.h"

#define YUV_BUF_COUNT 3
struct yuv_buf g_yuv_buf[YUV_BUF_COUNT];

static void null_draw_frame(vo_instance_t *instance, uint8_t *const *buf, void *id)
{
    Sint32 correct_elapsed_ms = 0;
    Sint32 actual_elapsed_ms  = 0;
    unsigned int uStallFrames = 0;

    if (!(s_frames_to_skip | s_skip_all)) {
        do {
            VLDP_BOOL bFrameNotShownDueToCmd = VLDP_FALSE;

            Sint64 s64Ms = s_uFramesShownSinceTimer;
            s64Ms        = (s64Ms * 1000000) / g_out_info.uFpks;

            correct_elapsed_ms = (Sint32)(s64Ms) + s_extra_delay_ms;
            actual_elapsed_ms  = g_in_info->uMsTimer - s_timer;

            s_extra_delay_ms = 0;

            if (actual_elapsed_ms < (correct_elapsed_ms + g_out_info.u2milDivFpks)) {
                if (g_in_info->prepare_frame(&g_yuv_buf[(uintptr_t)id])) {
                    while (((Sint32)(g_in_info->uMsTimer - s_timer) < correct_elapsed_ms) &&
                           (!bFrameNotShownDueToCmd)) {
                        SDL_Delay(1);
                        if (ivldp_got_new_command()) {
                            switch (g_req_cmdORcount & 0xF0) {
                            case VLDP_REQ_PAUSE:
                            case VLDP_REQ_STEP_FORWARD:
                                ivldp_respond_req_pause_or_step();
                                break;
                            case VLDP_REQ_SPEEDCHANGE:
                                ivldp_respond_req_speedchange();
                                break;
                            case VLDP_REQ_NONE:
                                break;
                            default:
                                bFrameNotShownDueToCmd = VLDP_TRUE;
                                break;
                            }
                        }
                    }

                    if (!bFrameNotShownDueToCmd) {
                        g_in_info->display_frame(&g_yuv_buf[(uintptr_t)id]);
                    }
                }
            }

            if (!bFrameNotShownDueToCmd) {
                ++s_uFramesShownSinceTimer;
            }

            if (s_paused) {
                paused_handler();
            } else {
                play_handler();

                if (!s_paused) {
                    if (uStallFrames == 0) {
                        if (s_uPendingSkipFrame == 0) {
                            if (!bFrameNotShownDueToCmd) {
                                ++g_out_info.current_frame;

                                if (s_stall_per_frame > 0) {
                                    uStallFrames = s_stall_per_frame;
                                }

                                if (s_skip_per_frame > 0) {
                                    s_frames_to_skip = s_frames_to_skip_with_inc =
                                        s_skip_per_frame;
                                }
                            }
                        } else {
                            g_out_info.current_frame = s_uPendingSkipFrame;
                            s_uPendingSkipFrame      = 0;
                        }
                    } else {
                        --uStallFrames;
                    }
                }
            }
        } while ((s_paused || uStallFrames > 0) && !s_skip_all && !s_step_forward);

        s_step_forward = 0;
    } else {
        if (s_frames_to_skip > 0) {
            --s_frames_to_skip;
            if (s_frames_to_skip_with_inc > 0) {
                --s_frames_to_skip_with_inc;
                ++g_out_info.current_frame;
            }
        }
    }

#ifdef VLDP_DEBUG
    if (s_skip_all) s_uSkipAllCount++;
#endif
}

static void null_setup_fbuf(vo_instance_t *_instance, uint8_t **buf, void **id)
{
    static uintptr_t buffer_index = 0;
    *id                           = (int *)buffer_index;
    buf[0]                        = g_yuv_buf[buffer_index].Y;
    buf[1]                        = g_yuv_buf[buffer_index].U;
    buf[2]                        = g_yuv_buf[buffer_index].V;

    buffer_index++;

    if (buffer_index >= YUV_BUF_COUNT) buffer_index = 0;
}

static void null_close(vo_instance_t *instance)
{
    for (int i = 0; i < YUV_BUF_COUNT; i++) {
        free(g_yuv_buf[i].Y);
        free(g_yuv_buf[i].U);
        free(g_yuv_buf[i].V);
        g_yuv_buf[i].Y = NULL;
        g_yuv_buf[i].U = NULL;
        g_yuv_buf[i].V = NULL;
    }
}

static vo_instance_t *
internal_open(int setup(vo_instance_t *, unsigned int, unsigned int,
                        unsigned int, unsigned int, vo_setup_result_t *),
              void draw(vo_instance_t *, uint8_t *const *, void *))
{
    vo_instance_t *instance;

    instance = (vo_instance_t *)malloc(sizeof(vo_instance_t));
    if (instance == NULL) return NULL;

    instance->setup      = setup;
    instance->setup_fbuf = null_setup_fbuf;
    instance->set_fbuf   = NULL;
    instance->start_fbuf = NULL;
    instance->draw       = draw;
    instance->discard    = NULL;
    instance->close      = null_close;

    memset(g_yuv_buf, 0, sizeof(g_yuv_buf));

    return instance;
}

static int null_setup(vo_instance_t *instance, unsigned int width,
                      unsigned int height, unsigned int chroma_width,
                      unsigned int chroma_height, vo_setup_result_t *result)
{

    for (int i = 0; i < YUV_BUF_COUNT; i++) {
        g_yuv_buf[i].Y_size  = width * height;
        g_yuv_buf[i].UV_size = width * height;
        if (!g_yuv_buf[i].Y) g_yuv_buf[i].Y = malloc(g_yuv_buf[i].Y_size);
        if (!g_yuv_buf[i].U) g_yuv_buf[i].U = malloc(g_yuv_buf[i].UV_size);
        if (!g_yuv_buf[i].V) g_yuv_buf[i].V = malloc(g_yuv_buf[i].UV_size);
    }

    result->convert = NULL;
    return 0;
}

vo_instance_t *vo_null_open(void)
{
    return internal_open(null_setup, null_draw_frame);
}

vo_instance_t *vo_nullskip_open(void)
{
    return internal_open(null_setup, NULL);
}

static void nullslice_start(void *id, const mpeg2_fbuf_t *fbuf,
                            const mpeg2_picture_t *picture, const mpeg2_gop_t *gop)
{
}

static void nullslice_copy(void *id, uint8_t *const *src, unsigned int v_offset)
{
}

static int nullslice_convert(int stage, void *id, const mpeg2_sequence_t *seq, int stride,
                             uint32_t accel, void *arg, mpeg2_convert_init_t *result)
{
    result->id_size     = 0;
    result->buf_size[0] = result->buf_size[1] = result->buf_size[2] = 0;
    result->start = nullslice_start;
    result->copy  = nullslice_copy;
    return 0;
}

static int nullslice_setup(vo_instance_t *instance, unsigned int width,
                           unsigned int height, unsigned int chroma_width,
                           unsigned int chroma_height, vo_setup_result_t *result)
{
    result->convert = nullslice_convert;
    return 0;
}

vo_instance_t *vo_nullslice_open(void)
{
    return internal_open(nullslice_setup, null_draw_frame);
}

static int nullrgb16_setup(vo_instance_t *instance, unsigned int width,
                           unsigned int height, unsigned int chroma_width,
                           unsigned int chroma_height, vo_setup_result_t *result)
{
    result->convert = mpeg2convert_rgb16;
    return 0;
}

static int nullrgb32_setup(vo_instance_t *instance, unsigned int width,
                           unsigned int height, unsigned int chroma_width,
                           unsigned int chroma_height, vo_setup_result_t *result)
{
    result->convert = mpeg2convert_rgb32;
    return 0;
}

vo_instance_t *vo_nullrgb16_open(void)
{
    return internal_open(nullrgb16_setup, null_draw_frame);
}

vo_instance_t *vo_nullrgb32_open(void)
{
    return internal_open(nullrgb32_setup, null_draw_frame);
}

static vo_driver_t video_out_drivers[] = {{"null", vo_null_open},
                                          {"nullslice", vo_nullslice_open},
                                          {"nullskip", vo_nullskip_open},
                                          {"nullrgb16", vo_nullrgb16_open},
                                          {"nullrgb32", vo_nullrgb32_open},
                                          {NULL, NULL}};

vo_driver_t const *vo_drivers(void) { return video_out_drivers; }
