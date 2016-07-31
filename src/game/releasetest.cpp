/*
 * ____ DAPHNE COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2005 Matt Ownby
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

// RELEASE TEST
// by Matt Ownby

// Does automatic tests that require no user intervention, useful for before
// doing an official release

#include "config.h"

#include "releasetest.h"
#include "../io/conout.h"
#include "../io/numstr.h"
#include "../io/input.h"
#include "../io/fileparse.h"
#include "../io/mpo_mem.h"
#include "../hypseus.h"
#include "../timer/timer.h"
#include "../video/rgb2yuv.h"
#include "../video/video.h" // for draw_string
#include "../ldp-out/ldp-vldp.h"
#include "../vldp/vldp.h"
#include "../sound/sound.h"
#include "../sound/samples.h"
#include "../sound/mix.h"

extern struct yuv_buf g_blank_yuv_buf; // to do overlay tests
extern Sint32 g_vertical_offset;       // to do overlay tests
extern Uint8 *g_line_buf;  // temp sys RAM for doing calculations so we can do
                           // fastest copies to slow video RAM
extern Uint8 *g_line_buf2; // 2nd buf
extern Uint8 *g_line_buf3; // 3rd buf
extern unsigned int g_filter_type;
extern SDL_Surface *g_screen; // to test video modes

//////////////////////////////////////////////////////////////////////////

const int REL_VID_W                = 320;
const int REL_VID_H                = 240;
const unsigned int REL_COLOR_COUNT = 256;

releasetest::releasetest()
    : m_test_all(true), // run all tests by default
      m_test_line_parse(false), m_test_framefile_parse(false), m_test_rgb2yuv(false),
      m_test_think_delay(false), m_test_vldp(false),
      m_test_vldp_render(false), m_test_mix(false),
      m_test_samples(false), m_test_sound_mixing(false)
// m_test_gp2x_timer(false)
{
    m_log_passed.clear();
    m_log_failed.clear();

    m_video_overlay_width     = REL_VID_W; // sensible default
    m_video_overlay_height    = REL_VID_H; // " " "
    m_palette_color_count     = REL_COLOR_COUNT;
    m_game_uses_video_overlay = true;
    m_overlay_size_is_dynamic = true; // to avoid ldp-vldp from printing a
                                      // warning message

    // load in a few samples for testing purposes ...
    m_num_sounds    = 3;
    m_sound_name[0] = "dl_credit.wav";
    m_sound_name[1] = "dl_accept.wav";
    m_sound_name[2] = "dl_buzz.wav";

    // REMOVE ME!!!
    // m_test_all = false;
    // m_test_think_delay = true;
    // m_test_vldp = true;
}

releasetest::~releasetest()
{
    // nothing to do here ...
}

void releasetest::start()
{
    string msg = "";
    size_t idx = 0;

    if (dotest(m_test_sound_mixing)) test_sound_mixing();
    if (dotest(m_test_line_parse)) test_line_parse();
    if (dotest(m_test_framefile_parse)) test_framefile_parse();
    if (dotest(m_test_samples)) test_samples();

// Only test these functions if we've built with MMX code,
//  otherwise the test is useless
#ifdef USE_MMX
    if (dotest(m_test_rgb2yuv)) test_rgb2yuv();
    if (dotest(m_test_mix)) test_mix();
#endif // USE_MMX

    if (dotest(m_test_think_delay)) test_think_delay();

    if (dotest(m_test_vldp)) test_vldp();

    // this test should come near the end as it messes with YUV overlay stuff
    // (which may be unstable on some platforms)
    if (dotest(m_test_vldp_render)) test_vldp_render();

    // grab any bugs that happened to be logged during this process!
    list<string> ldp_bug_log;
    g_ldp->get_bug_log(ldp_bug_log); // if this has any entries in it, it means
                                     // bugs were detected
    for (list<string>::iterator i = ldp_bug_log.begin(); i != ldp_bug_log.end(); i++) {
        logtest(false, *i); // make note of these bugs
    }

    // REPORT TIME! :)
    printline("--- RELEASETEST SUMMARY");
    printline("-----------------------");

    for (idx = 0; idx < m_log_passed.size(); idx++) {
        printline(m_log_passed[idx].c_str());
    }
    msg = "# of passed tests: " + numstr::ToStr((unsigned int)m_log_passed.size());
    printline(msg.c_str());

    for (idx = 0; idx < m_log_failed.size(); idx++) {
        printline(m_log_failed[idx].c_str());
    }
    msg = "# of failed tests: " + numstr::ToStr((unsigned int)m_log_failed.size());
    printline(msg.c_str());
}

void releasetest::repaint()
{
    unsigned int i = 0;
    Uint32 cur_w   = g_ldp->get_discvideo_width() >> 1; // width overlay should be
    Uint32 cur_h   = g_ldp->get_discvideo_height() >> 1; // height overlay should
                                                         // be

    // if the width or height of the mpeg video has changed since we last were
    // here (ie, opening a new mpeg)
    // then reallocate the video overlay buffer
    if ((cur_w != m_video_overlay_width) || (cur_h != m_video_overlay_height)) {
        printline("RELEASETEST : Surface does not match mpeg, re-allocating "
                  "surface!"); // remove me
        if (g_ldp->lock_overlay(1000)) {
            m_video_overlay_width  = cur_w;
            m_video_overlay_height = cur_h;
            shutdown_video();
            if (!init_video()) {
                printline(
                    "Fatal Error, trying to re-create the surface failed!");
                set_quitflag();
            }
            g_ldp->unlock_overlay(1000); // unblock game video overlay
        } else {
            printline("RELEASETEST : Timed out trying to get a lock on the yuv "
                      "overlay");
            return;
        }
    } // end if dimensions are incorrect

    SDL_FillRect(m_video_overlay[m_active_video_overlay], NULL,
                 0); // erase anything on video overlay
    unsigned char *pix = (unsigned char *)m_video_overlay[m_active_video_overlay]->pixels;

    // draw dotted one line on top
    for (i = 0; i < (cur_w >> 1); i++) {
        *pix = 0xFF;
        pix++;
        *pix = 0x00;
        pix++;
    }

    // draw dotted white line on the bottom
    pix = (unsigned char *)m_video_overlay[m_active_video_overlay]->pixels +
          (cur_w * (cur_h - 1)); // go to bottom line
    for (i = 0; i < (cur_w >> 1); i++) {
        *pix = 0xFF;
        pix++;
        *pix = 0x00;
        pix++;
    }

    // draw some text to make it interesting
    //draw_string("ReleaseTest video_repaint() was here :)", 1, 3,
    //            m_video_overlay[m_active_video_overlay]);
}

void releasetest::logtest(bool passed, const string &testname)
{
    string msg = "";
    if (passed) {
        msg = testname + " passed.";
        m_log_passed.push_back(msg);
    } else {
        msg = testname + " FAILED!";
        m_log_failed.push_back(msg);
    }
}

bool releasetest::dotest(bool b) { return (m_test_all || b); }

bool releasetest::i_close_enuf(int a, int b, int prec)
{
    int diff = a - b;
    if (diff < 0) diff *= -1; // make positive
    return (diff <= prec);
}

void releasetest::test_line_parse()
{
    const char *BUF1 = "single line";
    const char *BUF2 = "abc\ndef";
    const char *BUF3 = "hij\rlmn";
    const char *BUF4 = "\n\n\r\r";
    const char *BUF5 = "abc\n\n\n";
    const char *BUF6 = "a\n ";

    string s        = "";
    const char *res = NULL;

    // test 1
    res = read_line(BUF1, s);
    logtest((res == NULL) && (s == "single line"), "LineParse #1");

    // test 2
    res = read_line(BUF2, s);
    logtest((res == (BUF2 + 4)) && (s == "abc"), "LineParse #2");

    // test 3
    res = read_line(BUF3, s);
    logtest((res == (BUF3 + 4)) && (s == "hij"), "LineParse #3");

    // test 4
    res = read_line(BUF4, s);
    logtest((res == NULL) && (s.empty()), "LineParse #4");

    // test 5
    res = read_line(BUF5, s);
    logtest((res == NULL) && (s == "abc"), "LineParse #5");

    // test 6
    res = read_line(BUF6, s);
    logtest((res == (BUF6 + 2)) && (s == "a"), "LineParse #6");
}

void releasetest::test_framefile_parse()
{
    bool res     = false;
    string sPath = "";
    struct fileframes ffTmp[MAX_MPEG_FILES];
    string sErrMsg     = "";
    unsigned int f_idx = 0;

    delete g_ldp; // free ldp class temporarily to make sure it doesn't conflict
                  // with what we're doing

    // we can't use g_ldp as a pointer because we need to access a specific
    // function inside ldp_vldp
    ldp_vldp *vldp = new ldp_vldp();

    // test 1
    const char *FF1     = "\\abcpath\n\n1 asdf.m2v";
    const char *FFPATH1 = "c:/blah.txt";
    res = vldp->parse_framefile(FF1, FFPATH1, sPath, ffTmp, f_idx, MAX_MPEG_FILES, sErrMsg);
    logtest(res && (sPath == "/abcpath/") && (ffTmp[0].name == "asdf.m2v") &&
                (ffTmp[0].frame == 1) && (f_idx == 1),
            "FrameFile Parse #1");

    // test 2
    const char *FF2     = "../appendage\r0		blah.m2v";
    const char *FFPATH2 = "C:\\Documents and Settings\\Fisher Pricer\\My "
                          "Documents\\My Games\\Hypseus "
                          "Ver0.99.6\\framefile.txt";
    res = vldp->parse_framefile(FF2, FFPATH2, sPath, ffTmp, f_idx, MAX_MPEG_FILES, sErrMsg);
    logtest(res && (sPath == "C:/Documents and Settings/Fisher Pricer/My "
                             "Documents/My Games/Hypseus "
                             "Ver0.99.6/../appendage/") &&
                (ffTmp[0].name == "blah.m2v") && (ffTmp[0].frame == 0) && (f_idx == 1),
            "FrameFile Parse #2");

    // test 3 (multiple entries)
    const char *FF3     = ".\n\r\n\r\n\r\n\n\n\n-35 asdf\n\n\n5 qwerty";
    const char *FFPATH3 = "relpath/hi.txt";
    res = vldp->parse_framefile(FF3, FFPATH3, sPath, ffTmp, f_idx, MAX_MPEG_FILES, sErrMsg);
    logtest(res && (sPath == "relpath/./") && (ffTmp[0].name == "asdf") &&
                (ffTmp[0].frame == -35) && (ffTmp[1].name == "qwerty") &&
                (ffTmp[1].frame == 5) && (f_idx == 2),
            "FrameFile Parse #3");

    // test 4 (no subdirectory)
    const char *FF4     = ".\\\n1 hi.m2v";
    const char *FFPATH4 = "hi.txt";
    res = vldp->parse_framefile(FF4, FFPATH4, sPath, ffTmp, f_idx, MAX_MPEG_FILES, sErrMsg);
    logtest(res && (sPath == "./") && (ffTmp[0].name == "hi.m2v") &&
                (ffTmp[0].frame == 1) && (f_idx == 1),
            "FrameFile Parse #4");

    // test 5 (max # of entries, but not over)
    const char *FF5     = ".\n1 hi.m2v";
    const char *FFPATH5 = "hi.txt";
    res = vldp->parse_framefile(FF5, FFPATH5, sPath, ffTmp, f_idx, 1, sErrMsg);
    logtest(res && (sPath == "./") && (ffTmp[0].name == "hi.m2v") &&
                (ffTmp[0].frame == 1) && (f_idx == 1),
            "FrameFile Parse #5");

    const char *FF6     = ".\n \t \n32 space.m2v\n\t\n";
    const char *FFPATH6 = "space.txt";
    res = vldp->parse_framefile(FF6, FFPATH6, sPath, ffTmp, f_idx, 1, sErrMsg);
    logtest(res && (sPath == "./") && (ffTmp[0].name == "space.m2v") &&
                (ffTmp[0].frame == 32) && (f_idx == 1),
            "FrameFile Parse #6");

    const char *FFBAD1     = ".\n\n\n\n\n";
    const char *FFBADPATH1 = "whatever.txt";
    res = vldp->parse_framefile(FFBAD1, FFBADPATH1, sPath, ffTmp, f_idx,
                                MAX_MPEG_FILES, sErrMsg);
    logtest(!res, "FrameFile Bad Parse #1");

    const char *FFBAD2     = ".\nfilename.m2v";
    const char *FFBADPATH2 = "whatever.txt";
    res = vldp->parse_framefile(FFBAD2, FFBADPATH2, sPath, ffTmp, f_idx,
                                MAX_MPEG_FILES, sErrMsg);
    logtest(!res, "FrameFile Bad Parse #2");

    // overflow test
    const char *FFBAD3     = ".\n1 filename.m2v\n2 filename2.m2v";
    const char *FFBADPATH3 = "whatever.txt";
    res = vldp->parse_framefile(FFBAD3, FFBADPATH3, sPath, ffTmp, f_idx, 1, sErrMsg);
    logtest(!res, "FrameFile Bad Parse #3");

    // non-integer test
    const char *FFBAD4     = ".\nasdf filename.m2v";
    const char *FFBADPATH4 = "whatever.txt";
    res = vldp->parse_framefile(FFBAD4, FFBADPATH4, sPath, ffTmp, f_idx,
                                MAX_MPEG_FILES, sErrMsg);
    logtest(!res, "FrameFile Bad Parse #4");

    delete vldp;
    g_ldp = new ldp(); // use generic LDP class ...
}

void releasetest::test_rgb2yuv()
{
    bool passed    = true; // easier to default to true on this one
    const int PREC = 1;    // how much drift we allow our functions

    printline("Beginning RGB2YUV exerciser (this may take a long time) ...");

    for (unsigned int r = 0; (r <= 255) && passed && (!get_quitflag()); r++) {
        for (unsigned int g = 0; (g <= 255) && passed; g++) {
            for (unsigned int b = 0; (b <= 255) && passed; b++) {
                rgb2yuv_input[0] = r;
                rgb2yuv_input[1] = g;
                rgb2yuv_input[2] = b;
                rgb2yuv();

                // this is the reference C code which we consider to be the
                // standard to judge against
                unsigned char real_y =
                    (unsigned char)(((9798 * r) + (19235 * g) + (3736 * b)) >> 15);
                unsigned char real_u =
                    (unsigned char)((((b - real_y) * 18514) >> 15) + 128);
                unsigned char real_v =
                    (unsigned char)((((r - real_y) * 23364) >> 15) + 128);

                if ((!i_close_enuf(rgb2yuv_result_y, real_y, PREC)) ||
                    (!i_close_enuf(rgb2yuv_result_v, real_v, PREC)) ||
                    (!i_close_enuf(rgb2yuv_result_u, real_u, PREC))) {
                    string err = "ERROR : Y[" + numstr::ToStr(rgb2yuv_result_y) +
                                 "!=" + numstr::ToStr(real_y) + "] or V[" +
                                 numstr::ToStr(rgb2yuv_result_v) + "!=" +
                                 numstr::ToStr(real_v) + "] or " + "U[" +
                                 numstr::ToStr(rgb2yuv_result_u) + "!=" +
                                 numstr::ToStr(real_u) + "]";
                    printline(err.c_str());
                    passed = false;
                }
            }
        }
        SDL_check_input(); // give user some breathing room
    }

    logtest(passed, "RGB2YUV Complete Exerciser");
}

void releasetest::test_think_delay()
{
    unsigned int uStartTime = GET_TICKS();
    unsigned int uMsToDelay = 1000;
    unsigned int u          = 0;
    bool bTestResult        = false;
    unsigned int uElapsedMs = 0;

    // ensure that pre_init has been called.
    delete g_ldp;
    g_ldp = new ldp();
    g_ldp->pre_init();

    for (u = 0; u < uMsToDelay; u++) {
        g_ldp->think_delay(1);
    }

    uElapsedMs = elapsed_ms_time(uStartTime);

    bTestResult = i_close_enuf((int)uElapsedMs, uMsToDelay, 15);
    string msg = "Think Delay Test : wanted to delay " + numstr::ToStr(uMsToDelay) +
                 ", actual is " + numstr::ToStr(uElapsedMs);
    logtest(bTestResult, msg);
}

void releasetest::test_vldp()
{
    delete g_ldp; // whatever player we've got, de-allocate it so we can force
                  // VLDP as our player
    g_ldp          = new ldp_vldp(); // now we're using VLDP for sure ...
    ldp_vldp *vldp = dynamic_cast<ldp_vldp *>(g_ldp); // to access ldp_vldp
                                                      // functions

    m_disc_fps  = 29.97; // needed to make seeking work properly
    m_uDiscFPKS = 29970;

    vldp->set_framefile("test/framefile1.txt");

    if (g_ldp->pre_init()) {
        g_ldp->pre_search("00001", true); // render an image to the screen
        m_video_overlay_needs_update = true;
        blit(); // re-size our video overlay if necessary
        printline("Beginning VLDP comprehensive test ...");
        list<string> lstrPassed, lstrFailed;
        vldp->run_tests(lstrPassed, lstrFailed);

        list<string>::iterator i;
        // log positive results
        for (i = lstrPassed.begin(); i != lstrPassed.end(); i++) {
            logtest(true, "Internal " + *i);
        }
        // log negative results
        for (i = lstrFailed.begin(); i != lstrFailed.end(); i++) {
            logtest(false, "Internal " + *i);
        }

    } else {
        printline("Cannot run VLDP comprehensive test due to missing file(s)");
        logtest(false, "VLDP Internal Tests (get files from "
                       "http://www.daphne-emu.com/download/"
                       "daphne_test_files.zip)");
    }

    // force a complete shutdown to make sure all variables get properly
    // initialized
    delete g_ldp;
    g_ldp = new ldp_vldp();
    vldp  = dynamic_cast<ldp_vldp *>(g_ldp); // to access ldp_vldp functions

    vldp->set_framefile("test/framefile2.txt");

    if (g_ldp->pre_init()) {
        // turn off seek delay so that seeking goes really quickly
        g_ldp->set_min_seek_delay(0);
        g_ldp->set_seek_frames_per_ms(0);

        printline("Trying to seek to a file that isn't found");
        logtest(!g_ldp->pre_search("05000", true),
                "VLDP Search to Missing File #1");

        printline("Trying to seek to a file that isn't found (+1) ");
        logtest(!g_ldp->pre_search("05001", true),
                "VLDP Search to Missing File #2");

        printline("Trying to seek to a file that isn't found (+2) ");
        logtest(!g_ldp->pre_search("05002", true),
                "VLDP Search to Missing File #3");

        printline("Trying to seek to illegal frame");
        // in framefile2, frame 1 should be missing
        logtest(!g_ldp->pre_search("00001", true),
                "VLDP Search to Illegal Frame");

        printline("Trying to seek to valid 640x480 frame");
        logtest(g_ldp->pre_search("30000", true), "VLDP Seek to 640x480 Frame");
        m_video_overlay_needs_update = true;
        blit(); // resize-video overlay if needed
        logtest(m_video_overlay_width == 320, "Overlay is 320 wide");

        // g_ldp->think_delay(1000);	// let them see it ..

        printline("Trying to seek to valid 720x480 frame");
        logtest(g_ldp->pre_search("5", true), "VLDP Seek to 720x480 Frame");
        m_video_overlay_needs_update = true;
        blit(); // resize-video overlay if needed
        logtest(m_video_overlay_width == 360, "Overlay is 360 wide");

        // g_ldp->think_delay(1000);	// let them see it ..

        printline("Trying to play valid video");
        g_ldp->pre_play();
        logtest(g_ldp->get_status() == LDP_PLAYING, "VLDP is playing video");

        // if clip is playing, wait for its expected duration, then stop
        // (audio should be heard also)
        if (g_ldp->get_status() == LDP_PLAYING) {
            // pause for 15 seconds while clip plays
            g_ldp->think_delay(2000);
        }

        g_ldp->pre_search("30000", true); // make overlay go back to 640x480 for
                                          // future tests
        m_video_overlay_needs_update = true;
        blit(); // resize-video overlay if needed

    } else {
        printline("Cannot run 2nd set of VLDP tests due to missing file(s)");
        logtest(false, "VLDP 2nd set of tests (get files from "
                       "http://www.daphne-emu.com/download/"
                       "daphne_test_files.zip)");
    }

    delete g_ldp; // VLDP may be hogging cpu cycles if it is at the end of a
                  // stream, so it's best to shut it down now
    g_ldp = new ldp();
}

void releasetest::test_vldp_render()
{
    bool test_result = false;

        delete g_ldp; // whatever player we've got, de-allocate it so we can
                      // force VLDP as our player
        g_ldp = new ldp_vldp(); // now we're using VLDP for sure ...

        // no need to call init (it would fail anyway as we don't have a
        // framefile)

        // create the overlay
        unsigned int width  = REL_VID_W << 1;
        unsigned int height = REL_VID_H << 1;
        report_mpeg_dimensions_callback(width, height);

        m_video_overlay_needs_update = true;
        blit(); // prepare our video overlay for testing

        printline("Beginning VLDP Render test...");
        g_filter_type = FILTER_NONE; // no filter for this test
        test_result   = false; // in case g_hw_overlay is NULL, we want the test
                               // result to report failure
        free_yuv_overlay(); // we're done

        logtest(test_result, "VLDP Overlay w/ Vertical Offset Render");
}

void releasetest::test_mix()
{
    const int BUF_SIZE = 256;
    unsigned char line1[BUF_SIZE];
    unsigned char line2[BUF_SIZE];
    unsigned char dst_C[BUF_SIZE];
    unsigned char dst_MMX[BUF_SIZE];
    int i = 0;

    printline("Beginning AUDIO MIX accuracy test...");

    // fill lines with values (that are the same each time test is run, to make
    // reproducing bugs easier)
    for (i = 0; i < BUF_SIZE; i++) {
        line1[i] = i;
        line2[i] = i;
    }

    mix_s MixBufs1, MixBufs2;
    MixBufs1.pMixBuf = line1;
    MixBufs1.pNext   = &MixBufs2;
    MixBufs2.pMixBuf = line2;
    MixBufs2.pNext   = NULL;
    g_pMixBufs       = &MixBufs1;
    g_uBytesToMix    = BUF_SIZE;
    g_pSampleDst     = dst_C;

    mix_c(); // do the reference test

    g_pSampleDst = dst_MMX;
    g_mix_func(); // now do the MMX version (unless we have no MMX in which case
                  // this test is meaningless)

    bool result = true;
    for (i = 0; i < BUF_SIZE; i++) {
        if (dst_C[i] != dst_MMX[i]) {
            result = false;
            break;
        }
    }

    logtest(result, "AUDIO MIX accuracy test");
}

void releasetest::test_samples()
{
    const unsigned int WAIT_MS = 1000;
    //	unsigned int uTimer = GET_TICKS();

    printline("Playing samples in quick succession..");
    sound::play(1);
    make_delay(100);
    sound::play(2);
    make_delay(100);
    sound::play(0);
    make_delay(WAIT_MS);

    printline("Playing 1 sample alone..");
    sound::play(0);
    make_delay(WAIT_MS);

    printline("Playing 2 samples together...");
    sound::play(0);
    sound::play(1);
    make_delay(WAIT_MS);

    printline("Playing 1 sample twice...");
    sound::play(0);
    sound::play(0);
    make_delay(WAIT_MS);

    printline("Playing 1 sample thrice...");
    sound::play(0);
    sound::play(0);
    sound::play(0);
    make_delay(WAIT_MS);

    printline("Playing 3 samples simultaneously...");
    sound::play(0);
    sound::play(1);
    sound::play(2);
    make_delay(WAIT_MS);

    printline("Playing 1 sample");
    sound::play(1);
    make_delay(WAIT_MS);

    printline("Playing 2 samples");
    sound::play(1);
    sound::play(1);
    make_delay(WAIT_MS);

    printline("Playing 3 samples");
    sound::play(1);
    sound::play(1);
    sound::play(1);
    make_delay(WAIT_MS);
}

void releasetest::test_sound_mixing()
{
    SDL_PauseAudio(1); // stop other audio thread so we can call the mixing
                       // callbacks directlry
    MAKE_DELAY(1000);  // give time for the audio callbacks to finish up

    bool bTestPassed              = false;
    unsigned char u8Buf[4]        = {0x58, 0x7F, 0, 0};
    unsigned char u8BufClipped[4] = {0xFF, 0x7F, 0, 0}; // maximum value
    unsigned char u8Stream[4];

    int iSlot = samples::play(u8Buf, sizeof(u8Buf), 2, -1, NULL);

    if (iSlot >= 0) {
        samples::get_stream(u8Stream, 4, sizeof(u8Stream));

        // these should be the same ...
        if (memcmp(u8Stream, u8Buf, sizeof(u8Buf)) == 0) {
            bTestPassed = true;
        }
    }

    logtest(bTestPassed, "Sample Mixing");

    bTestPassed = false;
    iSlot = samples::play(u8Buf, sizeof(u8Buf), 2, -1, NULL);
    if (iSlot >= 0) {
        // test the sample mixer passed through the main audio mixer
        sound::callback(NULL, u8Stream, sizeof(u8Stream));

        // these should be the same ...
        if (memcmp(u8Stream, u8Buf, sizeof(u8Buf)) == 0) {
            bTestPassed = true;
        }
    }

    logtest(bTestPassed, "Sample Mixing + Main Audio Mixer");

    bTestPassed = false;
    // play the sample twice to ensure that it exceeds the threshold
    iSlot      = samples::play(u8Buf, sizeof(u8Buf), 2, -1, NULL);
    int iSlot2 = samples::play(u8Buf, sizeof(u8Buf), 2, -1, NULL);
    if ((iSlot >= 0) && (iSlot2 >= 0)) {
        // test the sample mixer passed through the main audio mixer
        sound::callback(NULL, u8Stream, sizeof(u8Stream));

        // these should be the same ...
        if (memcmp(u8Stream, u8BufClipped, sizeof(u8Buf)) == 0) {
            bTestPassed = true;
        }
    }

    logtest(bTestPassed, "Sample Mixing + Main Audio Mixer + Clipping");

    SDL_PauseAudio(0); // start up other audio thread again
}
