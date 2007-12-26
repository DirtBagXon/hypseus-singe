/*
** nester - NES emulator
** Copyright (C) 2000  Darren Ranalli
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

// NES_6502 interface class
// created to cut down on extra work required for retro-fitting of
// new releases of Matt Conte's nes6502

#ifndef _NES_6502_H_
#define _NES_6502_H_

#include "types.h"

#include "nes6502.h"

class NES;  // prototype of NES class

class NES_6502
{
public:
  struct Context : public nes6502_context {};

public:
  NES_6502();
  ~NES_6502();

  // Functions that govern the 6502's execution
  void Init()                           { /*nes6502_init();*/ }
  static void Reset()                          { nes6502_reset(); }
  int  Execute(int total_cycles)        { return nes6502_execute(total_cycles); }
  void DoNMI(void)                      { nes6502_nmi(); }
  void DoIRQ(void)                      { nes6502_irq(); }
  void SetDMA(int cycles)               { nes6502_burn(cycles); }
  uint8  GetByte(uint32 address)        { return nes6502_getbyte(address); }
  uint32 GetCycles(boolean reset_flag)  { return nes6502_getcycles(reset_flag); }

  // Context get/set
  void SetContext(Context *cpu);
  void GetContext(Context *cpu);

protected:

  NES* ParentNES;

  uint8 MemoryRead(uint32 addr);
  void  MemoryWrite(uint32 addr, uint8 data);

  friend void NES_write(uint32 address, uint8 value);
  friend uint8 NES_read(uint32 address);

};

#endif
