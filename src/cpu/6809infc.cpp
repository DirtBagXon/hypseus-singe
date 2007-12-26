// interface for mc6809.c
// by Mark Broadhead

#include <string.h>
#include "6809infc.h"
#include "cpu.h"
#include "../game/game.h"

#ifdef WIN32
#pragma warning (disable:4244)	// disable the warning about possible loss of data
#pragma warning (disable:4100) // disable warning about unreferenced parameter
#endif

Uint8 *g_cpumem = NULL;	// where this cpu's memory begins

// MATT : sets where the memory begins for the cpu core
void m6809_set_memory(Uint8 *cpumem)
{
	g_cpumem = cpumem;
}

static void FetchInstr(int addr, unsigned char fetch_buffer[])
{
    memcpy(fetch_buffer, &g_cpumem[addr], MC6809_FETCH_BUFFER_SIZE);
}

static int LoadByte(int addr)
{
	return (g_game->cpu_mem_read(static_cast<Uint16>(addr)) & 0xff);
}

static int LoadWord(int addr)
{
	unsigned char high_byte = (g_game->cpu_mem_read(static_cast<Uint16>(addr)) & 0xff);
	unsigned char low_byte = (g_game->cpu_mem_read(static_cast<Uint16>(addr + 1)) & 0xff);
	return ((high_byte << 8) | low_byte);
}

static void StoreByte(int addr, int value)
{
	g_game->cpu_mem_write(static_cast<Uint16>(addr & 0xffff), (value & 0xff));
}

static void StoreWord(int addr, int value)
{
	g_game->cpu_mem_write(static_cast<Uint16>(addr & 0xffff), ((value >> 8) & 0xff));
	g_game->cpu_mem_write(static_cast<Uint16>((addr + 1) & 0xffff), (value & 0xff));
}

// I don't know if we'll need this...
static int BiosCall(struct MC6809_REGS *regs)
{
    return 0x12;  /* NOP */
}

void initialize_m6809(void)
{
    
	MC6809_INTERFACE interface={	FetchInstr,
									LoadByte,
									LoadWord,
									StoreByte,
									StoreWord,
									BiosCall };
    mc6809_Init(&interface);
	mc6809_Reset();
}

void m6809_reset(void)
{
	mc6809_Reset();
}

