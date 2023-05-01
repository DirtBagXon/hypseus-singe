/*
* ____ DAPHNE COPYRIGHT NOTICE ____
*
* Copyright (C) 2023 DirtBagXon
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

/*
* This is SINGE - the Somewhat Interactive Nostalgic Game Engine!
*/

#include "../singe.h"

#include "../../scoreboard/scoreboard_collection.h"

int singe::scoreboard_format(int value)
{
    uint8_t digit;

    switch (value) {
    case -1:
       digit = 0xf; // off
       break;
    case -2:
       digit = 0xa; // -
       break;
    case -3:
       if (g_bezelboard.type == SINGE_SB_USB)
           digit = 0xf;
       else
           digit = 0xb; // blank
       break;
    case -4:
       digit = 0xc; // H
       break;
    case -5:
       digit = 0xd; // A
       break;
    default:
       digit = 0;
       break;
    }
    return digit;
}

void singe::scoreboard_score(int value, uint8_t player)
{
    uint8_t digit;
    const uint8_t which = (6 - 1); // six chars
    for(int i = which; i >= 0; i--) {
        if (value < 0)
            digit = scoreboard_format(value);
        else
        {
            digit = value % 10;
            value = value / 10;
        }
        m_pScoreboard->update_player_score(i, digit, player);
    }
}

void singe::scoreboard_lives(int value, uint8_t player)
{
    uint8_t digit;

    if (value < 0) digit = scoreboard_format(value);
    else digit = value % 10;

    m_pScoreboard->update_player_lives(digit, player);
}

void singe::scoreboard_credits(uint8_t value)
{
    const uint8_t which = (2 - 1); // two chars
    for(int i = which; i >= 0; i--) {
        uint8_t digit = value % 10;
        value = value / 10;
        m_pScoreboard->update_credits(i, digit);
    }
}
