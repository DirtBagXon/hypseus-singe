#ifndef MAMEWRAP_H
#define MAMEWRAP_H

// header file to replace MAME source code
// most of this obviously borrowed from MAME source

///////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <SDL.h>		// need SDL to get data types (Uint8, etc)

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define LSB_FIRST	/* little endian */
#endif

extern int mame_debug;	// declare mame_debug somewhere

///////////////////////////////////////////
// from osd_cpu.h

typedef Uint8		UINT8;
typedef Uint8		BYTE;
typedef Uint16		UINT16;
typedef Uint16		WORD;
typedef Uint32		UINT32;
typedef Sint8 		INT8;
typedef Sint16		INT16;
typedef Sint32		INT32;

/* Combine to 32-bit integers into a 64-bit integer */
#define COMBINE_64_32_32(A,B)     ((((UINT64)(A))<<32) | (UINT32)(B))
#define COMBINE_U64_U32_U32(A,B)  COMBINE_64_32_32(A,B)

/* Return upper 32 bits of a 64-bit integer */
#define HI32_32_64(A)		  (((UINT64)(A)) >> 32)
#define HI32_U32_U64(A)		  HI32_32_64(A)

/* Return lower 32 bits of a 64-bit integer */
#define LO32_32_64(A)		  ((A) & 0xffffffff)
#define LO32_U32_U64(A)		  LO32_32_64(A)

#define DIV_64_64_32(A,B)	  ((A)/(B))
#define DIV_U64_U64_U32(A,B)  ((A)/(UINT32)(B))

#define MOD_32_64_32(A,B)	  ((A)%(B))
#define MOD_U32_U64_U32(A,B)  ((A)%(UINT32)(B))

#define MUL_64_32_32(A,B)	  ((A)*(INT64)(B))
#define MUL_U64_U32_U32(A,B)  ((A)*(UINT64)(UINT32)(B))

/***************************** Common types ***********************************/

/******************************************************************************
 * Union of UINT8, UINT16 and UINT32 in native endianess of the target
 * This is used to access bytes and words in a machine independent manner.
 * The upper bytes h2 and h3 normally contain zero (16 bit CPU cores)
 * thus PAIR.d can be used to pass arguments to the memory system
 * which expects 'int' really.
 ******************************************************************************/
typedef union {
#ifdef LSB_FIRST
	struct { UINT8 l,h,h2,h3; } b;
	struct { UINT16 l,h; } w;
#else
	struct { UINT8 h3,h2,h,l; } b;
	struct { UINT16 h,l; } w;
#endif
	UINT32 d;
}	PAIR;

///////////////////////////////////////
// from cpuintrf.h

/* daisy-chain link */
typedef struct {
	void (*reset)(int);             /* reset callback     */
	int  (*interrupt_entry)(int);   /* entry callback     */
	void (*interrupt_reti)(int);    /* reti callback      */
	int irq_param;                  /* callback paramater */
}	Z80_DaisyChain;

#define Z80_MAXDAISY	4		/* maximum of daisy chan device */

#define Z80_INT_REQ     0x01    /* interrupt request mask       */
#define Z80_INT_IEO     0x02    /* interrupt disable mask(IEO)  */

#define Z80_VECTOR(device,state) (((device)<<8)|(state))

#define CLEAR_LINE		0		/* clear (a fired, held or pulsed) line */
#define ASSERT_LINE     1       /* assert an interrupt immediately */
#define HOLD_LINE       2       /* hold interrupt line until enable is true */
#define PULSE_LINE		3		/* pulse interrupt line for one instruction */

#define MAX_REGS		128 	/* maximum number of register of any CPU */

/* Values passed to the cpu_info function of a core to retrieve information */
enum {
	CPU_INFO_REG,
	CPU_INFO_FLAGS=MAX_REGS,
	CPU_INFO_NAME,
	CPU_INFO_FAMILY,
	CPU_INFO_VERSION,
	CPU_INFO_FILE,
	CPU_INFO_CREDITS,
	CPU_INFO_REG_LAYOUT,
	CPU_INFO_WIN_LAYOUT
};


/*
 * This value is passed to cpu_get_reg to retrieve the previous
 * program counter value, ie. before a CPU emulation started
 * to fetch opcodes and arguments for the current instrution.
 */
#define REG_PREVIOUSPC	-1

/*
 * This value is passed to cpu_get_reg/cpu_set_reg, instead of one of
 * the names from the enum a CPU core defines for it's registers,
 * to get or set the contents of the memory pointed to by a stack pointer.
 * You can specify the n'th element on the stack by (REG_SP_CONTENTS-n),
 * ie. lower negative values. The actual element size (UINT16 or UINT32)
 * depends on the CPU core.
 * This is also used to replace the cpu_geturnpc() function.
 */
#define REG_PC -2
#define REG_SP -3
#define REG_SP_CONTENTS -4		// was -2	modified PAB for I86 core

///////////////////////////////////
// from mamedbg.h

#ifdef CPU_DEBUG
/***************************************************************************
 * Convenience macro for the CPU cores, this is defined to empty
 * if CPU_DEBUG is not specified, so a CPU core can simply add
 * CALL_MAME_DEBUG; before executing an instruction
 ***************************************************************************/
#define CALL_MAME_DEBUG if( mame_debug ) MAME_Debug()
#else
#define CALL_MAME_DEBUG
#endif

/* What EA address to set with debug_ea_info (origin) */
enum {
    EA_DST,
    EA_SRC
};

/* Size of the data element accessed (or the immediate value) */
enum {
    EA_DEFAULT,
    EA_INT8,
    EA_UINT8,
    EA_INT16,
    EA_UINT16,
    EA_INT32,
    EA_UINT32,
    EA_SIZE
};

/* Access modes for effective addresses to debug_ea_info */
enum {
    EA_NONE,        /* no EA mode */
    EA_VALUE,       /* immediate value */
    EA_ABS_PC,      /* change PC absolute (JMP or CALL type opcodes) */
    EA_REL_PC,      /* change PC relative (BRA or JR type opcodes) */
	EA_ZPG_RD,		/* read zero page memory */
	EA_ZPG_WR,		/* write zero page memory */
	EA_ZPG_RDWR,	/* read then write zero page memory */
    EA_MEM_RD,      /* read memory */
    EA_MEM_WR,      /* write memory */
    EA_MEM_RDWR,    /* read then write memory */
    EA_PORT_RD,     /* read i/o port */
    EA_PORT_WR,     /* write i/o port */
    EA_COUNT
};

/***************************************************************************
 * This function can (should) be called by a disassembler to set
 * information for the debugger. It sets the address, size and type
 * of a memory or port access, an absolute or relative branch or
 * an immediate value and at the same time returns a string that
 * contains a literal hex string for that address.
 * Later it could also return a symbol for that address and access.
 ***************************************************************************/
extern const char *set_ea_info( int what, unsigned address, int size, int acc );

/* Startup and shutdown functions; called from cpu_run */
extern void mame_debug_init(void);
extern void mame_debug_exit(void);

/* This is the main entry into the mame debugger */
extern void MAME_Debug(void);

////////////////////////////////////////////
// from memory.h

/* ----- typedefs for data and offset types ----- */
typedef UINT8			data8_t;
typedef UINT16			data16_t;
typedef UINT32			data32_t;
typedef UINT32			offs_t;

/***************************************************************************

	Global variables

***************************************************************************/

extern UINT8 			opcode_entry;		/* current entry for opcode fetching */
#if !defined (USE_M80) || defined (USE_MAME_Z80_DEBUGGER) || defined (USE_I86)
extern UINT8 *			OP_ROM;				/* opcode ROM base */
extern UINT8 *			OP_RAM;				/* opcode RAM base */
#endif

extern UINT8 *			cpu_bankbase[];		/* array of bank bases */
extern UINT8 *			readmem_lookup;		/* pointer to the readmem lookup table */
extern offs_t			memory_amask;		/* memory address mask */
extern offs_t			mem_amask;		/* memory address mask */
extern struct ExtMemory	ext_memory[];		/* externally-allocated memory */

// MPO : these macros are only used by mame cores, hence they should always be defined
#define cpu_readop(A)				(OP_ROM[(A) & memory_amask])
#define cpu_readop16(A)				(*(data16_t *)&OP_ROM[(A) & memory_amask])
#define cpu_readop32(A)				(*(data32_t *)&OP_ROM[(A) & memory_amask])
#define cpu_readop_arg(A)			(OP_RAM[(A) & memory_amask])
#define cpu_readop_arg16(A)			(*(data16_t *)&OP_RAM[(A) & memory_amask])
#define cpu_readop_arg32(A)			(*(data32_t *)&OP_RAM[(A) & memory_amask])

///////////////////////////////////
// from state.h

void state_save_UINT8(void *state, const char *module,int instance,
	const char *name, const UINT8 *val, unsigned size);
void state_save_INT8(void *state, const char *module,int instance,
	const char *name, const INT8 *val, unsigned size);
void state_save_UINT16(void *state, const char *module,int instance,
	const char *name, const UINT16 *val, unsigned size);
void state_save_INT16(void *state, const char *module,int instance,
	const char *name, const INT16 *val, unsigned size);

void state_load_UINT8(void *state, const char *module,int instance,
	const char *name, UINT8 *val, unsigned size);
void state_load_INT8(void *state, const char *module,int instance,
	const char *name, INT8 *val, unsigned size);
void state_load_UINT16(void *state, const char *module,int instance,
	const char *name, UINT16 *val, unsigned size);
void state_load_INT16(void *state, const char *module,int instance,
	const char *name, INT16 *val, unsigned size);


//////////////////////////////////////////
// other schlop

#ifdef WIN32
#define INLINE __inline // MSVC++
#else
#define INLINE inline	// gcc
#endif

void set_amask(offs_t amask_val);	// for overriding the default address mask

int generic_z80_irq_callback(int irqline);
Sint32 generic_m80_irq_callback(int);
Sint32 generic_m80_nmi_callback();
void generic_z80_reset();
void mw_z80_set_mem(Uint8 *mem);
void mw_i86_set_mem(Uint8 *mem);

/////////////////////////////

#include "../game/game.h"
// included to make sure that g_game is defined, for the following macros

// MPO : changed all of these to macros to eliminate (possible) function call overhead in case compiler doesn't inline functions
#define cpu_readmem16(addr) g_game->cpu_mem_read(static_cast<Uint16>(addr))
#define cpu_readmem20(addr) g_game->cpu_mem_read(static_cast<Uint32>(addr))
#define cpu_writemem16(addr,value) g_game->cpu_mem_write(static_cast<Uint16>(addr), value)
#define cpu_writemem20(addr,value) g_game->cpu_mem_write(static_cast<Uint32>(addr), value)
#define cpu_readport16(port) g_game->port_read(port)
#define cpu_writeport16(port,value) g_game->port_write(port, value)
#define change_pc16(new_pc) g_game->update_pc(new_pc)
#define change_pc20(new_pc) g_game->update_pc(new_pc)

/////////////////////////////

unsigned char cpu_getactivecpu();

// from osdepend.h
void logerror(const char *text,...);

// don't know what this is, but i86dasm wants it
#define CLIB_DECL

#endif	// end MAMEWRAP_H
