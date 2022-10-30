/*
 * ____ DAPHNE COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2022 DirtBagXon
 *
 * This file is part of HYPSEUS, a laserdisc arcade game emulator
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

#ifndef USB_UTIL_H
#define USB_UTIL_H

#define ASCI(N) (N + 0x030)

#define SCOREBOARD  0x00000
#define ANNUNCIATOR 0x00001
#define STARTDELAY  0x0000f
#define BOOTCYCLE   0x0002c
#define BOOTBYPASS  0x3c668
#define LOWBAUD     0x04b00

#define vla  int('a')
#define vle  int('e')
#define vlh  int('h')
#define vll  int('l')
#define vlp  int('p')
#define spc  int(' ')
#define dsh  int('-')

typedef struct
{
   char unit;
   char digit;
   char value;
} DigitStruct;

bool g_usb_connected();

void write_usb_serial(DigitStruct);

#endif
