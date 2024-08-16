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

// Handles SDL input functions (low-level keyboard/joystick input)

#include "config.h"

#include <time.h>
#include <plog/Log.h>
#include "input.h"
#include "conout.h"
#include "keycodes.h"
#include "homedir.h"
#include "../video/video.h"
#include "../hypseus.h"
#include "../timer/timer.h"
#include "../game/game.h"
#include "../game/thayers.h"
#include "../game/singe.h" // by RDG2010
#include "../ldp-out/ldp.h"
#include "fileparse.h"
#include "../manymouse/manymouse.h"

#ifdef UNIX
#include <fcntl.h> // for non-blocking i/o
#endif

#include <queue> // STL queue for coin queue

using namespace std;

// Win32 doesn't use strcasecmp, it uses stricmp (lame)
#ifdef WIN32
#define strcasecmp stricmp
#endif

static bool hotkey = false;

const int JOY_AXIS_TRIG = (int)(32768 * (0.995)); // to trigger the trigger :)
const int JOY_AXIS_MID  = (int)(32768 * (0.75));  // how far they have to move the
                                                  // joystick before it 'grabs'

bool g_use_gamepad       = false;
bool g_use_joystick      = true;  // use a joystick by default
bool g_invert_hat        = false; // invert joystick hat up/down
bool g_open_hat          = false; // multiverse HAT (old behaviour)
unsigned int idle_timer;          // added by JFA for -idleexit

string g_inputini_file = "hypinput.ini"; // Default keymap file
bool m_altInputFileSet = false;

uint8_t thisGame = GAME_UNDEFINED;

const double STICKY_COIN_SECONDS =
    0.125; // how many seconds a coin acceptor is forced to be "depressed" and
           // how many seconds it is forced to be "released"
Uint32 g_sticky_coin_cycles = 0; // STICKY_COIN_SECONDS * cpu::get_hz(0), cannot
                                 // be calculated statically
queue<struct coin_input> g_coin_queue; // keeps track of coin input to guarantee
                                       // that coins don't get missed if the cpu
                                       // is busy (during seeks for example)
Uint64 g_last_coin_cycle_used = 0; // the cycle value that our last coin press
                                   // used

static int available_mice = 0;
static ManyMouseEvent mm_event;

static int g_assigned_hat = 0;
static int g_mouse_mode = SDL_MOUSE;
static SDL_GameController *g_gamepad_id[MAX_GAMECONTROLLER];
static SDL_Haptic *g_gamepad_haptic[MAX_GAMECONTROLLER];
static int g_padindex[MAX_GAMECONTROLLER] = {0};
static uint8_t g_gamepad_attached = 0;
static bool g_index_reset = false;

static bool enabled_haptic = true;
Uint16 g_haptic[2] = {0, 0};

// the ASCII key words that the parser looks at for the key values
// NOTE : these are in a specific order, corresponding to the enum in hypseus.h
const char *g_key_names[] = {"KEY_UP",      "KEY_LEFT",    "KEY_DOWN",
                             "KEY_RIGHT",   "KEY_START1",  "KEY_START2",
                             "KEY_BUTTON1", "KEY_BUTTON2", "KEY_BUTTON3",
                             "KEY_COIN1",   "KEY_COIN2",   "KEY_SKILL1",
                             "KEY_SKILL2",  "KEY_SKILL3",  "KEY_SERVICE",
                             "KEY_TEST",    "KEY_RESET",   "KEY_SCREENSHOT",
                             "KEY_QUIT",    "KEY_PAUSE",   "KEY_CONSOLE",
                             "KEY_TILT"};

// default key assignments, in case .ini file is missing
// Notice each switch can have two keys assigned to it
// NOTE : These are in a specific order, corresponding to the enum in hypseus.h
int g_key_defs[SWITCH_COUNT][2] = {
    {SDLK_UP, 0},             // up
    {SDLK_LEFT, 0},           // left
    {SDLK_DOWN, 0},           // down
    {SDLK_RIGHT, 0},          // right
    {SDLK_1, 0},              // 1 player start
    {SDLK_2, 0},              // 2 player start
    {SDLK_SPACE, SDLK_LCTRL}, // action button 1
    {SDLK_LALT, 0},           // action button 2
    {SDLK_LSHIFT, 0},         // action button 3
    {SDLK_5, SDLK_c},         // coin chute left
    {SDLK_6, 0},              // coin chute right
    {SDLK_KP_DIVIDE, 0},      // skill easy
    {SDLK_KP_MULTIPLY, 0},    // skill medium
    {SDLK_KP_MINUS, 0},       // skill hard
    {SDLK_9, 0},              // service coin
    {SDLK_F2, 0},             // test mode
    {SDLK_F3, 0},             // reset cpu
    {SDLK_F12, SDLK_F11},     // take screenshot
    {SDLK_ESCAPE, SDLK_q},    // Quit DAPHNE
    {SDLK_p, 0},              // pause game
    {SDLK_BACKQUOTE, 0},      // toggle console (TODO)
    {SDLK_t, 0}               // Tilt/Slam switch
};

////////////

int joystick_buttons_map[MAX_GAMECONTROLLER][SWITCH_COUNT][3] = { {0} };

//  {0, 0, 2, -1}, // up
//  {0, 0, 1, -1}, // left
//  {0, 0, 2, 1},  // down
//  {0, 0, 1, 1}   // right

int joystick_axis_map[MAX_GAMECONTROLLER][SWITCH_START1][4] = { {0} };

// Game controller triggers activated
bool controller_trigger_pressed[MAX_GAMECONTROLLER][SDL_CONTROLLER_AXIS_MAX] = { {false} };

// Hot-plugging
int controller_map[MAX_GAMECONTROLLER];

// Mouse button to key mappings
// Added by ScottD for Singe
int mouse_buttons_map[6] = {
    SWITCH_BUTTON1, // 0 (Left Button)
    SWITCH_BUTTON3, // 1 (Middle Button)
    SWITCH_BUTTON2, // 2 (Right Button)
    SWITCH_BUTTON1, // 3 (Wheel Up)
    SWITCH_BUTTON2, // 4 (Wheel Down)
    SWITCH_MOUSE_DISCONNECT
};

////////////

void CFG_Keys()
{
    struct mpo_io *io;
    string cur_line = "";
    string key_name = "", sval1 = "", sval2 = "", sval3 = "", sval4 = "", sval5 = "", sval6 = "", eq_sign = "";
    int val1 = 0, val2 = 0, val3 = 0, val4 = 0, val5 = 0, val6 = 0;
    const uint8_t minparam = 3;
    bool warn = true;
    bool end = false;

    if (m_altInputFileSet && !g_index_reset) {
       string keyinput_notice = "Loading alternate keymap file: ";
       keyinput_notice += g_inputini_file.c_str();
       LOGI << keyinput_notice.c_str();
    }

    // find where the keymap ini file is (if the file doesn't exist, this string will be empty)
    string strDapInput = g_homedir.find_file(g_inputini_file.c_str(), true);
    io = mpo_open(strDapInput.c_str(), MPO_OPEN_READONLY);
    if (io) {
        LOGD << "Remapping input ...";

        cur_line = "";

        // read lines until we find the keyboard header or we hit EOF
        while (strcasecmp(cur_line.c_str(), "[KEYBOARD]") != 0) {
            read_line(io, cur_line);
            if (io->eof) {
                LOGW <<
                    "CFG_Keys() : never found [KEYBOARD] header, aborting";
                break;
            }
        }

        // go until we hit EOF, or we break inside this loop (which is the
        // expected behavior)
        while (!io->eof) {
            // if we read in something besides a blank line
            if (read_line(io, cur_line) > 0) {
                bool corrupt_file = true; // we use this to avoid doing multiple
                                          // if/else/break statements

                // if we are able to read in the key name
                if (find_word(cur_line.c_str(), key_name, cur_line)) {
                    if (strcasecmp(key_name.c_str(), "END") == 0) {
                        end = true;
                        break; // if we hit the 'END' keyword, we're done
                    }

                    // equals sign
                    if (find_word(cur_line.c_str(), eq_sign, cur_line) && (eq_sign == "=")) {
                        if (find_word(cur_line.c_str(), sval1, cur_line)) {
                            if (find_word(cur_line.c_str(), sval2, cur_line)) {
                                if (find_word(cur_line.c_str(), sval3, cur_line)) {
                                    if (isdigit(sval1[0])) val1 = atoi(sval1.c_str());
                                    else val1 = sdl2_keycode(sval1.c_str());
                                    if (isdigit(sval2[0])) val2 = atoi(sval2.c_str());
                                    else val2 = sdl2_keycode(sval2.c_str());
                                    if (g_use_gamepad) {
                                        if (isdigit(sval3[0])) val3 = atoi(sval3.c_str());
                                        else val3 = sdl2_controller_button(sval3.c_str());
                                    } else {
                                        if (!isdigit(sval3[0]) && warn) {
                                            LOGW << "Found a button config string: " << sval3.c_str();
                                            LOGW << "Did you intend to use the '-gamepad' argument?";
                                            warn = false;
                                        }
                                        val3 = atoi(sval3.c_str());
                                        if (strcasecmp(key_name.c_str(), g_key_names[0]) == 0) {
                                            if (!g_open_hat && g_use_joystick &&
                                                    SDL_NumJoysticks() > 0) {
                                                int divider = (sval3.length() == 4) ? 1000 : 100;
                                                g_assigned_hat = (val3 / divider);
                                                LOGI << fmt("Joystick HAT enabled on stick: [%d]",
                                                    g_assigned_hat);
                                            }
                                        }
                                    }
                                    val4 = 0;
                                    if (find_word(cur_line.c_str(), sval4, cur_line)) {
                                        if (g_use_gamepad) {
                                            if (isdigit(sval4[0])) val4 = atoi(sval4.c_str());
                                            else val4 = sdl2_controller_axis(sval4.c_str());
                                        } else {
                                            val4 = atoi(sval4.c_str());
                                        }
                                    }
                                    if (g_use_gamepad) {
                                        val5 = val6 = 0;
                                        if (find_word(cur_line.c_str(), sval5, cur_line)) {
                                            if (isdigit(sval5[0])) val5 = atoi(sval5.c_str());
                                            else val5 = sdl2_controller_button(sval5.c_str());
                                            if (find_word(cur_line.c_str(), sval6, cur_line)) {
                                                if (isdigit(sval6[0])) val6 = atoi(sval6.c_str());
                                                else val6 = sdl2_controller_axis(sval6.c_str());
                                            }
                                        }
                                    }
                                    uint8_t id = 0;
                                    corrupt_file = false; // looks like we're good

                                    bool found_match = false;
                                    for (int i = 0; i < SWITCH_COUNT; i++) {
                                        // if we can match up a key name (see
                                        // list above) ...
                                        if (strcasecmp(key_name.c_str(), g_key_names[i]) == 0)
                                        {
                                            g_key_defs[i][0] = val1;
                                            g_key_defs[i][1] = val2;

                                            // if zero then no mapping
                                            // necessary, just use default, if any
                                            if (val3 > 0) {
                                                if (val3 > AXIS_TRIGGER) {
                                                    joystick_buttons_map[id][i][0] = (val3 / AXIS_TRIGGER);
                                                    joystick_buttons_map[id][i][1] = val3;
                                                } else {
                                                    // first digit=joystick index, remaining digits=button index
                                                    int divider = (sval3.length() == 4) ? 1000 : 100;
                                                    joystick_buttons_map[id][i][0] = (val3 / divider);
                                                    joystick_buttons_map[id][i][1] = (val3 % divider);
                                                }
                                            }
                                            // joystick axis
                                            if (val4 != 0) {
                                                // first digit=joystick index, remaining digits=axis index, sign=direction
                                                int divider = (sval4.length() > 4) ? 1000 : 100;
                                                joystick_axis_map[id][i][0] = abs(val4 / divider);
                                                joystick_axis_map[id][i][1] = abs(val4 % divider);
                                                joystick_axis_map[id][i][2] = (val4 == 0)?0:((val4 < 0) ? -1 : 1);
                                            }

                                            if (g_use_gamepad) {
                                                if (val5 != 0)
                                                {
                                                    id = 1;
                                                    int adj = (val5 > AXIS_TRIGGER) ? (val5 / AXIS_TRIGGER) : val5;
                                                    joystick_buttons_map[id][i][0] = adj;
                                                    joystick_buttons_map[id][i][1] = val5;

                                                    if (val6 != 0) {
                                                        joystick_axis_map[id][i][0] = abs(val6);
                                                        joystick_axis_map[id][i][1] = abs(val6);
                                                        joystick_axis_map[id][i][2] = (val6 == 0)?0:((val6 < 0) ? -1 : 1);
                                                    }
                                                }
                                            }
                                            found_match = true;
                                            break;
                                        }
                                    }

                                    // if the key line was unknown
                                    if (!found_match) {
                                        cur_line = "Unrecognized key name " +
                                                   key_name;
                                        LOGW << cur_line;
                                        corrupt_file = true;
                                    }

                                } else
                                    LOGW << fmt("Expected %d config parameters, only found %d", minparam, minparam - 1);
                            } else
                                LOGW << fmt("Expected %d config parameters, only found %d", minparam, minparam - 2);
                        } else
                            LOGW << fmt("Expected %d config parameters, found none", minparam);
                    } // end equals sign
                    else
                        LOGW << "Expected an '=' sign, didn't find it";
                } // end if we found key_name
                else
                    LOGW << "Weird unexpected error happened"; // this really shouldn't ever happen

                if (corrupt_file) {
                    LOGW << "input remapping file was not in proper format, so we are aborting";
                    break;
                }
            } // end if we didn't find a blank line
              // else it's a blank line so we just ignore it
        }     // end while not EOF

        if (!end) {
            LOGW << fmt("Didn't find \"END\" in %s, it may be corrupt.", g_inputini_file.c_str());
        }

        mpo_close(io);
    } else // end if file was opened successfully
        LOGW << fmt("%s not found, using defaults", g_inputini_file.c_str());
}

static void manymouse_init_mice(void)
{
    LOGI << "Using ManyMouse for mice input.";
    available_mice = ManyMouse_Init();
    static Mouse mice[MAX_MICE];

    if (available_mice > MAX_MICE)
        available_mice = MAX_MICE;

    g_game->set_mice_detected(available_mice);

    if (available_mice <= 0) {
        LOGW << "No mice detected!";
        return;
    }
    else
    {
        int i;
        if (available_mice == 1) {
            LOGI << "Only 1 mouse found.";
        }
        else
        {
            LOGI << fmt("Found %d mice devices:", available_mice);
        }

        for (i = 0; i < available_mice; i++)
        {
            const char *name = ManyMouse_DeviceName(i);
            strncpy(mice[i].name, name, sizeof (mice[i].name));
            mice[i].name[sizeof (mice[i].name) - 1] = '\0';
            mice[i].connected = 1;
            LOGI << fmt("#%d: %s", i, mice[i].name);
        }
        SDL_SetWindowGrab(video::get_window(), SDL_TRUE);
    }
}

static void manymouse_update_mice()
{
    while (ManyMouse_PollEvent(&mm_event))
    {
        Mouse *mouse;
        if (mm_event.device >= (unsigned int) available_mice)
            continue;

        static Mouse mice[MAX_MICE];
        mouse = &mice[mm_event.device];

#ifndef WIN32
        float val, maxval;
#endif

        switch(mm_event.type) {
        case MANYMOUSE_EVENT_RELMOTION:
            if (mm_event.item == 0)
                mouse->x += mm_event.value;
            else if (mm_event.item == 1)
                mouse->y += mm_event.value;

            if (mouse->x < 0) mouse->x = 0;
            else if (mouse->x >= video::get_video_width()) mouse->x = video::get_video_width();

            if (mouse->y < 0) mouse->y = 0;
            else if (mouse->y >= video::get_video_height()) mouse->y = video::get_video_height();

            g_game->OnMouseMotion(mouse->x, mouse->y, mouse->relx, mouse->rely, mm_event.device);
            break;
        case MANYMOUSE_EVENT_ABSMOTION:

#ifdef WIN32
            mouse->x = int((mm_event.minval / 65535.0f) * video::get_video_width());
            mouse->y = int((mm_event.maxval / 65535.0f) * video::get_video_height());
#else
            val = (float) (mm_event.value - mm_event.minval);
            maxval = (float) (mm_event.maxval - mm_event.minval);

            if (mm_event.item == 0)
                mouse->x = (val / maxval) * video::get_video_width();
            else if (mm_event.item == 1)
                mouse->y = (val / maxval) * video::get_video_height();
#endif

            g_game->OnMouseMotion(mouse->x, mouse->y, mouse->relx, mouse->rely, mm_event.device);
            break;
        case MANYMOUSE_EVENT_BUTTON:
            if (mm_event.item < 32)
            {
                if (mm_event.value == 1)
                {
                    input_enable((Uint8)mouse_buttons_map[mm_event.item], mm_event.device);
                    mouse->buttons |= (1 << mm_event.item);
                }
                else
                {
                    input_disable((Uint8)mouse_buttons_map[mm_event.item], mm_event.device);
                    mouse->buttons &= ~(1 << mm_event.item);
                }
            }
            break;
        case MANYMOUSE_EVENT_SCROLL:
            if (mm_event.item == 0)
            {
                if (mm_event.value > 0)
                    input_disable(SWITCH_MOUSE_SCROLL_UP, mm_event.device);
                else
                    input_disable(SWITCH_MOUSE_SCROLL_DOWN, mm_event.device);
            }
            break;
        case MANYMOUSE_EVENT_DISCONNECT:
            mice[mm_event.device].connected = 0;
            input_disable(SWITCH_MOUSE_DISCONNECT, mm_event.device);
            break;
        default:
            break;
        }
    }
}

int SDL_input_init()
// initializes the keyboard (and joystick if one is present)
// returns 1 if successful, 0 on error
// NOTE: Video has to be initialized BEFORE this is or else it won't work
{

    int result = 0;

    // make sure the coin queue is empty (it should be, this is just a safety
    // check)
    while (!g_coin_queue.empty()) {
        g_coin_queue.pop();
    }
    g_sticky_coin_cycles =
        (Uint32)(STICKY_COIN_SECONDS * cpu::get_hz(0)); // only needs to be
                                                       // calculated once

    for (int i = 0; i < MAX_GAMECONTROLLER; i++) {
        g_gamepad_haptic[i] = NULL;
        g_gamepad_id[i] = NULL;
        controller_map[i] = i;
    }

    if (g_use_gamepad) {

        Uint32 gamepad_init = SDL_INIT_GAMECONTROLLER;

        if (enabled_haptic) {
            gamepad_init |= SDL_INIT_HAPTIC;
            LOGI << "Loading Game Controller system with HAPTIC support";
        }

        if (SDL_InitSubSystem(gamepad_init) >= 0) {

            string strGCdb = g_homedir.find_file("gamecontrollerdb.txt", true);

            if (mpo_file_exists(strGCdb.c_str())) {
                LOGI << "Found " << strGCdb;
                int num = SDL_GameControllerAddMappingsFromFile(strGCdb.c_str());
                if (num > 0) {
                    LOGI << fmt("Loaded %d Game Controller mappings", num);
                } else {
                    LOGW << "Loading gamecontrollerdb.txt failed";
                }
            }

            SDL_gamepad_init();
            result = 1;

        } else
            LOGW << "GamePad initialization failed!";

    } else {

        if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) >= 0) {
            // if joystick usage is enabled
            if (g_use_joystick) {
                // open joysticks
                for (int i = 0; i < SDL_NumJoysticks(); i++) {
                    SDL_Joystick* joystick = SDL_JoystickOpen(i);
                    if (joystick != NULL) {
                        LOGD << "Joystick #" << i << " was successfully opened";
                    } else {
                        LOGW << "Error opening joystick #" << i << "!";
                    }
                }
                if (SDL_NumJoysticks() == 0) {
                    LOGI << "No joysticks detected";
                }
            }
            // notify user that their attempt to disable the joystick is successful
            else {
                LOGI << "Joystick usage disabled";
            }

            CFG_Keys(); // NOTE : for some freak reason, this should not be done
                        // BEFORE the joystick is initialized, I don't know why!
            result = 1;
        } else {
            LOGW << "Input initialization failed!";
        }
    }

    idle_timer = refresh_ms_time(); // added by JFA for -idleexit

    // if the mouse is disabled, then filter mouse events out ...
    if (!g_game->get_mouse_enabled())
    {
         FilterMouseEvents(true);
    }
    else
    {
         FilterMouseEvents(false);
         if (thisGame == GAME_UNDEFINED) thisGame = g_game->get_game_type();

         if (g_game->get_manymouse() && thisGame != GAME_THAYERS)
             g_mouse_mode = MANY_MOUSE;

         if (!set_mouse_mode(g_mouse_mode)) {
             LOGE << "Mouse initialization failed";
             set_quitflag();
         }
    }

    return (result);
}

void reOrderIndex()
{
    for (int i = 0; i < MAX_GAMECONTROLLER; ++i)
    {
        for (int j = 0; j < MAX_GAMECONTROLLER; ++j)
        {
            if (controller_map[i] == g_padindex[j])
            {
                controller_map[i] = j;
                break;
            }
        }
    }

    LOGW << fmt("Gamepad index re-ordering requested: %s", g_inputini_file.c_str());
}

void SDL_gamepad_init()
{
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
         if (SDL_IsGameController(i)) {
             g_gamepad_id[g_gamepad_attached] = SDL_GameControllerOpen(i);
             SDL_Joystick* joy = SDL_GameControllerGetJoystick(g_gamepad_id[g_gamepad_attached]);
             if (joy != NULL) {

                 LOGI << "Gamepad #" << i << ": "
			 << SDL_GameControllerName(g_gamepad_id[g_gamepad_attached]) << " connected";

                 if (enabled_haptic)
                 {
                     g_gamepad_haptic[g_gamepad_attached] = SDL_HapticOpenFromJoystick(joy);

		     if (g_gamepad_haptic[g_gamepad_attached] != NULL) {
                         if (SDL_HapticRumbleSupported(g_gamepad_haptic[g_gamepad_attached])) {
                             LOGI << "Gamepad #" << i << ": Haptic Rumble support";
                         } else {
                             SDL_HapticClose(g_gamepad_haptic[g_gamepad_attached]);
                             g_gamepad_haptic[g_gamepad_attached] = NULL;
                         }
                     }
                 }

                 g_gamepad_attached++;

                 if (g_gamepad_attached > (MAX_GAMECONTROLLER - 1)) {
                     LOGI << "Max Game Controller limit [" << MAX_GAMECONTROLLER << "] reached";
                     break;
                }
            }
        }
    }
    SDL_JoystickEventState(SDL_ENABLE);
    SDL_GameControllerEventState(SDL_ENABLE);

    if (g_index_reset) reOrderIndex();

    CFG_Keys();
}

SDL_GameController* get_gamepad_id(int i)
{
    if (g_gamepad_id[(uint8_t)i])
        return g_gamepad_id[(uint8_t)i];

    return nullptr;
}

void FilterMouseEvents(bool bFilteredOut)
{
    int iState = SDL_ENABLE;

    if (bFilteredOut) {
        iState = SDL_IGNORE;
    }

    SDL_EventState(SDL_MOUSEMOTION, iState);
    SDL_EventState(SDL_MOUSEBUTTONDOWN, iState);
    SDL_EventState(SDL_MOUSEBUTTONUP, iState);
}

// does any shutting down necessary
// 1 = success, 0 = failure
void SDL_input_shutdown(void)
{
    if (g_use_gamepad) {
        for (int i = 0; i < MAX_GAMECONTROLLER; i++) {
            if (g_gamepad_haptic[i]) {
                SDL_HapticClose(g_gamepad_haptic[i]);
                g_gamepad_haptic[i] = NULL;
            }
            if (g_gamepad_id[i]) {
                SDL_GameControllerClose(g_gamepad_id[i]);
                g_gamepad_id[i] = NULL;
            }
        }
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
    } else
        SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

// checks to see if there is incoming input, and acts on it
void SDL_check_input()
{

    SDL_Event event;

    while ((SDL_PollEvent(&event)) && (!get_quitflag())) {
        process_event(&event);
    }

    // added by JFA for -idleexit
    if (get_idleexit() > 0 && elapsed_ms_time(idle_timer) > get_idleexit())
        set_quitflag();

    // if the coin queue has something entered into it
    if (!g_coin_queue.empty()) {
        struct coin_input coin = g_coin_queue.front(); // examine the next
                                                       // element in the queue
                                                       // to be considered

        // NOTE : when cpu timers are flushed, the coin queue is automatically
        // "reshuffled"
        // so it is safe not to check to see whether the cpu timers were flushed
        // here

        // if it's safe to activate the coin
        if (cpu::get_total_cycles_executed(0) > coin.cycles_when_to_enable) {
            // if we're supposed to enable this coin
            if (coin.coin_enabled) {
                g_game->input_enable(coin.coin_val, NOMOUSE);
            }
            // else we are supposed to disable this coin
            else {
                g_game->input_disable(coin.coin_val, NOMOUSE);
            }
            g_coin_queue.pop(); // remove coin entry from queue
        }
        // else it's not safe to activate the coin, so we just wait
    }
    // else the coin queue is empty, so we needn't do anything ...
}

// processes incoming input
void process_event(SDL_Event *event)
{
    unsigned int i = 0;

    // by RDG2010
    // make things easier to read...
    SDL_Keycode keyPressed = event->key.keysym.sym;
    if (thisGame == GAME_UNDEFINED) thisGame = g_game->get_game_type();

    switch (event->type) {
    case SDL_KEYDOWN:
        reset_idle(); // added by JFA for -idleexit

        // by RDG2010
        // Get SINGE full access to keyboard input (like Thayers)

        if (thisGame != GAME_THAYERS && thisGame != GAME_SINGE) {
            process_keydown(keyPressed);
        } else {
            if (thisGame == GAME_THAYERS) {

                thayers *l_thayers = dynamic_cast<thayers *>(g_game);
                // cast game class to a thayers class so we can call a
                // thayers-specific function

                // make sure cast succeeded
                if (l_thayers) l_thayers->process_keydown(keyPressed);
// else cast failed, and we would crash if we tried to call process_keydown
// cast would fail if g_game is not a thayers class

#ifdef BUILD_SINGE
            } else {

                if (thisGame == GAME_SINGE) {
                    singe *l_singe = dynamic_cast<singe *>(g_game);
                    if (l_singe)
                        l_singe->process_keydown(keyPressed, g_key_defs);
                }
#endif
            }
        }

        break;
    case SDL_KEYUP:
        // MPO : since con_getkey doesn't catch the key up event, we shouldn't
        // call reset_idle here.
        // reset_idle(); // added by JFA for -idleexit

        // by RDG2010
        // Get SINGE full access to keyboard input (like Thayers)

        if (thisGame != GAME_THAYERS && thisGame != GAME_SINGE) {
            process_keyup(keyPressed);

        } else {
            if (thisGame == GAME_THAYERS) {

                thayers *l_thayers = dynamic_cast<thayers *>(g_game);
                // cast game class to a thayers class so we can call a
                // thayers-specific function

                // make sure cast succeeded
                if (l_thayers) l_thayers->process_keyup(keyPressed);

// else cast failed, and we would crash if we tried to call process_keydown
// cast would fail if g_game is not a thayers class
#ifdef BUILD_SINGE
            } else {

                if (thisGame == GAME_SINGE) {
                    singe *l_singe = dynamic_cast<singe *>(g_game);
                    if (l_singe) l_singe->process_keyup(keyPressed, g_key_defs);
                }
#endif
            }
        }

        break;
    case SDL_CONTROLLERDEVICEREMOVED:
        for (int i = 0; i < MAX_GAMECONTROLLER; i++) {
            if (g_gamepad_id[i] && event->cdevice.which == SDL_JoystickInstanceID(
                  SDL_GameControllerGetJoystick(g_gamepad_id[i]))) {
                LOGI << "GamePad '" << SDL_GameControllerName(g_gamepad_id[i]) << "' disconnected";
                if (g_gamepad_haptic[i]) {
                    SDL_HapticClose(g_gamepad_haptic[i]);
                    g_gamepad_haptic[i] = NULL;
                }
                SDL_GameControllerClose(g_gamepad_id[i]);
                g_gamepad_id[i] = NULL;
                g_gamepad_attached--;
                break;
            }
        }
        break;
    case SDL_CONTROLLERDEVICEADDED:
	if (g_index_reset) {
            LOGW << "Controller hotplugging disabled in reorder mode.";
            break;
        }
        if (g_gamepad_attached < MAX_GAMECONTROLLER) {
            for (int i = 0; i < MAX_GAMECONTROLLER; i++) {
                if (!g_gamepad_id[i]) {
                    g_gamepad_id[i] = SDL_GameControllerOpen(event->cdevice.which);
                    SDL_Joystick* joy = SDL_GameControllerGetJoystick(g_gamepad_id[i]);
                    if (joy != NULL) {
                        LOGI << "Gamepad #" << i << ": "
                            << SDL_GameControllerName(g_gamepad_id[i]) << " connected";
                        if (enabled_haptic && !g_gamepad_haptic[i]) {
                            g_gamepad_haptic[i] = SDL_HapticOpenFromJoystick(joy);

                            if (g_gamepad_haptic[i] != NULL) {
                                if (SDL_HapticRumbleSupported(g_gamepad_haptic[i])) {
                                    LOGI << "Gamepad #" << i << ": Haptic Rumble support";
                                } else {
                                    SDL_HapticClose(g_gamepad_haptic[i]);
                                    g_gamepad_haptic[i] = NULL;
                                }
                            }
                        }
                        controller_map[SDL_JoystickInstanceID(
				SDL_GameControllerGetJoystick(g_gamepad_id[i]))] = i;
                        g_gamepad_attached++;
                        break;
                    }
                }
            }
        }
        break;
    case SDL_CONTROLLERAXISMOTION:
        process_controller_motion(event);
        break;
    case SDL_JOYAXISMOTION:
        if (g_use_gamepad) break;
        process_joystick_motion(event);
        break;
    case SDL_JOYHATMOTION:
        if (g_use_gamepad) break;
        // only process events for the first hat
        if (event->jhat.hat == 0) {
            reset_idle();
            process_joystick_hat_motion(event);
        }
        break;
    case SDL_JOYBUTTONDOWN:
        if (g_use_gamepad) break;
        reset_idle(); // added by JFA for -idleexit

        // loop through map and find corresponding action
        for (i = 0; i < SWITCH_COUNT; i++) {
            if (event->jbutton.which == joystick_buttons_map[0][i][0]
                            && event->jbutton.button == joystick_buttons_map[0][i][1]-1) {
                if (i == SWITCH_COIN1) hotkey = true;
                input_enable(i, NOMOUSE);
                break;
            }
        }

        break;
    case SDL_CONTROLLERBUTTONDOWN:
        reset_idle(); // added by JFA for -idleexit
        // loop through map and find corresponding action
        for (i = 0; i < SWITCH_COUNT; i++) {
            if (event->cbutton.button == joystick_buttons_map[controller_map[event->cdevice.which]][i][1]-1) {
                if (i == SWITCH_COIN1) hotkey = true;
                input_enable(i, NOMOUSE);
                if (g_haptic[0] && g_gamepad_haptic[controller_map[event->cdevice.which]])
                    SDL_GameControllerRumble(SDL_GameControllerFromInstanceID(event->cdevice.which),
                                                 g_haptic[0], g_haptic[0], g_haptic[1]);
                break;
            }
        }

        break;
    case SDL_JOYBUTTONUP:
        if (g_use_gamepad) break;
        reset_idle(); // added by JFA for -idleexit
        hotkey = false;

        // loop through map and find corresponding action
        for (i = 0; i < SWITCH_COUNT; i++) {
            if (event->jbutton.which == joystick_buttons_map[0][i][0]
                           && event->jbutton.button == joystick_buttons_map[0][i][1]-1) {
                input_disable(i, NOMOUSE);
                break;
            }
        }

        break;
    case SDL_CONTROLLERBUTTONUP:
        reset_idle(); // added by JFA for -idleexit
        hotkey = false;

        // loop through map and find corresponding action
        for (i = 0; i < SWITCH_COUNT; i++) {
            if (event->cbutton.button == joystick_buttons_map[controller_map[event->cdevice.which]][i][1]-1) {
                input_disable(i, NOMOUSE);
                break;
            }
        }

        break;
    case SDL_QUIT:
        // if they are trying to close the window
        set_quitflag();
        break;
    default:
        break;
    }

    if (g_game->get_mouse_enabled())
    {
        if (g_mouse_mode == MANY_MOUSE)
        {
            manymouse_update_mice();

        } else {

           switch (event->type) {
           case SDL_MOUSEBUTTONDOWN:
               for (i = 0; i < (sizeof(mouse_buttons_map) / sizeof(int)); i++) {
                    if (event->button.button == i) {
                        g_game->input_enable((Uint8)mouse_buttons_map[i], NOMOUSE);
                        break;
                    }
               }
               break;
           case SDL_MOUSEBUTTONUP:
               for (i = 0; i < (sizeof(mouse_buttons_map) / sizeof(int)); i++) {
                    if (event->button.button == i) {
                        g_game->input_disable((Uint8)mouse_buttons_map[i], NOMOUSE);
                        break;
                    }
               }
               break;
           case SDL_MOUSEMOTION:
               g_game->OnMouseMotion(event->motion.x, event->motion.y,
                       event->motion.xrel, event->motion.yrel, NOMOUSE);
               break;
          }
       }
    }

    // added by JFA for -idleexit
    if (get_idleexit() > 0 && elapsed_ms_time(idle_timer) > get_idleexit())
        set_quitflag();
}

// if a key is pressed, we go here
void process_keydown(SDL_Keycode key)
{
    // go through each key def (defined in enum in hypseus.h) and check to see if
    // the key entered matches
    // If we have a match, the switch to be used is the value of the index
    // "move"
    for (Uint8 move = 0; move < SWITCH_COUNT; move++) {
        if ((key == g_key_defs[move][0]) || (key == g_key_defs[move][1])) {
            input_enable(move, NOMOUSE);
        }
    }

    if (g_game->alt_commands) {
        input_toolbox(key, g_game->alt_lastkey, false);
        g_game->alt_lastkey = key;
    }
    // end ALT-COMMAND checks

    // check for ALT-COMMANDS but not this pass
    if ((key == SDLK_LALT) || (key == SDLK_RALT)) {
        g_game->alt_commands = true;
    }
}

// if a key is released, we go here
void process_keyup(SDL_Keycode key)
{
    // go through each key def (defined in enum in hypseus.h) and check to see if
    // the key entered matches
    // If we have a match, the switch to be used is the value of the index
    // "move"
    for (Uint8 move = 0; move < SWITCH_COUNT; move++) {
        if ((key == g_key_defs[move][0]) || (key == g_key_defs[move][1])) {
            input_disable(move, NOMOUSE);
        }
    }

    // if they are releasing an ALT key
    if ((key == SDLK_LALT) || (key == SDLK_RALT)) {
        g_game->alt_commands = false;
    }

    if (g_game->alt_commands) g_game->alt_lastkey = SDLK_UNKNOWN;
}

// game controller axis
void process_controller_motion(SDL_Event *event)
{
    static int x_axis_in_use[MAX_GAMECONTROLLER] = { 0 }; // true if joystick is left or right
    static int y_axis_in_use[MAX_GAMECONTROLLER] = { 0 }; // true if joystick is up or down

    g_game->ControllerAxisProxy(event->caxis.axis, event->caxis.value, controller_map[event->cdevice.which]);

    // Deal with AXIS TRIGGERS
    for (int i = 0; i < SWITCH_COUNT; i++) {

        if (event->caxis.axis == joystick_buttons_map[controller_map[event->cdevice.which]][i][1]-AXIS_TRIGGER) {

            if ((abs(event->caxis.value) > JOY_AXIS_TRIG)
			    && !controller_trigger_pressed[controller_map[event->cdevice.which]][event->caxis.axis]) {
                input_enable(i, NOMOUSE);
                if (g_haptic[0] && g_gamepad_haptic[controller_map[event->cdevice.which]])
                    SDL_GameControllerRumble(SDL_GameControllerFromInstanceID(event->cdevice.which),
                                                 g_haptic[0], g_haptic[0], g_haptic[1]);
                controller_trigger_pressed[controller_map[event->cdevice.which]][event->caxis.axis] = true;
            } else {
                if (controller_trigger_pressed[controller_map[event->cdevice.which]][event->caxis.axis]) {
                    input_disable(i, NOMOUSE);
                    controller_trigger_pressed[controller_map[event->cdevice.which]][event->caxis.axis] = false;
                }
            }
            return;
        }
    }

    // loop through map and find corresponding action
    int key = -1;
    for (int i = 0; i < SWITCH_START1; i++) {

        if (event->caxis.axis == joystick_axis_map[controller_map[event->cdevice.which]][i][1]-1
			&& ((event->caxis.value < 0) ? -1 : 1) == joystick_axis_map[controller_map[event->cdevice.which]][i][2]) {
            key = i;
            break;
        }
    }
    if (key == -1) return;

    if (abs(event->caxis.value) > JOY_AXIS_MID) {
        input_enable(key, NOMOUSE);
        if (key == SWITCH_UP || key == SWITCH_DOWN)
            y_axis_in_use[controller_map[event->cdevice.which]] = 1;
        else
            x_axis_in_use[controller_map[event->cdevice.which]] = 1;
    }
    else {
        if ((key == SWITCH_UP ||
		key == SWITCH_DOWN) && y_axis_in_use[controller_map[event->cdevice.which]]) {
            input_disable(SWITCH_UP, NOMOUSE);
            input_disable(SWITCH_DOWN, NOMOUSE);
            y_axis_in_use[controller_map[event->cdevice.which]] = 0;

        } else if ((key == SWITCH_LEFT ||
		key == SWITCH_RIGHT) && x_axis_in_use[controller_map[event->cdevice.which]]) {
            input_disable(SWITCH_LEFT, NOMOUSE);
            input_disable(SWITCH_RIGHT, NOMOUSE);
            x_axis_in_use[controller_map[event->cdevice.which]] = 0;
        }
    }
}

// processes movements of the joystick
void process_joystick_motion(SDL_Event *event)
{
    static int x_axis_in_use = 0; // true if joystick is left or right
    static int y_axis_in_use = 0; // true if joystick is up or down

    // loop through map and find corresponding action
    int key = -1;
    for (int i = 0; i < SWITCH_START1; i++) {
        if (event->jaxis.which == joystick_axis_map[0][i][0] && event->jaxis.axis == joystick_axis_map[0][i][1]-1
			&& ((event->jaxis.value < 0) ? -1 : 1) == joystick_axis_map[0][i][2]) {
            key = i;
            break;
        }
    }
    if (key == -1) return;

    if (abs(event->jaxis.value) > JOY_AXIS_MID) {
        input_enable(key, NOMOUSE);
        if (key == SWITCH_UP || key == SWITCH_DOWN)
            y_axis_in_use = 1;
        else
            x_axis_in_use = 1;
    }
    else {
        if ((key == SWITCH_UP || key == SWITCH_DOWN) && y_axis_in_use) {
            input_disable(SWITCH_UP, NOMOUSE);
            input_disable(SWITCH_DOWN, NOMOUSE);
            y_axis_in_use = 0;

        } else if ((key == SWITCH_LEFT || key == SWITCH_RIGHT) && x_axis_in_use) {
            input_disable(SWITCH_LEFT, NOMOUSE);
            input_disable(SWITCH_RIGHT, NOMOUSE);
            x_axis_in_use = 0;
        }
    }
}

// processes movement of the joystick hat
void process_joystick_hat_motion(SDL_Event *event)
{
    if (!g_open_hat && (event->jaxis.which != g_assigned_hat)) return;

    static Uint8 prev_hat_position = SDL_HAT_CENTERED;
    Uint8 hat_movement = event->jhat.value ^ prev_hat_position;

    switch (hat_movement)
    {
        case SDL_HAT_UP:
            if (event->jhat.value & SDL_HAT_UP) {
                if (g_invert_hat) {
                    input_enable(SWITCH_DOWN, NOMOUSE);
                } else {
                    input_enable(SWITCH_UP, NOMOUSE);
                }
                prev_hat_position |= SDL_HAT_UP;
            } else {
                if (g_invert_hat) {
                    input_disable(SWITCH_DOWN, NOMOUSE);
                } else {
                    input_disable(SWITCH_UP, NOMOUSE);
                }
                prev_hat_position &= ~SDL_HAT_UP;
            }
            break;

        case SDL_HAT_RIGHT:
            if (event->jhat.value & SDL_HAT_RIGHT) {
                input_enable(SWITCH_RIGHT, NOMOUSE);
                prev_hat_position |= SDL_HAT_RIGHT;
            } else {
                input_disable(SWITCH_RIGHT, NOMOUSE);
                prev_hat_position &= ~SDL_HAT_RIGHT;
            }
            break;

        case SDL_HAT_DOWN:
            if (event->jhat.value & SDL_HAT_DOWN) {
                if (g_invert_hat) {
                    input_enable(SWITCH_UP, NOMOUSE);
                } else {
                    input_enable(SWITCH_DOWN, NOMOUSE);
                }
                prev_hat_position |= SDL_HAT_DOWN;
            } else {
                if (g_invert_hat) {
                    input_disable(SWITCH_UP, NOMOUSE);
                } else {
                    input_disable(SWITCH_DOWN, NOMOUSE);
                }
                prev_hat_position &= ~SDL_HAT_DOWN;
            }
            break;

        case SDL_HAT_LEFT:
            if (event->jhat.value & SDL_HAT_LEFT) {
                input_enable(SWITCH_LEFT, NOMOUSE);
                prev_hat_position |= SDL_HAT_LEFT;
            } else {
                input_disable(SWITCH_LEFT, NOMOUSE);
                prev_hat_position &= ~SDL_HAT_LEFT;
            }
            break;

        default:
            break;
    }
}

// if user has pressed a key/moved the joystick/pressed a button
void input_enable(Uint8 move, Sint8 mouseID)
{
    // first test universal input, then pass unknown input on to the game driver

    switch (move) {
    default:
        g_game->input_enable(move, mouseID);
        break;
    case SWITCH_RESET:
        g_game->reset();
        break;
    case SWITCH_SCREENSHOT:
        LOGD << "Screenshot requested!";
        g_ldp->request_screenshot();
        break;
    case SWITCH_PAUSE:
        if (thisGame == GAME_SINGE)
            g_game->input_disable(move, mouseID);
        g_game->toggle_game_pause();
        break;
    case SWITCH_QUIT:
        set_quitflag();
        break;
    case SWITCH_START1:
        if (hotkey)
            set_quitflag();
        else
            g_game->input_enable(move, mouseID);
        break;
    case SWITCH_COIN1:
    case SWITCH_COIN2:
        // coin inputs are buffered to ensure that they are not dropped while
        // the cpu is busy (such as during a seek)
        // therefore if the input is coin1 or coin2 AND we are using a real cpu
        // (and not a program such as seektest)
        if (cpu::get_hz(0) > 0) {
            add_coin_to_queue(true, move);
        }
        break;
    case SWITCH_CONSOLE:
        // TODO: implement for SDL2
        break;
    }
}

// if user has released a key/released a button/moved joystick back to center
// position
void input_disable(Uint8 move, Sint8 mouseID)
{
    // don't send reset or screenshots key-ups to the individual games because
    // they will return warnings that will alarm users
    if ((move != SWITCH_RESET) && (move != SWITCH_SCREENSHOT) &&
        (move != SWITCH_QUIT) && (move != SWITCH_PAUSE)) {
        // coin inputs are buffered to ensure that they are not dropped while
        // the cpu is busy (such as during a seek)
        // therefore if the input is coin1 or coin2 AND we are using a real cpu
        // (and not a program such as seektest)
        if (((move == SWITCH_COIN1) || (move == SWITCH_COIN2)) && (cpu::get_hz(0) > 0)) {
            add_coin_to_queue(false, move);
        } else {
            g_game->input_disable(move, mouseID);
        }
    }
    // else do nothing
}

inline void add_coin_to_queue(bool enabled, Uint8 val)
{
    Uint64 total_cycles = cpu::get_total_cycles_executed(0);
    struct coin_input coin;
    coin.coin_enabled = enabled;
    coin.coin_val     = val;

    // make sure that we are >= to the total cycles executed otherwise coin
    // insertions will be really quick
    if (g_last_coin_cycle_used < total_cycles) {
        g_last_coin_cycle_used = total_cycles;
    }
    g_last_coin_cycle_used += g_sticky_coin_cycles; // advance to the next safe
                                                    // slot
    coin.cycles_when_to_enable = g_last_coin_cycle_used; // and assign this safe
                                                         // slot to this current
                                                         // coin
    g_coin_queue.push(coin); // add the coin to the queue ...
}

// added by JFA for -idleexit
void reset_idle(void)
{
    static bool bSoundOn = false;

    // At this time, the only way the sound will be muted is if -startsilent was
    // passed via command line.
    // So the first key press should always unmute the sound.
    if (!bSoundOn) {
        bSoundOn = true;
        sound::set_mute(false);
    }

    idle_timer = refresh_ms_time();
}
// end edit

// primarily to disable joystick use if user wishes not to use one
void set_use_joystick(bool val) { g_use_joystick = val; }
void set_invert_hat(bool val) { g_invert_hat = val; }
void set_open_hat(bool val) { g_open_hat = val; }

// Allow us to specify an alternate keymap.ini file
void set_inputini_file(const char *inputFile) {
    m_altInputFileSet = true;
    g_inputini_file = inputFile;
}

// Use a gamepad?
void set_use_gamepad(bool value) {
    g_use_gamepad = value;
    g_use_joystick = !value;
}

void set_gamepad_order(int *c, int max) {

    for (int i = 0; i < max; i++) {
        g_padindex[i] = (uint8_t)(c[i] - 1);
    }

    g_index_reset = true;
}

void disable_haptics() { enabled_haptic = false; }

void set_haptic(Uint8 value) {
    g_haptic[0] = (1 << (value + 0xC)) - 1;
    g_haptic[1] = 0x96;
}

void do_gamepad_rumble(Uint8 str, Uint8 len, Uint8 id)
{
    if (g_gamepad_id[id] && g_gamepad_haptic[id]) {
        Uint16 s = (1 << (str + 0xC)) - 1;
        SDL_GameControllerRumble(g_gamepad_id[id], s, s, (0x4B << len));
    }
}

bool set_mouse_mode(int thisMode)
{
   bool result = false;

   if (g_game->get_mouse_enabled())
   {
       if (g_mouse_mode == MANY_MOUSE) ManyMouse_Quit();

       memset(mouse_buttons_map, 0, sizeof(mouse_buttons_map));

       if (thisMode == SDL_MOUSE) {

           mouse_buttons_map[0] = SWITCH_BUTTON1;  // 0 (Left Button)
           mouse_buttons_map[1] = SWITCH_BUTTON3;  // 1 (Middle Button)
           mouse_buttons_map[2] = SWITCH_BUTTON2;  // 2 (Right Button)
           mouse_buttons_map[3] = SWITCH_BUTTON1;  // 3 (Wheel Up)
           mouse_buttons_map[4] = SWITCH_BUTTON2;  // 4 (Wheel Down)
           mouse_buttons_map[5] = SWITCH_MOUSE_DISCONNECT;
           result = true;
       }
       else if (thisMode == MANY_MOUSE)
       {
           mouse_buttons_map[0] = SWITCH_BUTTON3;  // 0 (Left Button)
           mouse_buttons_map[1] = SWITCH_BUTTON1;  // 1 (Middle Button)
           mouse_buttons_map[2] = SWITCH_BUTTON2;  // 2 (Right Button)
           mouse_buttons_map[3] = SWITCH_MOUSE_SCROLL_UP;  // 3 (Wheel Up)
           mouse_buttons_map[4] = SWITCH_MOUSE_SCROLL_DOWN;  // 4 (Wheel Down)
           mouse_buttons_map[5] = SWITCH_MOUSE_DISCONNECT;

           manymouse_init_mice();
           result = true;

       }
   }
   return result;
}
