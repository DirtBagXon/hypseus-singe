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

#ifndef LDP_VLDP_H
#define LDP_VLDP_H

#include "../vldp/vldp.h"
#include <SDL.h>
#include <list>
#include <map>
#include <string>

using namespace std;

/////////////////////////////////////////////////////////////////////

// maximum # of mpeg files that we will handle
#define MAX_MPEG_FILES 500

struct fileframes {
    string name;  // name of mpeg file
    Sint32 frame; // ldp frame that mpeg file starts on
};

// these values can be OR'd together to create multiple filter effects
const int FILTER_NONE  = 0;        // no filtering
const int FILTER_BLEND = (1 << 0); // blend fields together (cheap de-interlace)
const int FILTER_SCANLINES = (1 << 1); // make every other field black (to give
                                       // it a more authentic look, this also
                                       // de-interlaces for free)

#include "../io/dll.h"
#include "ldp.h"

class ldp_vldp : public ldp
{
  public:
    ldp_vldp();
    ~ldp_vldp();
    bool init_player();
    void shutdown_player();

    // NOTE : open_and_block prepands m_mpeg_path to the filename
    bool open_and_block(const string &strFilename);
    bool precache_and_block(const string &strFilename);
    bool wait_for_status(unsigned int uStatus, const string &strFilename);
    bool nonblocking_search(char *);
    int get_search_result();
    unsigned int play();
    bool skip_forward(Uint32 frames_to_skip, Uint32 target_frame);
    void pause();
    bool change_speed(unsigned int uNumerator, unsigned int uDenominator);
    void think();
#ifdef DEBUG
    uint32_t get_current_frame(); // enable for accuracy testing only
#endif
    void request_screenshot();
    void set_search_blanking(bool);
    void set_skip_blanking(bool);
    void set_seek_frames_per_ms(double value);
    void set_min_seek_delay(unsigned int);
    void set_framefile(const char *filename);
    void set_altaudio(const char *audio_suffix);

    void test_helper(unsigned uIterations);

    // runs through a bunch of tests and returns results in 'lstrPassed' and
    // 'lstrFailed'
    void run_tests(list<string> &lstrPassed, list<string> &lstrFailed);

    bool handle_cmdline_arg(const char *arg);
    bool lock_overlay(Uint32);
    bool unlock_overlay(Uint32);

    unsigned int get_min_seek_delay();

    // parses framefile (contained in pszInBuf) and returns the
    // absolute/relative path to the mpegs in 'sMpegPath',
    //  and populates 'pFrames' until it runs out of data, or hits the
    //  'max_frames' limit.
    // 'pszFramefileFullPath' is the relative/absolute path to the framefile
    // including the filename. This is to make
    //  automatic testing easier, and to calculate sMpegPath correctly.
    // Returns true if parsing succeeded, or false if framefile is invalid.  If
    // returning false, 'err_msg' will have
    //  a description of the error.
    // NOTE : this function is public so that 'releasetest' can do some
    // framefile parse tests.
    bool parse_framefile(const char *pszInBuf, const char *pszFramefileFullPath,
                         string &sMpegPath, struct fileframes *pFrames,
                         Uint32 &frame_index, uint32_t max_frames,
                         string &err_msg);

  private:
    bool load_vldp_lib();
    void free_vldp_lib();
    bool read_frame_conversions();
    bool first_video_file_exists();
    bool last_video_file_parsed();
    void parse_all_video();

    // Attempts to precache all video, returns false if there isn't enough RAM
    // and we aren't overriding the safety check
    bool precache_all_video();

    // Gets the position in the audio stream to seak (in samples), using the
    //  target mpeg frame as input.  (The target mpeg frame is relative to the
    //  beginning
    //  of the mpeg, which is not necessarily the same as the laserdisc frame)
    Uint64 get_audio_sample_position(uint32_t uTargetMpegFrame);

    // NOTE : 'filename' does not include the prefix path
    Uint32 mpeg_info(string &filename, Sint32 ld_frame);

    Sint32 m_target_mpegframe;   // mpeg frame # we are seeking to
    Sint32 m_cur_ldframe_offset; // which laserdisc frame corresponds to the
                                 // first frame in current mpeg file

    // strings
    string m_cur_mpeg_filename; // name of the mpeg file we currently have open
    string m_mpeg_path;         // location of mpeg file(s) to play
    string m_framefile; // name of the framefile we load to get the names of the
                        // mpeg files and their frame #'s
    string m_altaudio_suffix; // adds a suffix to the ogg filename, to support
                              // alternate soundtracks

    struct fileframes m_mpeginfo[MAX_MPEG_FILES]; // names of mpeg files
    unsigned int m_file_index;                    // # of mpeg files in our list
    DLL_INSTANCE m_dll_instance;                  // pointer to DLL we load

    bool m_bFramefileSet; // whether m_framefile was set via commandline or if
                          // it's just the default from the constructor
    bool m_audio_file_opened; // whether we have audio to accompany the video
    bool m_blank_on_searches; // should we blank while searching?
    bool m_blank_on_skips;    // should we blank while skipping?
    double m_seek_frames_per_ms;     // max # of frames that VLDP will seek per
                                     // millisecond (0 = no limit)
    unsigned int m_min_seek_delay;   // min # of milliseconds to force seek to
                                     // last
    bool m_testing;   // should we do a few simple tests to make sure VLDP is
                      // functioning robustly?
    bool m_bPreCache; // should we precache all video?
    bool m_bPreCacheForce; // should we still precache all video even if we
                           // don't have enough RAM?

    unsigned int m_uSoundChipID; // so we can delete the soundchip once we're
                                 // finished

    // holds a record of all precached files (and their associated indices)
    map<string, unsigned int> m_mPreCachedFiles;

    //////////////////////////////////////////////////

    // stuff inside ldp-vldp-audio.cpp
  public:
    void enable_audio1();
    void enable_audio2();
    void disable_audio1();
    void disable_audio2();
    bool switch_altaudio(const char *);

  private:
    void set_audiocopy_callback();
    void oggize_path(string &, string);
    bool audio_init();
    void audio_shutdown();
    void close_audio_stream();
    bool open_audio_stream(const string &strFilename);
    bool seek_audio(Uint64 u64Samples);
    void audio_play(Uint32);
    void audio_pause();
};

// functions that cannot be part of the class because we may need to use them as
// function pointers
int prepare_frame_callback(uint8_t *Yplane, uint8_t *Uplane, uint8_t *Vplane,
                           int Ypitch, int Upitch, int Vpitch);
void display_frame_callback();
void set_blend_fields(bool val);
void update_parse_meter(const string &strFilename);
void report_parse_progress_callback(double percent_complete);
void report_mpeg_dimensions_callback(int, int);
void free_yuv_overlay();
void blank_overlay();
void ldp_vldp_audio_callback(Uint8 *stream, int len,
                             int unused); // declaration for callback in other
                                          // function

#endif
