
// mamewrap.c
// thrown together quickly by Matt Ownby

// the purpose of this code it to use cpu cores from MAME without modifying the CPU core source
// code (or at least with minimal modification)
// this file contains wrapper functions for functions that the mame cpu cores expect.

// at the time of this writing (Jan 6, 2001) there are enough wrappers for the z80 core to work.
// Additional code might be needed to add other mame cpu cores (such as the x86 one)

#ifdef WIN32
#pragma warning (disable:4100)	// disable the warning about unreferenced formal parameters (MSVC++)
#endif

#include <stdio.h>

#include "generic_z80.h"
#include "x86/i86.h"
#include "mamewrap.h"

// these globals may be used by mame cpu's, it doesn't hurt to have them defined
UINT8 *	OP_ROM = NULL;							/* opcode ROM base */
UINT8 *	OP_RAM = NULL;							/* opcode RAM base */

offs_t	memory_amask = 0xFFFFFFFF;					/* memory address mask */
offs_t	mem_amask = 0xFFFFFFFF;						/* memory address mask */

int mame_debug = 1;	// whether our debugger is running

///////////////////////////////////////////////////

// each cpu may have a different amask
void set_amask(offs_t amask_val)
{
	memory_amask = amask_val;
	mem_amask = amask_val;
}

// IRQ callback that gets executed when the ISR is called.  It immediately clears the IRQ line so
// that normal program execution can continue when the ISR returns

#ifdef USE_M80
int generic_m80_irq_callback(int irqline)
{
	m80_set_irq_line(CLEAR_LINE);
	return 0xFF;
}
#else
int generic_z80_irq_callback(int irqline)
{

	z80_set_irq_line(0, CLEAR_LINE);

    return 0xff;
}
#endif

int generic_m80_nmi_callback()
{
	m80_set_nmi_line(CLEAR_LINE);
	return 0;
}

#ifndef USE_M80
// generic function to set the memory location for the mame z80 core (even though we don't use it)
void mw_z80_set_mem(Uint8 *mem)
{
	OP_RAM = mem;	// set memory pointer to the beginning of our RAM
	OP_ROM = mem;
}
#endif

void mw_i86_set_mem(Uint8 *mem)
{
	OP_RAM = mem;	// set memory pointer to the beginning of our RAM
	OP_ROM = mem;
}

/*
// reads a byte from a memory location
UINT8 cpu_readmem16(UINT32 addr)
{

	return(g_game->cpu_mem_read(static_cast<Uint16>(addr)));

}

// reads a byte from a memory location
UINT8 cpu_readmem20(UINT32 addr)
{
	return(g_game->cpu_mem_read(addr));
}

// writes a byte to a memory location
void cpu_writemem16(UINT32 addr, UINT8 value)
{
	g_game->cpu_mem_write(static_cast<Uint16>(addr), value);
}

// writes a byte to a memory location
void cpu_writemem20(UINT32 addr, UINT8 value)
{
	g_game->cpu_mem_write(addr, value);
}

// reads data from a port
UINT8 cpu_readport16(UINT16 port)
{
	return(g_game->port_read(port));
}

// outputs data to a port
void cpu_writeport16(UINT16 port, UINT8 value)
{
	g_game->port_write(port, value);
}

// notifies us that the PC has changed, but it doesn't notify us every time =(
void change_pc16(UINT32 new_pc)
{
	g_game->update_pc(new_pc);
}

void change_pc20(UINT32 new_pc)
{
	g_game->update_pc(new_pc);
}
*/

///////////////////////////////////////////////////////////////////////////
// the rest of these functions are unsupported and do nothing


void logerror(const char *text, ...)
{

}

void state_save_UINT8(void *state, const char *module,int instance,
	const char *name, const UINT8 *val, unsigned size)
{
}

void state_save_INT8(void *state, const char *module,int instance,
	const char *name, const INT8 *val, unsigned size)
{
}

void state_save_UINT16(void *state, const char *module,int instance,
	const char *name, const UINT16 *val, unsigned size)
{
}

void state_save_INT16(void *state, const char *module,int instance,
	const char *name, const INT16 *val, unsigned size)
{
}

void state_load_UINT8(void *state, const char *module,int instance,
	const char *name, UINT8 *val, unsigned size)
{
}

void state_load_INT8(void *state, const char *module,int instance,
	const char *name, INT8 *val, unsigned size)
{
}

void state_load_UINT16(void *state, const char *module,int instance,
	const char *name, UINT16 *val, unsigned size)
{
}

void state_load_INT16(void *state, const char *module,int instance,
	const char *name, INT16 *val, unsigned size)
{
}
