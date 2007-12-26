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
// modified for daphne by Mark Broadhead

#include "nes_6502.h"
//#include "NES.h"
#include <stdio.h>
//#include "debug.h"
#include "../game/game.h"

// NOT SAFE FOR MULTIPLE NES_6502'S
static NES_6502 *NES_6502_nes = NULL;

// MATT : removed the 'static' on here in order to compile in linux
void NES_write(uint32 address, uint8 value)
{
  NES_6502_nes->MemoryWrite(address, value);
}

// MATT : removed the 'static' on here in order to compile in linux
uint8 NES_read(uint32 address)
{
  return NES_6502_nes->MemoryRead(address);
}

static nes6502_memread NESReadHandler[] =
{
   { 0x0000, 0xFFFF, NES_read },
   { (uint32) -1,     (uint32) -1,     NULL }
};

static nes6502_memwrite NESWriteHandler[] =
{
   { 0x0000, 0xFFFF, NES_write },
   { (uint32) -1,    (uint32) -1,     NULL}
};


NES_6502::NES_6502() 
{
  if(NES_6502_nes) throw "error: multiple NES_6502's";

  try {
    NES_6502_nes = this;

    Init();
  } catch(...) {
    NES_6502_nes = NULL;
    throw;
  }
}

NES_6502::~NES_6502()
{
  NES_6502_nes = NULL;
}

// Context get/set
void NES_6502::SetContext(Context *cpu)
{
//  ASSERT(0x00000000 == (cpu->pc_reg & 0xFFFF0000));
  cpu->read_handler = NESReadHandler;
  cpu->write_handler = NESWriteHandler;
  nes6502_setcontext(cpu);
}

void NES_6502::GetContext(Context *cpu)
{
  nes6502_getcontext(cpu);
  cpu->read_handler = NESReadHandler;
  cpu->write_handler = NESWriteHandler;
}
/*
uint8 NES_6502::MemoryRead(uint32 addr)
{
  return ParentNES->MemoryRead(addr);
}

void NES_6502::MemoryWrite(uint32 addr, uint8 data)
{
  ParentNES->MemoryWrite(addr, data);
}
*/
uint8 NES_6502::MemoryRead(uint32 addr)
{
  return g_game->cpu_mem_read(static_cast<uint16>(addr & 0xffff));
}

void NES_6502::MemoryWrite(uint32 addr, uint8 data)
{
  g_game->cpu_mem_write(static_cast<uint16>(addr & 0xffff), data);
}
