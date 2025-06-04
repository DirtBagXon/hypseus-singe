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

// Parses our command line and sets our variables accordingly
// by Matt Ownby

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cmdline.h"
#include "conout.h"
#include "network.h"
#include "numstr.h"
#include "homedir.h"
#include "input.h" // to disable joystick use
#include "../io/numstr.h"
#include "../video/video.h"
#include "../video/led.h"
#include "../hypseus.h"
#include "../cpu/cpu-debug.h" // for set_cpu_trace
#include "../game/lair.h"
#include "../game/cliff.h"
#include "../game/game.h"
#include "../game/superd.h"
#include "../game/thayers.h"
#include "../game/speedtest.h"
#include "../game/seektest.h"
#include "../game/releasetest.h"
#include "../game/cputest.h"
#include "../game/multicputest.h"
#include "../game/firefox.h"
#include "../game/ffr.h"
#include "../game/astron.h"
#include "../game/esh.h"
#include "../game/laireuro.h"
#include "../game/badlands.h"
#include "../game/starrider.h"
#include "../game/bega.h"
#include "../game/cobraconv.h"
#include "../game/gpworld.h"
#include "../game/interstellar.h"
#include "../game/benchmark.h"
#include "../game/lair2.h"
#include "../game/mach3.h"
#include "../game/lgp.h"
#include "../game/timetrav.h"
#ifdef BUILD_SINGE
#include "../game/singe.h"
#else
#include "error.h"
#endif // BUILD_SINGE
#include "../game/test_sb.h"
#include "../ldp-out/ldp.h"
#include "../ldp-out/ldp-vldp.h"
#include "../ldp-out/framemod.h"

#ifdef UNIX
#include <unistd.h> // for unlink
#endif

int g_argc      = 0;
char **g_argv   = NULL;
int g_arg_index = 0;
bool g_altmouse = true;

// Win32 doesn't use strcasecmp, it uses stricmp (lame)
#ifdef WIN32
#define strcasecmp stricmp
#endif

bool invalid_arg(const char *s)
{
    char e[432];
    snprintf(e, sizeof(e), "Invalid argument for game type: %s", s);
    printerror(e);
    return false;
}

// parses the command line looking for the -homedir switch, returns true if
// found and valid (or not found)
// (this must be done first because the the game and ldp classes may rely on the
// homedir being set to its proper place)
bool parse_homedir()
{
    bool result      = true;
    char s[81]       = {0};
    char e[128];
    bool bHomeDirSet = false; // whether set_homedir was called

    for (;;) {
        get_next_word(s, sizeof(s));
        // if there is nothing left for us to parse, break out of the while loop
        if (s[0] == 0) {
            break;
        }

        // if they are defining an alternate 'home' directory.
        // Primary used for OSX/linux to keep roms, framefiles and saverams in
        // the user-writable space.
        else if (strcasecmp(s, "-homedir") == 0) {
            get_next_word(s, sizeof(s));
            if (s[0] == 0) {
                printerror("homedir switch used but no homedir specified!");
                result = false;
                break;
            } else {
                g_homedir.set_homedir(s);
                bHomeDirSet = true;
                snprintf(e, sizeof(e), "Setting alternate home dir: %s", s);
                printline(e);
                break;
            }
        }
    }

    // if user didn't specify a homedir, then we have a default one.
    // (the reason this isn't in the homedir constructor is that we don't want
    // to create
    //  default directories in a default location if the user specifies his/her
    //  own homedir)
    if (!bHomeDirSet) {

#ifdef UNIX
#ifndef MAC_OSX // Zaph doesn't seem to like this behavior and he is maintaining
                // the Mac build, so...
        // on unix, if the user doesn't specify a homedir, then we want to
        // default to ~/.hypseus
        //  to follow standard unix convention.
        const char *homeenv = getenv("HOME");

        // this is always expected to be non-NULL
        if (homeenv != NULL) {
            string strHomeDir = homeenv;
            strHomeDir += "/.hypseus";
            g_homedir.set_homedir(strHomeDir);
        }
// else we couldn't set the homedir, so just leave it as default
#endif // not MAC_OSX
#else  // WIN32
        g_homedir.set_homedir("."); // for win32 set it to the current directory
                                    // "." is already the default,
//  so the the main purpose for this is to ensure that the ram and
//  framefile directories are created

#endif
    } // end if homedir was not set

    // Reset arg index and return
    g_arg_index = 1;
    return result;
}

// parses the command line looking for the -romdir switch, returns true if
// found and valid (or not found)
bool parse_romdir() {
    bool result = true;
    char s[81] = {0};
    char e[128];

    for (;;) {
        get_next_word(s, sizeof(s));
        if (s[0] == 0) {
            break;
        }
        else if (strcasecmp(s, "-romdir") == 0) {
            get_next_word(s, sizeof(s));
            if (s[0] == 0) {
                printerror("romdir switch used but no romdir specified!");
                result = false;
                break;
            } else {
                g_homedir.set_romdir(s);
                snprintf(e, sizeof(e), "Setting alternate rom dir: %s", s);
                printline(e);
                break;
            }
        }
    }

    g_arg_index = 1;
    return result;
}

bool parse_ramdir() {
    bool result = true;
    char s[81] = {0};
    char e[128];

    for (;;) {
        get_next_word(s, sizeof(s));
        if (s[0] == 0) {
            break;
        }
        else if (strcasecmp(s, "-ramdir") == 0) {
            get_next_word(s, sizeof(s));
            if (s[0] == 0) {
                printerror("ramdir switch used but no ramdir specified!");
                result = false;
                break;
            } else {
                g_homedir.set_ramdir(s);
                snprintf(e, sizeof(e), "Setting alternate ram dir: %s", s);
                printline(e);
                break;
            }
        }
    }

    g_arg_index = 1;
    return result;
}

// Some distros enforce subsystems early, provide a means to
// negate this in a later argument.
bool parse_subsystems() {
    bool result = true;
    char s[81] = {0};

    for (;;) {
        get_next_word(s, sizeof(s));
        if (s[0] == 0) {
            break;
        }
        else if (strcasecmp(s, "-nomanymouse") == 0) {
            g_altmouse = false;
        }
    }

    g_arg_index = 1;
    return result;
}

// parses the game type from the command line and allocates g_game
// returns true if successful or false of failed
bool parse_game_type()
{
    bool result = true;

    // whether SGN (short game name) should match game name from command-line
    // (it almost always should according to agreed-upon developer convention)
    // Exception is for a 'base' game name such as 'dle2' which doesn't specify
    // 2.0, or 2.1 version
    bool bSGNMatches = true;
    char s[81]       = {0};
    char e[128];

    // first thing we need to get from the command line is the game type
    get_next_word(s, sizeof(s));

    net_set_gamename(s); // report to server the game we are running

    if (strcasecmp(s, "ace") == 0) {
        g_game = new ace();
    } else if (strcasecmp(s, "ace_a2") == 0) {
        g_game = new ace();
        g_game->set_version(2);
    } else if (strcasecmp(s, "ace_a") == 0) {
        g_game = new ace();
        g_game->set_version(3);
    } else if (strcasecmp(s, "ace91") == 0) {
        g_game = new ace91();
    } else if (strcasecmp(s, "ace91_euro") == 0) {
        g_game = new ace91();
        g_game->set_version(1);
    } else if (strcasecmp(s, "aceeuro") == 0) {
        g_game = new aceeuro();
    } else if (strcasecmp(s, "astron") == 0) {
        g_game = new astronh();
    } else if (strcasecmp(s, "astronp") == 0) {
        g_game = new astron();
    } else if (strcasecmp(s, "badlandp") == 0) {
        g_game = new badlandp();
    } else if (strcasecmp(s, "badlands") == 0) {
        g_game = new badlands();
    } else if (strcasecmp(s, "bega") == 0) {
        g_game = new bega();
    } else if (strcasecmp(s, "begar1") == 0) {
        g_game = new bega();
        g_game->set_version(2);
    } else if (strcasecmp(s, "benchmark") == 0) {
        g_game = new benchmark();
    } else if (strcasecmp(s, "blazer") == 0) {
        g_game = new blazer();
    } else if (strcasecmp(s, "cliff") == 0) {
        g_game = new cliff();
    } else if (strcasecmp(s, "cliffalt") == 0) {
        g_game = new cliffalt();
    }
    // Added by Buzz
    else if (strcasecmp(s, "cliffalt2") == 0) {
        g_game = new cliffalt2();
    } else if (strcasecmp(s, "cobra") == 0) {
        g_game = new cobra();
    } else if (strcasecmp(s, "cobraab") == 0) {
        g_game = new cobraab();
    } else if (strcasecmp(s, "cobraconv") == 0) {
        g_game = new cobraconv();
    } else if (strcasecmp(s, "cobram3") == 0) {
        g_game = new cobram3();
    } else if (strcasecmp(s, "cputest") == 0) {
        g_game = new cputest();
    } else if (strcasecmp(s, "dle11") == 0) {
        g_game = new dle11();
    } else if (strcasecmp(s, "dle2") == 0) {
        bSGNMatches = false; // shortgame name will be dle20, dle21, etc
        g_game      = new dle2();
    }
    // alias with version included
    else if (strcasecmp(s, "dle20") == 0) {
        g_game = new dle2();
        g_game->set_version(0); // set to DLE2.0
    }
    // alias with version included
    else if (strcasecmp(s, "dle21") == 0) {
        g_game = new dle2();
        g_game->set_version(1); // set to DLE2.0
    } else if (strcasecmp(s, "esh") == 0) {
        g_game = new esh();
    } else if (strcasecmp(s, "eshalt") == 0) {
        g_game = new esh();
        g_game->set_version(2);
    } else if (strcasecmp(s, "eshalt2") == 0) {
        g_game = new esh();
        g_game->set_version(3);
    } else if (strcasecmp(s, "firefox") == 0) {
        g_game = new firefox();
    } else if (strcasecmp(s, "firefoxa") == 0) {
        g_game = new firefoxa();
    } else if (strcasecmp(s, "ffr") == 0) {
        g_game = new ffr();
    } else if (strcasecmp(s, "galaxy") == 0) {
        g_game = new galaxy();
    } else if (strcasecmp(s, "galaxyp") == 0) {
        g_game = new galaxyp();
    } else if (strcasecmp(s, "gpworld") == 0) {
        g_game = new gpworld();
    } else if (strcasecmp(s, "gtg") == 0) {
        g_game = new gtg();
    } else if (strcasecmp(s, "interstellar") == 0) {
        g_game = new interstellar();
    } else if (strcasecmp(s, "lair") == 0) {
        g_game = new lair();
    } else if (strcasecmp(s, "lair_f") == 0) {
        g_game = new lair();
        g_game->set_version(2);
    } else if (strcasecmp(s, "lair_e") == 0) {
        g_game = new lair();
        g_game->set_version(3);
    } else if (strcasecmp(s, "lair_d") == 0) {
        g_game = new lairalt();
        g_game->set_version(4);

    } else if (strcasecmp(s, "lair_c") == 0) {
        g_game = new lairalt();
        g_game->set_version(3);
    } else if (strcasecmp(s, "lair_b") == 0) {
        g_game = new lairalt();
        g_game->set_version(2);
    } else if (strcasecmp(s, "lair_a") == 0) {
        g_game = new lairalt();
        g_game->set_version(1); // this is default, but what the heck
    } else if (strcasecmp(s, "lair_n1") == 0) {
        g_game = new lairalt();
        g_game->set_version(5);
    } else if (strcasecmp(s, "lair_x") == 0) {
        g_game = new lairalt();
        g_game->set_version(6);
    } else if (strcasecmp(s, "lairalt") == 0) {
        g_game = new lairalt();
    } else if (strcasecmp(s, "laireuro") == 0) {
        g_game = new laireuro();
    } else if (strcasecmp(s, "lair_ita") == 0) {
        g_game = new laireuro();
        g_game->set_version(2);
    } else if (strcasecmp(s, "lair_d2") == 0) {
        g_game = new laireuro();
        g_game->set_version(3);
    } else if (strcasecmp(s, "lair2") == 0) {
        g_game = new lair2();
    } else if (strcasecmp(s, "lair2_319_euro") == 0) {
        g_game = new lair2();
        g_game->set_version(8);
    } else if (strcasecmp(s, "lair2_319_span") == 0) {
        g_game = new lair2();
        g_game->set_version(9);
    } else if (strcasecmp(s, "lair2_318") == 0) {
        g_game = new lair2();
        g_game->set_version(5);
    } else if (strcasecmp(s, "lair2_316_euro") == 0) {
        g_game = new lair2();
        g_game->set_version(7);
    } else if (strcasecmp(s, "lair2_315") == 0) {
        g_game = new lair2();
        g_game->set_version(0);
    } else if (strcasecmp(s, "lair2_314") == 0) {
        g_game = new lair2();
        g_game->set_version(3);
    } else if (strcasecmp(s, "lair2_300") == 0) {
        g_game = new lair2();
        g_game->set_version(2);
    } else if (strcasecmp(s, "lair2_211") == 0) {
        g_game = new lair2();
        g_game->set_version(1);
    } else if (strcasecmp(s, "lgp") == 0) {
        g_game = new lgp();
    } else if (strcasecmp(s, "mach3") == 0) {
        g_game = new mach3();
    } else if (strcasecmp(s, "mcputest") == 0) {
        g_game = new mcputest();
    } else if (strcasecmp(s, "releasetest") == 0) {
        bSGNMatches = false;
        g_game = new releasetest();
    } else if (strcasecmp(s, "roadblaster") == 0) {
        g_game = new roadblaster();
    } else if (strcasecmp(s, "sae") == 0) {
        g_game = new sae();
    } else if (strcasecmp(s, "seektest") == 0) {
        g_game = new seektest();
    }
// singe has a bunch of extra deps so we make it optional to build
#ifdef BUILD_SINGE
    else if (strcasecmp(s, "singe") == 0) {
        g_game = new singe();
    }
#endif // BUILD_SINGE
    else if (strcasecmp(s, "speedtest") == 0) {
        g_game = new speedtest();
    } else if (strcasecmp(s, "sdq") == 0) {
        g_game = new superd();
    } else if (strcasecmp(s, "sdqshort") == 0) {
        g_game = new sdqshort();
    } else if (strcasecmp(s, "sdqshortalt") == 0) {
        g_game = new sdqshortalt();
    } else if (strcasecmp(s, "starrider") == 0) {
        g_game = new starrider();
    } else if (strcasecmp(s, "superdon") == 0) // left in for old times' sake
    {
        bSGNMatches = false;
        g_game = new superd();
    } else if (strcasecmp(s, "timetrav") == 0) {
        g_game = new timetrav();
    } else if (strcasecmp(s, "test_sb") == 0) {
        g_game = new test_sb();
    } else if (strcasecmp(s, "tq") == 0) {
        g_game = new thayers();
    } else if (strcasecmp(s, "tq_alt") == 0) {
        g_game = new thayers();
        g_game->set_version(2);
    } else if (strcasecmp(s, "tq_swear") == 0) {
        g_game = new thayers();
        g_game->set_version(3);
    } else if (strcasecmp(s, "uvt") == 0) {
        g_game = new uvt();
    } else if (strcasecmp(s, "-v") == 0) {
#if defined(WIN32) || defined(__APPLE__)
        const char* l = "Hypseus Singe: ";
        const char* l1 = get_hypseus_version();
        const char* l2 = get_sdl_compile();
        const char* l3 = get_sdl_linked();
        const char* l4 = get_build_time();
        string s = string(l) + l1 + "\n\n" + l2 + "\n" + l3 + "\n\n" + l4;
#endif
#ifdef WIN32
        MessageBox(NULL, s.c_str(), "Version", MB_OK | MB_ICONINFORMATION);
#elif __APPLE__
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Version", s.c_str(), NULL);
#else
        printline(get_os_description());
        printline(get_sdl_compile());
        printline(get_sdl_linked());
        printline(get_build_time());
#endif
        result = true;
        exit(0);
    } else {
        snprintf(e, sizeof(e), "ERROR: Unknown game type specified : %s", s);
        printerror(e);
        printusage();
        result = false;
    }

    // safety check
    if (!g_game) {
        result = false;
    }

    // else if the Short Game Name should match the game name passed in via
    // command-line,
    //  then return an error if it doesn't, to force developers to follow this
    //  convention.
    else if (bSGNMatches) {
        if (strcasecmp(s, g_game->get_shortgamename()) != 0) {
            printerror("Developer ERROR : short game name does not match "
                      "command-line game name and it should!");
            string msg = "Cmdline Game name is: ";
            msg += s;
            msg += "; short game name is: ";
            msg += g_game->get_shortgamename();
            printline(msg.c_str());
            result = false;
        }
    }

    return (result);
}

// parses the LDP type from the command line and allocates g_ldp
// returns true if successful or false if failed
bool parse_ldp_type()
{
    bool result = true;
    char s[81]  = {0};

    get_next_word(s, sizeof(s));

    net_set_ldpname(s); // report to server which ldp we are using

    if (strcasecmp(s, "fast_noldp") == 0) {
        g_ldp = new fast_noldp(); // 'no seek-delay' version of noldp
    } else if (strcasecmp(s, "noldp") == 0) {
        g_ldp = new ldp(); // generic interface
    } else if (strcasecmp(s, "vldp") == 0) {
        g_ldp = new ldp_vldp();
    } else {
        printerror("ERROR: Unknown laserdisc player type specified");
        result = false;
    }

    // safety check
    if (!g_ldp) {
        result = false;
    }

    return (result);
}

// parses command line
// Returns 1 on success or 0 on failure
bool parse_cmd_line(int argc, char **argv)
{
    bool result = true;
    bool filter = false;
    char s[400] = {0}; // in case they pass in a huge folder as part of the framefile
    int i = 0;

    //////////////////////////////////////////////////////////////////////////////////////

    set_scoreboard(0);      // Now this is a bitmapped type, so set it to NONE to start
    g_argc      = argc;
    g_argv      = argv;
    g_arg_index = 1; // skip name of executable from command line

    // if game and ldp types are correct
    if (parse_subsystems() && parse_romdir() && parse_ramdir() && parse_homedir() &&
            parse_game_type() && parse_ldp_type()) {
        // while we have stuff left in the command line to parse
        for (;;) {
            get_next_word(s, sizeof(s));

            // if there is nothing left for us to parse, break out of the while
            // loop
            if (s[0] == 0) {
                break;
            }

            // if they are defining an alternate 'home' directory.
            // Primary used for OSX/linux to keep roms, framefiles and saverams
            // in the user-writable space.
            else if (strcasecmp(s, "-homedir") == 0) {
                // Ignore this one, already handled
                get_next_word(s, sizeof(s));
            }

            // if they are defining an alternate 'roms' directory.
            else if (strcasecmp(s, "-romdir") == 0) {
                // Ignore this one, already handled
                get_next_word(s, sizeof(s));
            }

            // if they are defining an alternate 'ram' directory.
            else if (strcasecmp(s, "-ramdir") == 0) {
                // Ignore this one, already handled
                get_next_word(s, sizeof(s));
            }

            // If they are defining an alternate 'data' directory, where all
            // other files aside from the executable live.
            // Primary used for linux to separate binary file (eg. hypseus.bin)
            // and other datafiles .
            else if (strcasecmp(s, "-datadir") == 0) {
                get_next_word(s, sizeof(s));

                if (s[0] == 0) {
                    printerror("datadir switch used but no datadir specified!");
                    result = false;
                } else change_dir(s);
            }

            // if user wants laserdisc player to blank video while searching
            // (VLDP only)
            else if (strcasecmp(s, "-blank_searches") == 0) {
                g_ldp->set_search_blanking(true);
            }

            // if user wants to laserdisc player to blank video while skipping
            // (VLDP only)
            else if (strcasecmp(s, "-blank_skips") == 0) {
                g_ldp->set_skip_blanking(true);
            }
            // provides a means to switch blanking YUV to blue
            else if (strcasecmp(s, "-blank_blue") == 0) {
                video::set_yuv_blue(true);
            }
            // if they are pointing to a framefile to be used by VLDP
            else if (strcasecmp(s, "-framefile") == 0) {
                ldp_vldp *cur_ldp =
                    dynamic_cast<ldp_vldp *>(g_ldp); // see if the currently
                                                     // selected LDP is VLDP
                get_next_word(s, sizeof(s));
                // if it is a vldp, then this option has meaning and we can use
                // it
                if (cur_ldp) {
                    cur_ldp->set_framefile(s);
                } else {
                    printerror("You can only set a framefile using VLDP as your "
                              "laserdisc player!");
                    result = false;
                }
            }
            // Ignore some deprecated arguments (Rather than error)
            else if (strcasecmp(s, "-noserversend") == 0 ||
                         strcasecmp(s, "-nolinear_scale") == 0 ||
                             strcasecmp(s, "-fullscale") == 0) {

                 char e[460];
                 snprintf(e, sizeof(e), "NOTE : Ignoring deprecated argument: %s", s);
                 printline(e);
            }
            // specify an alternate hypseus.ini file (located in home or app directory)
            else if (strcasecmp(s, "-keymapfile") == 0 || strcasecmp(s, "-config") == 0) {

                bool loadini = true;
                get_next_word(s, sizeof(s));
                int iLen = strlen(s);

                if (iLen < 5)
                    loadini = false;

                if (loadini) {
                    for (int k = 0; k < iLen; k++) // lowercase
                        s[k] = tolower(s[k]);

                    string s1(s);
                    string s2(s1.substr(iLen-4));

                    if (s2.compare(".ini") != 0) // .ini
                        loadini = false;

                    if (loadini) { // alphanum
                        string s3 = s1.substr(0, (iLen-4));
                        for (int i = 0; s3[i] != '\0'; i++) {
                            if (!isalnum(s3[i]) && s3[i] != int('-')
                                && s3[i] != int('_') && s3[i] != int('/')
#ifdef WIN32
                                    && s3[i] != int('\\')
#endif
                            ) {
                                loadini = false;
                            }
                        }

                        if (loadini)
                            set_inputini_file(s);
                    }
                }

                if (!loadini) {
                    char e[460];
                    snprintf(e, sizeof(e), "Invalid -keymapfile file: %s [Use .ini]", s);
                    printerror(e);
                    result = false;
                 }
            }
            // if they are defining an alternate soundtrack to be used by VLDP
            else if (strcasecmp(s, "-altaudio") == 0) {
                ldp_vldp *cur_ldp =
                    dynamic_cast<ldp_vldp *>(g_ldp); // see if the currently
                                                     // selected LDP is VLDP
                get_next_word(s, sizeof(s));
                // if it is a vldp, then this option has meaning and we can use
                // it
                if (cur_ldp) {
                    cur_ldp->set_altaudio(s);
                } else {
                    printerror("You can only set an alternate soundtrack when "
                              "using VLDP as your laserdisc player!");
                    result = false;
                }
            }

            // The # of frames that we can seek per millisecond (to simulate
            // seek delay)
            // Typical values for real laserdisc players are about 30.0 for
            // 29.97fps discs
            //  and 20.0 for 23.976fps discs (dragon's lair and space ace)
            // FLOATING POINT VALUES ARE ALLOWED HERE
            // Minimum value is 12.0 (5 seconds for 60,000 frames), maximum
            // value is
            //  600.0 (100 milliseconds for 60,000 frames).  If you want a value
            //  higher than
            //  the max, you should just use 0 (as fast as possible).
            // 0 = disabled (seek as fast as possible)
            else if (strcasecmp(s, "-seek_frames_per_ms") == 0) {
                get_next_word(s, sizeof(s));
                double d = numstr::ToDouble(s);

                // WARNING : VLDP will timeout if it hasn't received a response
                // after
                // a certain period of time (7.5 seconds at this moment) so you
                // will
                // cause problems by reducing the minimum value much below 12.0
                if ((d > 12.0) && (d < 600.0))
                    g_ldp->set_seek_frames_per_ms(d);
                else
                    printline("NOTE : Max seek delay disabled");
            }

            // the minimum amount of milliseconds to force a seek to take
            // (artificial delay)
            // 0 = disabled
            else if (strcasecmp(s, "-min_seek_delay") == 0) {
                get_next_word(s, sizeof(s));
                i = atoi(s);

                if ((i > 0) && (i < 5000))
                    g_ldp->set_min_seek_delay((unsigned int)i);
                else
                    printline("NOTE : Min seek delay disabled");
            }

            // if the user wants the searching to be the old blocking style
            // instead of non-blocking
            else if (strcasecmp(s, "-blocking") == 0) {
                g_ldp->set_use_nonblocking_searching(false);
            }

            // to disable any existing joysticks that may be plugged in that may
            // interfere with input
            else if (strcasecmp(s, "-nojoystick") == 0) {
                set_use_joystick(false);
            }
            else if (strcasecmp(s, "-gamepad") == 0) {
                set_use_gamepad(true);
            }
            else if (strcasecmp(s, "-gamepad_reorder") == 0) {

                bool b = true;
                int ctlr[MAX_GAMECONTROLLER] = {0}, j, k;

                for (j = 0; j < MAX_GAMECONTROLLER; j++) {

                    get_next_word(s, sizeof(s));

                    if (strcasecmp(s, "0") == 0) i = 1;
                    else {
                        i = atoi(s);
                        if (i) i++;
                    }

                    if ((i > 0) && (i <= MAX_GAMECONTROLLER)) {
                        ctlr[j] = i;
                    }
                }
                for (j = 0; j < MAX_GAMECONTROLLER && b; j++) {
                    if (ctlr[j] == 0) {
                        b = false;
                        break;
                    }
                    for (k = j + 1; k < MAX_GAMECONTROLLER; k++) {
                        if (ctlr[j] == ctlr[k]) {
                            b = false;
                            break;
                        }
                    }
                }
                if (b) {
                    set_use_gamepad(true);
                    set_gamepad_order(ctlr, MAX_GAMECONTROLLER);
                } else {
                    char e[460];
                    snprintf(e, sizeof(e), "Invalid gamepad unit input: -gamepad_order 0 .. %d",
                       MAX_GAMECONTROLLER - 1);
                    printline(e);
                    result = false;
                }
            }
            else if (strcasecmp(s, "-haptic") == 0) {
                bool disabled = false;
                get_next_word(s, sizeof(s));
                if (strcasecmp(s, "0") == 0) {
                    printline("All haptic feedback is disabled...");
                    disable_haptics();
                    disabled = true;
                }

                i = atoi(s);
                if ((i > 0) && (i < 5)) {
                    set_haptic(i);
                } else if (!disabled) {
                    printline("Invalid argument: -haptic [0-4]");
                    result = false;
                }
            }
            // Invert the Joystick HAT UP/DOWN
            else if (strcasecmp(s, "-tiphat") == 0) {
                set_invert_hat(true);
            }
            else if (strcasecmp(s, "-openhat") == 0) {
                set_open_hat(true);
            }
            // if want data sent to the server
            else if (strcasecmp(s, "-serversend") == 0) {
                net_server_send();
            } else if (strcasecmp(s, "-nosound") == 0) {
                sound::set_enabled_status(false);
                printline("Disabling sound...");
            } else if (strcasecmp(s, "-sound_buffer") == 0) {
                get_next_word(s, sizeof(s));
                Uint16 sbsize = (Uint16)atoi(s);
                sound::set_buf_size(sbsize);
                snprintf(s, sizeof(s), "Setting sound buffer size to %d", sbsize);
                printline(s);
            } else if (strcasecmp(s, "-volume_vldp") == 0) {
                get_next_word(s, sizeof(s));
                unsigned int uVolume = atoi(s);
                sound::set_chip_vldp_volume(uVolume);
            } else if (strcasecmp(s, "-volume_nonvldp") == 0) {
                get_next_word(s, sizeof(s));
                unsigned int uVolume = atoi(s);
                sound::set_chip_nonvldp_volume(uVolume);
            } else if (strcasecmp(s, "-nocrc") == 0) {
                g_game->disable_crc();
                printline("Disabling ROM CRC check...");
            } else if (strcasecmp(s, "-scoreboard") == 0) {
                set_scoreboard(get_scoreboard() | 0x01); // Bitmapped -- enable parallel SB
                printline("Enabling external scoreboard...");
            } else if (strcasecmp(s, "-scoreport") == 0) {
                get_next_word(s, sizeof(s));
                unsigned int u = (unsigned int)numstr::ToUint32(s, 16);
                set_scoreboard_port(u);
                snprintf(s, sizeof(s), "Setting scoreboard port to %x", u);
                printline(s);
            } else if (strcasecmp(s, "-usbscoreboard") == 0) {
                if (g_game->m_software_scoreboard) return false;
                int baud = 0;
                int impl = 0;
                char e[100];
                get_next_word(s, sizeof(s));
#ifdef WIN32
                snprintf(e, sizeof(e), "-usbscoreboard requires a COM port number\n and baud rate: COM [1-9] [BAUD]");
                if (strcasecmp(s, "COM") == 0) impl = 0x01;
                get_next_word(s, sizeof(s));
                i = atoi(s);
                get_next_word(s, sizeof(s));
                baud = atoi(s);
                if ((i > 0 && i < 10) && impl && baud) {
                    set_scoreboard_usb_port(i);
                    set_scoreboard_usb_baud(baud);
#elif defined(LINUX)
                snprintf(e, sizeof(e), "-usbscoreboard requires device type, number and baud rate: [USB|ACM] [0-9] [BAUD]");
                if (strcasecmp(s, "USB") == 0) impl = 0x01;
                else if (strcasecmp(s, "ACM") == 0) impl = 0x02;
                get_next_word(s, sizeof(s));
                if (strcasecmp(s, "0") == 0) i = 0x01;
                else {
                    i = atoi(s);
                    if (i) i++;
                }
                get_next_word(s, sizeof(s));
                baud = atoi(s);

                if ((i > 0 && i <= 10) && impl && baud) {
                    set_scoreboard_usb_port(i-1);
                    set_scoreboard_usb_baud(baud);
                    set_scoreboard_usb_impl(impl);
#else
                snprintf(e, sizeof(e), "-usbscoreboard is unsupported on this platform");
                if (baud == -1 && impl == -1) {
#endif
                    set_scoreboard(get_scoreboard() | 0x02); // Bitmapped -- enable USB SB
                    printline("Enabling USB scoreboard...");
                } else {
                    printerror(e);
                    result = false;
                }
            }
            else if (strcasecmp(s, "-usbserial_rts_on") == 0) {
                set_scoreboard_usb_rts(true);
            }
            else if (strcasecmp(s, "-scorebezel") == 0) {
                lair *game_lair_or_sa = dynamic_cast<lair *>(g_game);
                thayers *game_thayers = dynamic_cast<thayers *>(g_game);

                if ((game_lair_or_sa || game_thayers) && !get_scoreboard()) {
                    video::set_score_bezel(true);
                    g_game->m_software_scoreboard = true;
                    printline("Enabling Scoreboard bezel...");
                } else {
                    printline("NOTE: Scoreboard bezel is not available");
                }
            }
            else if (strcasecmp(s, "-scorebezel_scale") == 0 ||
                         strcasecmp(s, "-scorepanel_scale") == 0 ||
                             strcasecmp(s, "-auxbezel_scale") == 0) {

                bool aux = (strcasecmp(s, "-auxbezel_scale") == 0);

                get_next_word(s, sizeof(s));
                i = atoi(s);
                if (i >= 1 && i <= 25) {
                    if (aux)
                        video::set_aux_bezel_scale(i);
                    else
                        video::set_score_bezel_scale(i);
                } else {
                    printerror("Scale values: 1-25");
                    result = false;
                }
            }
            else if (strcasecmp(s, "-scorebezel_alpha") == 0 ||
                         strcasecmp(s, "-auxbezel_alpha") == 0) {

                bool aux = (strcasecmp(s, "-auxbezel_alpha") == 0);

                get_next_word(s, sizeof(s));
                i = atoi(s);

                if (i >= 1 && i <= 2) {
                    if (aux)
                        video::set_aux_bezel_alpha((int8_t)i);
                    else
                        video::set_score_bezel_alpha((int8_t)i);
                } else {
                    printerror("Bezel alpha values: 1-2");
                    result = false;
                }
            }
            else if (strcasecmp(s, "-scorepanel") == 0) {
                lair *game_lair_or_sa = dynamic_cast<lair *>(g_game);
                thayers *game_thayers = dynamic_cast<thayers *>(g_game);

                if ((game_lair_or_sa || game_thayers) && !get_scoreboard()) {
                    video::set_score_bezel(false);
                    g_game->m_software_scoreboard = true;
                    printline("Enabling Software scoreboard...");
                } else {
                    printline("NOTE: Software scoreboard is not available");
                }
            }
            else if (strcasecmp(s, "-scorepanel_position") == 0 ||
                         strcasecmp(s, "-scorebezel_position") == 0 ||
                            strcasecmp(s, "-auxbezel_position") == 0) {
                const int vMax = 5120 + 1; // This should handle DQHD+
                int xVal = 0;
                int yVal = 0;

                bool aux =  (strcasecmp(s, "-auxbezel_position") == 0);

                get_next_word(s, sizeof(s));
                bool xn = (s[0] == '-') ? true : false;

                if (strcasecmp(s, "0") == 0) xVal = 1;
                else {
                    xVal = atoi(xn ? s + 1 : s);
                    if (xVal) xVal++;
                }

                get_next_word(s, sizeof(s));
                bool yn = (s[0] == '-') ? true : false;

                if (strcasecmp(s, "0") == 0) yVal = 1;
                else {
                    yVal = atoi(yn ? s + 1 : s);
                    if (yVal) yVal++;
                }

                if ((xVal > 0) && (xVal <= vMax) && (yVal > 0) && (yVal <= vMax)) {

                    if (aux)
                        video::set_aux_bezel_position((xn ? -xVal+1 : xVal-1),
                                                      (yn ? -yVal+1 : yVal-1));
                    else
                        video::set_sb_window_position((xn ? -xVal+1 : xVal-1),
                                                      (yn ? -yVal+1 : yVal-1));
                } else {
                    printerror("Positions requires x and y values");
                    result = false;
                }
            }
            else if (strcasecmp(s, "-bezelflip") == 0) {
                    video::set_bezel_reverse(false);
            }
            else if (strcasecmp(s, "-scorescreen") == 0) {
                get_next_word(s, sizeof(s));
                i = atoi(s);

                if (i >= 2 && i <= 255)
                    video::set_score_screen(i);
            }
            else if (strcasecmp(s, "-tq_keyboard") == 0 ||
                         strcasecmp(s, "-tqkeys") == 0) {
                thayers *game_thayers = dynamic_cast<thayers *>(g_game);

                if (game_thayers)
                    video::set_tq_keyboard(true);
                else result = invalid_arg(s);
            }
            else if (strcasecmp(s, "-annunbezel") == 0 ||
                         strcasecmp(s, "-dedannunbezel") == 0 ||
                             strcasecmp(s, "-annunlamps") == 0) {
                lair *game_ace = dynamic_cast<ace *>(g_game);

                if (game_ace) {
                    video::set_aux_bezel(true);
                    enable_bannun(true);
                    if (strcasecmp(s, "-dedannunbezel") == 0)
                        video::set_ded_annun_bezel(true);
                    if (strcasecmp(s, "-annunlamps") == 0)
                        video::set_annun_lamponly(true);
                } else result = invalid_arg(s);
            }
            // used to modify the dip switch settings of the game in question
            else if (strcasecmp(s, "-bank") == 0) {
                get_next_word(s, sizeof(s));
                i = s[0] - '0'; // bank to modify.  We convert to a number this
                                // way to catch more errors

                get_next_word(s, sizeof(s));
                unsigned char value =
                    (unsigned char)(strtol(s, NULL, 2)); // value to be set is
                                                         // in base 2 (binary)
                if (result)
                    result = g_game->set_bank((unsigned char)i, (unsigned char)value);
            }
            else if (strcasecmp(s, "-latency") == 0) {
                get_next_word(s, sizeof(s));
                i = atoi(s);

                // safety check
                if (i >= 0) {
                    g_ldp->set_search_latency(i);
                    snprintf(s, sizeof(s), "Setting Search Latency to %d milliseconds", i);
                    printline(s);
                } else {
                    printerror("Search Latency value cannot be negative!");
                    result = false;
                }
            } else if (strcasecmp(s, "-cheat") == 0) {
                g_game->enable_cheat(); // enable any cheat we have available :)
            }

            // enable keyboard LEDs for use with games such as Space Ace
            else if (strcasecmp(s, "-enable_leds") == 0) {
                enable_leds(true);
            }
            // hacks roms for certain games such as DL, SA, and CH to bypass
            // slow boot process
            else if (strcasecmp(s, "-fastboot") == 0) {
                g_game->set_fastboot(true);
            }

            // stretch overlay vertically by x amount (a value of 24 removes
            // letterboxing effect in Cliffhanger)
            else if (strcasecmp(s, "-vertical_stretch") == 0) {
                cliff *game_cliff = dynamic_cast<cliff *>(g_game);

                get_next_word(s, sizeof(s));
                i = atoi(s);

                if (i > 0 && i < 25) {
                    if (game_cliff != NULL) {
                        g_game->set_stretch_value(i);
                    } else {
                        video::set_yuv_scale(i, YUV_V);
                    }
                } else {
                    printerror("Vertical stretch. Values [1-24]");
                    result = false;
                }
            }
            else if (strcasecmp(s, "-horizontal_stretch") == 0) {
                get_next_word(s, sizeof(s));
                i = atoi(s);

                if (i > 0 && i < 25) {
                    video::set_yuv_scale(i, YUV_H);
                } else {
                    printerror("Horizontal stretch. Values [1-24]");
                    result = false;
                }
            }
            // SDL regression: FOURCC isn't supported by renderer back-ends for target access
            // This avoids a CPU intensive conversion on SBC's
            else if (strcasecmp(s, "-texturestream") == 0) {
                video::set_textureaccess(SDL_TEXTUREACCESS_STREAMING);
                printline("Forcing TEXTUREACCESS_STREAMING");
            }
            // Default (or override)
            else if (strcasecmp(s, "-texturetarget") == 0) {
                if (video::get_textureaccess() == SDL_TEXTUREACCESS_STREAMING)
                    printline("Reassigning to TEXTUREACCESS_TARGET");
                video::set_textureaccess(SDL_TEXTUREACCESS_TARGET);
            }
            else if (strcasecmp(s, "-opengl") == 0) {
                video::set_opengl(true);
                printline("Enabling SDL_OPENGL");
                if (video::get_vulkan()) result = false;
            }
            else if (strcasecmp(s, "-vulkan") == 0) {
                video::set_vulkan(true);
                printline("Enabling SDL_VULKAN");
                if (video::get_opengl()) result = false;
            }
            else if (strcasecmp(s, "-monochrome") == 0) {
                video::set_grayscale(true);
                printline("VLDP video will be displayed in monochrome");
            }
            else if (strcasecmp(s, "-alwaysontop") == 0) {
                video::set_forcetop(true);
                printline("Setting WINDOW_ALWAYS_ON_TOP");
            }
            else if (strcasecmp(s, "-novsync") == 0) {
                video::set_vsync(false);
            }
            else if (strcasecmp(s, "-nosplash") == 0) {
                video::set_intro(false);
            }
            else if (strcasecmp(s, "-force_aspect_ratio") == 0) {
                printline("Forcing 4:3 aspect ratio.");
                video::set_force_aspect_ratio(true);
                video::set_ignore_aspect_ratio(false);
            }
            else if (strcasecmp(s, "-ignore_aspect_ratio") == 0) {
                printline("Ignoring MPEG aspect ratio headers.");
                video::set_ignore_aspect_ratio(true);
                video::set_force_aspect_ratio(false);
            }
            else if (strcasecmp(s, "-scanline_shunt") == 0) {
                get_next_word(s, sizeof(s));
                i = atoi(s);
                if (i > 1 && i < 11) {
                    video::set_shunt((uint8_t)i);
                } else {
                    printerror("Shunt values: 2-10");
                    result = false;
                }
            }
            else if (strcasecmp(s, "-scanline_alpha") == 0) {
                get_next_word(s, sizeof(s));
                i = atoi(s);
                if (i >= 1 && i <= 255) {
                    video::set_alpha((uint8_t)i);
                } else {
                    printerror("Scanline alpha values: 1-255");
                    result = false;
                }
            }
            else if (strcasecmp(s, "-screen") == 0) {
                get_next_word(s, sizeof(s));
                i = atoi(s);

                if (i > 0 && i < 255)
                    video::set_display_screen(i);
            }
            else if (strcasecmp(s, "-teardown_window") == 0) {
                video::set_teardown();
            }
            // run hypseus in fullscreen mode
            else if (strcasecmp(s, "-fullscreen") == 0) {
                video::set_fullscreen(true);
                video::set_fakefullscreen(false);
            }
            // run hypseus in borderless fullscreen window mode
            else if (strcasecmp(s, "-fullscreen_window") == 0) {
                video::set_fakefullscreen(true);
                video::set_fullscreen(false);
            }
            // Capture mouse within SDL window and enable manymouse
            else if (strcasecmp(s, "-grabmouse") == 0) {
                video::set_grabmouse(true);
                if (g_altmouse) g_game->set_manymouse(true);
            }
            // Make manymouse a global argument
            else if (strcasecmp(s, "-manymouse") == 0) {
                if (g_altmouse) g_game->set_manymouse(true);
            }
            else if (strcasecmp(s, "-nomanymouse") == 0) {
                // Ignore this one, already handled
            }
#ifdef LINUX
            // Manymouse only returns mice with absolute positioning [Linux evdev]
            else if (strcasecmp(s, "-filter-absolutes") == 0) {
                filter = true;
            }
#endif
            // Enable SDL_HINT_RENDER_SCALE_QUALITY(linear)
            else if (strcasecmp(s, "-linear_scale") == 0) {
                video::set_scale_linear(true);
            }
            // disable log file
            else if (strcasecmp(s, "-nolog") == 0) {
                set_log_was_disabled(true);
            }
            // specify a bezel file (located in home 'bezels' directory)
            else if (strcasecmp(s, "-bezel") == 0) {

                bool loadbezel = true;
                get_next_word(s, sizeof(s));
                int iLen = strlen(s);

                if (iLen < 5)
                    loadbezel = false;

                if (loadbezel) {

                    string s1(s);
                    string s2(s1.substr(iLen-4));

                    if (s2.compare(".png") != 0) // .png
                        loadbezel = false;

                    if (loadbezel) { // alphanum
                        string s3 = s1.substr(0, (iLen-4));
                        for (int i = 0; s3[i] != '\0'; i++) {
                            if (!isalnum(s3[i]) && s3[i] != int('-')
                                && s3[i] != int('_') && s3[i] != int('/')
#ifdef WIN32
                                    && s3[i] != int('\\')
#endif
                                ) {
                                loadbezel = false;
                            }
                        }

                        if (loadbezel)
                            video::set_bezel_file(s);
                    }
                }

                if (!loadbezel) {
                    char e[460];
                    snprintf(e, sizeof(e), "Invalid -bezel file: %s [Use .png]", s);
                    printerror(e);
                    result = false;
                 }
            }
            else if (strcasecmp(s, "-bezeldir") == 0) {
                get_next_word(s, sizeof(s));
                bool path = true;

                if (s[0] == 0) {
                    printerror("bezeldir switch used but no bezeldir specified!");
                    path = false;
                } else if (!safe_dir(s, 0xff)) {
                    printerror("Invalid charaters in bezelpath");
                    path = false;
                }

                result = path;
                if (path) video::set_bezel_path(s);
            }
            // by DBX - This switches logical axis calculations
            else if (strcasecmp(s, "-vertical_screen") == 0) {
                video::set_vertical_orientation(true);
            }
            // by RDG2010
            // Scales video image to something smaller than the window size.
            // Helpful for users with overscan issues on arcade monitors or CRT
            // TVs.
            // Valid values are 50-100, where 50 means half the size of the
            // window, 100 means the same size.
            else if (strcasecmp(s, "-scalefactor") == 0) {
                get_next_word(s, sizeof(s));
                i = atoi(s);
                if (i >= 25 && i <= 100) {
                    snprintf(s, sizeof(s), "Scaling video by %d%%", i);
                    printline(s);
                    video::set_scalefactor((Uint16)i);
                } else {
                    printerror("Scaling values: 25 to 100");
                    result = false;
                }
            }
            else if ((strcasecmp(s, "-shiftx") == 0) ||
                       (strcasecmp(s, "-shifty") == 0)) {

                bool f = false;
                bool x = (strcasecmp(s, "-shiftx") == 0);

                get_next_word(s, sizeof(s));
                int iLen = strlen(s);
                for (int i = 0; i < iLen; i++) {
                     if (!isdigit(s[i]) && s[0] != int('-'))
                         f = true; // print help
                }
                i = atoi(s);
                if (i >= -100 && i != 0 && i <= 100 && !f) {
                    i = i + 0x64;
                    if (i == 0x0) i = 0x1;
                    if (x)
                        video::set_scale_h_shift(i);
                    else
                        video::set_scale_v_shift(i);
                } else {
                    printerror("Shift values: -100 to 100");
                    result = false;
                }
            }
            else if (strcasecmp(s, "-pal_dl") == 0) {
                set_frame_modifier(MOD_PAL_DL);
                printline("Setting up for the PAL Dragon's Lair disc");
                cpu::change_irq(0, 0,
                               LAIR_IRQ_PERIOD * (23.976 / 25.0)); // change IRQ
                                                                   // 0 of CPU 0
                // DL PAL runs at a different speed so we need to overclock the
                // IRQ slightly
            } else if (strcasecmp(s, "-pal_sa") == 0) {
                set_frame_modifier(MOD_PAL_SA);
                printline("Setting up for the PAL Space Ace disc");
            } else if (strcasecmp(s, "-pal_dl_sc") == 0) {
                set_frame_modifier(MOD_PAL_DL_SC);
                printline("Setting up for the PAL Dragon's Lair "
                          "Software Corner disc");
                cpu::change_irq(0, 0, LAIR_IRQ_PERIOD * (23.976 / 25.0));
                // DL Amiga runs at a different speed so we need to overclock
                // the IRQ slightly
            } else if (strcasecmp(s, "-pal_sa_sc") == 0) {
                set_frame_modifier(MOD_PAL_SA_SC);
                printline("Setting up for the PAL Space Ace Software "
                          "Corner disc");
            } else if (strcasecmp(s, "-spaceace91") == 0) {
                set_frame_modifier(MOD_SA91);
                printline("Setting to play a Space Ace '91 disc");
            } else if (strcasecmp(s, "-preset") == 0) {
                get_next_word(s, sizeof(s));
                i = atoi(s);
                g_game->set_preset(i);
            } else if (strcasecmp(s, "-rotate") == 0) {
                get_next_word(s, sizeof(s));
                float f = (float)numstr::ToDouble(s);
                if (f < 0 || f >= 360) f = 0;
                video::set_rotate_degrees(f);
            }

            // continuously updates SRAM (after every seek)
            // ensures sram is saved even if Hypseus is terminated improperly
            else if (strcasecmp(s, "-sram_continuous_update") == 0) {
                g_ldp->set_sram_continuous_update(true);
            }

            else if (strcasecmp(s, "-version") == 0) {
                get_next_word(s, sizeof(s));
                i = atoi(s);
                g_game->set_version(i);
            } else if (strcasecmp(s, "-x") == 0) {
                get_next_word(s, sizeof(s));
                i = atoi(s);
                if (i == 0) result = invalid_arg(s);
                video::set_video_width((Uint16)i);
                snprintf(s, sizeof(s), "Setting screen width to %d", i);
                printline(s);
            } else if (strcasecmp(s, "-y") == 0) {
                get_next_word(s, sizeof(s));
                i = atoi(s);
                if (i == 0) result = invalid_arg(s);
                video::set_video_height((Uint16)i);
                snprintf(s, sizeof(s), "Setting screen height to %d", i);
                printline(s);
            } else if (strcasecmp(s, "-trace") == 0) {
#ifdef CPU_DEBUG
                printline("CPU tracing enabled");
                printerror("TODO....");
#else
                printerror("Needs to be compiled in debug mode for this "
                          "to work");
                result = false;
#endif
            }
            // added by JFA for -idleexit
            else if (strcasecmp(s, "-idleexit") == 0) {
                get_next_word(s, sizeof(s));
                i = atoi(s);
                set_idleexit(i * 1000);
                snprintf(s, sizeof(s), "Setting idleexit to %d ", i);
                printline(s);
            }
            // end edit

            // added by JFA for -startsilent
            else if (strcasecmp(s, "-startsilent") == 0) {
                set_startsilent(1);
                printline("Starting silent...");
            }
            // end edit

            // if they are requesting to stop the laserdisc when the program
            // exits
            else if (strcasecmp(s, "-stoponquit") == 0) {
                g_ldp->set_stop_on_quit(true);
            }
            // Use old style overlays (lair, ace, tq)
            else if (strcasecmp(s, "-original_overlay") == 0) {
                g_game->m_old_overlay = true;
                video::set_sboverlay_white(true);
            }
            // this switch only supported by the ldp-vldp player class.
            else if (strcasecmp(s, "-useoverlaysb") == 0) {
                get_next_word(s, sizeof(s));
                i = atoi(s);

                // make sure that if we read 0 as the argument, that it is
                // really zero.. :)
                if (((i == 0) && (s[0] == '0')) || (i != 0)) {
                    video::set_sboverlay_characterset(i);

                    lair *game_lair_or_sa = dynamic_cast<lair *>(g_game);
                    thayers *game_thayers = dynamic_cast<thayers *>(g_game);

                    // print a warning instead of an error to make hypseus more
                    // friendly to non-hypseusloader frontends
                    if (NULL == game_lair_or_sa && NULL == game_thayers) {
                        printline("WARNING: -useoverlaysb is not supported for "
                                  "this game and will be ignored");
                    } else {
                        if (game_lair_or_sa)
                            game_lair_or_sa->init_overlay_scoreboard();
                        else
                            // Must be Thayer's Quest
                            game_thayers->init_overlay_scoreboard();
                    }
                } else {
                    char e[460];
                    snprintf(e, sizeof(e), "-useoverlaysb requires an argument such as 0 or 1, found: %s", s);
                    printerror(e);
                    result = false;
                }
            }
            else if (strcasecmp(s, "-sboverlaymono") == 0) {
                video::set_sboverlay_white(true);
            }
            // Playing Thayer's Quest, and don't want speech synthesis?
            else if (strcasecmp(s, "-nospeech") == 0) {
                thayers *game_thayers = dynamic_cast<thayers *>(g_game);

                if (game_thayers)
                    game_thayers->no_speech();
                else {
                    printerror(
                        "-nospeech: Switch not supported for this game...");
                    result = false;
                }
            } else if (strcasecmp(s, "-prefer_samples") == 0) {
                // If a game doesn't support "prefer samples" then it is not an
                // error.
                // That's why it is _prefer_ samples instead of _require_
                // samples :)
                g_game->set_prefer_samples(true);
            }

            else if (strcasecmp(s, "-noissues") == 0) {
                g_game->set_issues(NULL);
            }
            // check if we need to use the SDL software renderer
            else if (strcasecmp(s, "-nohwaccel") == 0) {
                g_game->m_sdl_software_rendering = true;
            }
            // check for any game-specific arguments ...
            else if (g_game->handle_cmdline_arg(s)) {
                // don't do anything in here, it has already been handled by the
                // game class ...
            }
            // check for any ldp-specific arguments ...
            else if (g_ldp->handle_cmdline_arg(s)) {
                // don't do anything in here, it has already been handled by the
                // ldp class ...
            } else {
                printerror("Unknown command line parameter or parameter value:");
                printerror(s);
                return false;
            }
        } // end for
    }     // end if we know our game type

    // if game or ldp was unknown
    else {
        result = false;
    }

    if (filter)
        if (g_game->get_manymouse())
            absolute_only();

    return (result);
}

// copies a single word from the command line into result (a word is any group
// of characters separate on both sides by whitespace)
// if there are no more words to be copied, result will contain a null string
void get_next_word(char *result, int result_size)
{
    // make sure we still have command line left to parse
    if (g_arg_index < g_argc) {
        strncpy(result, g_argv[g_arg_index], result_size-1);
        result[result_size - 1] = 0; // terminate end of string just in case we
                                     // hit the limit
        g_arg_index++;
    }

    // if we have no command line left to parse ...
    else {
        result[0] = 0; // return a null string
    }
}
