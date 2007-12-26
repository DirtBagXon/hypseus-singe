/*
 * cpu.cpp
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

// cpu
// by Matt Ownby
// Designed to do all of the universal CPU functions needed for the emulator

#ifdef _XBOX
#include "stdafx.h"
#include "xbox_grafx.h"
#endif

#ifdef DEBUG
#include <assert.h>
#endif

#include "cpu.h"
#include <stdio.h>	// for stderr
#include <string.h>	// for memcpy
#include "../daphne.h"
#include "../game/game.h"
#include "../ldp-out/ldp.h"	// to call pre_think
#include "../timer/timer.h"
#include "../io/input.h"
#include "../io/conout.h"
#include "../sound/sound.h"
#include "6809infc.h"
#include "nes6502.h"
#include "nes_6502.h"
#include "mamewrap.h"
#include "generic_z80.h"
#include "cop.h"
#include "x86/i86intf.h"
#include "../ldp-in/ldv1000.h"	// for ldv1000_reset for strobe stuff
#include "../ldp-in/ldp1000.h"	// for ldp1000_reset
#include "../ldp-in/vp931.h"

#ifdef _XBOX
#include "gamepad.h"
#include "video\video.h"
#include "xstuff.h"
#include "daphne_xbox.h"
#include "ldp-out\ldp-vldp.h"

#endif

#include <stack>	// for cpu pausing operations

using namespace std;

stack <Uint32> g_cpu_paused_timer;	// the time we were at when cpu_pause_timer was called
bool g_cpu_paused = false;

struct cpudef *g_head = NULL;	// pointer to the first cpu in our linked list of cpu's
unsigned char g_cpu_count = 0;	// how many cpu's have been added
bool g_cpu_initialized[CPU_COUNT] = { false };	// whether cpu core has been initialized
Uint32 g_cpu_timer = 0;	// used to make cpu's run at the right speed
Uint32 g_expected_elapsed_ms = 0;	// how many ms we expect to have elapsed since last cpu execution loop
Uint8 g_active_cpu = 0;	// which cpu is currently active
unsigned int g_uInterleavePerMs = 1; // number of times the cpus switch in 1 ms 

// How many milliseconds the CPU emulation is lagging behind.
// So that OpenGL mode knows when to drop frames to get back up to speed (vsync-enabled only)
unsigned int g_uCPUMsBehind = 0;

// Uncomment this if you want to test the CPUs to make sure they are running at the proper speed
//#define CPU_DIAG 1

#ifdef CPU_DIAG
// how many cpu's cpu diag can support (this has nothing to do with how many cpu's daphne can support)
#define CPU_DIAG_CPUCOUNT 10
	static unsigned int cd_cycle_count[CPU_DIAG_CPUCOUNT];	// for speed test
	static unsigned int cd_old_time[CPU_DIAG_CPUCOUNT] = { 0 }; // " " "
	static unsigned int cd_irq_count[CPU_DIAG_CPUCOUNT][MAX_IRQS] = { { 0 } };
	static unsigned int cd_nmi_count[CPU_DIAG_CPUCOUNT] = { 0 };
	static double cd_avg_mhz[CPU_DIAG_CPUCOUNT] = { 0.0 };	// average MHz of current cpu
	static int cd_report_count[CPU_DIAG_CPUCOUNT] = { 0 };	// how many reports have been given
	
	static unsigned int cd_extra_ms = 0;	// how many extra ms we have leftover (so we can calculate resource usage)
#endif

//////////////////////////////////////////////////////////////////////////////////

// adds a cpu to our linked list.  The data is copied, so you can clobber the original data after this call.
void add_cpu (struct cpudef *candidate)
{
	struct cpudef *cur = NULL;
	
	// if this is the first cpu to be added to the list
	if (!g_head)
	{
		g_head = new struct cpudef;	// allocate a new cpu, assume allocation is successful
		cur = g_head;	// point to the new cpu so we can populate it with info
	}
	// else we have to move to the end of the list
	else
	{
		cur = g_head;

		// go to the last cpu in the list
		while (cur->next_cpu)
		{
			cur = cur->next_cpu;
		}

		cur->next_cpu = new struct cpudef;	// allocate a new cpu at the end of our list
		cur = cur->next_cpu;	// point to the new cpu so we can populate it with info
	}

	// now we must copy over the relevant info
	memcpy(cur, candidate, sizeof(struct cpudef));	// copy entire thing over
	cur->id = g_cpu_count;
	g_cpu_count++;

	// DEFAULT VALUES
	cur->ascii_info_callback = generic_ascii_info_stub;
	cur->elapsedcycles_callback = generic_cpu_elapsedcycles_stub;
	cur->getpc_callback = NULL;
	cur->dasm_callback = generic_dasm_stub;
	// END DEFAULT VALUES

	// now we must assign the appropriate callbacks
	switch (cur->type)
	{
	case CPU_Z80:
#ifdef USE_M80
		cur->init_callback = m80_reset;
		cur->shutdown_callback = NULL;
		cur->setmemory_callback = m80_set_opcode_base;
		cur->execute_callback = m80_execute;
		cur->getcontext_callback = m80_get_context;
		cur->setcontext_callback = m80_set_context;
		cur->getpc_callback = m80_get_pc;
		cur->setpc_callback = m80_set_pc;
		cur->elapsedcycles_callback = m80_get_cycles_executed;
		cur->reset_callback = m80_reset;
		cur->ascii_info_callback = m80_info;
		cur->dasm_callback = m80_dasm;
		m80_set_irq_callback(generic_m80_irq_callback); // this needs to be done here to allow games to override
		m80_set_nmi_callback(generic_m80_nmi_callback); // " " "
#endif
#ifdef USE_Z80
		cur->init_callback = z80_reset;
		cur->shutdown_callback = z80_exit;
		cur->setmemory_callback = mw_z80_set_mem;
		cur->execute_callback = z80_execute;
		cur->getcontext_callback = z80_get_context;
		cur->setcontext_callback = z80_set_context;
		cur->getpc_callback = z80_get_pc;
		cur->setpc_callback = z80_set_pc;
		cur->reset_callback = z80_reset;
		cur->ascii_info_callback = z80_info;
		cur->dasm_callback = z80_dasm;
		z80_set_irq_callback(generic_z80_irq_callback); // this needs to be called here to allow games to override
		set_amask(0xFFFF);	// 16-bit address space
#endif
		break;

	case CPU_M6809:
		cur->init_callback = initialize_m6809;
		cur->shutdown_callback = NULL;
		cur->setmemory_callback = m6809_set_memory;
		cur->execute_callback = mc6809_StepExec;
		cur->getcontext_callback = NULL;
		cur->setcontext_callback = NULL;
		cur->getpc_callback = mc6809_GetPC;
		cur->setpc_callback = NULL;
		cur->reset_callback = m6809_reset;
		cur->ascii_info_callback = mc6809_info;
		break;

	case CPU_M6502:
		cur->init_callback = generic_6502_init;
		cur->shutdown_callback = generic_6502_shutdown;
		cur->setmemory_callback = generic_6502_setmemory;
		cur->execute_callback = nes6502_execute;
		cur->getcontext_callback = generic_6502_getcontext;
		cur->setcontext_callback = generic_6502_setcontext;
		cur->setpc_callback = NULL;
		cur->getpc_callback = nes6502_get_pc;
		cur->elapsedcycles_callback = nes6502_getcycles_sofar;
		cur->reset_callback = generic_6502_reset;
		cur->ascii_info_callback = generic_6502_info;
		break;
	case CPU_COP421:
		cur->init_callback = cop421_reset;
		cur->shutdown_callback = NULL;
		cur->setmemory_callback = cop421_setmemory;
		cur->execute_callback = cop421_execute;
		cur->getcontext_callback = NULL;
		cur->setcontext_callback = NULL;
		cur->setpc_callback = NULL;
		cur->reset_callback = cop421_reset;
		break;
	case CPU_I88:
		cur->init_callback = i86_init;
		cur->shutdown_callback = i86_exit;
		cur->setmemory_callback = mw_i86_set_mem;
		cur->execute_callback = i86_execute;
		cur->getcontext_callback = i86_get_context;
		cur->setcontext_callback = i86_set_context;
		cur->getpc_callback = i86_get_pc;
		cur->setpc_callback = i86_set_pc;
		cur->reset_callback = i86_reset;
		cur->ascii_info_callback = i86_info;
		cur->dasm_callback = i86_dasm;
		set_amask(0xFFFFF);	// 20-bit address space
		break;
	default:
		printline("FATAL ERROR : unknown cpu added");
		set_quitflag();	// force user to deal with this problem
		break;
	}
}

// de-allocate all cpu's that have been allocated
void del_all_cpus()
{
	struct cpudef *cur = g_head;
	struct cpudef *tmp = NULL;
	
	// while we have cpu's left to delete
	while (cur)
	{
		tmp = cur;
		cur = cur->next_cpu;
		delete tmp;	// de-allocate
	}
	g_head = NULL;
	g_cpu_count = 0;

}

// recalculations all expensive calculations
// (put in one place to make maintenance easier)
void cpu_recalc()
{
	struct cpudef *cpu = g_head;

	// re-calculate all cycles per interleave for the new interleave value
	while (cpu)
	{
		cpu->uCyclesPerInterleave = cpu->hz / g_uInterleavePerMs / 1000;
		cpu->uNMIMicroPeriod = (unsigned int) ((cpu->nmi_period * 1000) + 0.5);	// convert to int for faster math on gp2x
	
		for (int i = 0; i < MAX_IRQS; i++)
		{
			cpu->uIRQMicroPeriod[i] = (unsigned int) ((cpu->irq_period[i] * 1000) + 0.5);	// convert to int for faster math on gp2x
		}

		cpu = cpu->next_cpu;
	}
}

// initializes all cpus
void cpu_init()
{
	struct cpudef *cur = g_head;
	
	while (cur)
	{
		g_active_cpu = cur->id;
#ifdef CPU_DIAG
		cd_old_time[g_active_cpu] = refresh_ms_time();
#endif

		cpu_recalc();

		cur->pending_nmi_count = 0;
		for (int i = 0; i < MAX_IRQS; i++)
		{
			cur->pending_irq_count[i] = 0;
		}
		cur->total_cycles_executed = 0;
		cur->uEventCyclesEnd = 0;
		cur->uEventCyclesExecuted = 0;
		cur->event_callback = NULL;

		// if the cpu core has not been initialized yet, then do so .. it should only be done once per cpu core
		if (!g_cpu_initialized[cur->type])
		{
			(cur->init_callback)();	// initialize the cpu
			g_cpu_initialized[cur->type] = true;
		}
		(cur->setmemory_callback)(cur->mem);	// set where the memory is located
		
		// if we have a set PC callback defined
		if (cur->setpc_callback != NULL)
		{
			(cur->setpc_callback)(cur->initial_pc);	// set the initial program counter for the cpu
		}

		// if we are required to copy the cpu context, then get the info now
		if (cur->must_copy_context)
		{
			unsigned int context_size = (cur->getcontext_callback)(cur->context);

			// sanity check
			if (context_size > MAX_CONTEXT_SIZE)
			{
				fprintf(stderr, "FATAL ERROR : Increase MAX_CONTEXT_SIZE to at least %u and recompile\n", context_size);
				set_quitflag();
			}
		}
		
		cur = cur->next_cpu;	// advance to the next cpu

	} // end while

	// this is needed to get multiple 6502's running properly, and it shouldn't hurt other
	//  CPUs if they are implemented correctly.
	cpu_reset();

	reset_ldv1000();	// calculate strobe stuff, most games won't need this but it doesn't hurt
	reset_ldp1000();	// calculate ACK latency stuff, most games won't need this but it doesn't hurt
	reset_vp931();
}

// shutdown all cpu's
void cpu_shutdown()
{
	struct cpudef *cur = g_head;
	
	// go through each cpu and shut it down
	while (cur)
	{
		g_active_cpu = cur->id;
		// if we have a shutdown callback defined
		if (cur->shutdown_callback)
		{
			// we only want to shutdown the cpu core once
			if (g_cpu_initialized[cur->type] == true)
			{
				(cur->shutdown_callback)();	// shutdown the cpu
				g_cpu_initialized[cur->type] = false;
			}
		}
		cur = cur->next_cpu; // move to the next cpu entry
	}
	
	del_all_cpus();
}



// executes all cpu cores "simultaneously".  this function only returns when the game exits
void cpu_execute()
{
	int i = 0;
	Uint32 last_inputcheck = 0; //time we last polled for input events
	struct cpudef *cpu = g_head;

	// flush the cpu timers one time so we don't begin with the cpu's running too quickly
	g_expected_elapsed_ms = 0;
	g_cpu_timer = refresh_ms_time();	// so the cpu doesn't run too quickly when we first start

	// clear each cpu
	while (cpu)
	{
		for (int i = 0; i < MAX_IRQS; i++)
		{
			cpu->uIRQTickCount[i] = 0;
			cpu->uIRQTickBoundaryMs[i] = cpu->uIRQMicroPeriod[i] / 1000;	// when the 1st IRQ will tick
		//	cpu->irq_cycle_count[i] = 0;
		}
		cpu->uNMITickCount = 0;
		cpu->uNMITickBoundaryMs = cpu->uNMIMicroPeriod / 1000;	// when the 1st NMI will tick
		//cpu->nmi_cycle_count = 0;
		cpu->total_cycles_executed = 0;		
		cpu = cpu->next_cpu;
	}
	// end flushing the cpu timers

	// loop until the quit flag is set which means the user wants to quit the program
	while (!get_quitflag())
	{
		unsigned int actual_elapsed_ms = 0;
		bool nmi_asserted = false;
		Uint32 elapsed_cycles = 0;
		Uint32 cycles_to_execute = 0;	// how many cycles to execute this time around

		// we want to execute enough cycles to reach our expectation for # of elapsed ms
		g_expected_elapsed_ms++;

		// run all cpu's for 1 ms's worth of cycles, interleaving them according to
		//  the value of g_uInterlavePerMs.
		for (unsigned int uInterleaveCount = 1; uInterleaveCount <= g_uInterleavePerMs; uInterleaveCount++)
		{
			cpu = g_head;
			// go through each cpu and execute 1 ms worth of cycles
			while (cpu)
			{
				// if we are required to copy the cpu context, then set the context for the current cpu
				if (cpu->must_copy_context)
				{
					(cpu->setcontext_callback)(cpu->context);	// restore registers
					(cpu->setmemory_callback)(cpu->mem);	// restore memory we're working with
				}
				g_active_cpu = cpu->id;

				nmi_asserted = false;

				// NOTE: if g_uInterleavePerMs is 1, then this calculation is the same as
				//  (g_expected_elapsed_ms * cpu->hz) / 1000
				Uint64 u64ExpectedCycles = (( ((Uint64) (g_expected_elapsed_ms - 1)) * cpu->hz) / 1000) +
					(cpu->uCyclesPerInterleave * uInterleaveCount);

				if (u64ExpectedCycles > cpu->total_cycles_executed)
				{
#ifdef DEBUG
					// make sure this will fit in a 32-bit number
					assert((u64ExpectedCycles - cpu->total_cycles_executed) < (unsigned int) (1 << 31));
#endif
					// calculate # of cycles to execute
					cycles_to_execute = (Uint32) (u64ExpectedCycles - cpu->total_cycles_executed);

					// if we have no upcoming event
					if (cpu->uEventCyclesEnd == 0)
					{
						// get us up to our expected elapsed MS
						elapsed_cycles = (cpu->execute_callback)((Uint32) cycles_to_execute);

						cpu->total_cycles_executed += elapsed_cycles;	// always track how many cycles have elapsed
					}
					// else we have an active event going on, check to see if we need to execute less cycles in order to fire event
					else
					{
						// loop while we have the possibility of an event looming near in our future
						while (cpu->uEventCyclesEnd != 0)
						{
							unsigned int uCyclesTilEvent = 0;
	
							// if we haven't yet reached our event
							if (cpu->uEventCyclesEnd > cpu->uEventCyclesExecuted)
							{
								uCyclesTilEvent = cpu->uEventCyclesEnd - cpu->uEventCyclesExecuted;
							}
							// else we overshot our event so just leave uCyclesTilEvent at 0
							// (this can happen because the cpu emulator can execute more cycles than we request)
	
							// if we need to execute less cycles than we planned in order to do our event
							if (cycles_to_execute > uCyclesTilEvent)
							{
								elapsed_cycles = (cpu->execute_callback)(uCyclesTilEvent);
								cpu->total_cycles_executed += elapsed_cycles;	// always track how many cycles have elapsed
	
#ifdef CPU_DIAG
								cd_cycle_count[g_active_cpu] += elapsed_cycles;
#endif // CPU_DIAG
	
#ifdef DEBUG
								assert(cycles_to_execute >= uCyclesTilEvent);
#endif // DEBUG
								cycles_to_execute -= uCyclesTilEvent;	// NOTE : we can't subtract elapsed_cycles because it may be greater than cycles_to_execute
								cpu->uEventCyclesEnd = 0;	// event has been fired, we're done ...
	
								// This callback should be called after uEventcycles is set to 0 because the callback may immediately
								//  setup another event.
								(cpu->event_callback)(cpu->event_data);	// call event callback
							}
							// else the event isn't in our near future, so proceed as normal
							else
							{
								break;
							}
						}

						// get us up to our expected elapsed MS
						elapsed_cycles = (cpu->execute_callback)((Uint32) cycles_to_execute);

						cpu->total_cycles_executed += elapsed_cycles;	// always track how many cycles have elapsed
						cpu->uEventCyclesExecuted += elapsed_cycles;
					}
				}
				// else if we executed too many cycles last time, then we have to just kill time
				else
				{
					elapsed_cycles = 0;
				}

#ifdef CPU_DIAG
				cd_cycle_count[g_active_cpu] += elapsed_cycles;
				cd_avg_mhz[g_active_cpu] = (cpu->total_cycles_executed * 0.001) / elapsed_ms_time(g_cpu_timer);
#endif

				// NOW WE CHECK TO SEE IF IT'S TIME TO DO AN NMI

				// if NMI's are enabled
				if (cpu->uNMIMicroPeriod)
				{
					if (g_expected_elapsed_ms > cpu->uNMITickBoundaryMs)
					{
						++cpu->pending_nmi_count;
						++cpu->uNMITickCount;
						cpu->uNMITickBoundaryMs = (Uint32) (( ((Uint64) (cpu->uNMITickCount + 1)) * cpu->uNMIMicroPeriod) / 1000);
#ifdef CPU_DIAG
						++cd_nmi_count[cpu->id];
#endif
					}
				}

				// if we have an NMI waiting
				// (this can be created either by a timer, or by calling cpu_generate_nmi)
				if (cpu->pending_nmi_count != 0)
				{
					g_game->do_nmi();
					nmi_asserted = true;
					--cpu->pending_nmi_count;
				}

				// NOW WE CHECK TO SEE IF IT'S TIME TO DO AN IRQ

				// go through each IRQ
				for (i = 0; i < MAX_IRQS; i++)
				{
					// if IRQ exists
					if (cpu->uIRQMicroPeriod[i])
					{
						// if it's time to do an IRQ
						if (g_expected_elapsed_ms > cpu->uIRQTickBoundaryMs[i])
						{
							++cpu->pending_irq_count[i];
							++cpu->uIRQTickCount[i];
							cpu->uIRQTickBoundaryMs[i] = (Uint32) (( ((Uint64) (cpu->uIRQTickCount[i] + 1)) *
								cpu->uIRQMicroPeriod[i]) / 1000);
#ifdef CPU_DIAG
							++cd_irq_count[cpu->id][i];
#endif
						}
					} // end if there is an IRQ timer

					// if we have an IRQ waiting
					// (this can be created either by a timer or by calling cpu_generate_irq)
					if (cpu->pending_irq_count[i] != 0)
					{
						// we don't want to do IRQ's and NMI's at the same time
						if (!nmi_asserted)
						{
							g_game->do_irq(i);
#ifdef DEBUG
							assert(cpu->pending_irq_count[i] > 0);
#endif
							--cpu->pending_irq_count[i];
							break;	// break out of for loop because we only want to assert 1 IRQ per loop
						}
#ifdef DEBUG
						// make sure NMI's aren't smothering IRQ's
						else if (cpu->pending_irq_count[i] > 5)
						{
							printline("cpu.cpp WARNING : IRQ's are piling up and not having a chance to get used");
						}
						// else nothing ...
#endif
					}
				} // end for loop
				// END CHECK FOR IRQ

				// this chunk of code tests to make sure the CPU is running
				// at the proper speed.  It should be undef'd unless we are debugging cpu stuff

#ifdef CPU_DIAG
				char s[160] = { 0 };

#define CPU_DIAG_ACCURACY 24000000
				// the bigger the #, the more accurate the result

				// if it's time to print some statistics
				if (cd_cycle_count[g_active_cpu] >= CPU_DIAG_ACCURACY)
				{
					Uint32 elapsed_ms = elapsed_ms_time(cd_old_time[g_active_cpu]);
					double cur_mhz = ((double) cd_cycle_count[g_active_cpu] / (double) elapsed_ms) * 0.001;
					cd_report_count[g_active_cpu]++;

					sprintf(s,"CPU #%d : cycles = %d, time = %d ms, MHz = %f, avg MHz = %f",
						g_active_cpu,
						cd_cycle_count[g_active_cpu], elapsed_ms,
						cur_mhz, cd_avg_mhz[g_active_cpu]);
					printline(s);
					sprintf(s, "         NMI's = %d ", cd_nmi_count[g_active_cpu]);
					cd_nmi_count[g_active_cpu] = 0;
					outstr(s);
					for (int irqi = 0; irqi < MAX_IRQS; irqi++)
					{
						sprintf(s, "IRQ%d's = %d ", irqi, cd_irq_count[g_active_cpu][irqi]);
						outstr(s);
						cd_irq_count[g_active_cpu][irqi] = 0;
					}
					newline();
					cd_old_time[g_active_cpu] += elapsed_ms;
					cd_cycle_count[g_active_cpu] -= CPU_DIAG_ACCURACY;
					
					sprintf(s, "Resource Usage: %u percent", 100 - ((cd_extra_ms * 100) / elapsed_ms));
					printline(s);
					cd_extra_ms = 0;	// reset
				}
#endif




				// if we are required to copy the cpu context, then preserve the context for the next time around
				if (cpu->must_copy_context)
				{
					(cpu->getcontext_callback)(cpu->context);	// preserve registers
				}

				cpu = cpu->next_cpu; // go to the next cpu

			} // end while looping through each cpu
		} // end for loop

		// 1 ms has elapsed, so notify the LDP to keep it in sync (we must do this after every ms)
		g_ldp->pre_think();
 
		// Update the sound buffers for the sound chips
		update_soundbuffer();

		// BEGIN FORCING EMULATOR TO RUN AT PROPER SPEED

		// we have executed 1 ms worth of cpu cycles before this point, so slow down if 1 ms has not passed
		actual_elapsed_ms = elapsed_ms_time(g_cpu_timer);

#ifdef CPU_DIAG
		unsigned int uStartMs = actual_elapsed_ms;
#endif

		// if we're behind, then compute how far behind we are ...
		if (actual_elapsed_ms > g_expected_elapsed_ms)
		{
			g_uCPUMsBehind = actual_elapsed_ms - g_expected_elapsed_ms;
		}
		// else we're caught up or ahead
		else
		{
			g_uCPUMsBehind = 0;

			// if not enough time has elapsed, slow down
			while (g_expected_elapsed_ms > actual_elapsed_ms)
			{
#ifndef _XBOX
				SDL_Delay(1);
#else
				XBOX_Delay(1);
#endif
				actual_elapsed_ms = elapsed_ms_time(g_cpu_timer);
			}
		}

#ifdef CPU_DIAG
		// track ms that we slept
		cd_extra_ms += (actual_elapsed_ms - uStartMs);
#endif
		
		// END FORCING CPU TO RUN AT PROPER SPEED

#ifdef DEBUG
		// the cpu should ideally not be paused at this point because if it is, it will
		// lead to inaccuracies.  It would be better to have a boolean that requests
		// for the cpu to be paused, and then if that boolean is true, to pause the
		// cpu at this point.  That would be more accurate.
		assert(!g_cpu_paused);
#endif

		do
		{
			// limit checks for input events to every 16 ms
			//(this is really expensive in Windows for some reason)
			actual_elapsed_ms = elapsed_ms_time(last_inputcheck);
			if (actual_elapsed_ms > 16)
			{
				last_inputcheck = refresh_ms_time();
#ifndef _XBOX
				SDL_check_input();	// check for input events (keyboard, joystick, etc)
#else
				//if(g_game->get_game_type() != GAME_LAIR2)
					XBOX_ReadPads();

				// check for quit to menu
				if (XBOX_QuitGame())
				{
					if (VideoThreadActive || GameInfo.LaserIndex == NOLDP)
						set_quitflag();
				}

				// draw the scoreboard if we need to
				if ( (GameInfo.LaserIndex == NOLDP) || (VideoThreadActive == FALSE))
				{
					display_repaint();
					XBOX_RenderScene();
				}			
#endif
			}

			// be nice to cpu if we're looping here ...
			if (g_cpu_paused)
			{
				make_delay(1);
			}

		} while (g_cpu_paused && !get_quitflag());	// the only time this should loop is if the user pauses the game
	} // end while quitflag is not true
}

// sets the PC on all cpu's to their initial PC values.
// in the future this might reset the context too, but for now let's see if this is sufficient
// to reboot all our games
void cpu_reset()
{
	struct cpudef *cpu = g_head;
	
	// reset each cpu
	while (cpu)
	{
		// set the context if we need to
		if (cpu->must_copy_context)
		{
			(cpu->setcontext_callback)(cpu->context);	// restore registers
			(cpu->setmemory_callback)(cpu->mem);	// restore memory we're working with
		}
		
		(cpu->reset_callback)();

		// if this callback has been defined, then make use of it
		if (cpu->setpc_callback)
		{
			(cpu->setpc_callback)(cpu->initial_pc);	// set the initial program counter
		}
		
		// save the context if we need to
		if (cpu->must_copy_context)
		{
			(cpu->getcontext_callback)(cpu->context);	// preserve registers
		}

		cpu = cpu->next_cpu;
	}
}

void cpu_set_event(unsigned int uCpuID, unsigned int uCyclesTilEvent, void (*event_callback)(void *data), void *event_data)
{
	struct cpudef *cpu = get_cpu_struct(uCpuID);

	if (cpu)
	{
		// reset event stuff
		cpu->event_callback = event_callback;
		cpu->uEventCyclesEnd = uCyclesTilEvent;
		cpu->uEventCyclesExecuted = 0;
		cpu->event_data = event_data;
	}

	// make programmer fix this problem :)
	else
	{
		printline("cpu_set_event() : can't find CPU, fix this!");
		set_quitflag();
	}
}

// Recursively pauses cpu execution
//  call this right before you do a function that may take a long time to return from (such as spinning up a laserdisc player)
// Why is this recursive? Because ldp, cpu-debug and thayer's quest can all call cpu_pause,
//  and the user can pause the game (which calls cpu_pause).  It's conceivable that the
//  user could pause the game, then break into debug mode which would give us two
//  pauses on top of each other.
// Personally, I'd prefer to put in an assert that guarantees that the cpu can only be
//  paused by 1 function at a time, but that is on the future TODO ...
void cpu_pause()
{
	g_cpu_paused_timer.push(refresh_ms_time());
	g_cpu_paused = true;
#ifdef DEBUG
//	printline("CPU paused...");
#endif
}

// Recursively unpauses cpu execution
// call this right after you do a function that may take a long time to return from (such as spinning up a laserdisc player)
// This function simply adjust the cpu timer so it appears as if no time has elapsed since cpu_pause_timer was called.
void cpu_unpause()
{
	// safety check
	if (g_cpu_paused_timer.size() > 0)
	{
		g_cpu_timer = refresh_ms_time() - (g_cpu_paused_timer.top() - g_cpu_timer);
		g_cpu_paused_timer.pop();

		// if our pause stack is empty, then we can finally, safely, unpause
		if (g_cpu_paused_timer.size() == 0)
		{
			g_cpu_paused = false;
		}
		// else we are still paused because our stack isn't empty

#ifdef DEBUG
//		printline("CPU unpaused...");
#endif
	}
	else
	{
		printline("cpu_unpause_timer() error : cpu wasn't paused!");
	}
}

// returns the timer used by all cpu's to run at the proper speed
// WARNING: this timer is reset by flush_cpu_timers
Uint32 get_cpu_timer()
{
	return g_cpu_timer;
}

// returns the total # of cycles that have elapsed 
// This is very useful in determining how much "time" has elapsed for time critical things like controlling the PR-8210
// laserdisc player
// WARNING : flush_cpu_timers will reset the total_cycles_executed so you must always check
// for this by making sure latest result is greater than previous result
// Failure to check for this will result in some very puzzling and frustrating bugs
Uint64 get_total_cycles_executed(Uint8 id)
{
	Uint64 result = 0;	
	struct cpudef *cpu = get_cpu_struct(id);
	
	if (cpu)
	{
		result = cpu->total_cycles_executed + (cpu->elapsedcycles_callback)();
	}
	return result;
}

// returns the pointer to the cpu structure using the cpu id as input
// returns NULL if the cpu doesn't exist
struct cpudef *get_cpu_struct(Uint8 id)
{
	struct cpudef *result = NULL;
	struct cpudef *cpu = g_head;
	
	while (cpu)
	{
		if (cpu->id == id)
		{
			result = cpu;
			break;
		}
		cpu = cpu->next_cpu;
	}
	
	return result;
}

// returns the current active cpu.  First cpu is 0
unsigned char cpu_getactivecpu()
{
	return(g_active_cpu);
}

// returns the location of the memory for the indicated cpu
// returns NULL of the indicated cpu has no memory (ie if the cpu does not exist)
Uint8 *get_cpu_mem(Uint8 id)
{
	Uint8 *result = NULL;
	struct cpudef *cpustruct = get_cpu_struct(id);

	// if the cpu exists, then we can return its memory	
	if (cpustruct)
	{
		result = cpustruct->mem;
	}
	// else the cpu does not exist in which case we return null

	return result;
}

// returns the Hz of the CPU indicated, or 0 if the cpu does not exist
Uint32 get_cpu_hz(Uint8 id)
{
	Uint32 result = 0;
	struct cpudef *cpustruct = get_cpu_struct(id);

	// if the cpu exists, then we can return its memory	
	if (cpustruct)
	{
		result = cpustruct->hz;
	}

	return result;
}

void cpu_change_nmi(Uint8 id, double new_period)
{
	struct cpudef *cpu = get_cpu_struct(id);
	
	if (cpu)
	{
		cpu->nmi_period = new_period;
		cpu_recalc();
	}
	else
	{
		fprintf(stderr, "ERROR : Attempted to change nmi period for cpu %d which does not exist\n", id);
	}
}

void cpu_generate_nmi(Uint8 cpu_id)
{
	struct cpudef *cpu = get_cpu_struct(cpu_id);

#ifdef DEBUG
	assert(cpu);
#endif

	cpu->pending_nmi_count++;
}

void cpu_change_irq(Uint8 id, unsigned int which_irq, double new_period)
{
#ifdef DEBUG
	assert(which_irq < MAX_IRQS);
#endif

	struct cpudef *cpu = get_cpu_struct(id);
	
#ifdef DEBUG
	assert(cpu);
#endif

	cpu->irq_period[which_irq] = new_period;
	cpu_recalc();
//	cpu->cycles_per_irq[which_irq] = (Uint32) (cpu->cycles_per_ms * cpu->irq_period[which_irq]);
//	cpu->irq_cycle_count[which_irq] = 0;

}

void cpu_generate_irq(Uint8 cpu_id, unsigned int which_irq)
{
#ifdef DEBUG
	assert(which_irq < MAX_IRQS);
#endif

	struct cpudef *cpu = get_cpu_struct(cpu_id);

#ifdef DEBUG
	assert (cpu);
#endif

	cpu->pending_irq_count[which_irq]++;
}

//////////////////////////////////////////////////////////////////////////////////

static NES_6502* g_6502 = NULL;

// the glue between daphne and the 6502 core we're using
void generic_6502_init()
{
	g_6502 = new NES_6502();

	generic_6502_setmemory(get_cpu_mem(g_active_cpu));

	NES_6502::Reset();
}

void generic_6502_shutdown()
{
	delete g_6502;
	g_6502 = NULL;
}

void generic_6502_reset()
{
	NES_6502::Reset();
}

void generic_6502_setmemory(Uint8 *buf)
{
    NES_6502::Context context;

    memset((void*)&context, 0x00, sizeof(context));
    g_6502->GetContext(&context);
			
    context.mem_page[0] = &buf[0x0000];
    context.mem_page[1] = &buf[0x2000];
    context.mem_page[2] = &buf[0x4000];
    context.mem_page[3] = &buf[0x6000];
    context.mem_page[4] = &buf[0x8000];
    context.mem_page[5] = &buf[0xa000];
    context.mem_page[6] = &buf[0xc000];
    context.mem_page[7] = &buf[0xe000];

    g_6502->SetContext(&context);

}

Uint32 generic_6502_getcontext(void *context_buf)
{
	g_6502->GetContext((NES_6502::Context *) context_buf);
	return sizeof(NES_6502::Context);
}

void generic_6502_setcontext(void *context_buf)
{
	g_6502->SetContext( (NES_6502::Context *) context_buf);
}

// returns an ASCII string giving info about registers and other stuff ...
const char *generic_6502_info(void *unused, int regnum)
{
	static char buffer[1][81];
	static int which = 0;

	NES_6502::Context context;

	memset((void*)&context, 0x00, sizeof(context));
	g_6502->GetContext(&context);

	buffer[which][0] = 0;	// make sure the current string is empty

	// find which register they are requesting info about
	switch( regnum )
	{
	case CPU_INFO_REG+0: sprintf(buffer[which], "PC:%04X", context.pc_reg); break;
	case CPU_INFO_REG+1: sprintf(buffer[which], " A:%02X", context.a_reg); break;
	case CPU_INFO_REG+2: sprintf(buffer[which], " X:%02X", context.x_reg); break;
	case CPU_INFO_REG+3: sprintf(buffer[which], " Y:%02X", context.y_reg); break;

	case CPU_INFO_FLAGS:
		sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
				context.p_reg & 0x80 ? 'N':'.',
				context.p_reg & 0x40 ? 'V':'.',
				context.p_reg & 0x20 ? 'R':'.',
				context.p_reg & 0x10 ? 'B':'.',
				context.p_reg & 0x08 ? 'D':'.',
				context.p_reg & 0x04 ? 'I':'.',
				context.p_reg & 0x02 ? 'Z':'.',
				context.p_reg & 0x01 ? 'C':'.');
		break;
	}

	return buffer[which];
}

////////////////////////////////////////

// just a stub if the cpu core cannot return current # of elapsed cycles
// this always returns 0
Uint32 generic_cpu_elapsedcycles_stub()
{
	return 0;
}

// just a stub if the cpu doesn't support info-styled functions
const char *generic_ascii_info_stub(void *context, int regnum)
{
	const char *result = "";

	if (regnum == 0)
	{
		result = "<NO INFO FUNCTION AVAILABLE>";
	}
	return result;
}

// just a stub in case cpu doesn't support disassembly
unsigned int generic_dasm_stub( char *buffer, unsigned pc )
{
	strcpy(buffer, "<DASM NOT AVAILABLE>");
	return 1;
}

// WARNING : this function appears not to de-allocate anything in g_head's linked list,
//  so don't use it unless you have verified that g_head has been de-allocated first.
// (I think it was added for xbox daphne)
void reset_cpu_globals()
{
	g_head = NULL;
	g_cpu_count = 0;
	for (int i=0; i<CPU_COUNT; i++)
		g_cpu_initialized[i] = false;
	g_expected_elapsed_ms = 0;
	g_active_cpu = 0;
}

void cpu_change_interleave(unsigned int uInterleave)
{
	// safety check, interleave must be >= 1, as it is used as a denominator
	if (uInterleave > 0)
	{
		g_uInterleavePerMs = uInterleave;

		cpu_recalc();	// recalculate interleave value
	}
	// else we got an illegal value
	else
	{
		printline("cpu_change_interlave got 0, which is illegal.. fix this!");
		set_quitflag();	// force developer to fix this :)
	}
}
