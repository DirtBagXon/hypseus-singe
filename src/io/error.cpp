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

// error.cpp
// hypseus error handling

#include "config.h"

#ifdef WIN32
#include <windows.h>
#endif

#include <SDL.h>
#include <plog/Log.h>
#include <string>

#include "../sound/sound.h"

static bool g_usage = false;

// notifies the user of an error that has occurred
void printerror(const char *s)
{
    if (g_usage) return;
    static bool alert = true;

    if (alert && sound::get_initialized()) {
        sound::play_saveme();
        SDL_Delay(1000);
        alert = false;
    }

#ifdef WIN32
    MessageBox(NULL, s, "Encountered an error", MB_OK | MB_ICONERROR);
#else
    LOGE << s;
#endif

    if (!plog::get())
#ifdef LINUX
        fprintf(stderr, "[ error ] \033[31;1m%s\033[0m\n", s);
#elif __APPLE__
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", s, NULL);
#else
        return;
#endif
}

// prints a notice to the screen
void printnotice(const char *s)
{
    LOGW << s;
}

// notifies user that the game does not work correctly and gives a reason
// this should be called after video has successfully been initialized
void printnowookin(const char *s)
{
    printnotice(s);
}

void printusage()
{
  const std::string version = hypseus_VERSION;
  const std::string usage = R"USAGE(
  Hypseus Singe (c) 2021-2026 DirtBagXon )USAGE" + version + R"USAGE(

  Usage: hypseus <game> vldp -framefile <framefile.txt> ...

  The above arguments are, at minimum, required:

      - <game> consists of a supported Daphne ROM or 'singe'

      - Singe requires the '-script <game.singe>' argument
        <game.singe> defines the startup Singe LUA file.

        Zipped LUA ROM files are supported in Singe via:

        -zlua <game.zip>

      - Other common arguments are:

        -bezel <game.png>
        -fastboot
        -fullscreen
        -gamepad
        -linear_scale

      - Common Singe arguments:

        -script <game.singe>  : (One argument is required)
        -zlua <game.zip>

        -usealt <game>
        -blend_sprites
        -manymouse

  See documentation for a full list of available arguments:

  Visit: https://github.com/DirtBagXon/hypseus-singe

  Use -version to display build information.

)USAGE";

#ifdef WIN32
    MessageBox(NULL, usage.c_str(), "Usage", MB_OK | MB_ICONINFORMATION);
#elif __APPLE__
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Usage", usage.c_str(), NULL);
#else
    fprintf(stdout, "%s", usage.c_str());
#endif

    g_usage = true;
}
