/*
 * ____ VLDP COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2026 DirtBagXon
 *
 * This file is part of VLDP, a virtual laserdisc player.
 *
 * VLDP is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * VLDP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef VLDP_CUSTMPEG_H
#define VLDP_CUSTMPEG_H

struct DiscCustFrameRate
{
    int fps;
    unsigned int fpks;
};

static constexpr DiscCustFrameRate g_disc_custom_rates[] =
{
    {10, 10000},
    {12, 12000},
    {15, 15000},
    {16, 16000},
    {20, 20000},
    {35, 35000}
};

bool force_mpeg_rate(int);
int get_mpeg_rate();

#endif
