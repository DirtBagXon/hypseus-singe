/*
** Nofrendo (c) 1998-2000 Matthew Conte (matt@conte.com)
**
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of version 2 of the GNU Library General 
** Public License as published by the Free Software Foundation.
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
**
**
** nes6502.c
**
** NES custom 6502 (2A03) CPU implementation
** $Id$
*/

/*
** changes for nester:
** changes are marked with DCR
**
** removed dis6502 #include
** nullified ASSERT
** nullified INLINE
**
** - Darren Ranalli
*/

/*
** changes for hypseus:
**
** commented out the stuff in the memory read/write
** functions marked as TODO: following cases are N2A03-specific
**
** - Mark Broadhead
**/ 

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>	// for memcpy
#include "nes6502.h"
//#include "dis6502.h" //DCR

#ifdef TARGET_CPU_PPC
// for the __lhbrx instruction
#include "ppc_intrinsics.h"
#endif

#include "cpu-debug.h"	// MPO

//DCR
#define ASSERT(CONDITION)

//DCR
#define INLINE

//#define  NES6502_DISASM

// MPO : we don't want to use the jumptable if we're in CPU_DEBUG mode because otherwise we'll never break
#if defined(__GNUC__) && !defined(NES6502_DISASM) && !defined(CPU_DEBUG)
#define  NES6502_JUMPTABLE
#endif /* __GNUC__ */


#define  ADD_CYCLES(x) \
{ \
   remaining_cycles -= (x); \
   cpu.total_cycles += (x); \
}

/*
** Check to see if an index reg addition overflowed to next page
*/
#define PAGE_CROSS_CHECK(addr, reg) \
{ \
   if ((reg) > (uint8_t) (addr)) \
      ADD_CYCLES(1); \
}

#define EMPTY_READ(value)  /* empty */

/*
** Addressing mode macros
*/

/* Immediate */
#define IMMEDIATE_BYTE(value) \
{ \
   value = bank_readbyte(PC++); \
}

/* Absolute */
#define ABSOLUTE_ADDR(address) \
{ \
   address = bank_readword(PC); \
   PC += 2; \
}

#define ABSOLUTE(address, value) \
{ \
   ABSOLUTE_ADDR(address); \
   value = mem_readbyte(address); \
}

#define ABSOLUTE_BYTE(value) \
{ \
   ABSOLUTE(temp, value); \
}

/* Absolute indexed X */
#define ABS_IND_X_ADDR(address) \
{ \
   ABSOLUTE_ADDR(address); \
   address = (address + X) & 0xFFFF; \
   PAGE_CROSS_CHECK(address, X); \
}

#define ABS_IND_X(address, value) \
{ \
   ABS_IND_X_ADDR(address); \
   value = mem_readbyte(address); \
}

#define ABS_IND_X_BYTE(value) \
{ \
   ABS_IND_X(temp, value); \
}

/* Absolute indexed Y */
#define ABS_IND_Y_ADDR(address) \
{ \
   ABSOLUTE_ADDR(address); \
   address = (address + Y) & 0xFFFF; \
   PAGE_CROSS_CHECK(address, Y); \
}

#define ABS_IND_Y(address, value) \
{ \
   ABS_IND_Y_ADDR(address); \
   value = mem_readbyte(address); \
}

#define ABS_IND_Y_BYTE(value) \
{ \
   ABS_IND_Y(temp, value); \
}

/* Zero-page */
#define ZERO_PAGE_ADDR(address) \
{ \
   IMMEDIATE_BYTE(address); \
}

#define ZERO_PAGE(address, value) \
{ \
   ZERO_PAGE_ADDR(address); \
   value = ZP_READBYTE(address); \
}

#define ZERO_PAGE_BYTE(value) \
{ \
   ZERO_PAGE(btemp, value); \
}

/* Zero-page indexed X */
#define ZP_IND_X_ADDR(address) \
{ \
   ZERO_PAGE_ADDR(address); \
   address += X; \
}

#define ZP_IND_X(address, value) \
{ \
   ZP_IND_X_ADDR(address); \
   value = ZP_READBYTE(address); \
}

#define ZP_IND_X_BYTE(value) \
{ \
   ZP_IND_X(btemp, value); \
}

/* Zero-page indexed Y */
/* Not really an adressing mode, just for LDx/STx */
#define ZP_IND_Y_ADDR(address) \
{ \
   ZERO_PAGE_ADDR(address); \
   address += Y; \
}

#define ZP_IND_Y_BYTE(value) \
{ \
   ZP_IND_Y_ADDR(btemp); \
   value = ZP_READBYTE(btemp); \
}  

/* Indexed indirect */
#define INDIR_X_ADDR(address) \
{ \
   IMMEDIATE_BYTE(btemp); \
   btemp += X; \
   address = zp_readword(btemp); \
}

#define INDIR_X(address, value) \
{ \
   INDIR_X_ADDR(address); \
   value = mem_readbyte(address); \
} 

#define INDIR_X_BYTE(value) \
{ \
   INDIR_X(temp, value); \
}

/* Indirect indexed */
#define INDIR_Y_ADDR(address) \
{ \
   IMMEDIATE_BYTE(btemp); \
   address = (zp_readword(btemp) + Y) & 0xFFFF; \
   PAGE_CROSS_CHECK(address, Y); \
}

#define INDIR_Y(address, value) \
{ \
   INDIR_Y_ADDR(address); \
   value = mem_readbyte(address); \
} 

#define INDIR_Y_BYTE(value) \
{ \
   INDIR_Y(temp, value); \
}



/* Stack push/pull */
#define  PUSH(value)             stack[S--] = (uint8_t) (value)
#define  PULL()                  stack[++S]


/*
** flag register helper macros
*/

/* Theory: Z and N flags are set in just about every
** instruction, so we will just store the value in those
** flag variables, and mask out the irrelevant data when
** we need to check them (branches, etc).  This makes the
** zero flag only really be 'set' when z_flag == 0.
** The rest of the flags are stored as true booleans.
*/

/* Scatter flags to separate variables */
#define  SCATTER_FLAGS(value) \
{ \
   n_flag = (value) & N_FLAG6502; \
   v_flag = (value) & V_FLAG6502; \
   b_flag = (value) & B_FLAG6502; \
   d_flag = (value) & D_FLAG6502; \
   i_flag = (value) & I_FLAG6502; \
   z_flag = !((value) & Z_FLAG6502); \
   c_flag = (value) & C_FLAG6502; \
}

/* Combine flags into flag register */
#define  COMBINE_FLAGS() \
( \
   (n_flag & N_FLAG6502) | \
   (v_flag ? V_FLAG6502 : 0) | \
   R_FLAG6502 | \
   (b_flag ? B_FLAG6502 : 0) | \
   (d_flag ? D_FLAG6502 : 0) | \
   (i_flag ? I_FLAG6502 : 0) | \
   (z_flag ? 0 : Z_FLAG6502) | \
   (c_flag ? C_FLAG6502 : 0) \
)

/* Set N and Z flags based on given value */
#define  SET_NZ_FLAGS(value)     n_flag = z_flag = (value);

/* For BCC, BCS, BEQ, BMI, BNE, BPL, BVC, BVS */
#define RELATIVE_BRANCH(condition) \
{ \
   if (condition) \
   { \
      IMMEDIATE_BYTE(btemp); \
      if (((int8_t) btemp + (PC & 0x00FF)) & 0x100) \
         ADD_CYCLES(1); \
      ADD_CYCLES(3); \
      PC += ((int8_t) btemp); \
   } \
   else \
   { \
      PC++; \
      ADD_CYCLES(2); \
   } \
}

#define JUMP(address) \
{ \
   PC = bank_readword((address)); \
}

/*
** Interrupt macros
*/
#define NMI_PROC() \
{ \
   PUSH(PC >> 8); \
   PUSH(PC & 0xFF); \
   b_flag = 0; \
   PUSH(COMBINE_FLAGS()); \
   i_flag = 1; \
   JUMP(NMI_VECTOR); \
}

#define IRQ_PROC() \
{ \
   PUSH(PC >> 8); \
   PUSH(PC & 0xFF); \
   b_flag = 0; \
   PUSH(COMBINE_FLAGS()); \
   i_flag = 1; \
   JUMP(IRQ_VECTOR); \
}

#define NMI() \
{ \
   NMI_PROC(); \
   ADD_CYCLES(INT_CYCLES); \
}

#define IRQ() \
{ \
   IRQ_PROC(); \
   ADD_CYCLES(INT_CYCLES); \
}

/*
** Instruction macros
*/

/* Warning! NES CPU has no decimal mode, so by default this does no BCD! */
#ifdef NES6502_DECIMAL
#define ADC(cycles, read_func) \
{ \
   read_func(data); \
   if (d_flag) \
   { \
      temp = (A & 0x0F) + (data & 0x0F) + (c_flag ? 1 : 0); \
      if (temp >= 10) \
         temp = (temp - 10) | 0x10; \
      temp += (A & 0xF0) + (data & 0xF0); \
      z_flag = (A + data + (c_flag ? 1 : 0)) & 0xFF; \
      n_flag = temp; \
      v_flag = ((~(A ^ data)) & (A ^ temp) & 0x80); \
      if (temp > 0x9F) \
         temp += 0x60; \
      c_flag = (temp > 0xFF); \
      A = (uint8_t) temp; \
   } \
   else \
   { \
      temp = A + data + (c_flag ? 1 : 0); \
      c_flag = (temp > 0xFF); \
      v_flag = ((~(A ^ data)) & (A ^ temp) & 0x80); \
      A = (uint8_t) temp; \
      SET_NZ_FLAGS(A); \
   }\
   ADD_CYCLES(cycles); \
}
#else
#define ADC(cycles, read_func) \
{ \
   read_func(data); \
   temp = A + data + (c_flag ? 1 : 0); \
   c_flag = (temp > 0xFF); \
   v_flag = ((~(A ^ data)) & (A ^ temp) & 0x80); \
   A = (uint8_t) temp; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}
#endif /* NES6502_DECIMAL */

/* undocumented */
#define ANC(cycles, read_func) \
{ \
   read_func(data); \
   A &= data; \
   SET_NZ_FLAGS(A); \
   c_flag = (n_flag & N_FLAG6502) ? 1 : 0; \
   ADD_CYCLES(cycles); \
}

#define AND(cycles, read_func) \
{ \
   read_func(data); \
   A &= data; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}

/* undocumented */
#define ANE(cycles, read_func) \
{ \
   read_func(data); \
   A = (A | 0xEE) & X & data; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}

/* undocumented */
#ifdef NES6502_DECIMAL
#define ARR(cycles, read_func) \
{ \
   read_func(data); \
   data &= A; \
   if (d_flag) \
   { \
      temp = (data >> 1) | (c_flag ? 0x80 : 0); \
      SET_NZ_FLAGS(temp); \
      v_flag = (temp ^ data) & 0x40; \
      if (((data & 0x0F) + (data & 0x01)) > 5) \
         temp = (temp & 0xF0) | ((temp + 0x6) & 0x0F); \
      if (((data & 0xF0) + (data & 0x10)) > 0x50) \
      { \
         temp = (temp & 0x0F) | ((temp + 0x60) & 0xF0); \
         c_flag = 1; \
      } \
      else \
         c_flag = 0; \
      A = (uint8_t) temp; \
   } \
   else \
   { \
      A = (data >> 1) | (c_flag ? 0x80 : 0); \
      SET_NZ_FLAGS(A); \
      c_flag = A & 0x40; \
      v_flag = ((A >> 6) ^ (A >> 5)) & 1; \
   }\
   ADD_CYCLES(cycles); \
}
#else
#define ARR(cycles, read_func) \
{ \
   read_func(data); \
   data &= A; \
   A = (data >> 1) | (c_flag ? 0x80 : 0); \
   SET_NZ_FLAGS(A); \
   c_flag = A & 0x40; \
   v_flag = ((A >> 6) ^ (A >> 5)) & 1; \
   ADD_CYCLES(cycles); \
}
#endif /* NES6502_DECIMAL */

#define ASL(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   c_flag = data & 0x80; \
   data <<= 1; \
   write_func(addr, data); \
   SET_NZ_FLAGS(data); \
   ADD_CYCLES(cycles); \
}

#define ASL_A() \
{ \
   c_flag = A & 0x80; \
   A <<= 1; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(2); \
}

/* undocumented */
#define ASR(cycles, read_func) \
{ \
   read_func(data); \
   data &= A; \
   c_flag = data & 0x01; \
   A = data >> 1; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}

#define BCC() \
{ \
   RELATIVE_BRANCH(0 == c_flag); \
}

#define BCS() \
{ \
   RELATIVE_BRANCH(0 != c_flag); \
}

#define BEQ() \
{ \
   RELATIVE_BRANCH(0 == z_flag); \
}

#define BIT(cycles, read_func) \
{ \
   read_func(data); \
   z_flag = data & A; \
   /* move bit 7/6 of data into N/V flags */ \
   n_flag = data; \
   v_flag = data & V_FLAG6502; \
   ADD_CYCLES(cycles); \
}

#define BMI() \
{ \
   RELATIVE_BRANCH(n_flag & N_FLAG6502); \
}

#define BNE() \
{ \
   RELATIVE_BRANCH(0 != z_flag); \
}

#define BPL() \
{ \
   RELATIVE_BRANCH(0 == (n_flag & N_FLAG6502)); \
}

/* Software interrupt type thang */
#define BRK() \
{ \
   PC++; \
   PUSH(PC >> 8); \
   PUSH(PC & 0xFF); \
   b_flag = 1; \
   PUSH(COMBINE_FLAGS()); \
   i_flag = 1; \
   JUMP(IRQ_VECTOR); \
   ADD_CYCLES(7); \
}

#define BVC() \
{ \
   RELATIVE_BRANCH(0 == v_flag); \
}

#define BVS() \
{ \
   RELATIVE_BRANCH(0 != v_flag); \
}

#define CLC() \
{ \
   c_flag = 0; \
   ADD_CYCLES(2); \
}

#define CLD() \
{ \
   d_flag = 0; \
   ADD_CYCLES(2); \
}

#define CLI() \
{ \
   i_flag = 0; \
   ADD_CYCLES(2); \
   if (cpu.int_pending && (remaining_cycles > 0)) \
   { \
      IRQ(); \
      cpu.int_pending = 0; \
   } \
}

#define CLV() \
{ \
   v_flag = 0; \
   ADD_CYCLES(2); \
}

#define _COMPARE(reg, value) \
{ \
   temp = (reg) - (value); \
   /* C is clear when data > A */ \
   c_flag = (0 == (temp & 0x100)); \
   SET_NZ_FLAGS((uint8_t) temp); \
}

#define CMP(cycles, read_func) \
{ \
   read_func(data); \
   _COMPARE(A, data); \
   ADD_CYCLES(cycles); \
}

#define CPX(cycles, read_func) \
{ \
   read_func(data); \
   _COMPARE(X, data); \
   ADD_CYCLES(cycles); \
}

#define CPY(cycles, read_func) \
{ \
   read_func(data); \
   _COMPARE(Y, data); \
   ADD_CYCLES(cycles); \
}

/* undocumented */
#define DCP(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   data--; \
   write_func(addr, data); \
   CMP(cycles, EMPTY_READ); \
}

#define DEC(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   data--; \
   write_func(addr, data); \
   SET_NZ_FLAGS(data); \
   ADD_CYCLES(cycles); \
}

#define DEX() \
{ \
   X--; \
   SET_NZ_FLAGS(X); \
   ADD_CYCLES(2); \
}

#define DEY() \
{ \
   Y--; \
   SET_NZ_FLAGS(Y); \
   ADD_CYCLES(2); \
}

/* undocumented (double-NOP) */
#define DOP(cycles) \
{ \
   PC++; \
   ADD_CYCLES(cycles); \
}

#define EOR(cycles, read_func) \
{ \
   read_func(data); \
   A ^= data; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}

#define INC(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   data++; \
   write_func(addr, data); \
   SET_NZ_FLAGS(data); \
   ADD_CYCLES(cycles); \
}

#define INX() \
{ \
   X++; \
   SET_NZ_FLAGS(X); \
   ADD_CYCLES(2); \
}

#define INY() \
{ \
   Y++; \
   SET_NZ_FLAGS(Y); \
   ADD_CYCLES(2); \
}

/* undocumented */
#define ISB(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   data++; \
   write_func(addr, data); \
   SBC(cycles, EMPTY_READ); \
}

#ifdef NES6502_TESTOPS
#define JAM() \
{ \
   cpu_Jam(); \
}
#else /* !NES6502_TESTOPS */
#define JAM() \
{ \
   PC--; \
   cpu.jammed = TRUE; \
   cpu.int_pending = 0; \
   ADD_CYCLES(2); \
}
#endif /* !NES6502_TESTOPS */

#define JMP_INDIRECT() \
{ \
   temp = bank_readword(PC); \
   /* bug in crossing page boundaries */ \
   if (0xFF == (temp & 0xFF)) \
      PC = (bank_readbyte(temp & 0xFF00) << 8) | bank_readbyte(temp); \
   else \
      JUMP(temp); \
   ADD_CYCLES(5); \
}

#define JMP_ABSOLUTE() \
{ \
   JUMP(PC); \
   ADD_CYCLES(3); \
}

#define JSR() \
{ \
   PC++; \
   PUSH(PC >> 8); \
   PUSH(PC & 0xFF); \
   JUMP(PC - 1); \
   ADD_CYCLES(6); \
}

/* undocumented */
#define LAS(cycles, read_func) \
{ \
   read_func(data); \
   A = X = S = (S & data); \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}

/* undocumented */
#define LAX(cycles, read_func) \
{ \
   read_func(A); \
   X = A; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}

#define LDA(cycles, read_func) \
{ \
   read_func(A); \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}

#define LDX(cycles, read_func) \
{ \
   read_func(X); \
   SET_NZ_FLAGS(X);\
   ADD_CYCLES(cycles); \
}

#define LDY(cycles, read_func) \
{ \
   read_func(Y); \
   SET_NZ_FLAGS(Y);\
   ADD_CYCLES(cycles); \
}

#define LSR(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   c_flag = data & 0x01; \
   data >>= 1; \
   write_func(addr, data); \
   SET_NZ_FLAGS(data); \
   ADD_CYCLES(cycles); \
}

#define LSR_A() \
{ \
   c_flag = A & 0x01; \
   A >>= 1; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(2); \
}

/* undocumented */
#define LXA(cycles, read_func) \
{ \
   read_func(data); \
   A = X = ((A | 0xEE) & data); \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}

#define NOP() \
{ \
   ADD_CYCLES(2); \
}

#define ORA(cycles, read_func) \
{ \
   read_func(data); \
   A |= data; \
   SET_NZ_FLAGS(A);\
   ADD_CYCLES(cycles); \
}

#define PHA() \
{ \
   PUSH(A); \
   ADD_CYCLES(3); \
}

#define PHP() \
{ \
   /* B flag is pushed on stack as well */ \
   PUSH(COMBINE_FLAGS() | B_FLAG6502); \
   ADD_CYCLES(3); \
}

#define PLA() \
{ \
   A = PULL(); \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(4); \
}

#define PLP() \
{ \
   btemp = PULL(); \
   SCATTER_FLAGS(btemp); \
   ADD_CYCLES(4); \
}

/* undocumented */
#define RLA(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   if (c_flag) \
   { \
      c_flag = data & 0x80; \
      data = (data << 1) | 1; \
   } \
   else \
   { \
      c_flag = data & 0x80; \
      data <<= 1; \
   } \
   write_func(addr, data); \
   A &= data; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}

/* 9-bit rotation (carry flag used for rollover) */
#define ROL(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   if (c_flag) \
   { \
      c_flag = data & 0x80; \
      data = (data << 1) | 1; \
   } \
   else \
   { \
      c_flag = data & 0x80; \
      data <<= 1; \
   } \
   write_func(addr, data); \
   SET_NZ_FLAGS(data); \
   ADD_CYCLES(cycles); \
}

#define ROL_A() \
{ \
   if (c_flag) \
   { \
      c_flag = A & 0x80; \
      A = (A << 1) | 1; \
   } \
   else \
   { \
      c_flag = A & 0x80; \
      A <<= 1; \
   } \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(2); \
}

#define ROR(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   if (c_flag) \
   { \
      c_flag = data & 1; \
      data = (data >> 1) | 0x80; \
   } \
   else \
   { \
      c_flag = data & 1; \
      data >>= 1; \
   } \
   write_func(addr, data); \
   SET_NZ_FLAGS(data); \
   ADD_CYCLES(cycles); \
}

#define ROR_A() \
{ \
   if (c_flag) \
   { \
      c_flag = A & 1; \
      A = (A >> 1) | 0x80; \
   } \
   else \
   { \
      c_flag = A & 1; \
      A >>= 1; \
   } \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(2); \
}

/* undocumented */
#define RRA(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   if (c_flag) \
   { \
      c_flag = data & 1; \
      data = (data >> 1) | 0x80; \
   } \
   else \
   { \
      c_flag = data & 1; \
      data >>= 1; \
   } \
   write_func(addr, data); \
   ADC(cycles, EMPTY_READ); \
}

#define RTI() \
{ \
   btemp = PULL(); \
   SCATTER_FLAGS(btemp); \
   PC = PULL(); \
   PC |= PULL() << 8; \
   ADD_CYCLES(6); \
   if (0 == i_flag && cpu.int_pending && (remaining_cycles > 0)) \
   { \
      cpu.int_pending = 0; \
      IRQ(); \
   } \
}

#define RTS() \
{ \
   PC = PULL(); \
   PC = (PC | (PULL() << 8)) + 1; \
   ADD_CYCLES(6); \
}

/* undocumented */
#define SAX(cycles, read_func, write_func, addr) \
{ \
   read_func(addr); \
   data = A & X; \
   write_func(addr, data); \
   ADD_CYCLES(cycles); \
}

/* Warning! NES CPU has no decimal mode, so by default this does no BCD! */
#ifdef NES6502_DECIMAL
#define SBC(cycles, read_func) \
{ \
   read_func(data); \
   temp = A - data - (c_flag ? 0 : 1); \
   if (d_flag) \
   { \
      uint8_t al, ah; \
      al = (A & 0x0F) - (data & 0x0F) - (c_flag ? 0 : 1); \
      ah = (A >> 4) - (data >> 4); \
      if (al & 0x10) \
      { \
         al -= 6; \
         ah--; \
      } \
      if (ah & 0x10) \
         ah -= 6; \
      c_flag = temp < 0x100; \
      v_flag = ((A ^ temp) & 0x80) && ((A ^ data) & 0x80); \
      SET_NZ_FLAGS(temp & 0xFF); \
      A = (ah << 4) | (al & 0x0F); \
   } \
   else \
   { \
      v_flag = ((A ^ temp) & 0x80) && ((A ^ data) & 0x80); \
      c_flag = temp < 0x100; \
      A = (uint8_t) temp; \
      SET_NZ_FLAGS(A & 0xFF); \
   } \
   ADD_CYCLES(cycles); \
}
#else
#define SBC(cycles, read_func) \
{ \
   read_func(data); \
   temp = A - data - (c_flag ? 0 : 1); \
   v_flag = ((A ^ data) & (A ^ temp) & 0x80); \
   c_flag = temp < 0x100; \
   A = (uint8_t) temp; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}
#endif /* NES6502_DECIMAL */

/* undocumented */
#define SBX(cycles, read_func) \
{ \
   read_func(data); \
   temp = (A & X) - data; \
   c_flag = temp < 0x100; \
   X = temp & 0xFF; \
   SET_NZ_FLAGS(X); \
   ADD_CYCLES(cycles); \
}

#define SEC() \
{ \
   c_flag = 1; \
   ADD_CYCLES(2); \
}

#define SED() \
{ \
   d_flag = 1; \
   ADD_CYCLES(2); \
}

#define SEI() \
{ \
   i_flag = 1; \
   ADD_CYCLES(2); \
}

/* undocumented */
#define SHA(cycles, read_func, write_func, addr) \
{ \
   read_func(addr); \
   data = A & X & ((uint8_t) ((addr >> 8) + 1)); \
   write_func(addr, data); \
   ADD_CYCLES(cycles); \
}

/* undocumented */
#define SHS(cycles, read_func, write_func, addr) \
{ \
   read_func(addr); \
   S = A & X; \
   data = S & ((uint8_t) ((addr >> 8) + 1)); \
   write_func(addr, data); \
   ADD_CYCLES(cycles); \
}

/* undocumented */
#define SHX(cycles, read_func, write_func, addr) \
{ \
   read_func(addr); \
   data = X & ((uint8_t) ((addr >> 8) + 1)); \
   write_func(addr, data); \
   ADD_CYCLES(cycles); \
}

/* undocumented */
#define SHY(cycles, read_func, write_func, addr) \
{ \
   read_func(addr); \
   data = Y & ((uint8_t) ((addr >> 8 ) + 1)); \
   write_func(addr, data); \
   ADD_CYCLES(cycles); \
}

/* undocumented */
#define SLO(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   c_flag = data & 0x80; \
   data <<= 1; \
   write_func(addr, data); \
   A |= data; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}

/* undocumented */
#define SRE(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   c_flag = data & 1; \
   data >>= 1; \
   write_func(addr, data); \
   A ^= data; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}

#define STA(cycles, read_func, write_func, addr) \
{ \
   read_func(addr); \
   write_func(addr, A); \
   ADD_CYCLES(cycles); \
}

#define STX(cycles, read_func, write_func, addr) \
{ \
   read_func(addr); \
   write_func(addr, X); \
   ADD_CYCLES(cycles); \
}

#define STY(cycles, read_func, write_func, addr) \
{ \
   read_func(addr); \
   write_func(addr, Y); \
   ADD_CYCLES(cycles); \
}

#define TAX() \
{ \
   X = A; \
   SET_NZ_FLAGS(X);\
   ADD_CYCLES(2); \
}

#define TAY() \
{ \
   Y = A; \
   SET_NZ_FLAGS(Y);\
   ADD_CYCLES(2); \
}

/* undocumented (triple-NOP) */
#define TOP() \
{ \
   PC += 2; \
   ADD_CYCLES(4); \
}

#define TSX() \
{ \
   X = S; \
   SET_NZ_FLAGS(X);\
   ADD_CYCLES(2); \
}

#define TXA() \
{ \
   A = X; \
   SET_NZ_FLAGS(A);\
   ADD_CYCLES(2); \
}

#define TXS() \
{ \
   S = X; \
   ADD_CYCLES(2); \
}

#define TYA() \
{ \
   A = Y; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(2); \
}



/* internal CPU context */
static nes6502_context cpu;

/* memory region pointers */
static uint8_t *ram = NULL, *stack = NULL;
static uint8_t dead_page[NES6502_BANKSIZE];


/*
** Zero-page helper macros
*/

#define  ZP_READBYTE(addr)          ram[(addr)]
#define  ZP_WRITEBYTE(addr, value)  ram[(addr)] = (uint8_t) (value)


INLINE uint32_t zp_readword(uint8_t address)
{
#ifdef HOST_LITTLE_ENDIAN
   /* TODO: this fails if host architecture doesn't support byte alignment */
   return (uint32_t) (*(uint16_t *)(ram + address));
#else
#ifdef TARGET_CPU_PPC
   return __lhbrx(ram, address);
#else
   uint32_t x = (uint32_t) *(uint16_t *)(ram + address);
   return (x << 8) | (x >> 8);
#endif /* TARGET_CPU_PPC */
#endif /* HOST_LITTLE_ENDIAN */
}

INLINE uint8_t bank_readbyte(uint32_t address)
{
   return cpu.mem_page[address >> NES6502_BANKSHIFT][address & NES6502_BANKMASK];
}

INLINE uint32_t bank_readword(uint32_t address)
{
#ifdef HOST_LITTLE_ENDIAN
   /* TODO: this fails if src address is $xFFF */
   /* TODO: this fails if host architecture doesn't support byte alignment */
   return (uint32_t) (*(uint16_t *)(cpu.mem_page[address >> NES6502_BANKSHIFT] + (address & NES6502_BANKMASK)));
#else
#ifdef TARGET_CPU_PPC
   return __lhbrx(cpu.mem_page[address >> NES6502_BANKSHIFT], address & NES6502_BANKMASK);
#else
   uint32_t x = (uint32_t) *(uint16_t *)(cpu.mem_page[address >> NES6502_BANKSHIFT] + (address & NES6502_BANKMASK));
   return (x << 8) | (x >> 8);
#endif /* TARGET_CPU_PPC */
#endif /* HOST_LITTLE_ENDIAN */
}

INLINE void bank_writebyte(uint32_t address, uint8_t value)
{
   cpu.mem_page[address >> NES6502_BANKSHIFT][address & NES6502_BANKMASK] = value;
}

/* read a byte of 6502 memory */
static uint8_t mem_readbyte(uint32_t address)
{
   nes6502_memread *mr;

   /* TODO: following cases are N2A03-specific */
   /* RAM */
//   if (address < 0x800)
  //    return ram[address];
   /* always paged memory */
//   else if (address >= 0x8000)
//      return bank_readbyte(address);
   /* check memory range handlers */
//   else
   {
      for (mr = cpu.read_handler; mr->min_range != 0xFFFFFFFF; mr++)
      {
         if (address >= mr->min_range && address <= mr->max_range)
            return mr->read_func(address);
      }
   }

   /* return paged memory */
   return bank_readbyte(address);
}

/* write a byte of data to 6502 memory */
static void mem_writebyte(uint32_t address, uint8_t value)
{
   nes6502_memwrite *mw;

   /* RAM */
//   if (address < 0x800)
//   {
//      ram[address] = value;
//      return;
//   }
   /* check memory range handlers */
//   else
   {
      for (mw = cpu.write_handler; mw->min_range != 0xFFFFFFFF; mw++)
      {
         if (address >= mw->min_range && address <= mw->max_range)
         {
            mw->write_func(address, value);
            return;
         }
      }
   }

   /* write to paged memory */
   bank_writebyte(address, value);
}

/* set the current context */
void nes6502_setcontext(nes6502_context *context)
{
   int loop;

   ASSERT(context);

   memcpy(&cpu, context, sizeof(nes6502_context));

   for (loop = 0; loop < NES6502_NUMBANKS; loop++)
   {
      if (NULL == cpu.mem_page[loop])
         cpu.mem_page[loop] = dead_page;
   }

   ram = cpu.mem_page[0];  /* quick zero-page/RAM references */
   stack = ram + STACK_OFFSET;

   cpu.jammed = FALSE;
}

/* get the current context */
void nes6502_getcontext(nes6502_context *context)
{
   int loop;

   ASSERT(context);

   memcpy(context, &cpu, sizeof(nes6502_context));

   for (loop = 0; loop < NES6502_NUMBANKS; loop++)
   {
      if (dead_page == context->mem_page[loop])
         context->mem_page[loop] = NULL;
   }
}

/* DMA a byte of data from ROM */
uint8_t nes6502_getbyte(uint32_t address)
{
   return bank_readbyte(address);
}

/* get number of elapsed cycles */
uint32_t nes6502_getcycles(bool reset_flag)
{
   uint32_t cycles = cpu.total_cycles;

   if (reset_flag)
      cpu.total_cycles = 0;

   return cycles;
}

#define  GET_GLOBAL_REGS() \
{ \
   PC = cpu.pc_reg; \
   A = cpu.a_reg; \
   X = cpu.x_reg; \
   Y = cpu.y_reg; \
   SCATTER_FLAGS(cpu.p_reg); \
   S = cpu.s_reg; \
}

#define  STORE_LOCAL_REGS() \
{ \
   cpu.pc_reg = PC; \
   cpu.a_reg = A; \
   cpu.x_reg = X; \
   cpu.y_reg = Y; \
   cpu.p_reg = COMBINE_FLAGS(); \
   cpu.s_reg = S; \
}

#define  MIN(a,b)    (((a) < (b)) ? (a) : (b))

/* Execute instructions until count expires
**
** Returns the number of cycles *actually* executed, which will be
** anywhere from remaining_cycles to remaining_cycles + 6
*/

uint32_t g_old_cycles = 0;	// MPO : moved this here so other functions could access it

//int nes6502_execute(int remaining_cycles)
// MATT : changed this to match callback prototype
unsigned int nes6502_execute(unsigned int cycles_to_execute)
{
//   int old_cycles = cpu.total_cycles;
	g_old_cycles = cpu.total_cycles;	// MPO
   int remaining_cycles = cycles_to_execute;
   uint32_t temp, addr; /* for macros */
   uint8_t btemp, baddr; /* for macros */
   uint8_t data;

   /* flags */
   uint8_t n_flag, v_flag, b_flag;
   uint8_t d_flag, i_flag, z_flag, c_flag;

   /* local copies of regs */
   uint32_t PC;
   uint8_t A, X, Y, S;

#ifdef NES6502_JUMPTABLE

#define  OPCODE_BEGIN(xx)  op##xx:
#define  OPCODE_END \
   if (remaining_cycles <= 0) \
      goto end_execute; \
   goto *opcode_table[bank_readbyte(PC++)];
   
   static void *opcode_table[256] =
   {
      &&op00, &&op01, &&op02, &&op03, &&op04, &&op05, &&op06, &&op07,
      &&op08, &&op09, &&op0A, &&op0B, &&op0C, &&op0D, &&op0E, &&op0F,
      &&op10, &&op11, &&op12, &&op13, &&op14, &&op15, &&op16, &&op17,
      &&op18, &&op19, &&op1A, &&op1B, &&op1C, &&op1D, &&op1E, &&op1F,
      &&op20, &&op21, &&op22, &&op23, &&op24, &&op25, &&op26, &&op27,
      &&op28, &&op29, &&op2A, &&op2B, &&op2C, &&op2D, &&op2E, &&op2F,
      &&op30, &&op31, &&op32, &&op33, &&op34, &&op35, &&op36, &&op37,
      &&op38, &&op39, &&op3A, &&op3B, &&op3C, &&op3D, &&op3E, &&op3F,
      &&op40, &&op41, &&op42, &&op43, &&op44, &&op45, &&op46, &&op47,
      &&op48, &&op49, &&op4A, &&op4B, &&op4C, &&op4D, &&op4E, &&op4F,
      &&op50, &&op51, &&op52, &&op53, &&op54, &&op55, &&op56, &&op57,
      &&op58, &&op59, &&op5A, &&op5B, &&op5C, &&op5D, &&op5E, &&op5F,
      &&op60, &&op61, &&op62, &&op63, &&op64, &&op65, &&op66, &&op67,
      &&op68, &&op69, &&op6A, &&op6B, &&op6C, &&op6D, &&op6E, &&op6F,
      &&op70, &&op71, &&op72, &&op73, &&op74, &&op75, &&op76, &&op77,
      &&op78, &&op79, &&op7A, &&op7B, &&op7C, &&op7D, &&op7E, &&op7F,
      &&op80, &&op81, &&op82, &&op83, &&op84, &&op85, &&op86, &&op87,
      &&op88, &&op89, &&op8A, &&op8B, &&op8C, &&op8D, &&op8E, &&op8F,
      &&op90, &&op91, &&op92, &&op93, &&op94, &&op95, &&op96, &&op97,
      &&op98, &&op99, &&op9A, &&op9B, &&op9C, &&op9D, &&op9E, &&op9F,
      &&opA0, &&opA1, &&opA2, &&opA3, &&opA4, &&opA5, &&opA6, &&opA7,
      &&opA8, &&opA9, &&opAA, &&opAB, &&opAC, &&opAD, &&opAE, &&opAF,
      &&opB0, &&opB1, &&opB2, &&opB3, &&opB4, &&opB5, &&opB6, &&opB7,
      &&opB8, &&opB9, &&opBA, &&opBB, &&opBC, &&opBD, &&opBE, &&opBF,
      &&opC0, &&opC1, &&opC2, &&opC3, &&opC4, &&opC5, &&opC6, &&opC7,
      &&opC8, &&opC9, &&opCA, &&opCB, &&opCC, &&opCD, &&opCE, &&opCF,
      &&opD0, &&opD1, &&opD2, &&opD3, &&opD4, &&opD5, &&opD6, &&opD7,
      &&opD8, &&opD9, &&opDA, &&opDB, &&opDC, &&opDD, &&opDE, &&opDF,
      &&opE0, &&opE1, &&opE2, &&opE3, &&opE4, &&opE5, &&opE6, &&opE7,
      &&opE8, &&opE9, &&opEA, &&opEB, &&opEC, &&opED, &&opEE, &&opEF,
      &&opF0, &&opF1, &&opF2, &&opF3, &&opF4, &&opF5, &&opF6, &&opF7,
      &&opF8, &&opF9, &&opFA, &&opFB, &&opFC, &&opFD, &&opFE, &&opFF
   };

#else /* !NES6502_JUMPTABLE */
#define  OPCODE_BEGIN(xx)  case 0x##xx:
#define  OPCODE_END        break;
#endif /* !NES6502_JUMPTABLE */

   GET_GLOBAL_REGS();

   if (cpu.int_pending && remaining_cycles)
   {
      if (0 == i_flag)
      {
         cpu.int_pending = 0;
         IRQ();
      }
   }

   /* check for DMA cycle burning */
   if (cpu.burn_cycles && remaining_cycles)
   {
      int burn_for;
      
      burn_for = MIN(remaining_cycles, cpu.burn_cycles);
      ADD_CYCLES(burn_for);
      cpu.burn_cycles -= burn_for;
   }
      
#ifdef NES6502_JUMPTABLE
   /* fetch first instruction */
   OPCODE_END

#else /* !NES6502_JUMPTABLE */

   /* Continue until we run out of cycles */
   while (remaining_cycles > 0)
   {
#ifdef NES6502_DISASM
      log_printf(nes6502_disasm(PC, COMBINE_FLAGS(), A, X, Y, S));
#endif /* NES6502_DISASM */

	  // start MPO
#ifdef CPU_DEBUG
	  STORE_LOCAL_REGS();
	  MAME_Debug();
	  GET_GLOBAL_REGS();
#endif
	  // end MPO

      /* Fetch and execute instruction */
      switch (bank_readbyte(PC++))
      {
#endif /* !NES6502_JUMPTABLE */

      OPCODE_BEGIN(00)  /* BRK */
         BRK();
         OPCODE_END

      OPCODE_BEGIN(01)  /* ORA ($nn,X) */
         ORA(6, INDIR_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(02)  /* JAM */
      OPCODE_BEGIN(12)  /* JAM */
      OPCODE_BEGIN(22)  /* JAM */
      OPCODE_BEGIN(32)  /* JAM */
      OPCODE_BEGIN(42)  /* JAM */
      OPCODE_BEGIN(52)  /* JAM */
      OPCODE_BEGIN(62)  /* JAM */
      OPCODE_BEGIN(72)  /* JAM */
      OPCODE_BEGIN(92)  /* JAM */
      OPCODE_BEGIN(B2)  /* JAM */
      OPCODE_BEGIN(D2)  /* JAM */
      OPCODE_BEGIN(F2)  /* JAM */
         JAM();
         /* kill the CPU */
         remaining_cycles = 0;
         OPCODE_END

      OPCODE_BEGIN(03)  /* SLO ($nn,X) */
         SLO(8, INDIR_X, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(04)  /* NOP $nn */
      OPCODE_BEGIN(44)  /* NOP $nn */
      OPCODE_BEGIN(64)  /* NOP $nn */
         DOP(3);
         OPCODE_END

      OPCODE_BEGIN(05)  /* ORA $nn */
         ORA(3, ZERO_PAGE_BYTE); 
         OPCODE_END

      OPCODE_BEGIN(06)  /* ASL $nn */
         ASL(5, ZERO_PAGE, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(07)  /* SLO $nn */
         SLO(5, ZERO_PAGE, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(08)  /* PHP */
         PHP(); 
         OPCODE_END

      OPCODE_BEGIN(09)  /* ORA #$nn */
         ORA(2, IMMEDIATE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(0A)  /* ASL A */
         ASL_A();
         OPCODE_END

      OPCODE_BEGIN(0B)  /* ANC #$nn */
         ANC(2, IMMEDIATE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(0C)  /* NOP $nnnn */
         TOP(); 
         OPCODE_END

      OPCODE_BEGIN(0D)  /* ORA $nnnn */
         ORA(4, ABSOLUTE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(0E)  /* ASL $nnnn */
         ASL(6, ABSOLUTE, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(0F)  /* SLO $nnnn */
         SLO(6, ABSOLUTE, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(10)  /* BPL $nnnn */
         BPL();
         OPCODE_END

      OPCODE_BEGIN(11)  /* ORA ($nn),Y */
         ORA(5, INDIR_Y_BYTE);
         OPCODE_END
      
      OPCODE_BEGIN(13)  /* SLO ($nn),Y */
         SLO(8, INDIR_Y, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(14)  /* NOP $nn,X */
      OPCODE_BEGIN(34)  /* NOP */
      OPCODE_BEGIN(54)  /* NOP $nn,X */
      OPCODE_BEGIN(74)  /* NOP $nn,X */
      OPCODE_BEGIN(D4)  /* NOP $nn,X */
      OPCODE_BEGIN(F4)  /* NOP ($nn,X) */
         DOP(4);
         OPCODE_END

      OPCODE_BEGIN(15)  /* ORA $nn,X */
         ORA(4, ZP_IND_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(16)  /* ASL $nn,X */
         ASL(6, ZP_IND_X, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(17)  /* SLO $nn,X */
         SLO(6, ZP_IND_X, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(18)  /* CLC */
         CLC();
         OPCODE_END

      OPCODE_BEGIN(19)  /* ORA $nnnn,Y */
         ORA(4, ABS_IND_Y_BYTE);
         OPCODE_END
      
      OPCODE_BEGIN(1A)  /* NOP */
      OPCODE_BEGIN(3A)  /* NOP */
      OPCODE_BEGIN(5A)  /* NOP */
      OPCODE_BEGIN(7A)  /* NOP */
      OPCODE_BEGIN(DA)  /* NOP */
      OPCODE_BEGIN(FA)  /* NOP */
         NOP();
         OPCODE_END

      OPCODE_BEGIN(1B)  /* SLO $nnnn,Y */
         SLO(7, ABS_IND_Y, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(1C)  /* NOP $nnnn,X */
      OPCODE_BEGIN(3C)  /* NOP $nnnn,X */
      OPCODE_BEGIN(5C)  /* NOP $nnnn,X */
      OPCODE_BEGIN(7C)  /* NOP $nnnn,X */
      OPCODE_BEGIN(DC)  /* NOP $nnnn,X */
      OPCODE_BEGIN(FC)  /* NOP $nnnn,X */
         TOP();
         OPCODE_END

      OPCODE_BEGIN(1D)  /* ORA $nnnn,X */
         ORA(4, ABS_IND_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(1E)  /* ASL $nnnn,X */
         ASL(7, ABS_IND_X, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(1F)  /* SLO $nnnn,X */
         SLO(7, ABS_IND_X, mem_writebyte, addr);
         OPCODE_END
      
      OPCODE_BEGIN(20)  /* JSR $nnnn */
         JSR();
         OPCODE_END

      OPCODE_BEGIN(21)  /* AND ($nn,X) */
         AND(6, INDIR_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(23)  /* RLA ($nn,X) */
         RLA(8, INDIR_X, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(24)  /* BIT $nn */
         BIT(3, ZERO_PAGE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(25)  /* AND $nn */
         AND(3, ZERO_PAGE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(26)  /* ROL $nn */
         ROL(5, ZERO_PAGE, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(27)  /* RLA $nn */
         RLA(5, ZERO_PAGE, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(28)  /* PLP */
         PLP();
         OPCODE_END

      OPCODE_BEGIN(29)  /* AND #$nn */
         AND(2, IMMEDIATE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(2A)  /* ROL A */
         ROL_A();
         OPCODE_END

      OPCODE_BEGIN(2B)  /* ANC #$nn */
         ANC(2, IMMEDIATE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(2C)  /* BIT $nnnn */
         BIT(4, ABSOLUTE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(2D)  /* AND $nnnn */
         AND(4, ABSOLUTE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(2E)  /* ROL $nnnn */
         ROL(6, ABSOLUTE, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(2F)  /* RLA $nnnn */
         RLA(6, ABSOLUTE, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(30)  /* BMI $nnnn */
         BMI();
         OPCODE_END

      OPCODE_BEGIN(31)  /* AND ($nn),Y */
         AND(5, INDIR_Y_BYTE);
         OPCODE_END

      OPCODE_BEGIN(33)  /* RLA ($nn),Y */
         RLA(8, INDIR_Y, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(35)  /* AND $nn,X */
         AND(4, ZP_IND_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(36)  /* ROL $nn,X */
         ROL(6, ZP_IND_X, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(37)  /* RLA $nn,X */
         RLA(6, ZP_IND_X, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(38)  /* SEC */
         SEC();
         OPCODE_END

      OPCODE_BEGIN(39)  /* AND $nnnn,Y */
         AND(4, ABS_IND_Y_BYTE);
         OPCODE_END

      OPCODE_BEGIN(3B)  /* RLA $nnnn,Y */
         RLA(7, ABS_IND_Y, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(3D)  /* AND $nnnn,X */
         AND(4, ABS_IND_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(3E)  /* ROL $nnnn,X */
         ROL(7, ABS_IND_X, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(3F)  /* RLA $nnnn,X */
         RLA(7, ABS_IND_X, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(40)  /* RTI */
         RTI();
         OPCODE_END

      OPCODE_BEGIN(41)  /* EOR ($nn,X) */
         EOR(6, INDIR_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(43)  /* SRE ($nn,X) */
         SRE(8, INDIR_X, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(45)  /* EOR $nn */
         EOR(3, ZERO_PAGE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(46)  /* LSR $nn */
         LSR(5, ZERO_PAGE, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(47)  /* SRE $nn */
         SRE(5, ZERO_PAGE, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(48)  /* PHA */
         PHA();
         OPCODE_END

      OPCODE_BEGIN(49)  /* EOR #$nn */
         EOR(2, IMMEDIATE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(4A)  /* LSR A */
         LSR_A();
         OPCODE_END

      OPCODE_BEGIN(4B)  /* ASR #$nn */
         ASR(2, IMMEDIATE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(4C)  /* JMP $nnnn */
         JMP_ABSOLUTE();
         OPCODE_END

      OPCODE_BEGIN(4D)  /* EOR $nnnn */
         EOR(4, ABSOLUTE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(4E)  /* LSR $nnnn */
         LSR(6, ABSOLUTE, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(4F)  /* SRE $nnnn */
         SRE(6, ABSOLUTE, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(50)  /* BVC $nnnn */
         BVC();
         OPCODE_END

      OPCODE_BEGIN(51)  /* EOR ($nn),Y */
         EOR(5, INDIR_Y_BYTE);
         OPCODE_END

      OPCODE_BEGIN(53)  /* SRE ($nn),Y */
         SRE(8, INDIR_Y, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(55)  /* EOR $nn,X */
         EOR(4, ZP_IND_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(56)  /* LSR $nn,X */
         LSR(6, ZP_IND_X, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(57)  /* SRE $nn,X */
         SRE(6, ZP_IND_X, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(58)  /* CLI */
         CLI();
         OPCODE_END

      OPCODE_BEGIN(59)  /* EOR $nnnn,Y */
         EOR(4, ABS_IND_Y_BYTE);
         OPCODE_END

      OPCODE_BEGIN(5B)  /* SRE $nnnn,Y */
         SRE(7, ABS_IND_Y, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(5D)  /* EOR $nnnn,X */
         EOR(4, ABS_IND_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(5E)  /* LSR $nnnn,X */
         LSR(7, ABS_IND_X, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(5F)  /* SRE $nnnn,X */
         SRE(7, ABS_IND_X, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(60)  /* RTS */
         RTS();
         OPCODE_END

      OPCODE_BEGIN(61)  /* ADC ($nn,X) */
         ADC(6, INDIR_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(63)  /* RRA ($nn,X) */
         RRA(8, INDIR_X, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(65)  /* ADC $nn */
         ADC(3, ZERO_PAGE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(66)  /* ROR $nn */
         ROR(5, ZERO_PAGE, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(67)  /* RRA $nn */
         RRA(5, ZERO_PAGE, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(68)  /* PLA */
         PLA();
         OPCODE_END

      OPCODE_BEGIN(69)  /* ADC #$nn */
         ADC(2, IMMEDIATE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(6A)  /* ROR A */
         ROR_A();
         OPCODE_END

      OPCODE_BEGIN(6B)  /* ARR #$nn */
         ARR(2, IMMEDIATE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(6C)  /* JMP ($nnnn) */
         JMP_INDIRECT();
         OPCODE_END

      OPCODE_BEGIN(6D)  /* ADC $nnnn */
         ADC(4, ABSOLUTE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(6E)  /* ROR $nnnn */
         ROR(6, ABSOLUTE, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(6F)  /* RRA $nnnn */
         RRA(6, ABSOLUTE, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(70)  /* BVS $nnnn */
         BVS();
         OPCODE_END

      OPCODE_BEGIN(71)  /* ADC ($nn),Y */
         ADC(5, INDIR_Y_BYTE);
         OPCODE_END

      OPCODE_BEGIN(73)  /* RRA ($nn),Y */
         RRA(8, INDIR_Y, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(75)  /* ADC $nn,X */
         ADC(4, ZP_IND_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(76)  /* ROR $nn,X */
         ROR(6, ZP_IND_X, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(77)  /* RRA $nn,X */
         RRA(6, ZP_IND_X, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(78)  /* SEI */
         SEI();
         OPCODE_END

      OPCODE_BEGIN(79)  /* ADC $nnnn,Y */
         ADC(4, ABS_IND_Y_BYTE);
         OPCODE_END

      OPCODE_BEGIN(7B)  /* RRA $nnnn,Y */
         RRA(7, ABS_IND_Y, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(7D)  /* ADC $nnnn,X */
         ADC(4, ABS_IND_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(7E)  /* ROR $nnnn,X */
         ROR(7, ABS_IND_X, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(7F)  /* RRA $nnnn,X */
         RRA(7, ABS_IND_X, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(80)  /* NOP #$nn */
      OPCODE_BEGIN(82)  /* NOP #$nn */
      OPCODE_BEGIN(89)  /* NOP #$nn */
      OPCODE_BEGIN(C2)  /* NOP #$nn */
      OPCODE_BEGIN(E2)  /* NOP #$nn */
         DOP(2);
         OPCODE_END

      OPCODE_BEGIN(81)  /* STA ($nn,X) */
         STA(6, INDIR_X_ADDR, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(83)  /* SAX ($nn,X) */
         SAX(6, INDIR_X_ADDR, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(84)  /* STY $nn */
         STY(3, ZERO_PAGE_ADDR, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(85)  /* STA $nn */
         STA(3, ZERO_PAGE_ADDR, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(86)  /* STX $nn */
         STX(3, ZERO_PAGE_ADDR, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(87)  /* SAX $nn */
         SAX(3, ZERO_PAGE_ADDR, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(88)  /* DEY */
         DEY();
         OPCODE_END

      OPCODE_BEGIN(8A)  /* TXA */
         TXA();
         OPCODE_END

      OPCODE_BEGIN(8B)  /* ANE #$nn */
         ANE(2, IMMEDIATE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(8C)  /* STY $nnnn */
         STY(4, ABSOLUTE_ADDR, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(8D)  /* STA $nnnn */
         STA(4, ABSOLUTE_ADDR, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(8E)  /* STX $nnnn */
         STX(4, ABSOLUTE_ADDR, mem_writebyte, addr);
         OPCODE_END
      
      OPCODE_BEGIN(8F)  /* SAX $nnnn */
         SAX(4, ABSOLUTE_ADDR, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(90)  /* BCC $nnnn */
         BCC();
         OPCODE_END

      OPCODE_BEGIN(91)  /* STA ($nn),Y */
         STA(6, INDIR_Y_ADDR, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(93)  /* SHA ($nn),Y */
         SHA(6, INDIR_Y_ADDR, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(94)  /* STY $nn,X */
         STY(4, ZP_IND_X_ADDR, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(95)  /* STA $nn,X */
         STA(4, ZP_IND_X_ADDR, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(96)  /* STX $nn,Y */
         STX(4, ZP_IND_Y_ADDR, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(97)  /* SAX $nn,Y */
         SAX(4, ZP_IND_Y_ADDR, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(98)  /* TYA */
         TYA();
         OPCODE_END

      OPCODE_BEGIN(99)  /* STA $nnnn,Y */
         STA(5, ABS_IND_Y_ADDR, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(9A)  /* TXS */
         TXS();
         OPCODE_END

      OPCODE_BEGIN(9B)  /* SHS $nnnn,Y */
         SHS(5, ABS_IND_Y_ADDR, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(9C)  /* SHY $nnnn,X */
         SHY(5, ABS_IND_X_ADDR, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(9D)  /* STA $nnnn,X */
         STA(5, ABS_IND_X_ADDR, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(9E)  /* SHX $nnnn,Y */
         SHX(5, ABS_IND_Y_ADDR, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(9F)  /* SHA $nnnn,Y */
         SHA(5, ABS_IND_Y_ADDR, mem_writebyte, addr);
         OPCODE_END
      
      OPCODE_BEGIN(A0)  /* LDY #$nn */
         LDY(2, IMMEDIATE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(A1)  /* LDA ($nn,X) */
         LDA(6, INDIR_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(A2)  /* LDX #$nn */
         LDX(2, IMMEDIATE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(A3)  /* LAX ($nn,X) */
         LAX(6, INDIR_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(A4)  /* LDY $nn */
         LDY(3, ZERO_PAGE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(A5)  /* LDA $nn */
         LDA(3, ZERO_PAGE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(A6)  /* LDX $nn */
         LDX(3, ZERO_PAGE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(A7)  /* LAX $nn */
         LAX(3, ZERO_PAGE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(A8)  /* TAY */
         TAY();
         OPCODE_END

      OPCODE_BEGIN(A9)  /* LDA #$nn */
         LDA(2, IMMEDIATE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(AA)  /* TAX */
         TAX();
         OPCODE_END

      OPCODE_BEGIN(AB)  /* LXA #$nn */
         LXA(2, IMMEDIATE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(AC)  /* LDY $nnnn */
         LDY(4, ABSOLUTE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(AD)  /* LDA $nnnn */
         LDA(4, ABSOLUTE_BYTE);
         OPCODE_END
      
      OPCODE_BEGIN(AE)  /* LDX $nnnn */
         LDX(4, ABSOLUTE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(AF)  /* LAX $nnnn */
         LAX(4, ABSOLUTE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(B0)  /* BCS $nnnn */
         BCS();
         OPCODE_END

      OPCODE_BEGIN(B1)  /* LDA ($nn),Y */
         LDA(5, INDIR_Y_BYTE);
         OPCODE_END

      OPCODE_BEGIN(B3)  /* LAX ($nn),Y */
         LAX(5, INDIR_Y_BYTE);
         OPCODE_END

      OPCODE_BEGIN(B4)  /* LDY $nn,X */
         LDY(4, ZP_IND_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(B5)  /* LDA $nn,X */
         LDA(4, ZP_IND_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(B6)  /* LDX $nn,Y */
         LDX(4, ZP_IND_Y_BYTE);
         OPCODE_END

      OPCODE_BEGIN(B7)  /* LAX $nn,Y */
         LAX(4, ZP_IND_Y_BYTE);
         OPCODE_END

      OPCODE_BEGIN(B8)  /* CLV */
         CLV();
         OPCODE_END

      OPCODE_BEGIN(B9)  /* LDA $nnnn,Y */
         LDA(4, ABS_IND_Y_BYTE);
         OPCODE_END

      OPCODE_BEGIN(BA)  /* TSX */
         TSX();
         OPCODE_END

      OPCODE_BEGIN(BB)  /* LAS $nnnn,Y */
         LAS(4, ABS_IND_Y_BYTE);
         OPCODE_END

      OPCODE_BEGIN(BC)  /* LDY $nnnn,X */
         LDY(4, ABS_IND_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(BD)  /* LDA $nnnn,X */
         LDA(4, ABS_IND_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(BE)  /* LDX $nnnn,Y */
         LDX(4, ABS_IND_Y_BYTE);
         OPCODE_END

      OPCODE_BEGIN(BF)  /* LAX $nnnn,Y */
         LAX(4, ABS_IND_Y_BYTE);
         OPCODE_END

      OPCODE_BEGIN(C0)  /* CPY #$nn */
         CPY(2, IMMEDIATE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(C1)  /* CMP ($nn,X) */
         CMP(6, INDIR_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(C3)  /* DCP ($nn,X) */
         DCP(8, INDIR_X, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(C4)  /* CPY $nn */
         CPY(3, ZERO_PAGE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(C5)  /* CMP $nn */
         CMP(3, ZERO_PAGE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(C6)  /* DEC $nn */
         DEC(5, ZERO_PAGE, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(C7)  /* DCP $nn */
         DCP(5, ZERO_PAGE, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(C8)  /* INY */
         INY();
         OPCODE_END

      OPCODE_BEGIN(C9)  /* CMP #$nn */
         CMP(2, IMMEDIATE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(CA)  /* DEX */
         DEX();
         OPCODE_END

      OPCODE_BEGIN(CB)  /* SBX #$nn */
         SBX(2, IMMEDIATE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(CC)  /* CPY $nnnn */
         CPY(4, ABSOLUTE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(CD)  /* CMP $nnnn */
         CMP(4, ABSOLUTE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(CE)  /* DEC $nnnn */
         DEC(6, ABSOLUTE, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(CF)  /* DCP $nnnn */
         DCP(6, ABSOLUTE, mem_writebyte, addr);
         OPCODE_END
      
      OPCODE_BEGIN(D0)  /* BNE $nnnn */
         BNE();
         OPCODE_END

      OPCODE_BEGIN(D1)  /* CMP ($nn),Y */
         CMP(5, INDIR_Y_BYTE);
         OPCODE_END

      OPCODE_BEGIN(D3)  /* DCP ($nn),Y */
         DCP(8, INDIR_Y, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(D5)  /* CMP $nn,X */
         CMP(4, ZP_IND_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(D6)  /* DEC $nn,X */
         DEC(6, ZP_IND_X, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(D7)  /* DCP $nn,X */
         DCP(6, ZP_IND_X, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(D8)  /* CLD */
         CLD();
         OPCODE_END

      OPCODE_BEGIN(D9)  /* CMP $nnnn,Y */
         CMP(4, ABS_IND_Y_BYTE);
         OPCODE_END

      OPCODE_BEGIN(DB)  /* DCP $nnnn,Y */
         DCP(7, ABS_IND_Y, mem_writebyte, addr);
         OPCODE_END                  

      OPCODE_BEGIN(DD)  /* CMP $nnnn,X */
         CMP(4, ABS_IND_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(DE)  /* DEC $nnnn,X */
         DEC(7, ABS_IND_X, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(DF)  /* DCP $nnnn,X */
         DCP(7, ABS_IND_X, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(E0)  /* CPX #$nn */
         CPX(2, IMMEDIATE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(E1)  /* SBC ($nn,X) */
         SBC(6, INDIR_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(E3)  /* ISB ($nn,X) */
         ISB(8, INDIR_X, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(E4)  /* CPX $nn */
         CPX(3, ZERO_PAGE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(E5)  /* SBC $nn */
         SBC(3, ZERO_PAGE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(E6)  /* INC $nn */
         INC(5, ZERO_PAGE, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(E7)  /* ISB $nn */
         ISB(5, ZERO_PAGE, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(E8)  /* INX */
         INX();
         OPCODE_END

      OPCODE_BEGIN(E9)  /* SBC #$nn */
      OPCODE_BEGIN(EB)  /* USBC #$nn */
         SBC(2, IMMEDIATE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(EA)  /* NOP */
         NOP();
         OPCODE_END

      OPCODE_BEGIN(EC)  /* CPX $nnnn */
         CPX(4, ABSOLUTE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(ED)  /* SBC $nnnn */
         SBC(4, ABSOLUTE_BYTE);
         OPCODE_END

      OPCODE_BEGIN(EE)  /* INC $nnnn */
         INC(6, ABSOLUTE, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(EF)  /* ISB $nnnn */
         ISB(6, ABSOLUTE, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(F0)  /* BEQ $nnnn */
         BEQ();
         OPCODE_END

      OPCODE_BEGIN(F1)  /* SBC ($nn),Y */
         SBC(5, INDIR_Y_BYTE);
         OPCODE_END

      OPCODE_BEGIN(F3)  /* ISB ($nn),Y */
         ISB(8, INDIR_Y, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(F5)  /* SBC $nn,X */
         SBC(4, ZP_IND_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(F6)  /* INC $nn,X */
         INC(6, ZP_IND_X, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(F7)  /* ISB $nn,X */
         ISB(6, ZP_IND_X, ZP_WRITEBYTE, baddr);
         OPCODE_END

      OPCODE_BEGIN(F8)  /* SED */
         SED();
         OPCODE_END

      OPCODE_BEGIN(F9)  /* SBC $nnnn,Y */
         SBC(4, ABS_IND_Y_BYTE);
         OPCODE_END

      OPCODE_BEGIN(FB)  /* ISB $nnnn,Y */
         ISB(7, ABS_IND_Y, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(FD)  /* SBC $nnnn,X */
         SBC(4, ABS_IND_X_BYTE);
         OPCODE_END

      OPCODE_BEGIN(FE)  /* INC $nnnn,X */
         INC(7, ABS_IND_X, mem_writebyte, addr);
         OPCODE_END

      OPCODE_BEGIN(FF)  /* ISB $nnnn,X */
         ISB(7, ABS_IND_X, mem_writebyte, addr);
         OPCODE_END

#ifdef NES6502_JUMPTABLE
end_execute:

#else /* !NES6502_JUMPTABLE */
      }
   }
#endif /* !NES6502_JUMPTABLE */

   /* store local copy of regs */
   STORE_LOCAL_REGS();

   /* Return our actual amount of executed cycles */
   return (cpu.total_cycles - g_old_cycles);	// MPO : changed old_cycles to g_old_cycles
}

#if 0
void nes6502_init(void)
{
   cpu.a_reg = cpu.x_reg = cpu.y_reg = 0;
   cpu.s_reg = 0xFF;                         /* Stack grows down */
   cpu.burn_cycles = 0;
}
#endif

/* Issue a CPU Reset */
void nes6502_reset(void)
{
   cpu.p_reg = Z_FLAG6502 | R_FLAG6502 | I_FLAG6502;     /* Reserved bit always 1 */
   cpu.int_pending = 0;                      /* No pending interrupts */
   cpu.pc_reg = bank_readword(RESET_VECTOR); /* Fetch reset vector */
   cpu.burn_cycles = RESET_CYCLES;
   cpu.jammed = FALSE;
}

/* Non-maskable interrupt */
void nes6502_nmi(void)
{
   /* local copies of regs */
   uint32_t PC;
   uint8_t A, X, Y, S;

   /* flags */
   uint8_t n_flag, v_flag, b_flag;
   uint8_t d_flag, i_flag, z_flag, c_flag;

   if (FALSE == cpu.jammed)
   {
      GET_GLOBAL_REGS();
      NMI_PROC();
      cpu.burn_cycles += INT_CYCLES;
      STORE_LOCAL_REGS();
   }
}

/* Interrupt request */
void nes6502_irq(void)
{
   /* local copies of regs */
   uint32_t PC;
   uint8_t A, X, Y, S;

   /* flags */
   uint8_t n_flag, v_flag, b_flag;
   uint8_t d_flag, i_flag, z_flag, c_flag;

   if (FALSE == cpu.jammed)
   {
      GET_GLOBAL_REGS();
      if (0 == i_flag)
      {
         IRQ_PROC();
         cpu.burn_cycles += INT_CYCLES;
      }
      else
         cpu.int_pending = 1;
      STORE_LOCAL_REGS();
   }
}

/* Set dead cycle period */
void nes6502_burn(int cycles)
{
   cpu.burn_cycles += cycles;
}

// start MPO
// returns the current _global_ PC
// WARNING : this value will PROBABLY be invalid unless it is called within the cpu-debug.cpp set of functions
// Make sure you understand this or you may be in for some very frustrating debug sessions!
uint32_t nes6502_get_pc()
{
	return cpu.pc_reg;
}

// returns how many cycles have elapsed relative to the beginning of nes6502_execute
uint32_t nes6502_getcycles_sofar()
{
	return (cpu.total_cycles - g_old_cycles);
}

// end MPO
