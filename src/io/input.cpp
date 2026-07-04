/*
 * ____ DAPHNE COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2001 Matt Ownby / 2025 DirtBagXon
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
#include "configfile.h"

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

static bool g_hotkey = false;
static bool m_config_valid = true;

constexpr double DEFAULT_TRIGGER_FACTOR = 0.995; // to trigger the trigger :)
const int JOY_AXIS_MID  = (int)(MAX_AXIS * (0.75));  // how far they have to move the
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

static int g_available_mice = -1;
static ManyMouseEvent mm_event;
static unsigned char mm_absolute_only = 0;

static int g_assigned_hat = 0;
static int g_mouse_mode = SDL_MOUSE;
static SDL_GameController *g_gamepad_id[MAX_GAMECONTROLLER];
static SDL_Haptic *g_gamepad_haptic[MAX_GAMECONTROLLER];
static int g_padindex[MAX_GAMECONTROLLER] = {0};
static uint8_t g_gamepad_attached = 0;
static bool g_index_reset = false;

static uint8_t RELFORMAT = 1;

static int JOY_AXIS_TRIG = static_cast<int>(MAX_AXIS * DEFAULT_TRIGGER_FACTOR);

static int g_gamepad_wad = 0;

static bool enabled_haptic = true;
Uint16 g_haptic[2] = {0, 0};

// the ASCII key words that the parser looks at for the key values
// NOTE : these are in a specific order, corresponding to the enum in hypseus.h
const char *g_key_names[] =
{
    "KEY_UP",      "KEY_LEFT",    "KEY_DOWN",
    "KEY_RIGHT",   "KEY_START1",  "KEY_START2",
    "KEY_BUTTON1", "KEY_BUTTON2", "KEY_BUTTON3",
    "KEY_COIN1",   "KEY_COIN2",   "KEY_SKILL1",
    "KEY_SKILL2",  "KEY_SKILL3",  "KEY_SERVICE",
    "KEY_TEST",    "KEY_RESET",   "KEY_SCREENSHOT",
    "KEY_QUIT",    "KEY_PAUSE",   "KEY_CONSOLE",
    "KEY_TILT"
};

// default key assignments, in case .ini file is missing
// Notice each switch can have two keys assigned to it
// NOTE : These are in a specific order, corresponding to the enum in input.h
int g_key_defs[SWITCH_COUNT][2] =
{
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

int joystick_buttons_map[MAX_GAMECONTROLLER][SWITCH_COUNT][3] = {{{0}}};

//  {0, 0, 2, -1}, // up
//  {0, 0, 1, -1}, // left
//  {0, 0, 2, 1},  // down
//  {0, 0, 1, 1}   // right

int joystick_axis_map[MAX_GAMECONTROLLER][SWITCH_START1][4] = {{{0}}};

// Game controller triggers activated
bool controller_trigger_pressed[MAX_GAMECONTROLLER][SDL_CONTROLLER_AXIS_MAX] = { {false} };

// Hot-plugging
int controller_map[MAX_GAMECONTROLLER];

// Mouse button to key mappings
// Added by ScottD for Singe
int mouse_buttons_map[6] =
{
    SWITCH_BUTTON1, // 0 (Left Button)
    SWITCH_BUTTON3, // 1 (Middle Button)
    SWITCH_BUTTON2, // 2 (Right Button)
    SWITCH_BUTTON1, // 3 (Wheel Up)
    SWITCH_BUTTON2, // 4 (Wheel Down)
    SWITCH_MOUSE_DISCONNECT
};

int mouse_remap[MOUSE_BUTTONS] =
{
    SWITCH_BUTTON1,
    SWITCH_BUTTON2,
    SWITCH_BUTTON3
};

////////////

static void defaultConfig(string config, bool gamepad)
{
    FILE *pf = fopen(config.c_str(), "r");

    if (pf) {
        fclose(pf);
        return;
    }

    pf = fopen(config.c_str(), "w");

    if (!pf) {
        LOGE << fmt("Unable to create a default keymap file: %s", config.c_str());
        return;
    }

    fputs(gamepad ? k_defaultGamePad : k_defaultJoystick, pf);
    LOGI << fmt("Wrote a default keymap file to: %s", config.c_str());
    fclose(pf);
}

static bool mouseButtonMap(SDL_Event *event, bool enable)
{
    const int which = controller_map[event->cdevice.which];
    const int button = event->cbutton.button;

    for (int j = 0; j < MAX_CONTROLLERCONFIG; j++) {
        for (int i = SWITCH_BUTTON1; i < SWITCH_COIN1; ++i) {
            if (button == joystick_buttons_map[controller_map[j]][i][1]-1) {

                (enable ? input_enable : input_disable)(i, which + g_gamepad_wad);

                if (g_haptic[0] && g_gamepad_haptic[which])
                    SDL_GameControllerRumble(SDL_GameControllerFromInstanceID(event->cdevice.which),
                        g_haptic[0], g_haptic[0], g_haptic[1]);
                return true;
            }
        }
    }
    return false;
}

void set_trigger_threshold(double adj)
{
    JOY_AXIS_TRIG = static_cast<int>(MAX_AXIS * (adj / 100.0));
}

static float absLevel(Sint16 limit)
{
    return (limit - MIN_AXIS) / float(MAX_AXIS - MIN_AXIS);
}

static bool parse_bool(const char* val) {
    return val && strcasecmp(val, "TRUE") == 0;
}

static void CFG_System()
{
    struct mpo_io *io;
    string cur_line = "";
    string key_name = "", sval1 = "",  eq_sign = "";

    if (m_altInputFileSet && !g_index_reset) {
        string keyinput_notice = "Loading alternate keymap file: ";
        keyinput_notice += g_inputini_file.c_str();
        LOGI << keyinput_notice.c_str();
    }

    string strHypInput = g_homedir.find_file(g_inputini_file.c_str(), true);

    if (!m_altInputFileSet)
        defaultConfig(strHypInput, g_use_gamepad);

    io = mpo_open(strHypInput.c_str(), MPO_OPEN_READONLY);

    if (!io) {
        LOGW << fmt("%s inaccessible, using defaults", g_inputini_file.c_str());
        m_config_valid = false;
        return;
    }

    LOGD << "Configuring system ...";

    auto system_config = [&](const std::string& key, const char* val) {
        if (strcasecmp(key.c_str(), "SCREEN") == 0) {
            video::set_display_screen((uint8_t)atoi(val));
        } else if (strcasecmp(key.c_str(), "VLDPVOLUME") == 0) {
            sound::set_chip_vldp_volume((unsigned int)atoi(val));
        } else if (strcasecmp(key.c_str(), "NONVLDPVOLUME") == 0) {
            sound::set_chip_nonvldp_volume((unsigned int)atoi(val));
        } else if (strcasecmp(key.c_str(), "SERVERSEND") == 0) {
            net_server_send(parse_bool(val));
        } else if (strcasecmp(key.c_str(), "SOUNDBUFFER") == 0) {
            sound::set_buf_size((Uint16)atoi(val));
        } else if (strcasecmp(key.c_str(), "ALWAYSONTOP") == 0) {
            video::set_forcetop(parse_bool(val));
        } else if (strcasecmp(key.c_str(), "FULLSCREEN") == 0) {
            video::set_fullscreen(parse_bool(val));
        } else if (strcasecmp(key.c_str(), "MANYMOUSE") == 0) {
            g_game->set_manymouse(parse_bool(val));
        } else if (strcasecmp(key.c_str(), "GAMEPAD") == 0) {
            g_use_gamepad = parse_bool(val);
        } else if (strcasecmp(key.c_str(), "SCANLINES") == 0) {
            video::set_scanlines(parse_bool(val));
        } else if (strcasecmp(key.c_str(), "SPLASH") == 0) {
            video::set_intro(parse_bool(val));
        } else if (strcasecmp(key.c_str(), "GRAPHICAPI") == 0)
	{
            bool opengl = (strcasecmp(val, "OPENGL") == 0);
            bool vulkan = (strcasecmp(val, "VULKAN") == 0);

            if (opengl || vulkan) {
                video::set_opengl(opengl);
                video::set_vulkan(vulkan);
            }
        } else {
            LOGW << "Unknown [GLOBAL] config: " << key.c_str();
        }
    };

    while (strcasecmp(cur_line.c_str(), "[GLOBAL]") != 0) {
        read_line(io, cur_line);
        if (io->eof) {
            LOGD << fmt("[GLOBAL] section missing in %s", g_inputini_file.c_str());
            goto cleanup;
        }
    }

    while (!io->eof) {

        if (read_line(io, cur_line) <= 0)
            continue;

        if (!find_word(cur_line.c_str(), key_name, cur_line))
            continue;

        if (!key_name.empty() && key_name[0] == '#')
            continue;

        if (strcasecmp(key_name.c_str(), "END") == 0)
            break;

        if (!(find_word(cur_line.c_str(), eq_sign, cur_line) && eq_sign == "="))
            continue;

        if (!find_word(cur_line.c_str(), sval1, cur_line))
            continue;

        system_config(key_name, sval1.c_str());
    }

cleanup:
    mpo_close(io);
}

static void CFG_Keys()
{
    if (!m_config_valid) return;

    struct mpo_io *io;
    string cur_line = "";
    string key_name = "", sval1 = "", sval2 = "", sval3 = "", sval4 = "",
                          sval5 = "", sval6 = "", eq_sign = "";
    int val1 = 0, val2 = 0, val3 = 0, val4 = 0, val5 = 0, val6 = 0;
    bool warn = true;

    string strHypInput = g_homedir.find_file(g_inputini_file.c_str(), true);

    io = mpo_open(strHypInput.c_str(), MPO_OPEN_READONLY);

    if (!io) {
        LOGE << fmt("Something weird with %s, we shouldn't get here...",
                    g_inputini_file.c_str());
        return;
    }

    LOGD << "Remapping input ...";

    cur_line = "";

    while (strcasecmp(cur_line.c_str(), "[KEYBOARD]") != 0) {
        read_line(io, cur_line);
        if (io->eof) {
            LOGW << "Didn't find [KEYBOARD] header, aborting";
            mpo_close(io);
            return;
        }
    }

    while (!io->eof) {

        if (read_line(io, cur_line) <= 0)
            continue;

        bool corrupt_file = true;

        if (find_word(cur_line.c_str(), key_name, cur_line)) {

            if (strcasecmp(key_name.c_str(), "END") == 0)
                break;

            if (find_word(cur_line.c_str(), eq_sign, cur_line) && eq_sign == "=") {

                if (find_word(cur_line.c_str(), sval1, cur_line) &&
                    find_word(cur_line.c_str(), sval2, cur_line) &&
                    find_word(cur_line.c_str(), sval3, cur_line)) {

                    val1 = isdigit(sval1[0]) ? atoi(sval1.c_str()) : sdl2_keycode(sval1.c_str());
                    val2 = isdigit(sval2[0]) ? atoi(sval2.c_str()) : sdl2_keycode(sval2.c_str());

                    if (g_use_gamepad)
                        val3 = isdigit(sval3[0]) ? atoi(sval3.c_str()) :
                               sdl2_controller_button(sval3.c_str());
                    else {

                        if (!isdigit(sval3[0]) && warn)
                        {
                            LOGW << "Found a button config string: " << sval3.c_str();
                            LOGW << "Did you intend to use the '-gamepad' argument?";
                            warn = false;
                        }

                        val3 = atoi(sval3.c_str());

                        if (strcasecmp(key_name.c_str(), g_key_names[0]) == 0)
                        {
                            if (!g_open_hat && g_use_joystick && SDL_NumJoysticks() > 0)
                            {
                                int divider = (sval3.length() == 4) ? 1000 : 100;
                                g_assigned_hat = (val3 / divider);
                                LOGI << fmt("Joystick HAT enabled on stick: [%d]", g_assigned_hat);
                            }
                        }
                    }

                    val4 = 0;
                    if (find_word(cur_line.c_str(), sval4, cur_line)) {
                        if (g_use_gamepad)
                            val4 = (isdigit(sval4[0])) ? atoi(sval4.c_str()) :
                                    sdl2_controller_axis(sval4.c_str());
                        else
                            val4 = atoi(sval4.c_str());
                    }

                    if (g_use_gamepad) {
                        val5 = val6 = 0;
                        if (find_word(cur_line.c_str(), sval5, cur_line)) {
                            val5 = (isdigit(sval5[0])) ? atoi(sval5.c_str()) :
                                    sdl2_controller_button(sval5.c_str());
                            if (find_word(cur_line.c_str(), sval6, cur_line))
                                val6 = (isdigit(sval6[0])) ? atoi(sval6.c_str()) :
                                        sdl2_controller_axis(sval6.c_str());
                        }
                    }

                    corrupt_file = false;

                    for (int i = 0; i < SWITCH_COUNT; i++) {
                        if (strcasecmp(key_name.c_str(), g_key_names[i]) == 0) {

                            g_key_defs[i][0] = val1;
                            g_key_defs[i][1] = val2;
                            int id;

                            if (val3 > 0)
                            {
                                id = 0;

                                if (val3 > AXIS_TRIGGER) {
                                    joystick_buttons_map[id][i][0] = (val3 / AXIS_TRIGGER);
                                    joystick_buttons_map[id][i][1] = val3;
                                } else {
                                    int divider = (sval3.length() == 4) ? 1000 : 100;
                                    joystick_buttons_map[id][i][0] = (val3 / divider);
                                    joystick_buttons_map[id][i][1] = (val3 % divider);
                                }
                            }

                            if (val4 != 0)
                            {
                                id = 0;
                                int divider = (sval4.length() > 4) ? 1000 : 100;

                                joystick_axis_map[id][i][0] = abs(val4 / divider);
                                joystick_axis_map[id][i][1] = abs(val4 % divider);
                                joystick_axis_map[id][i][2] = (val4 == 0) ? 0 : ((val4 < 0) ? -1 : 1);
                            }

                            if (g_use_gamepad && val5 != 0)
                            {
                                id = 1;
                                int adj = (val5 > AXIS_TRIGGER)
                                    ? (val5 / AXIS_TRIGGER)
                                    : val5;

                                joystick_buttons_map[id][i][0] = adj;
                                joystick_buttons_map[id][i][1] = val5;

                                if (val6 != 0) {
                                    joystick_axis_map[id][i][0] = abs(val6);
                                    joystick_axis_map[id][i][1] = abs(val6);
                                    joystick_axis_map[id][i][2] = (val6 == 0) ? 0 : ((val6 < 0) ? -1 : 1);
                                }
                            }

                            break;
                        }
                    }
                }
            }
        }

        if (!corrupt_file) continue;

        LOGW << "Corrupt config file, aborting";
        mpo_close(io);
        return;
    }

    LOGD << "Remapping mouse ...";

    cur_line = "";
    mpo_rewind(io);
    int valid_remap[MOUSE_BUTTONS] = {0};
    bool remapped[MOUSE_BUTTONS] = {false};

    auto mouse_map = [&](const string &name) -> int {
        for (int i = 0; i < SWITCH_COUNT; i++) {
            if (strcasecmp(name.c_str(), g_key_names[i]) == 0)
                return i;
        }
        LOGW << fmt("Invalid [MOUSE] config parameter: %s", name.c_str());
        return -1;
    };

    auto mouse_button = [&](const std::string& key, int val) {
        if (strcasecmp(key.c_str(), "MOUSE_BUTTON1") == 0) {
            valid_remap[2] = val; remapped[2] = true;
        } else if (strcasecmp(key.c_str(), "MOUSE_BUTTON3") == 0) {
            valid_remap[1] = val; remapped[1] = true;
        } else if (strcasecmp(key.c_str(), "MOUSE_BUTTON2") == 0) {
            valid_remap[0] = val; remapped[0] = true;
        } else {
            LOGW << "Unknown [MOUSE] config: " << key.c_str();
        }
    };

    while (strcasecmp(cur_line.c_str(), "[MOUSE]") != 0) {
        read_line(io, cur_line);
        if (io->eof) {
            LOGW << fmt("[MOUSE] section missing in %s", g_inputini_file.c_str());
            goto cleanup;
        }
    }

    while (!io->eof) {

        if (read_line(io, cur_line) <= 0)
            continue;

        if (!find_word(cur_line.c_str(), key_name, cur_line))
            continue;

        if (strcasecmp(key_name.c_str(), "END") == 0)
            break;

        if (!(find_word(cur_line.c_str(), eq_sign, cur_line) && eq_sign == "="))
            continue;

        if (!find_word(cur_line.c_str(), sval1, cur_line))
            continue;

        mouse_button(key_name, mouse_map(sval1));
    }

    for (int i = 0; i < MOUSE_BUTTONS; i++) {
        if (!remapped[i] || valid_remap[i] == -1) {
            LOGW << "[MOUSE] config is invalid, using defaults";
            goto cleanup;
        }
    }

    for (int i = 0; i < MOUSE_BUTTONS; ++i) {
        for (int j = i + 1; j < MOUSE_BUTTONS; ++j) {
            if (valid_remap[i] == valid_remap[j]) {
                LOGW << "Invalid [MOUSE] config: Duplicate values";
                goto cleanup;
            }
        }
    }

    for (int i = 0; i < MOUSE_BUTTONS; ++i)
        mouse_remap[i] = valid_remap[i];

cleanup:
    mpo_close(io);
}

static void reOrderIndex()
{
    int lookup[MAX_GAMECONTROLLER];

    for (int j = 0; j < MAX_GAMECONTROLLER; ++j)
    {
        lookup[g_padindex[j]] = j;
    }

    for (int i = 0; i < MAX_GAMECONTROLLER; ++i)
    {
        controller_map[i] = lookup[controller_map[i]];
    }

    LOGW << fmt("Gamepad index re-ordering requested: %s", g_inputini_file.c_str());
}

void absolute_only()
{
    mm_absolute_only = 1;
    printline("Filtering absolute devices in ManyMouse [evdev].");
}

static void SDL_gamepad_init()
{
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
         if (SDL_IsGameController(i)) {
             g_gamepad_id[g_gamepad_attached] = SDL_GameControllerOpen(i);
             SDL_Joystick* joy = SDL_GameControllerGetJoystick(g_gamepad_id[g_gamepad_attached]);
             if (joy != NULL) {
                 SDL_JoystickID id = SDL_JoystickInstanceID(joy);
                 LOGI << "Gamepad #" << i << "|[" << id << "]" << ": "
			 << SDL_GameControllerName(g_gamepad_id[g_gamepad_attached]) << " connected";

                 if (enabled_haptic)
                 {
                     g_gamepad_haptic[g_gamepad_attached] = SDL_HapticOpenFromJoystick(joy);

                     if (g_gamepad_haptic[g_gamepad_attached] != NULL) {
                         if (SDL_HapticRumbleSupported(g_gamepad_haptic[g_gamepad_attached])) {
                             LOGI << "Gamepad #" << i << "|[" << id << "]"
                                     << ": Haptic Rumble support";
                         } else {
                             SDL_HapticClose(g_gamepad_haptic[g_gamepad_attached]);
                             g_gamepad_haptic[g_gamepad_attached] = NULL;
                         }
                     }
                 }

                 g_gamepad_attached++;

                 if (g_gamepad_attached > (MAX_GAMECONTROLLER - 1)) {
                     LOGW << "Max Game Controller limit [" << MAX_GAMECONTROLLER << "] reached";
                     break;
                }
            }
        }
    }
    SDL_Event event{};
    SDL_JoystickEventState(SDL_ENABLE);
    SDL_GameControllerEventState(SDL_ENABLE);

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_CONTROLLERDEVICEADDED) {
            SDL_FlushEvent(SDL_CONTROLLERDEVICEADDED);
        }
    }

    if (g_index_reset) reOrderIndex();

    CFG_Keys();
}

static void FilterMouseEvents(bool bFilteredOut)
{
    int iState = SDL_ENABLE;

    if (bFilteredOut) {
        iState = SDL_IGNORE;
    }

    SDL_EventState(SDL_MOUSEMOTION, iState);
    SDL_EventState(SDL_MOUSEBUTTONDOWN, iState);
    SDL_EventState(SDL_MOUSEBUTTONUP, iState);
}

static void manymouse_init_mice(void)
{
    LOGI << "Using ManyMouse for mice input.";

    g_available_mice = ManyMouse_Init(mm_absolute_only);

    static Mouse mice[MAX_MICE];

    g_available_mice = std::max(0, std::min(g_available_mice, MAX_MICE));

    g_game->set_mice_detected(g_available_mice);

    if (g_available_mice == 0) {
        LOGW << "No mice detected!";
        if (g_use_gamepad) g_game->set_mice_detected(g_gamepad_attached);
        return;
    }
    else
    {
        LOGI << fmt("Driver: %s", ManyMouse_DriverName());

        LOGI << fmt("Found %d mouse device%s.",
            g_available_mice, g_available_mice == 1 ? "" : "s");

        for (int i = 0; i < g_available_mice; i++)
        {
            const char *name = ManyMouse_DeviceName(i);
            strncpy(mice[i].name, name, sizeof (mice[i].name));
            mice[i].name[sizeof (mice[i].name) - 1] = '\0';
            mice[i].connected = 1;
            LOGI << fmt("Mouse #%d: %s", i, mice[i].name);
        }
    }

    if (g_use_gamepad) {
        LOGI << "Manage overlap in Mouse and Gamepad #id allocations in device selection.";
        if (g_gamepad_attached > g_available_mice)
            g_game->set_mice_detected(g_gamepad_attached);
    }
}

static bool set_mouse_mode(int thisMode)
{
    bool result = false;

    if (g_game->get_mouse_enabled())
    {
        if (g_mouse_mode == MANY_MOUSE) ManyMouse_Quit();

        memset(mouse_buttons_map, 0, sizeof(mouse_buttons_map));

        if (thisMode == SDL_MOUSE) {

            mouse_buttons_map[0] = mouse_remap[0];           // 0 (Left Button)
            mouse_buttons_map[1] = mouse_remap[2];           // 1 (Middle Button)
            mouse_buttons_map[2] = mouse_remap[1];           // 2 (Right Button)
            mouse_buttons_map[3] = mouse_remap[0];           // 3 (Wheel Up)
            mouse_buttons_map[4] = mouse_remap[1];           // 4 (Wheel Down)
            mouse_buttons_map[5] = SWITCH_MOUSE_DISCONNECT;
            result = true;
        }
        else if (thisMode == MANY_MOUSE)
        {
            mouse_buttons_map[0] = mouse_remap[2];           // 0 (Left Button)
            mouse_buttons_map[1] = mouse_remap[0];           // 1 (Middle Button)
            mouse_buttons_map[2] = mouse_remap[1];           // 2 (Right Button)
            mouse_buttons_map[3] = SWITCH_MOUSE_SCROLL_UP;   // 3 (Wheel Up)
            mouse_buttons_map[4] = SWITCH_MOUSE_SCROLL_DOWN; // 4 (Wheel Down)
            mouse_buttons_map[5] = SWITCH_MOUSE_DISCONNECT;

            manymouse_init_mice();
            result = true;
        }
    }
    return result;
}

static void manymouse_update_mice()
{
    static Mouse mice[MAX_MICE];

    static const int max_width  = video::get_video_width();
    static const int max_height = video::get_video_height();

    while (ManyMouse_PollEvent(&mm_event))
    {
        if (mm_event.device >= (unsigned int)g_available_mice)
            continue;

        Mouse &mouse = mice[mm_event.device];

        switch(mm_event.type) {
        case MANYMOUSE_EVENT_RELMOTION:
        {
            if (mm_event.item > 1) break;

            if (mm_event.item == 0) {
                mouse.x += mm_event.value;
                mouse.relx = mm_event.value;

                if (mouse.x < 0) mouse.x = 0;
                else if (mouse.x >= max_width) mouse.x = max_width;
            }
            else {
                mouse.y += mm_event.value;
                mouse.rely = mm_event.value;

                if (mouse.y < 0) mouse.y = 0;
                else if (mouse.y >= max_height) mouse.y = max_height;
            }
            g_game->OnMouseMotion(mouse.x, mouse.y, mouse.relx, mouse.rely, mm_event.device);
            break;
        }
        case MANYMOUSE_EVENT_ABSMOTION:
        {
            if (mm_event.item > 1) break;

            const int idx = mm_event.item;
            float val, range;
#ifdef WIN32
            val   = float(idx == 0 ? mm_event.minval : mm_event.maxval);
            range = 65535.0f;
#else
            val   = float(mm_event.value - mm_event.minval);
            range = float(mm_event.maxval - mm_event.minval);
#endif
            int loc = int((val / range) * ((idx == 0) ? max_width : max_height));

            (idx == 0 ? mouse.x : mouse.y) = loc;

            g_game->OnMouseMotion(mouse.x, mouse.y, 0, 0, mm_event.device);
            break;
        }
        case MANYMOUSE_EVENT_BUTTON:
            if (mm_event.item < MAX_MICE)
            {
                if (mm_event.value > 1) break;

                if (mm_event.value == 1)
                {
                    input_enable((Uint8)mouse_buttons_map[mm_event.item], mm_event.device);
                    mouse.buttons |= (1 << mm_event.item);
                }
                else
                {
                    input_disable((Uint8)mouse_buttons_map[mm_event.item], mm_event.device);
                    mouse.buttons &= ~(1 << mm_event.item);
                }
            }
            break;
        case MANYMOUSE_EVENT_SCROLL:
            if (mm_event.item == 0)
            {
                int s = (mm_event.value > 0 ? SWITCH_MOUSE_SCROLL_UP : SWITCH_MOUSE_SCROLL_DOWN);
                input_disable(s, mm_event.device);
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
{
    int result = 0;

    // Set ini based argument list (Note: will override cli arguments)
    CFG_System();

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

        // if joystick usage is enabled
        if (g_use_joystick) {
            // open joysticks
            for (int i = 0; i < SDL_NumJoysticks(); i++) {
                SDL_Joystick* joystick = SDL_JoystickOpen(i);
                if (joystick != NULL) {
                    LOGI << "Joystick #" << i << " was successfully opened";
                } else {
                    LOGW << "Error opening joystick #" << i << "!";
                }
            }
            if (SDL_NumJoysticks() == 0) {
                LOGI << "No joysticks detected";
            }
        }
        // notify user that their attempt to disable the joystick is successful
        else
            LOGI << "Joystick usage disabled";

        CFG_Keys(); // NOTE : for some freak reason, this should not be done
                    // BEFORE the joystick is initialized, I don't know why!
        result = 1;
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

         if (thisGame == GAME_UNDEFINED)
             thisGame = g_game->get_game_type();

         if (g_game->get_manymouse() && thisGame != GAME_THAYERS)
             g_mouse_mode = MANY_MOUSE;

         if (thisGame == GAME_THAYERS) RELFORMAT = 0;

         if (!set_mouse_mode(g_mouse_mode)) {
             LOGE << "Mouse initialization failed.";
             set_quitflag();
         }
    }

    return (result);
}

SDL_GameController* get_gamepad_id(int i)
{
    if (g_gamepad_id[(uint8_t)i])
        return g_gamepad_id[(uint8_t)i];

    return nullptr;
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
    }
}

// checks to see if there is incoming input, and acts on it
void SDL_check_input()
{
    SDL_Event event{};

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

                thayers* l_thayers = dynamic_cast<thayers*>(g_game);
                // cast game class to a thayers class so we can call a
                // thayers-specific function

                // make sure cast succeeded
                if (l_thayers) l_thayers->process_keydown(keyPressed);
// else cast failed, and we would crash if we tried to call process_keydown
// cast would fail if g_game is not a thayers class

#ifdef BUILD_SINGE
            } else {

                if (thisGame == GAME_SINGE) {
                    singe* l_singe = dynamic_cast<singe*>(g_game);
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

                thayers* l_thayers = dynamic_cast<thayers*>(g_game);
                // cast game class to a thayers class so we can call a
                // thayers-specific function

                // make sure cast succeeded
                if (l_thayers) l_thayers->process_keyup(keyPressed);

// else cast failed, and we would crash if we tried to call process_keydown
// cast would fail if g_game is not a thayers class
#ifdef BUILD_SINGE
            } else {

                if (thisGame == GAME_SINGE) {
                    singe* l_singe = dynamic_cast<singe*>(g_game);
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
                        SDL_JoystickID newid = SDL_JoystickInstanceID(joy);
                        LOGI << "Gamepad #" << i << "|[" << newid << "]" << ": "
                            << SDL_GameControllerName(g_gamepad_id[i]) << " connected";
                        if (enabled_haptic && !g_gamepad_haptic[i]) {
                            g_gamepad_haptic[i] = SDL_HapticOpenFromJoystick(joy);

                            if (g_gamepad_haptic[i] != NULL) {
                                if (SDL_HapticRumbleSupported(g_gamepad_haptic[i])) {
                                    LOGI << "Gamepad #" << i << "|[" << newid << "]"
                                            <<  ": Haptic Rumble support";
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
    case SDL_CONTROLLERBUTTONDOWN:
        reset_idle(); // added by JFA for -idleexit
        if (g_mouse_mode == MANY_MOUSE)
            if (mouseButtonMap(event, true))
                break;
        // loop through map and find corresponding action
        for (i = 0; i < SWITCH_COUNT; i++) {
            const int which = controller_map[event->cdevice.which];
            if (event->cbutton.button == joystick_buttons_map[which][i][1]-1) {
                if (i == SWITCH_COIN1) g_hotkey = true;
                input_enable(i, (g_mouse_mode == MANY_MOUSE) ? which + g_gamepad_wad : NOMOUSE);
                if (g_haptic[0] && g_gamepad_haptic[which])
                    SDL_GameControllerRumble(SDL_GameControllerFromInstanceID(event->cdevice.which),
                        g_haptic[0], g_haptic[0], g_haptic[1]);
                break;
            }
        }
        break;
    case SDL_CONTROLLERBUTTONUP:
        reset_idle(); // added by JFA for -idleexit
        g_hotkey = false;
        if (g_mouse_mode == MANY_MOUSE)
            if (mouseButtonMap(event, false))
                break;
        // loop through map and find corresponding action
        for (i = 0; i < SWITCH_COUNT; i++) {
            const int which = controller_map[event->cdevice.which];
            if (event->cbutton.button == joystick_buttons_map[which][i][1]-1) {
                input_disable(i, (g_mouse_mode == MANY_MOUSE) ? which + g_gamepad_wad : NOMOUSE);
                break;
            }
        }
        break;
    case SDL_JOYAXISMOTION:
        if (g_use_gamepad) break;
        process_joystick_motion(event);
        break;
    case SDL_JOYHATMOTION:
        if (g_use_gamepad) break;
        // only process events for the first hat on device
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
                if (i == SWITCH_COIN1) g_hotkey = true;
                input_enable(i, NOMOUSE);
                break;
            }
        }
        break;
    case SDL_JOYBUTTONUP:
        if (g_use_gamepad) break;
        reset_idle(); // added by JFA for -idleexit
        g_hotkey = false;

        // loop through map and find corresponding action
        for (i = 0; i < SWITCH_COUNT; i++) {
            if (event->jbutton.which == joystick_buttons_map[0][i][0]
                           && event->jbutton.button == joystick_buttons_map[0][i][1]-1) {
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
               switch (RELFORMAT) {
               case 0:
                   g_game->OnMouseMotion(event->motion.x, event->motion.y, event->motion.xrel,
                       event->motion.yrel, NOMOUSE);
                   break;
               case 1:
                   static int vX = 0, vY = 0;
                   static bool relative = false;

                   static const int max_width = video::get_video_width();
                   static const int max_height = video::get_video_height();

                   if (!relative) {
                       if (SDL_SetRelativeMouseMode(SDL_TRUE) == 0) {
                           LOGI << "Relative mouse mode enabled, mouse is now captured.";
                       }
                       relative = true;
                   }

                   vX += event->motion.xrel;
                   vY += event->motion.yrel;

                   if (vX < 0) vX = 0;
                   else if (vX > max_width) vX = max_width;

                   if (vY < 0) vY = 0;
                   else if (vY > max_height) vY = max_height;

                   g_game->OnMouseMotion(vX, vY, event->motion.xrel, event->motion.yrel, NOMOUSE);
                   break;
               }
               break;
           case SDL_MOUSEWHEEL:
               int s = (event->wheel.y > 0 ? SWITCH_MOUSE_SCROLL_UP : SWITCH_MOUSE_SCROLL_DOWN);
               g_game->input_disable(s, NOMOUSE);
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
    const int axis = event->caxis.axis;
    const int value = event->caxis.value;
    const int which = controller_map[event->cdevice.which];
    g_game->ControllerAxisProxy(axis, value, which);

    // Deal with AXIS TRIGGERS
    if (g_mouse_mode == MANY_MOUSE) {

        // Process the LEFT AXIS as absolute co-ordinates
        static Sint16 x_axis[MAX_GAMECONTROLLER] = { 0 };
        static Sint16 y_axis[MAX_GAMECONTROLLER] = { 0 };
        static int prev_x[MAX_GAMECONTROLLER] = { 0 };
        static int prev_y[MAX_GAMECONTROLLER] = { 0 };

        static const int width = video::get_video_width();
        static const int height = video::get_video_height();

        if (axis == SDL_CONTROLLER_AXIS_LEFTX) {
            x_axis[which] = value;
        } else if (axis == SDL_CONTROLLER_AXIS_LEFTY) {
            y_axis[which] = value;
        }

        int x = (int)(absLevel(x_axis[which]) * width);
        int y = (int)(absLevel(y_axis[which]) * height);

        int relx = x - prev_x[which];
        int rely = y - prev_y[which];

        if (relx | rely) {
            g_game->OnMouseMotion(x, y, relx, rely, which + g_gamepad_wad);

            prev_x[which] = x;
            prev_y[which] = y;
        }

        for (int j = 0; j < MAX_CONTROLLERCONFIG; j++) {
            for (int i = 0; i < SWITCH_COUNT; i++) {
                if (axis == joystick_buttons_map[controller_map[j]][i][1]-AXIS_TRIGGER) {

                    if ((abs(value) > JOY_AXIS_TRIG) &&
                        !controller_trigger_pressed[controller_map[j]][axis]) {
                        input_enable(i, which + g_gamepad_wad);
                        if (g_haptic[0] && g_gamepad_haptic[which])
                            SDL_GameControllerRumble(SDL_GameControllerFromInstanceID(event->cdevice.which),
                                g_haptic[0], g_haptic[0], g_haptic[1]);
                        controller_trigger_pressed[controller_map[j]][axis] = true;
                    } else if (controller_trigger_pressed[controller_map[j]][axis]) {
                        input_disable(i, which + g_gamepad_wad);
                        controller_trigger_pressed[controller_map[j]][axis] = false;
                    }
                    return;
                }
            }
        }

    } else {
        for (int i = 0; i < SWITCH_COUNT; i++) {
            if (axis == joystick_buttons_map[which][i][1]-AXIS_TRIGGER) {

                if ((abs(value) > JOY_AXIS_TRIG)
                       && !controller_trigger_pressed[which][axis]) {
                    input_enable(i, NOMOUSE);
                    if (g_haptic[0] && g_gamepad_haptic[which])
                        SDL_GameControllerRumble(SDL_GameControllerFromInstanceID(event->cdevice.which),
                            g_haptic[0], g_haptic[0], g_haptic[1]);
                    controller_trigger_pressed[which][axis] = true;
                } else if (controller_trigger_pressed[which][axis]) {
                    input_disable(i, NOMOUSE);
                    controller_trigger_pressed[which][axis] = false;
                }
                return;
            }
        }
    }

    int key = -1;

    for (int i = 0; i < SWITCH_START1; i++) {
        if (axis == joystick_axis_map[which][i][1]-1 &&
            ((value < 0) ? -1 : 1) == joystick_axis_map[which][i][2]) {
            key = i;
            break;
        }
    }

    if (key == -1) return;

    static bool x_axis_in_use[MAX_GAMECONTROLLER] = { false };
    static bool y_axis_in_use[MAX_GAMECONTROLLER] = { false };

    if (abs(value) > JOY_AXIS_MID) {
        input_enable(key, NOMOUSE);
        if (key == SWITCH_UP || key == SWITCH_DOWN)
            y_axis_in_use[which] = true;
        else
            x_axis_in_use[which] = true;
    }
    else {
        if ((key == SWITCH_UP || key == SWITCH_DOWN) &&
                y_axis_in_use[which]) {
            input_disable(SWITCH_UP, NOMOUSE);
            input_disable(SWITCH_DOWN, NOMOUSE);
            y_axis_in_use[which] = false;

        } else if ((key == SWITCH_LEFT || key == SWITCH_RIGHT) &&
                x_axis_in_use[which]) {
            input_disable(SWITCH_LEFT, NOMOUSE);
            input_disable(SWITCH_RIGHT, NOMOUSE);
            x_axis_in_use[which] = false;
        }
    }
}

// processes movements of the joystick
void process_joystick_motion(SDL_Event *event)
{
    static bool x_axis_in_use = false; // true if joystick is left or right
    static bool y_axis_in_use = false; // true if joystick is up or down

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
            y_axis_in_use = true;
        else
            x_axis_in_use = true;
    }
    else {
        if ((key == SWITCH_UP || key == SWITCH_DOWN) && y_axis_in_use) {
            input_disable(SWITCH_UP, NOMOUSE);
            input_disable(SWITCH_DOWN, NOMOUSE);
            y_axis_in_use = false;

        } else if ((key == SWITCH_LEFT || key == SWITCH_RIGHT) && x_axis_in_use) {
            input_disable(SWITCH_LEFT, NOMOUSE);
            input_disable(SWITCH_RIGHT, NOMOUSE);
            x_axis_in_use = false;
        }
    }
}

// processes movement of the joystick hat
void process_joystick_hat_motion(SDL_Event *event)
{
    if (!g_open_hat && (event->jaxis.which != g_assigned_hat)) return;

    static Uint8 prev_hat_position = SDL_HAT_CENTERED;
    Uint8 hat_movement = event->jhat.value ^ prev_hat_position;
    int hat;

    switch (hat_movement)
    {
        case SDL_HAT_UP:
            hat = g_invert_hat ? SWITCH_DOWN : SWITCH_UP;
            break;
        case SDL_HAT_DOWN:
            hat = g_invert_hat ? SWITCH_UP : SWITCH_DOWN;
            break;
        case SDL_HAT_LEFT:
            hat = SWITCH_LEFT;
            break;
        case SDL_HAT_RIGHT:
            hat = SWITCH_RIGHT;
            break;
        default:
            return;
    }

    if (event->jhat.value & hat_movement) {
        input_enable(hat, NOMOUSE);
        prev_hat_position |= hat_movement;
    }
    else
    {
        input_disable(hat, NOMOUSE);
        prev_hat_position &= ~hat_movement;
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
                g_game->input_enable(move, mouseID);
            g_game->toggle_game_pause();
            break;
        case SWITCH_QUIT:
            set_quitflag();
            break;
        case SWITCH_START1:
            if (g_hotkey)
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
            if (cpu::get_hz(0) > 0)
                add_coin_to_queue(true, move);
            break;
        case SWITCH_CONSOLE:
            break;
    }
}

// if user has released a key/released a button/moved joystick back to center
// position
void input_disable(Uint8 move, Sint8 mouseID)
{
    if (move == SWITCH_PAUSE && thisGame == GAME_SINGE)
        g_game->input_disable(move, mouseID);

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

int get_realmouse_attached() {

    return g_available_mice;
}

void set_mouse_raw(bool format) {

    RELFORMAT = format ? 0 : 1;
}

// Use a gamepad?
void set_use_gamepad(bool value) {
    g_use_gamepad = value;
    g_use_joystick = !value;

    if (!m_altInputFileSet && value)
        g_inputini_file = "hypinput_gamepad.ini";
}

void set_gamepad_order(int *c, int max) {

    for (int i = 0; i < max; i++) {
        g_padindex[i] = (uint8_t)(c[i] - 1);
    }

    g_index_reset = true;
}

void set_gamepad_wad(bool enable) {

    g_gamepad_wad = enable ? 100 : 0;
}

int get_gamepad_wad() {

    return g_gamepad_wad;
}

int get_gamepad_attached() {

    return g_gamepad_attached;
}

void disable_haptics() { enabled_haptic = false; }

void set_haptic(Uint8 value) {
    g_haptic[0] = (1 << (value + 0xc)) - 1;
    g_haptic[1] = 0x96;
}

void do_gamepad_rumble(Uint8 str, Uint8 len, Uint8 id)
{
    if (g_gamepad_id[id] && g_gamepad_haptic[id]) {
        Uint16 s = (Uint16)((65535u * str) / 4);
        SDL_GameControllerRumble(g_gamepad_id[id], s, s, (0x4b << len));
    }
}
