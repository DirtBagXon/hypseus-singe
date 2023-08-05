/*
 * ____ DAPHNE COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2006 Scott C. Duensing
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

#include "../hypseus.h" // for get_quitflag
#include "game.h"
#include "../io/conout.h"
#include "../io/mpo_fileio.h"
#include "../io/error.h"
#include "../io/dll.h"
#include "../io/cmdline.h" // for get_next_word  (rdg2010)
#include "../ldp-out/ldp.h"
#include "../sound/samples.h"
#include "../video/palette.h"
#include "../video/video.h"
#include "../vldp/vldp.h" // to get the vldp structs
#include "../scoreboard/scoreboard_collection.h"

#include <string>

using namespace std;

// by rdg2010
#define SINGE_VERSION 1.82 // Update this number whenever you issue a major change

#define SDL_MOUSE 100
#define MANY_MOUSE 200

enum { KEYBD_NORMAL, KEYBD_FULL };
enum { S_B_PLAYER1, S_B_PLAYER2 };
enum { SINGE_SB_IMAGE, SINGE_SB_BEZEL, SINGE_SB_USB, SINGE_SB_PANEL };

typedef struct singeJoyStruct {
    int8_t slide = 5;
    int16_t xpos;
    int16_t ypos;
    int16_t jrelx;
    int16_t jrely;
    int16_t xmov; // signed
    int16_t ymov; // ...
    bool bjx = false;
    bool bjy = false;
} singeJoyStruct;

typedef struct singeScoreboard {
    int8_t credits = 0;
    bool altscore = false;
    uint8_t type = 0;
    int8_t player1_lives = 0;
    int8_t player2_lives = 0;
    int player1_score = 0;
    int player2_score = 0;
    bool repaint;
    bool clear;
} singeScoreboard;

////////////////////////////////////////////////////////////////////////////////

class singe : public game
{
  public:
    singe();
    bool init();
    void start();
    void shutdown();
    void JoystickMotion();
    void input_enable(Uint8, Sint8);
    void input_disable(Uint8, Sint8);
    void OnMouseMotion(Uint16 x, Uint16 y, Sint16 xrel, Sint16 yrel, Sint8 mouseID);
    bool handle_cmdline_arg(const char *arg);
    void palette_calculate();
    void repaint();

    void scoreboard_score(int, uint8_t);
    void scoreboard_lives(int, uint8_t);
    void scoreboard_credits(uint8_t);
    int scoreboard_format(int);

    // g_ldp function wrappers (to make function pointers out of them)
    static void enable_audio1() { g_ldp->enable_audio1(); }
    static void enable_audio2() { g_ldp->enable_audio2(); }
    static void disable_audio1() { g_ldp->disable_audio1(); }
    static void disable_audio2() { g_ldp->disable_audio2(); }
    static void set_search_blanking(bool enabled)
    {
        g_ldp->set_search_blanking(enabled);
    }
    static void set_skip_blanking(bool enabled)
    {
        g_ldp->set_skip_blanking(enabled);
    }
    static bool pre_change_speed(unsigned int uNumerator, unsigned int uDenominator)
    {
        return g_ldp->pre_change_speed(uNumerator, uDenominator);
    }
    static uint32_t get_current_frame()
    {
        return g_ldp->get_current_frame();
    }
    static void pre_play() { g_ldp->pre_play(); }
    static void pre_pause() { g_ldp->pre_pause(); }
    static void pre_stop() { g_ldp->pre_stop(); }
    static bool pre_search(const char *cpszFrame, bool block_until_search_finished)
    {
        return g_ldp->pre_search(cpszFrame, block_until_search_finished);
    }
    static void framenum_to_frame(Uint32 u32Frame, char *pszFrame)
    {
        g_ldp->framenum_to_maxframe(u32Frame, pszFrame);
    }
    static bool pre_skip_forward(Uint32 u32Frames)
    {
        return g_ldp->pre_skip_forward(u32Frames);
    }
    static bool pre_skip_backward(Uint32 u32Frames)
    {
        return g_ldp->pre_skip_backward(u32Frames);
    }
    static void pre_step_forward() { g_ldp->pre_step_forward(); }
    static void pre_step_backward() { g_ldp->pre_step_backward(); }

    static bool get_retro_path()
    {
        return g_ldp->get_console_status();
    }

    static void set_singe_errors(short value)
    {
        g_ldp->set_runtime_error(value);
    }

    // by RDG2010
    // Sometimes it's useful to know the status of the vldp.
    // Lets give Singe the ability to query for this.
    static int get_status() { return g_ldp->get_status(); }
    static double get_singe_version(); // Returns version of the Singe engine
    // Controls VLDP message displays on hypseus.log
    static void set_ldp_verbose(bool thisBol) { g_ldp->setVerbose(thisBol); }

    // These wrapper functions makes the function set_keyboard_mode and
    // get_keyboard_mode
    // available for the DLL/so library side of SINGE.
    // Take a look at the comments in singe::init on singe.cpp for more info.
    //
    // GFM - Get For Me
    static void gfm_set_keyboard_mode(void *pInstance, int thisVal)
    {
        singe *pSingeInstance = (singe *)pInstance;
        pSingeInstance->set_keyboard_mode(thisVal);
    }

    static int gfm_get_keyboard_mode(void *pInstance)
    {
        singe *pSingeInstance = (singe *)pInstance;
        return pSingeInstance->get_keyboard_mode();
    }

    static int gfm_number_of_mice(void *pInstance)
    {
        singe *pSingeInstance = (singe *)pInstance;
        return pSingeInstance->get_mice_detected();
    }

    static double gfm_get_xratio(void *pInstance)
    {
        singe *pSingeInstance = (singe *)pInstance;
        return pSingeInstance->get_xratio();
    }

    static double gfm_get_yratio(void *pInstance)
    {
        singe *pSingeInstance = (singe *)pInstance;
        return pSingeInstance->get_yratio();
    }

    static double gfm_get_fvalue(void *pInstance)
    {
        singe *pSingeInstance = (singe *)pInstance;
        return pSingeInstance->get_fvalue();
    }

    static uint8_t gfm_get_overlaysize(void *pInstance)
    {
        singe *pSingeInstance = (singe *)pInstance;
        return pSingeInstance->get_overlaysize();
    }

    static void gfm_set_overlaysize(void *pInstance, uint8_t thisVal)
    {
        singe *pSingeInstance = (singe *)pInstance;
        pSingeInstance->set_overlaysize(thisVal);
    }

    static void gfm_set_custom_overlay(void *pInstance, uint16_t w, uint16_t h)
    {
        singe *pSingeInstance = (singe *)pInstance;
        pSingeInstance->set_custom_overlay(w, h);
    }

    static void gfm_bezel_enable(void *pInstance, bool bEnable)
    {
        singe *pSingeInstance = (singe *)pInstance;
        pSingeInstance->bezel_enable(bEnable);
    }

    static bool gfm_bezel_is_enabled(void *pInstance)
    {
        singe *pSingeInstance = (singe *)pInstance;
        return pSingeInstance->bezel_is_enabled();
    }

    static void gfm_bezel_type(void *pInstance, uint8_t thisValue)
    {
        singe *pSingeInstance = (singe *)pInstance;
        pSingeInstance->bezel_type(thisValue);
    }

    static void gfm_bezel_clear(void *pInstance, bool bEnable)
    {
        singe *pSingeInstance = (singe *)pInstance;
        pSingeInstance->bezel_clear(bEnable);
    }

    static void gfm_second_score(void *pInstance, bool bEnable)
    {
        singe *pSingeInstance = (singe *)pInstance;
        pSingeInstance->second_score(bEnable);
    }

    static void gfm_bezel_credits(void *pInstance, uint8_t thisVal)
    {
        singe *pSingeInstance = (singe *)pInstance;
        pSingeInstance->bezel_credits(thisVal);
    }

    static void gfm_player1_score(void *pInstance, int thisVal)
    {
        singe *pSingeInstance = (singe *)pInstance;
        pSingeInstance->player1_score(thisVal);
    }

    static void gfm_player2_score(void *pInstance, int thisVal)
    {
        singe *pSingeInstance = (singe *)pInstance;
        pSingeInstance->player2_score(thisVal);
    }

    static void gfm_player1_lives(void *pInstance, uint8_t thisVal)
    {
        singe *pSingeInstance = (singe *)pInstance;
        pSingeInstance->player1_lives(thisVal);
    }

    static void gfm_player2_lives(void *pInstance, uint8_t thisVal)
    {
        singe *pSingeInstance = (singe *)pInstance;
        pSingeInstance->player2_lives(thisVal);
    }

    void set_keyboard_mode(int); // Sets value of private member i_keyboard_mode
    int get_keyboard_mode();     // Retrieves the value of i_keyboard_mode

    // By RDG2010
    // Copying expanded keyboard handling technique from Thayer's driver.
    void process_keydown(SDL_Keycode, int[][2]);
    void process_keyup(SDL_Keycode, int[][2]);


  private:
    // callback function for singe to pass error messages to us
    static void set_last_error(const char *cpszErrMsg);

    string m_strName;       // name of the game
    string m_strGameScript; // script name for the game

    DLL_INSTANCE m_dll_instance; // pointer to DLL we load (if we aren't
                                 // statically linked)

    static void request_screenshot() { video::set_queue_screenshot(true); }

    // by DBX
    double get_xratio();
    double get_yratio();
    double get_fvalue();
    double singe_xratio;
    double singe_yratio;
    double singe_fvalue;

    struct singeJoyStruct g_js;
    struct singeScoreboard g_bezelboard;

    void set_overlaysize(uint8_t);
    void set_custom_overlay(uint16_t, uint16_t);

    uint8_t get_overlaysize();

    uint8_t m_overlay_size;
    uint16_t m_custom_overlay_w;
    uint16_t m_custom_overlay_h;

    void bezel_enable(bool);
    void bezel_type(uint8_t);
    void bezel_clear(bool);
    void second_score(bool);
    void bezel_credits(uint8_t);
    void player1_score(int);
    void player2_score(int);
    void player1_lives(uint8_t);
    void player2_lives(uint8_t);
    bool bezel_is_enabled();

    bool m_bezel_scoreboard;
    bool m_upgrade_overlay;
    bool singe_alt_pressed;
    bool singe_joymouse;
    bool m_muteinit;
    bool m_notarget;
    bool singe_ocv;
    bool singe_oc;

    IScoreboard *m_pScoreboard;

    // by RDG2010

    int i_keyboard_mode; // Stores the keyboard access mode. Valid values are:
                         // KEYBD_NORMAL - Tells SINGE to use hypseus.ini keys.
    // KEYBD_FULL   - Tells SINGE to scan the full keyboard (a la Thayers).
    static const int i_full_keybd_defs[]; // Array with discrete SDL_Keycode values.
                                          // Used in process_keyup/keydown.
};
