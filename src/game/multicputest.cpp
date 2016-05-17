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

// loads two small programs and runs them in parallel to test multi z80 support

#include "config.h"

#include <stdio.h>
#include <string.h> // for memset
#include "multicputest.h"
#include "../cpu/generic_z80.h"
#include "../io/conout.h"
#include "../timer/timer.h"

mcputest::mcputest()
{
    struct cpu::def cpu;

    m_shortgamename = "mcputest";
    memset(&cpu, 0, sizeof(struct cpu::def)); // clear struct for safety purposes
    cpu.type              = cpu::type::Z80;
    cpu.hz                = 4000000; // cpu speed is irrelevant
    cpu.irq_period[0]     = 1000.0;  // 1 IRQ every second
    cpu.nmi_period        = 500.0;   // 2 NMIs per second
    cpu.initial_pc        = 0;
    cpu.must_copy_context = true;
    cpu.mem = m_cpumem;
    cpu::add(&cpu); // add this cpu to the list

    m_game_uses_video_overlay = false;

    memset(&cpu, 0, sizeof(struct cpu::def)); // for safety purposes
    cpu.type              = cpu::type::Z80;
    cpu.hz                = 5000000; // cpu speed is irrelevant
    cpu.irq_period[0]     = 2000.0;  // 1 IRQ every second
    cpu.nmi_period        = 1000.0;  // 2 NMIs per second
    cpu.initial_pc        = 0;
    cpu.must_copy_context = true;
    cpu.mem = m_cpumem2;
    cpu::add(&cpu); // add this cpu to the list

    const static struct rom_def multicputest_roms[] =
        {{"prog1.bin", "cputest", &m_cpumem[0], 113, 0xAEFCA341},
         {"prog2.bin", "cputest", &m_cpumem2[0], 113, 0xC6C8CE9},
         {NULL}};

    m_rom_list = multicputest_roms;
}

void mcputest::do_irq(unsigned int which_irq)
{
    if (which_irq == 0) {
        Z80_ASSERT_IRQ;
    } else {
        printline("Illegal IRQ asserted!");
    }
}

void mcputest::do_nmi() { Z80_ASSERT_NMI; }

Uint8 mcputest::cpu_mem_read(Uint16 addr)
{
    // cpu #0
    if (cpu::get_active() == 0) {
        return m_cpumem[addr];
    }
    // cpu #1
    else {
        return m_cpumem2[addr];
    }
}

void mcputest::cpu_mem_write(Uint16 addr, Uint8 value)
{
    // cpu #0
    if (cpu::get_active() == 0) {
        m_cpumem[addr] = value;
    }
    // cpu #1
    else {
        m_cpumem2[addr] = value;
    }
}

void mcputest::port_write(Uint16 Port, Uint8 Value)
{

    char s[81] = {0};
    Port &= 0xFF; // strip off high byte
    static Uint8 old_val[2][2] = {{255, 255}, {1, 1}};
    Uint8 expected_val[2][2]   = {{0}};
    Uint8 which_cpu            = cpu::get_active();

    if (Port > 1) {
        printline("WTF?");
        return;
    }

    printline("CPU #%u sends %u to port %u ... ", which_cpu, Value, Port);

    if (which_cpu == 0) {
        expected_val[0][Port] = (Uint8)(old_val[0][Port] + 1);
    } else {
        expected_val[1][Port] = (Uint8)(old_val[1][Port] - 1);
    }

    old_val[which_cpu][Port] = Value;

    if (expected_val[which_cpu][Port] == Value) {
        printline("CORRECT!");
    } else {
        sprintf(s, "*** INCORRECT *** Expecting %u", expected_val[which_cpu][Port]);
        printline(s);
    }
}
