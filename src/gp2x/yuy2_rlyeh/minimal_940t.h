/*

  GP2X minimal library v0.A by rlyeh, (c) 2005. emulnation.info@rlyeh (swap it!)

  Thanks to Squidge, Robster, snaff, Reesy and NK, for the help & previous work! :-)

  License
  =======

  Free for non-commercial projects (it would be nice receiving a mail from you).
  Other cases, ask me first.

  GamePark Holdings is not allowed to use this library and/or use parts from it.

*/

#include "minimal.h"

#ifndef __MINIMAL_H_STUB_940T__
#define __MINIMAL_H_STUB_940T__

#undef    gp2x_dualcore_data
#define   gp2x_dualcore_data(v)         gp2x_2ndcore_data(v)

#define   main                          gp2x_2ndcore_run

static void gp2x_2ndcore_start(void) __attribute__((naked));

static void gp2x_2ndcore_start(void)
{
  unsigned long gp2x_dequeue(gp2x_queue *q)
  {
   unsigned long data;

   while(!q->items); //waiting for head to increase...

   data=q->place940t[q->tail = (q->tail < q->max_items ? q->tail+1 : 0)];
   q->items--;

   return data;
  }

#define gp2x_dualcore_name_subprogram(name) \
/* 00000020 <_start>:                                                                             */ \
/*    0:*/   asm volatile (".word 0xe59ff02c");  /*        ldr     pc, [pc, #44]  ; 34 <ioffset>  */ \
/*    4:*/   asm volatile (".word 0xe59ff028");  /*        ldr     pc, [pc, #40]  ; 34 <ioffset>  */ \
/*    8:*/   asm volatile (".word 0xe59ff024");  /*        ldr     pc, [pc, #36]  ; 34 <ioffset>  */ \
/*    c:*/   asm volatile (".word 0xe59ff020");  /*        ldr     pc, [pc, #32]  ; 34 <ioffset>  */ \
/*   10:*/   asm volatile (".word 0xe59ff01c");  /*        ldr     pc, [pc, #28]  ; 34 <ioffset>  */ \
/*   14:*/   asm volatile (".word 0xe59ff018");  /*        ldr     pc, [pc, #24]  ; 34 <ioffset>  */ \
/*   18:*/   asm volatile (".word 0xe59ff014");  /*        ldr     pc, [pc, #20]  ; 34 <ioffset>  */ \
/*   1c:*/   asm volatile (".word 0xe59ff010");  /*        ldr     pc, [pc, #16]  ; 34 <ioffset>  */ \
/* 00000020 <_init>:                                                                              */ \
/*   20:*/   asm volatile (".word 0xe59fd010");  /*        ldr     sp, [pc, #16]   ; 38 <stack>         */ \
/*   24:*/   asm volatile (".word 0xe59fc010");  /*        ldr     ip, [pc, #16]   ; 3c <deadbeef>      */ \
/*   28:*/   asm volatile (".word 0xe59fb010");  /*        ldr     fp, [pc, #16]   ; 40 <leetface>      */ \
/*   2c:*/   asm volatile (".word 0xe92d1800");  /*        stmdb   sp!, {fp, ip}                        */ \
/*   30:*/   asm volatile (".word 0xea000004");  /*        b       48 <realinit>                        */ \
/* 00000034 <ioffset>:                                                                                  */ \
/*   34:*/   asm volatile (".word 0x00000020");  /*                                                     */ \
/* 00000038 <stack>:                                                                                    */ \
/*   38:*/   asm volatile (".word 0x000ffffc");  /*                                                     */ \
/* 0000003c <deadbeef>:                                                                                 */ \
/*   3c:*/   asm volatile (".word 0xdeadbeef");  /*                                                     */ \
/* 00000040 <leetface>:                                                                                 */ \
/*   40:*/   asm volatile (".word 0x1ee7face");  /*                                                     */ \
/* 00000044 <area1>:                                                                                    */ \
/*   44:*/   asm volatile (".word 0x00100019");  /*                                                     */ \
/* 00000048 <realinit>:                                                                                 */ \
/*  our main code starts here... */ \
/*  48:*/   asm volatile (".word 0xe3a0003f"); /*        mov     r0, #63 ; 0x3f */ \
/*  4c:*/   asm volatile (".word 0xee060f10"); /*        mcr     15, 0, r0, cr6, cr0, {0} */ \
/*  50:*/   asm volatile (".word 0xee060f30"); /*        mcr     15, 0, r0, cr6, cr0, {1} */ \
/*  54:*/   asm volatile (".word 0xe51f0018"); /*        ldr     r0, [pc, #-24]  ; 44 <area1> */ \
/*  58:*/   asm volatile (".word 0xee060f11"); /*        mcr     15, 0, r0, cr6, cr1, {0} */ \
/*  5c:*/   asm volatile (".word 0xee060f31"); /*        mcr     15, 0, r0, cr6, cr1, {1} */ \
/*  60:*/   asm volatile (".word 0xe3a00001"); /*        mov     r0, #1  ; 0x1            */ \
/*  64:*/   asm volatile (".word 0xee020f10"); /*        mcr     15, 0, r0, cr2, cr0, {0} */ \
/*  68:*/   asm volatile (".word 0xee020f30"); /*        mcr     15, 0, r0, cr2, cr0, {1} */ \
/*  6c:*/   asm volatile (".word 0xee030f10"); /*        mcr     15, 0, r0, cr3, cr0, {0} */ \
/*  70:*/   asm volatile (".word 0xe3a0000f"); /*        mov     r0, #15 ; 0xf            */ \
/*  74:*/   asm volatile (".word 0xee050f10"); /*        mcr     15, 0, r0, cr5, cr0, {0} */ \
/*  78:*/   asm volatile (".word 0xee050f30"); /*        mcr     15, 0, r0, cr5, cr0, {1} */ \
/*  7c:*/   asm volatile (".word 0xee110f10"); /*        mrc     15, 0, r0, cr1, cr0, {0} */ \
/*  80:*/   asm volatile (".word 0xe3800001"); /*        orr     r0, r0, #1      ; 0x1    */ \
/*  84:*/   asm volatile (".word 0xe3800004"); /*        orr     r0, r0, #4      ; 0x4    */ \
/*  88:*/   asm volatile (".word 0xe3800a01"); /*        orr     r0, r0, #4096   ; 0x1000 */ \
/*  8c:*/   asm volatile (".word 0xe3800103"); /*        orr     r0, r0, #-1073741824    ; 0xc0000000 */ \
/*  90:*/   asm volatile (".word 0xee010f10"); /*        mcr     15, 0, r0, cr1, cr0, {0} */ \
   while(1) gp2x_2ndcore_run(gp2x_dequeue((gp2x_queue *)gp2x_2ndcore_data_ptr(GP2X_QUEUE_ARRAY_PTR))); \
} \
void gp2x_dualcore_launch_##name##_subprogram(void) { gp2x_dualcore_launch_program((unsigned long *)&gp2x_2ndcore_start, ((int)&gp2x_dualcore_launch_##name##_subprogram)-((int)&gp2x_2ndcore_start)); }

#endif


