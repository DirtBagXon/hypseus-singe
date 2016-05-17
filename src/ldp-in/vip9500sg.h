/*
 * ____ DAPHNE COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2001 Mark Broadhead
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

// VIP9500SG.H

#ifndef VIP9500SG_H
#define VIP9500SG_H

#define VIP9500SG_SEARCH 0x01
#define VIP9500SG_SKIP_FORWARD 0x02
#define VIP9500SG_SKIP_BACKWARD 0x04

namespace vip9500sg
{
unsigned char read();
void write(unsigned char value);
void enter(void);
bool result_ready(void);
int stack_push(unsigned char value);
void add_digit(char);
}

#endif
