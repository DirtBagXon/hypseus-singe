/*
* ____ DAPHNE COPYRIGHT NOTICE ____
*
* Copyright (C) 2001 Matt Ownby
*
* This file is part of DAPHNE, a laserdisc arcade game emulator
*
* DAPHNE is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* DAPHNE is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

// ldp-vldp.c
// by Matt Ownby

// pretends to be an LDP, but uses the VLDP library for output
// (for people who have no laserdisc player)

#ifdef DEBUG
#include <assert.h>
#endif
//
#include "../game/game.h"
#include "../hypseus.h" // for get_quitflag, set_quitflag
#include "../io/conout.h"
#include "../io/error.h"
#include "../io/fileparse.h"
#include "../io/homedir.h"
#include "../io/input.h"
#include "../io/mpo_mem.h"
#include "../io/network.h" // to query amount of RAM the system has (get_sys_mem)
#include "../io/numstr.h"  // for debug
#include "../timer/timer.h"
#include "../video/palette.h"
#include "../video/rgb2yuv.h"
#include "../video/video.h"
#include "../vldp/vldp.h" // to get the vldp structs
#include "framemod.h"
#include "ldp-vldp.h"
#include <plog/Log.h>
#include <set>
#include <stdlib.h>
#include <time.h>

#define API_VERSION 11

// let compiler compute this ...
static const unsigned int FREQ1000 = sound::FREQ * 1000;

// video overlay stuff
Sint32 g_vertical_offset = 0; // (used a lot, we only want to calc it once)

// send by child thread to indicate how far our parse is complete
double g_dPercentComplete01 = 0.0;
// if true, it means we've received a parse update from VLDP
bool g_bGotParseUpdate = false;
// if true, a screenshot will be taken at the next available opportunity
bool g_take_screenshot = false;

unsigned int g_vertical_stretch = 0;
// what type of filter to use on our data (if any)
unsigned int g_filter_type = FILTER_NONE;

// these are globals because they are used by our callback functions
// used a lot, we only want to calculate once
SDL_Rect *g_screen_clip_rect = NULL;

// this will contain a blank YUV overlay suitable for search/seek blanking
struct yuv_buf g_blank_yuv_buf;
Uint8 *g_line_buf = NULL;  // temp sys RAM for doing calculations so we can do
                           // fastest copies to slow video RAM
Uint8 *g_line_buf2 = NULL; // 2nd buf
Uint8 *g_line_buf3 = NULL; // 3rd buf

////////////////////////////////////////

// 2 pixels of black in YUY2 format (different for big and little endian)
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define YUY2_BLACK 0x7f007f00
#else
#define YUY2_BLACK 0x007f007f
#endif

/////////////////////////////////////////

// We have to dynamically load the .DLL/.so file due to incompatibilities
// between MSVC++ and mingw32 library files
// These pointers and typedefs assist us in doing so.

typedef const struct vldp_out_info *(*initproc)(const struct vldp_in_info *in_info);

// pointer to all functions the VLDP exposes to us ...
const struct vldp_out_info *g_vldp_info = NULL;

// info that we provide to the VLDP DLL
struct vldp_in_info g_local_info;

/////////////////////////////////////////

ldp_vldp::ldp_vldp()
{
    m_bIsVLDP        = true; // this is VLDP, so this value is true ...
    blitting_allowed = true; // blitting is allowed until we get to a certain
                             // point in initialization
    m_target_mpegframe  = 0; // mpeg frame # we are seeking to
    m_mpeg_path         = "";
    m_cur_mpeg_filename = "";
    m_file_index        = 0; // # of mpeg files in our list
    m_framefile         = ".txt";
    // create a sensible default framefile name
    m_framefile         = g_game->get_shortgamename() + m_framefile;
    m_bFramefileSet      = false;
    m_altaudio_suffix    = ""; // no alternate audio by default
    m_audio_file_opened  = false;
    m_cur_ldframe_offset = 0;
    m_blank_on_searches  = false;
    m_blank_on_skips     = false;
    m_seek_frames_per_ms = 0;
    m_min_seek_delay     = 0;
    m_vertical_stretch   = 0;

    m_testing = false; // don't run tests by default

    m_bPreCache = m_bPreCacheForce = false;
    m_mPreCachedFiles.clear();

    m_uSoundChipID = 0;

    enable_audio1(); // both channels should be on by default
    enable_audio2();
}

ldp_vldp::~ldp_vldp() { pre_shutdown(); }

// called when hypseus starts up
bool ldp_vldp::init_player()
{
    bool result        = false;
    bool need_to_parse = false; // whether we need to parse all video

    g_vertical_stretch = m_vertical_stretch; // callbacks don't have access to
                                             // m_vertical_stretch

    // try to read in the framefile
    if (read_frame_conversions()) {
        // just a sanity check to make sure their frame file is correct
        if (first_video_file_exists()) {
            // if the last video file has not been parsed, assume none have been
            // This is safe because if parsed, it will just skip them
            if (!last_video_file_parsed()) {
                printnotice("Press any key to parse your video file(s). "
                            "This may take a while. Press ESC if you'd "
                            "rather quit.");
                need_to_parse = true;
            }

            if (audio_init() && !get_quitflag()) {
                g_local_info.prepare_frame         = prepare_frame_callback;
                g_local_info.display_frame         = display_frame_callback;
                g_local_info.report_parse_progress = report_parse_progress_callback;
                g_local_info.report_mpeg_dimensions = report_mpeg_dimensions_callback;
                g_local_info.render_blank_frame    = blank_overlay;
                g_local_info.blank_during_searches = m_blank_on_searches;
                g_local_info.blank_during_skips    = m_blank_on_skips;
                g_local_info.GetTicksFunc          = GetTicksFunc;

                g_vldp_info = vldp_init(&g_local_info);

                // if we successfully made contact with VLDP ...
                if (g_vldp_info != NULL) {
                    // this number is used repeatedly, so we calculate
                    // it once
                    g_vertical_offset = g_game->get_video_row_offset();

                    // if testing has been requested then run them ...
                    if (m_testing) {
                        list<string> lstrPassed, lstrFailed;
                        run_tests(lstrPassed, lstrFailed);
                        // run releasetest to see actual results now ...
                        LOGI << "Run releasetest to see printed results!";
                        set_quitflag();
                    }

                    // bPreCacheOK will be true if precaching succeeds
                    // or is never attempted
                    bool bPreCacheOK = true;

                    // If precaching has been requested, do it now.
                    // The check for RAM requirements is done inside the
                    // precache_all_video function, so we don't need to
                    // worry about that here.
                    if (m_bPreCache) {
                        bPreCacheOK = precache_all_video();
                    }

                    // if we need to parse all the video
                    if (need_to_parse) {
                        parse_all_video();
                    }

                    // if precaching succeeded or we didn't request
                    // precaching
                    if (bPreCacheOK) {
                        // this is the point where blitting isn't allowed anymore
                        blitting_allowed = false;

                        // open first file so that we can draw video overlay
                        // even if the disc is not playing
                        if (open_and_block(m_mpeginfo[0].name)) {
                            // although we just opened a video file, we have
                            // not opened an audio file, so we want to force a
                            // re-open of the same video file when we do a real
                            // search, in order to ensure that the audio file is
                            // opened also.
                            m_cur_mpeg_filename = "";

                            // set MPEG size ASAP in case different from
                            // NTSC default
                            m_discvideo_width  = g_vldp_info->w;
                            m_discvideo_height = g_vldp_info->h;

                            if (sound::is_enabled()) {
                                struct sound::chip soundchip;
                                soundchip.type = sound::CHIP_VLDP;
                                // no further parameters necessary
                                m_uSoundChipID = sound::add_chip(&soundchip);
                            }

                            result = true;
                        } else {
                            LOGW << fmt("LDP-VLDP: first video "
                                        "file could not be opened!");
                        }
                    } // end if it's ok to proceed
                    // else precaching failed
                    else {
                        LOGW << "precaching failed";
                    }

                } // end if reading the frame conversion file worked
                else {
                    LOGW << "vldp_init returned NULL (which shouldn't ever "
                            "happen)";
                }
            } // if audio init succeeded
            else {
                // only report an audio problem if there is one
                if (!get_quitflag()) {
                    LOGW << "Could not initialize VLDP audio!";
                }

                // otherwise report that they quit
                else {
                    LOGI << "Quit requested, shutting down!";
                }
            } // end if audio init failed or if user opted to quit instead
              // of parse
        }     // end if first file was present (sanity check)
        // else if first file was not found, we do just quit because an
        // error is printed elsewhere
    } // end if framefile was read in properly
    else {
        // if the user didn't specify a framefile from the command-line,
        // then give them a little hint.
        if (!m_bFramefileSet) {
            LOGW << "You must specify a -framefile argument when using VLDP.";
        }
        // else the other error messages are more than sufficient
    }

    // if init didn't completely finish, then we need to shutdown everything
    if (!result) {
        shutdown_player();
    }

    return result;
}

void ldp_vldp::shutdown_player()
{
    // if VLDP has been loaded
    if (g_vldp_info) {
        g_vldp_info->shutdown();
        g_vldp_info = NULL;
    }

    if (sound::is_enabled()) {
        if (!sound::delete_chip(m_uSoundChipID)) {
            LOGW << "sound chip could not be deleted";
        }
    }
    audio_shutdown();
    free_yuv_overlay(); // de-allocate overlay if we have one allocated ...
}

bool ldp_vldp::open_and_block(const string &strFilename)
{
    bool bResult = false;

    // during parsing, blitting is allowed
    // NOTE: we want this here so that it is true before the initial open
    // command is called. Otherwise we'd put it inside wait_for_status ...
    blitting_allowed = true;

    // see if this filename has been precached
    map<string, unsigned int>::const_iterator mi = m_mPreCachedFiles.find(strFilename);

    // if the file has not been precached and we are able to open it
    if ((mi == m_mPreCachedFiles.end() &&
         (g_vldp_info->open((m_mpeg_path + strFilename).c_str())))
        // OR if the file has been precached and we are able to refer to it
        || (g_vldp_info->open_precached(mi->second, (m_mpeg_path + strFilename).c_str()))) {
        bResult = wait_for_status(STAT_STOPPED);
        if (bResult) {
            m_cur_mpeg_filename = strFilename;
        }
    }

    blitting_allowed = false;

    return bResult;
}

bool ldp_vldp::precache_and_block(const string &strFilename)
{
    bool bResult = false;

    // during precaching, blitting is allowed
    // NOTE: we want this here so that it is true before the initial open
    // command is called. Otherwise we'd put it inside wait_for_status ...
    blitting_allowed = true;

    if (g_vldp_info->precache((m_mpeg_path + strFilename).c_str())) {
        bResult = wait_for_status(STAT_STOPPED);
    }

    blitting_allowed = false;

    return bResult;
}

bool ldp_vldp::wait_for_status(unsigned int uStatus)
{
    bool bResult = false;

    while (g_vldp_info->status == STAT_BUSY) {
        // if we got a parse update, then show it ...
        if (g_bGotParseUpdate) {
            // redraw screen blitter before we display it
            update_parse_meter();
            video::vid_blank();
            // vid_blit(get_screen_blitter(), 0, 0);
            // video::vid_flip();
            g_bGotParseUpdate = false;
        }

        SDL_check_input(); // so that windows events are handled
        make_delay(20);    // be nice to CPU
    }

    // if opening succeeded
    if ((unsigned int)g_vldp_info->status == uStatus) {
        bResult = true;
    }

    return bResult;
}

bool ldp_vldp::nonblocking_search(char *frame)
{

    bool result                = false;
    string filename            = "";
    string oggname             = "";
    Uint16 target_ld_frame     = (Uint16)atoi(frame);
    Uint64 u64AudioTargetPos   = 0; // position in audio to seek to (in samples)
    unsigned int seek_delay_ms = 0; // how many ms this seek must be delayed (to
                                    // simulate laserdisc lag)

    audio_pause(); // pause the audio before we seek so we don't have overrun

    // do we need to compute seek_delay_ms?
    // (This is best done sooner than later so get_current_frame() is more
    // accurate
    if (m_seek_frames_per_ms > 0) {
        // this value should be approximately the last frame we displayed
        // it doesn't get changed to the new frame until the seek is complete
        Uint16 cur_frame         = get_current_frame();
        unsigned int frame_delta = 0; // how many frames we're skipping

        // if we're seeking forward
        if (target_ld_frame > cur_frame) {
            frame_delta = target_ld_frame - cur_frame;
        } else {
            frame_delta = cur_frame - target_ld_frame;
        }

        seek_delay_ms = (unsigned int)(frame_delta / m_seek_frames_per_ms);

#ifdef DEBUG
        LOGD << fmt("LDP-VLDP: frame delta is %d and seek_delay_ms is %d",
                    frame_delta, seek_delay_ms);
#endif
    }

    // make sure our seek delay does not fall below our minimum
    if (seek_delay_ms < m_min_seek_delay) seek_delay_ms = m_min_seek_delay;

    m_target_mpegframe = mpeg_info(filename, target_ld_frame); // try to get a
                                                               // filename

    // if we can convert target frame into a filename, do it!
    if (filename != "") {
        result = true; // now we assume success unless we fail

        // if the file to be opened is different from the one we have opened
        // OR if we don't yet have a file open ...
        // THEN open the file! :)
        if (filename != m_cur_mpeg_filename) {
            // if we were able to successfully open the video file
            if (open_and_block(filename)) {
                result = true;

                // this is done inside open_and_block now ...
                // m_cur_mpeg_filename = filename; // make video file our new
                // current file

                // if sound is enabled, try to open an audio stream to match the
                // video stream
                if (sound::is_enabled()) {
                    // try to open an optional audio file to go along with video
                    oggize_path(oggname, filename);
                    m_audio_file_opened = open_audio_stream(oggname.c_str());
                }
            } else {
                LOGW << fmt("LDP-VLDP: Could not open video file %s", filename.c_str());
                result = false;
            }
        }

        // if we're ok so far, try the search
        if (result) {
            // IMPORTANT : 'uFPKS' _must_ be queried AFTER a new mpeg has been
            // opened, because sometimes a framefile can include mpegs that have
            // different framerates from each other.
            unsigned int uFPKS = g_vldp_info->uFpks;

            m_discvideo_width  = g_vldp_info->w;
            m_discvideo_height = g_vldp_info->h;

            // IMPORTANT : this must come before the optional FPS adjustment
            // takes place!!!
            u64AudioTargetPos = get_audio_sample_position(m_target_mpegframe);

            if (!need_frame_conversion()) {
                // If the mpeg's FPS and the disc's FPS differ, we need to
                // adjust the mpeg frame
                // NOTE: AVOID this if you can because it makes seeking less
                // accurate
                if (g_game->get_disc_fpks() != uFPKS) {
                    LOGI << fmt("NOTE: converting FPKS from %d to %d. This may "
                                "be less accurate.",
                                g_game->get_disc_fpks(), uFPKS);
                    m_target_mpegframe =
                        (m_target_mpegframe * uFPKS) / g_game->get_disc_fpks();
                }
            }

            // try to search to the requested frame
            if (g_vldp_info->search((Uint16)m_target_mpegframe, seek_delay_ms)) {
                // if we have an audio file opened, do an audio seek also
                if (m_audio_file_opened) {
                    result = seek_audio(u64AudioTargetPos);
                }
            } else {
                LOGW << "Search failed in video file";
            }
        }
        // else opening the file failed
    }
    // else mpeg_info() wasn't able to provide a filename ...
    else {
        LOGW << "LDP-VLDP.CPP ERROR: frame could not be converted to file, "
                "probably due to a framefile error."
                "Your framefile must begin no later than frame "
                "This most likely is your problem!";
    }

    return (result);
}

// it should be safe to assume that if this function is getting called, that we
// have not yet got a result from the search
int ldp_vldp::get_search_result()
{
    int result = SEARCH_BUSY; // default to no change

    // if search is finished and has succeeded
    if (g_vldp_info->status == STAT_PAUSED) {
        result = SEARCH_SUCCESS;
    }

    // if the search failed
    else if (g_vldp_info->status == STAT_ERROR) {
        result = SEARCH_FAIL;
    }

    // else it's busy so we just wait ...

    return result;
}

unsigned int ldp_vldp::play()
{
    unsigned int result = 0;
    string ogg_path     = "";
    bool bOK            = true; // whether it's ok to issue the play command

    // if we haven't opened any mpeg file yet, then do so now
    if (m_cur_mpeg_filename == "") {
        bOK = open_and_block(m_mpeginfo[0].name); // get the first mpeg
                                                  // available in our list
        if (bOK) {
            // this is done inside open_and_block now ...
            // m_cur_mpeg_filename = m_mpeginfo[0].name;

            // if sound is enabled, try to load an audio stream to go with video
            // stream ...
            if (sound::is_enabled()) {
                // try to open an optional audio file to go along with video
                oggize_path(ogg_path, m_mpeginfo[0].name);
                m_audio_file_opened = open_audio_stream(ogg_path.c_str());
            }

        } else {
            LOGW << fmt("in play() function, could not open mpeg file %s",
                        m_mpeginfo[0].name.c_str());
        }
    } // end if we haven't opened a file yet

    // we need to keep this separate in case an mpeg is already opened
    if (bOK) {
#ifdef DEBUG
        // we always expect this to be true, because we've just played :)
        assert(m_uElapsedMsSincePlay == 0);
#endif
        audio_play(0);
        if (g_vldp_info->play(0)) {
            result = GET_TICKS();
        }
    }

    if (!result) {
        LOGW << "play command failed!";
    }

    return result;
}

// skips forward a certain # of frames during playback without pausing
// Caveats: Does not work with an mpeg of the wrong framerate, does not work
// with an mpeg that uses fields, and does not skip across file boundaries.
// Returns true if skip was successful
bool ldp_vldp::skip_forward(Uint16 frames_to_skip, Uint16 target_frame)
{
    bool result = false;

    target_frame = (Uint16)(target_frame - m_cur_ldframe_offset); // take offset
                                                                  // into
                                                                  // account
    // this is ok (and possible) because we don't support skipping across files

    unsigned int uFPKS     = g_vldp_info->uFpks;
    unsigned int uDiscFPKS = g_game->get_disc_fpks();

    // We don't support skipping on mpegs that differ from the disc framerate
    if (uDiscFPKS == uFPKS) {
        // make sure they're not using fields
        // UPDATE : I don't see any reason why using fields would be a problem
        // anymore, but since I am not aware of any mpegs that use fields that
        // require skipping, I am leaving this restriction in here just to be
        // safe.
        if (!g_vldp_info->uses_fields) {
            // advantage to this method is no matter how many times we skip, we
            // won't drift because we are using m_play_time as our base
            // if we have an audio file opened
            if (m_audio_file_opened) {
                // Uint64 u64AudioTargetPos = (((Uint64) target_frame) *
                // FREQ1000) / uDiscFPKS;
                Uint64 u64AudioTargetPos = get_audio_sample_position(target_frame);

                // seek and play if seeking was successful
                if (seek_audio(u64AudioTargetPos)) {
                    audio_play(m_uElapsedMsSincePlay);
                }
            }
            // else we have no audio file open, but that's ok ...

            // if VLDP was able to skip successfully
            if (g_vldp_info->skip(target_frame)) {
                result = true;
            } else {
                LOGW << "video skip failed";
            }
        } else {
            LOGW << "Skipping not supported with mpegs that use fields (such "
                    "as this one)";
        }
    } else {
        LOGW << fmt("Skipping not supported when the mpeg's "
                    "framerate differs from the disc's (%f vs %f)",
                    (uFPKS / 1000.0), (uDiscFPKS / 1000.0));
    }

    return result;
}

void ldp_vldp::pause()
{
#ifdef DEBUG
    LOGD << fmt("g_vldp_info's current frame is %d (%d adjusted)", g_vldp_info->current_frame,
                (m_cur_ldframe_offset + g_vldp_info->current_frame));
#endif
    g_vldp_info->pause();
    audio_pause();
}

bool ldp_vldp::change_speed(unsigned int uNumerator, unsigned int uDenominator)
{
    bool bResult = false;

    // if we aren't doing 1X, then stop the audio (this can be enhanced later)
    if ((uNumerator != 1) || (uDenominator != 1)) {
        audio_pause();
    }
    // else the audio should be played at the correct location
    else {
        string filename; // dummy, not used

        // calculate where our audio position should be
        unsigned int target_mpegframe = mpeg_info(filename, get_current_frame());
        Uint64 u64AudioTargetPos = get_audio_sample_position(target_mpegframe);

        // try to get the audio playing again
        if (seek_audio(u64AudioTargetPos)) {
            audio_play(m_uElapsedMsSincePlay);
        } else {
            LOGW << "trying to seek audio after playing at 1X failed";
        }
    }

    if (g_vldp_info->speedchange(m_uFramesToSkipPerFrame, m_uFramesToStallPerFrame)) {
        bResult = true;
    }

    return bResult;
}

void ldp_vldp::think()
{
    // VLDP relies on this number
    // (m_uBlockedMsSincePlay is only non-zero when we've used blocking seeking)
    g_local_info.uMsTimer = m_uElapsedMsSincePlay + m_uBlockedMsSincePlay;
}

#ifdef DEBUG
// This function tests to make sure VLDP's current frame is the same as our
// internal current frame.
unsigned int ldp_vldp::get_current_frame()
{
    Sint32 result = 0;

    // safety check
    if (!g_vldp_info) return 0;

    unsigned int uFPKS = g_vldp_info->uFpks;

    // the # of frames that have advanced since our search
    unsigned int uOffset = g_vldp_info->current_frame - m_target_mpegframe;

    unsigned int uStartMpegFrame = m_target_mpegframe;

    // since the mpeg's beginning does not correspond to the laserdisc's
    // beginning, we add the offset
    result = m_cur_ldframe_offset + uStartMpegFrame + uOffset;

    // if we're out of bounds, just set it to 0
    if (result <= 0) {
        result = 0;
    }

    // if we got a legitimate frame
    else {
        // FIXME : THIS CODE HAS BUGS IN IT, I HAVEN'T TRACKED THEM DOWN YET
        // HEHE
        // if the mpeg's FPS and the disc's FPS differ, we need to adjust the
        // frame that we return
        if (g_game->get_disc_fpks() != uFPKS) {
            return m_uCurrentFrame;
            // result = (result * g_game->get_disc_fpks()) / uFPKS;
        }
    }

    // if there's even a slight mismatch, report it ...
    if ((unsigned int)result != m_uCurrentFrame) {
        // if VLDP is ahead, that is especially disturbing
        if ((unsigned int)result > m_uCurrentFrame) {
            LOGD << fmt("ldp-vldp::get_current_frame() [vldp ahead]: internal "
                        "frame is %d, "
                        "vldp's current frame is %d",
                        m_uCurrentFrame, result);

            LOGD << fmt(
                "g_local_info.uMsTimer is %d, which means frame offset %f (%f)",
                g_local_info.uMsTimer, (g_local_info.uMsTimer * uFPKS) / 1000000,
                (g_local_info.uMsTimer * uFPKS * 0.000001));

            LOGD
                << fmt("m_uCurrentOffsetFrame is %d, m_last_seeked frame is %d",
                       m_uCurrentOffsetFrame, m_last_seeked_frame);

            LOGD << fmt(
                "correct elapsed ms is %x (%f), which is frame offset %d (%f)",
                ((result - m_last_seeked_frame) * 1000000 / uFPKS),
                ((result - m_last_seeked_frame) * 1000000.0 / uFPKS),
                ((uMsCorrect * uFPKS) / 1000000), (uMsCorrect * uFPKS * 0.000001));
        }
    }

    return m_uCurrentFrame;
}
#endif

// takes a screenshot of the current frame + any video overlay
void ldp_vldp::request_screenshot() { g_take_screenshot = true; }

void ldp_vldp::set_search_blanking(bool enabled)
{
    m_blank_on_searches = enabled;
}

void ldp_vldp::set_skip_blanking(bool enabled) { m_blank_on_skips = enabled; }

void ldp_vldp::set_seek_frames_per_ms(double value)
{
    m_seek_frames_per_ms = value;
}

void ldp_vldp::set_min_seek_delay(unsigned int value)
{
    m_min_seek_delay = value;
}

// sets the name of the frame file
void ldp_vldp::set_framefile(const char *filename)
{
    m_bFramefileSet = true;
    m_framefile     = filename;
}

// sets alternate soundtrack
void ldp_vldp::set_altaudio(const char *audio_suffix)
{
    m_altaudio_suffix = audio_suffix;
}

// sets alternate soundtrack
void ldp_vldp::set_vertical_stretch(unsigned int value)
{
    m_vertical_stretch = value;
}

void ldp_vldp::test_helper(unsigned uIterations)
{
    // We aren't calling think_delay because we want to have a lot of
    // milliseconds pass quickly without actually waiting.

    // make a certain # of milliseconds elapse ....
    for (unsigned int u = 0; u < uIterations; u++) {
        pre_think();
    }
}

void ldp_vldp::run_tests(list<string> &lstrPassed, list<string> &lstrFailed)
{
    // these tests just basically stress the VLDP .DLL to make sure it will
    // never crash under any circumstances
    string s    = "";
    bool result = false;

    // if we have at least 1 entry in our framefile
    if (m_file_index > 0) {
        string path = m_mpeginfo[0].name; // create full pathname to file

        // TEST #1 : try opening and playing without seeking
        s = "VLDP TEST #1 (playing and skipping)";
        if (open_and_block(path) == 1) {
            g_local_info.uMsTimer = GET_TICKS();
            if (g_vldp_info->play(g_local_info.uMsTimer) == 1) {
                test_helper(1000);
                if (g_vldp_info->skip(5) == 1) {
                    lstrPassed.push_back(s);
                } else {
                    lstrFailed.push_back(
                        s + " (opened and played, but could not skip)");
                }
            } else
                lstrFailed.push_back(s + " (opened but could not play)");
        } else
            lstrFailed.push_back(s);

        // TEST #2 : try opening the file repeatedly
        // This should not cause any problems
        s      = "VLDP TEST #2 (opening repeatedly)";
        result = true;
        for (int i = 0; i < 10; i++) {
            if (!open_and_block(path)) {
                result = false;
                break;
            }
        }
        if (result)
            lstrPassed.push_back(s);
        else
            lstrFailed.push_back(s);

        // TEST #3 : try opening the file and seeking to an illegal frame
        s = "VLDP TEST #3 (seeking to illegal frame)";
        if (!g_vldp_info->search_and_block(65534, 0)) {
            lstrPassed.push_back(s);
        } else
            lstrFailed.push_back(s);

        // TEST #4 : try seeking to a (hopefully) legitimate frame
        s = "VLDP TEST #4 (seeking to legit frame)";
        if (g_vldp_info->search_and_block(50, 0) == 1) {
            lstrPassed.push_back(s);
            // test_helper(1000);
        } else {
            lstrFailed.push_back(s);
        }

        // TEST #5 : play to the end of the file, then try playing again to see
        // what happens
        s = "VLDP TEST #5 (playing to end of file, then playing again)";
        if (open_and_block(path) == 1) {
            g_local_info.uMsTimer = m_uElapsedMsSincePlay = m_uBlockedMsSincePlay = 0;

            if (g_vldp_info->play(g_local_info.uMsTimer) == 1) {
                // wait for file to end ...
                while ((g_vldp_info->status == STAT_PLAYING) && (!get_quitflag())) {
                    SDL_check_input();
                    test_helper(250);
                }

                if (g_vldp_info->play(g_local_info.uMsTimer) == 1) {
                    lstrPassed.push_back(s);
                    test_helper(1000);
                } else {
                    lstrFailed.push_back(s + " - the second time around");
                }
            } else
                lstrFailed.push_back(s + "opened file, but failed to play it)");
        } else
            lstrFailed.push_back(s);

        // TEST #6 : play to the end of the file, then try seeking to see what
        // happens
        s = "VLDP TEST #6 (play to end of file, then seek)";
        if (open_and_block(path) == 1) {
            g_local_info.uMsTimer = m_uElapsedMsSincePlay = m_uBlockedMsSincePlay = 0;

            if (g_vldp_info->play(g_local_info.uMsTimer) == 1) {
                // wait for file to end ...
                while ((g_vldp_info->status == STAT_PLAYING) && (!get_quitflag())) {
                    SDL_check_input();
                    test_helper(250);
                }

                if (g_vldp_info->search_and_block(50, 0) == 1) {
                    lstrPassed.push_back(s);
                    test_helper(1000);
                } else {
                    lstrFailed.push_back(s + "(failed the second time around)");
                }
            } else
                lstrFailed.push_back(s +
                                     "(opened file, but failed to play it)");
        } else
            lstrFailed.push_back(s);

        // TEST #7 : open, seek, play, all testing timing
        s = "VLDP TEST #7 (seeking, playing with timing tested)";
        if (open_and_block(path) == 1) {
            g_local_info.uMsTimer = m_uElapsedMsSincePlay = m_uBlockedMsSincePlay = 0;

            // search to beginning frame ...
            if (g_vldp_info->search_and_block(0, 0) == 1) {
                test_helper(1); // make 1 ms elapse ...
                SDL_Delay(1);   // give VLDP thread a chance to make its own
                                // updates ...

                // make sure that frame is what we expect it to be ...
                if (g_vldp_info->current_frame == 0) {
                    test_helper(50); // make 50 ms elapse
                    SDL_Delay(1); // give VLDP thread a chance to make updates

                    // make sure current frame has not changed
                    if (g_vldp_info->current_frame == 0) {
                        g_local_info.uMsTimer     = m_uElapsedMsSincePlay =
                            m_uBlockedMsSincePlay = 0;

                        // so now we start playing ...
                        if (g_vldp_info->play(0) == 1) {
                            test_helper(32); // pause 32 ms (right before frame
                                             // should change)
                            SDL_Delay(50);   // vldp thread blah blah ...

                            // current frame still should not have changed
                            if (g_vldp_info->current_frame == 0) {
                                test_helper(1); // 1 more ms, frame should
                                                // change now
                                SDL_Delay(50);  // don't fail simply due to the
                                                // cpu being overloaded
                                if (g_vldp_info->current_frame == 1) {
                                    lstrPassed.push_back(s);
                                } else
                                    lstrFailed.push_back(
                                        s + " (frame didn't change to 1)");
                            } else
                                lstrFailed.push_back(s + " (frame changed to " +
                                                     numstr::ToStr(g_vldp_info->current_frame) +
                                                     " after play)");
                        } else
                            lstrFailed.push_back(s + " (play failed)");
                    } else
                        lstrFailed.push_back(
                            s + " (current frame changed from 0 after seek)");
                } else
                    lstrFailed.push_back(
                        s + " (opened, but current frame was not 0)");
            } else
                lstrFailed.push_back(s + " (opened but could not search)");
        } else
            lstrFailed.push_back(s);

    } // end if there was 1 file in the framefile
    else
        lstrFailed.push_back("VLDP TESTS (Framefile had no entries)");
}

// handles VLDP-specific command line args
bool ldp_vldp::handle_cmdline_arg(const char *arg)
{
    bool result = true;

    if (strcasecmp(arg, "-blend") == 0) {
        g_filter_type |= FILTER_BLEND;
    } else if (strcasecmp(arg, "-scanlines") == 0) {
        g_filter_type |= FILTER_SCANLINES;
    }
    // should we run a few VLDP tests when the player is initialized?
    else if (strcasecmp(arg, "-vldptest") == 0) {
        m_testing = true;
    }
    // should we precache all video streams to RAM?
    else if (strcasecmp(arg, "-precache") == 0) {
        m_bPreCache = true;
    }
    // even if we don't have enough RAM, should we still precache all video
    // streams to RAM?
    else if (strcasecmp(arg, "-precache_force") == 0) {
        m_bPreCache      = true;
        m_bPreCacheForce = true;
    }

    // else it's unknown
    else {
        result = false;
    }

    return result;
}

//////////////////////////////////

// read frame conversions in from LD-frame to mpeg-frame data file
bool ldp_vldp::read_frame_conversions()
{
    struct mpo_io *p_ioFileConvert;
    string s            = "";
    string frame_string = "";
    bool result         = false;
    string framefile_path;

    framefile_path = m_framefile;

    p_ioFileConvert = mpo_open(framefile_path.c_str(), MPO_OPEN_READONLY);

    // if the file was not found in the relative directory, try looking for it
    // in the framefile directory
    if (!p_ioFileConvert) {
        framefile_path =
            g_homedir.get_framefile(framefile_path); // add directory to front
                                                     // of path
        p_ioFileConvert = mpo_open(framefile_path.c_str(), MPO_OPEN_READONLY);
    }

    // if the framefile was opened successfully
    if (p_ioFileConvert) {
        MPO_BYTES_READ bytes_read = 0;
        char *ff_buf              = (char *)MPO_MALLOC((unsigned int)(p_ioFileConvert->size + 1)); // add an extra byte to null terminate
        if (ff_buf != NULL) {
            if (mpo_read(ff_buf, (unsigned int)p_ioFileConvert->size,
                         &bytes_read, p_ioFileConvert)) {
                // if we successfully read in the whole framefile
                if (bytes_read == p_ioFileConvert->size) {
                    string err_msg = "";

                    ff_buf[bytes_read] = 0; // NULL terminate the end of the
                                            // file to be safe

                    // if parse was successful
                    if (parse_framefile((const char *)ff_buf, framefile_path.c_str(),
                                        m_mpeg_path, &m_mpeginfo[0], m_file_index,
                                        sizeof(m_mpeginfo) / sizeof(struct fileframes),
                                        err_msg)) {
                        LOGI << fmt("Framefile parse succeeded. Video/Audio "
                                    "directory is: %s",
                                    m_mpeg_path.c_str());
                        result = true;
                    } else {
                        LOGW << fmt("Framefile Parse Error: %s", err_msg.c_str());
                        LOGW << fmt("Mpeg Path: %s", m_mpeg_path.c_str());
                        // print the entire contents of the framefile to make it
                        // easier to us to debug newbie problems using their
                        // hypseus_log.txt
                        LOGW << fmt("---BEGIN FRAMEFILE CONTENTS---\n%s---END "
                                    "FRAMEFILE CONTENTS---",
                                    ff_buf);
                    }
                } else
                    LOGW << "framefile read error";
            } else
                LOGW << "framefile read error";
        } else
            LOGW << "mem alloc error";
        mpo_close(p_ioFileConvert);
    } else {
        LOGW << fmt("Could not open framefile: %s", m_framefile.c_str());
    }

    return result;
}

// if file does not exist, we print an error message
bool ldp_vldp::first_video_file_exists()
{
    string full_path = "";
    bool result      = false;

    // if we have at least one file
    if (m_file_index) {
        full_path = m_mpeg_path;
        full_path += m_mpeginfo[0].name;
        if (mpo_file_exists(full_path.c_str())) {
            result = true;
        } else {
            full_path = "Could not open file : " + full_path; // keep using
                                                              // full_path just
                                                              // because it's
                                                              // convenient
            printerror(full_path.c_str());
        }
    } else {
        LOGW << "Framefile seems empty, it's probably invalid. Read the "
                "documentation to learn how to create framefiles.";
    }

    return result;
}

// returns true if the last video file has been parsed
// This is so we don't parse_all_video if all files are already parsed
bool ldp_vldp::last_video_file_parsed()
{
    string full_path = "";
    bool result      = false;

    // if we have at least one file
    if (m_file_index > 0) {
        full_path = m_mpeg_path;
        full_path += m_mpeginfo[m_file_index - 1].name;
        full_path.replace(full_path.length() - 3, 3,
                          "dat"); // replace pre-existing suffix (which is
                                  // probably .m2v) with 'dat'

        if (mpo_file_exists(full_path.c_str())) {
            result = true;
        }
    }
    // else there is a problem with the frame file so return false

    return result;
}

// opens (and closes) all video files, forcing any unparsed video files to get
// parsed
void ldp_vldp::parse_all_video()
{
    unsigned int i = 0;

    for (i = 0; i < m_file_index; i++) {
        // if the file can be opened...
        if (open_and_block(m_mpeginfo[i].name)) {
            g_vldp_info->search_and_block(0, 0); // search to frame 0 to render
                                                 // it so the user has something
                                                 // to watch while he/she waits
            think(); // let opengl have a chance to display the frame
        } else {
            LOGW << fmt(
                "Could not parse video because file %s could not be opened.",
                m_mpeginfo[i].name.c_str());
            break;
        }
    }
}

bool ldp_vldp::precache_all_video()
{
    bool bResult = true;

    unsigned int i   = 0;
    string full_path = "";
    mpo_io *io       = NULL;

    // to make sure the total file size is correct
    // (it's legal for a framefile to have the same file listed more than once)
    set<string> sDupePreventer;

    uint64_t u64TotalBytes = 0;

    // first compute file size ...
    for (i = 0; i < m_file_index; i++) {
        full_path = m_mpeg_path + m_mpeginfo[i].name; // create full pathname to
                                                      // file

        // if this file's size hasn't already been taken into account
        if (sDupePreventer.find(m_mpeginfo[i].name) == sDupePreventer.end()) {
            io = mpo_open(full_path.c_str(), MPO_OPEN_READONLY);
            if (io) {
                u64TotalBytes += io->size;
                mpo_close(io); // we're done with this file ...
                sDupePreventer.insert(m_mpeginfo[i].name); // we've used it now
                                                           // ...
            }
            // else file can't be opened ...
            else {
                LOGW
                    << fmt("when precaching, the file  %s could not be opened.",
                           full_path.c_str());
                bResult = false;
                break;
            }
        } // end if the filesize hasn't been taken into account
          // else the filesize has already been taken into account
    }

    // if we were able to compute the file size ...
    if (bResult) {
        const unsigned int uFUDGE = 256; // how many megs we assume the OS needs
                                         // in addition to our application
                                         // running
        unsigned int uReqMegs = (unsigned int)((u64TotalBytes / 1048576) + uFUDGE);
        unsigned int uMegs    = get_sys_mem();

        // if we have enough memory (accounting for OS overhead, which may need
        // to increase in the future)
        //  OR if the user wants to force precaching despite our check ...
        if ((uReqMegs < uMegs) || (m_bPreCacheForce)) {
            for (i = 0; i < m_file_index; i++) {
                // if the file in question has not yet been precached
                if (m_mPreCachedFiles.find(m_mpeginfo[i].name) ==
                    m_mPreCachedFiles.end()) {
                    // try to precache and if it fails, bail ...
                    if (precache_and_block(m_mpeginfo[i].name)) {
                        // store the index of the file that we last precached
                        m_mPreCachedFiles[m_mpeginfo[i].name] = g_vldp_info->uLastCachedIndex;
                    } else {
                        full_path = m_mpeg_path + m_mpeginfo[i].name;

                        LOGW << fmt("precaching of file %s failed.", full_path.c_str());
                        bResult = false;
                    }
                } // end if file has not been precached
                  // else file has already been precached, so don't precache it
                  // again
            }
        } else {
            LOGW << fmt("Not enough memory to precache video stream. You have "
                        "about %d but need %d",
                        uMegs, uReqMegs);
            bResult = false;
        }
    }

    return bResult;
}

Uint64 ldp_vldp::get_audio_sample_position(unsigned int uTargetMpegFrame)
{
    Uint64 u64AudioTargetPos = 0;

    if (!need_frame_conversion()) {
        u64AudioTargetPos =
            (((Uint64)uTargetMpegFrame) * FREQ1000) / g_game->get_disc_fpks();
        // # of samples to seek to in the audio stream
    }
    // If we are already doing a frame conversion elsewhere, we don't want to do
    // it here again twice
    //  but we do need to set the audio to the correct time
    else {
        u64AudioTargetPos =
            (((Uint64)uTargetMpegFrame) * FREQ1000) / get_frame_conversion_fpks();
    }

    return u64AudioTargetPos;
}

// returns # of frames to seek into file, and mpeg filename
// if there's an error, filename is ""
// WARNING: This assumes the mpeg and disc are running at exactly the same FPS
// If they aren't, you will need to calculate the actual mpeg frame to seek to
// The reason I don't return time here instead of frames is because it is more
// accurate to
//  return frames if they are at the same FPS (which hopefully they are hehe)
Uint16 ldp_vldp::mpeg_info(string &filename, Uint16 ld_frame)
{
    unsigned int index = 0;
    Uint16 mpeg_frame  = 0; // which mpeg frame to seek (assuming mpeg and disc
                            // have same FPS)
    filename = ""; // blank 'filename' means error, so we default to this
                   // condition for safety reasons

    // find the mpeg file that has the LD frame inside of it
    while ((index + 1 < m_file_index) && (ld_frame >= m_mpeginfo[index + 1].frame)) {
        index = index + 1;
    }

    // make sure that the frame they've requested comes after the first frame in
    // our framefile
    if (ld_frame >= m_mpeginfo[index].frame) {
        // make sure a filename exists (when can this ever fail? verify!)
        if (m_mpeginfo[index].name != "") {
            filename             = m_mpeginfo[index].name;
            mpeg_frame           = (Uint16)(ld_frame - m_mpeginfo[index].frame);
            m_cur_ldframe_offset = m_mpeginfo[index].frame;
        } else {
            LOGW << "no filename found";
            mpeg_frame = 0;
        }
    }
    // else frame is out of range ...

    return (mpeg_frame);
}

////////////////////////////////////////////////////////////////////////////////////////

// puts the yuv_callback into a blocking state
// This is necessary if the gamevid ever becomes invalid for a period of time
// (ie it gets free'd and re-allocated in seektest)
// timeout is how many ms to wait before giving up
// returns true if it got the lock or false if it couldn't get a lock
bool ldp_vldp::lock_overlay(Uint32 timeout)
{
    bool bRes = false;

    if (g_vldp_info) {
        bRes = g_vldp_info->lock(timeout) == VLDP_TRUE;
    }
    // else g_vldp_info is NULL which means the init function hasn't been called
    // yet probably

    return bRes;
}

// releases the yuv_callback from its blocking state
bool ldp_vldp::unlock_overlay(Uint32 timeout)
{
    /*
        Uint32 time = refresh_ms_time();

        mutex_lock_request = false;

        // sleep until the yuv callback acknowledges that it has control once
       again
        while (mutex_lock_acknowledge)
        {
            SDL_Delay(0);

            // if we've timed out
            if (elapsed_ms_time(time) > timeout)
            {
                break;
            }
        }

        return (!mutex_lock_acknowledge);
        */

    return (g_vldp_info->unlock(timeout) == VLDP_TRUE);
}

bool ldp_vldp::parse_framefile(const char *pszInBuf,
                               const char *pszFramefileFullPath, string &sMpegPath,
                               struct fileframes *pFrames, unsigned int &frame_idx,
                               unsigned int max_frames, string &err_msg)
{
    bool result              = false;
    const char *pszPtr       = pszInBuf;
    unsigned int line_number = 0; // for debugging purposes
    char ch                  = 0;

    frame_idx = 0;
    err_msg   = "";

    // read absolute or relative directory
    pszPtr = read_line(pszPtr, sMpegPath);

    // if there are no additional lines
    if (!pszPtr) {
        // if there is at least 1 line
        if (sMpegPath.size() > 0) {
            err_msg = "Framefile only has 1 line in it. Framefiles must have "
                      "at least 2 lines in it.";
        } else
            err_msg = "Framefile appears to be empty. Framefile must have at "
                      "least 2 lines in it.";
        return false; // normally I only like to have 1 return per function, but
                      // this is a good spot to return..
    }

    ++line_number; // if we get this far, it means we've read our first line

    // If sMpegPath is NOT an absolute path (doesn't begin with a unix '/' or
    // have the second char as a win32 ':') then we want to use the framefile's
    // path for convenience purposes (this should be win32 and unix compatible)
    if ((sMpegPath[0] != '/') && (sMpegPath[0] != '\\') && (sMpegPath[1] != ':')) {
        string path = "";

        // try to isolate the path of the framefile
        if (get_path_of_file(pszFramefileFullPath, path)) {
            // put the path of the framefile in front of the relative path of
            // the mpeg files
            // This will allow people to move the location of their mpegs around
            // to different directories without changing the framefile at all.
            // For example, if the framefile and the mpegs are in the same
            // directory, then the framefile's first line could be "./"
            sMpegPath = path + sMpegPath;
        }
    }
    // else it is an absolute path, so we ignore the framefile's path

    string s = "";
    // convert all \'s to /'s to be more unix friendly (doesn't hurt win32)
    for (unsigned int i = 0; i < sMpegPath.length(); i++) {
        ch                 = sMpegPath[i];
        if (ch == '\\') ch = '/';
        s += ch;
    }
    sMpegPath = s;

    // Clean up after the user if they didn't end the path with a '/' character
    if (ch != '/') {
        sMpegPath += "/"; // windows will accept / as well as \, so we're ok
                          // here
    }

    string word = "", remaining = "";
    result       = true; // from this point, we should assume success
    Sint32 frame = 0;

    // parse through entire file
    while (pszPtr != NULL) {
        pszPtr = read_line(pszPtr, s); // read in a line
        ++line_number;

        // if we can find the first word (frame #)
        if (find_word(s.c_str(), word, remaining)) {
            // check for overflow
            if (frame_idx >= max_frames) {
                err_msg = "Framefile has too many entries in it."
                          " You can increase the value of MAX_MPEG_FILES and "
                          "recompile.";
                result = false;
                break;
            }

            frame = numstr::ToInt32(word.c_str()); // this should work, even
                                                   // with whitespace after it

            // If frame is valid AND we are able to find the name of the
            // (a non-integer will be converted to 0, so we need to make sure it
            // really is supposed to be 0)
            if (((frame != 0) || (word == "0")) &&
                find_word(remaining.c_str(), word, remaining)) {
                pFrames[frame_idx].frame = (Sint32)frame;
                pFrames[frame_idx].name  = word;
                ++frame_idx;
            } else {
                // This error has been stumping self-proclaimed "experienced
                // emu'ers" so I am going to extra effort to make it clear to
                // them what the problem is.
                err_msg =
                    "Expected a number followed by a string, but on line " +
                    numstr::ToStr(line_number) + ", found this: " + s + "(";
                // print hex values of bad string to make troubleshooting easier
                for (size_t idx = 0; idx < s.size(); idx++) {
                    err_msg += "0x" + numstr::ToStr(s[idx], 16) + " ";
                }

                err_msg += ")";
                result = false;
                break;
            }
        }
        // else it is probably just an empty line, so we can ignore it

    } // end while file hasn't been completely parsed

    // if we ended up with no entries AND didn't get any error, then the
    // framefile is bad
    if ((frame_idx == 0) && (result == true)) {
        err_msg = "Framefile appears to not have any entries in it.";
        result  = false;
    }

    return result;
}

//////////////////////////////////////////////////////////////////////

// returns VLDP_TRUE on success, VLDP_FALSE on failure
int prepare_frame_callback(uint8_t *Yplane, uint8_t *Uplane, uint8_t *Vplane,
                           int Ypitch, int Upitch, int Vpitch)
{
    int result = VLDP_FALSE;

    // MAC: We only update the YUV surface we have invented (because YUV surfaces don't exist in SDL2).
    // The corresponding YUV texture is updated by the main thread "hypseus" in vid_blit().
    result = (video::vid_update_yuv_overlay (Yplane, Uplane, Vplane, Ypitch, Upitch, Vpitch) == 0)
                 ? VLDP_TRUE
                 : VLDP_FALSE;

    return result;
}

// displays the frame as fast as possible
void display_frame_callback()
{
    // MAC: vid_blit() updates all textures from their corresponding surfaces as needed.
    // It's not called from here anymore, but from game::blit() instead, because it runs on every frame-complete
    // emulation loop, and "takes" the yuv "surface" I invented to "mix" it with the other surfaces (using
    // textures and alpha blending) to build each "complete frame" with YUV video and overlay.
    
    //video::vid_blit();
}

///////////////////

Uint32 g_parse_start_time = 0; // when mpeg parsing began approximately ...
double g_parse_start_percentage = 0.0; // the first percentage report we
                                       // received ...
bool g_parsed = false; // whether we've received any data at all ...

// this should be called from parent thread
void update_parse_meter()
{
    // if we have some data collected
    if (g_dPercentComplete01 >= 0) {
        // how many seconds have elapsed since we began this ...
        double elapsed_s = 0;
        // how many seconds the entire operation is likely to take
        double total_s = 0;
        // how many seconds are remaining
        double remaining_s = 0;
        // switch it from [0-1] to [0-100]
        double percent_complete = g_dPercentComplete01 * 100.0;

        // compute elapsed seconds
        elapsed_s = (elapsed_ms_time(g_parse_start_time)) * 0.001;
        // how much 'percentage' points we've accomplished
        double percentage_accomplished = percent_complete - g_parse_start_percentage;

        // 100 signifies 100 percent
        // (I got this equation by doing some algebra on paper)
        total_s = (elapsed_s * 100.0) / percentage_accomplished;

        // as long as percent_complete is always 100 or lower, total_s will
        // always be >= elapsed_s, so no checking necessary here
        remaining_s = total_s - elapsed_s;

        // the main screen that we can draw on ...
        SDL_Surface *screen = video::get_screen_blitter();
        // erase previous stuff on the screen blitter
        SDL_FillRect(screen, NULL, 0);

        // if we have some progress to report ...
        if (remaining_s > 0) {
            char s[160];
            // calculations to center message on screen ...
            int half_h = screen->h >> 1;
            int half_w = screen->w >> 1;
            sprintf(s, "Video parsing is %02.f percent complete, %02.f seconds "
                       "remaining.\n",
                    percent_complete, remaining_s);
            FC_Draw(video::get_font(), video::get_renderer(),
                    (half_w - ((strlen(s) / 2) * video::FONT_SMALL_W)),
                    half_h - video::FONT_SMALL_H, s);

            // now draw a little graph thing ...
            SDL_Rect clip       = screen->clip_rect;
            const int THICKNESS = 10; // how thick our little status bar will be
            // where to start our little status bar
            clip.y = (clip.h - THICKNESS) / 2;
            clip.h = THICKNESS;
            // give us some padding
            clip.y += video::FONT_SMALL_H + 5;

            // draw a white bar across the screen ...
            SDL_FillRect(screen, &clip, SDL_MapRGB(screen->format, 255, 255, 255));

            clip.x++;    // move left boundary in 1 pixel
            clip.y++;    // move upper boundary down 1 pixel
            clip.w -= 2; // move right boundary in 1 pixel
            clip.h -= 2; // move lower boundary in 1 pixel

            // fill inside with black
            SDL_FillRect(screen, &clip, SDL_MapRGB(screen->format, 0, 0, 0));

            // compute how wide our progress bar should be (-1 to take into account left pixel border)
            clip.w = (Uint16)((screen->w * g_dPercentComplete01) + 0.5) - 1;

            // go from full red (hardly complete) to full green (fully complete)
            SDL_FillRect(screen, &clip,
                         SDL_MapRGB(screen->format, (Uint8)(255 * (1.0 - g_dPercentComplete01)),
                                    (Uint8)(255 * g_dPercentComplete01), 0));
        }
    }
}

// percent_complete is between 0 and 1
// a negative value means that we are starting to parse a new file NOW
void report_parse_progress_callback(double percent_complete_01)
{
    g_dPercentComplete01 = percent_complete_01;
    g_bGotParseUpdate    = true;
    g_parsed             = true; // so we can know to re-create the overlay

    // if a new parse is starting
    if (percent_complete_01 < 0) {
        // NOTE : this would be a good place to automatically free the yuv
        // overlay BUT it was causing crashes... free_yuv_overlay here if
        // you find any compatibility problems on other platforms
        g_parse_start_time       = refresh_ms_time();
        g_parse_start_percentage = 0;
    }
}

///////////////////

// this always gets called before the draw_callback and always after
// report_parse_progress callback
void report_mpeg_dimensions_callback(int width, int height)
{
    unsigned int uTimer = refresh_ms_time();

    // if we haven't blitted this information to the screen, then wait for other
    // thread to do so before we continue ...
    while ((g_bGotParseUpdate) && (elapsed_ms_time(uTimer) < 3000)) {
        make_delay(1);
    }

    // used a lot, we only want to calculate it once
    g_screen_clip_rect = &video::get_screen_blitter()->clip_rect;

    // if draw width is less than the screen width
    if (video::get_draw_width() < (unsigned int)g_screen_clip_rect->w) {
        // center horizontally
        unsigned int uDiff = g_screen_clip_rect->w - video::get_draw_width();
        g_screen_clip_rect->x += (uDiff / 2);
        g_screen_clip_rect->w = video::get_draw_width();
    }

    // if draw height is less than the screen height
    if (video::get_draw_height() < (unsigned int)g_screen_clip_rect->h) {
        // center vertically
        unsigned int uDiff = g_screen_clip_rect->h - video::get_draw_height();
        g_screen_clip_rect->y += (uDiff / 2);
        g_screen_clip_rect->h = video::get_draw_height();
    }

    // create overlay, taking into account any letterbox removal we're doing
    // (*4 because our pixels are *2 the height of the graphics, AND we're
    // doing it at the top and bottom)

    // MAC: Set the new YUV texture dimensions and tell the video object we need to create 
    // the YUV texture before drawing next frame, on vid_bit(), instead of creating it here.
    // REMEMBER vid_setup_yuv_surface() will destroy both the YUV surface and texture
    // internally if they already exist.

    // if we drew some stuff on the screen, then we need to free the overlay and
    // re-create it
    if (g_parsed) {
        video::vid_setup_yuv_overlay(width, height);
        g_parsed = false;
    }   
 
    // Delete and create the YUV overlay (YUV surface + YUV texture) if it doesn't have the
    // new dimensions we need, or just create it for the first time if we haven't done so yet, 
    // in wich case the get_* functions will return zero.
    if (video::get_yuv_overlay_width() != width && video::get_yuv_overlay_height() != height)
    {
            video::vid_setup_yuv_overlay(width, height);
    }
    
    // blitting is not allowed once we create the YUV overlay ...
    g_ldp->set_blitting_allowed(false);
}

void free_yuv_overlay()
{
    // We free both the YUV surface and the YUV texture
    video::vid_free_yuv_overlay();
}

// makes the laserdisc video black while drawing game's video overlay on top
void blank_overlay()
{
    // FIXME
    // only do this if the HW overlay has already been allocated
    if (video::get_yuv_overlay_ready()) {
        g_local_info.display_frame();
    }
}
