/*
 * game.h
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

// game.h
// by Matt Ownby

// declares a generic game class structure

// To add a new game to DAPHNE, make a new class that inherits from the game class
// You must supply a constructor, but you do not need to supply
// any other functions if you don't want to (but your game won't work if you don't hehe)
// See Dragon's Lair (no port functions) and Cliff Hanger (no memory functions) for examples of how you don't
// have to redeclare every single function

#ifndef GAME_H
#define GAME_H

// we allow up to triple buffering
#define MAX_VIDEO_OVERLAY_BUFFERS	3

enum
{
	GAME_UNDEFINED, GAME_LAIR, GAME_LAIR2, GAME_ACE, GAME_ACE91, GAME_CLIFF, GAME_GTG, GAME_SUPERD, GAME_THAYERS, GAME_ASTRON,
	GAME_GALAXY, GAME_ESH, GAME_LAIREURO, GAME_BADLANDS, GAME_STARRIDER, GAME_BEGA, GAME_INTERSTELLAR,
	GAME_SAE, GAME_MACH3, GAME_UVT, GAME_BADLANDP, GAME_DLE1, GAME_DLE2
}; // game types

//////////////////////////////////

#include <SDL.h>
#include "../sound/sound.h"
#include "../cpu/cpu.h"	// for CPU_MEM_SIZE
#include "../io/input.h"	// for SWITCH definitions, most/all games need them
#include "../io/logger.h"

typedef void * unzFile;	// because including the unzip header file gives some compiler error

// structure for the cpu debugger... a memory address and its corresponding name
// This is so when debugging, you can have function names instead of numbers (it is easier to read)
struct addr_name
{
	const char *name;	// arbitrary name assigned to that address
	unsigned int address;	// memory address
};

void g_cpu_break(char *);

// each game can define an array of these to faciliate ROM loading
struct rom_def
{
	const char *filename;	// name of the rom to be loaded
	const char *dir;	// name of the subdirectory where rom is (NULL means to use m_shortgamename)
	Uint8 *buf;	// where in memory to load the rom
	unsigned int size;	// expected size of the rom
	unsigned int crc32;	// CRC32 of the ROM
};

class game
{
public:
	game();
	virtual ~game();	// to avoid compiler warnings
	bool pre_init();
	virtual bool init();
	virtual void start();
	void pre_shutdown();

	// saves this game's static ram to a compressed file
	// (can be called to ensure sram is saved even if Daphne is terminated improperly)
	void save_sram();

	virtual void shutdown();
	virtual void reset();
	virtual void do_irq(unsigned int);		// does an IRQ tick
	virtual void do_nmi();		// does an NMI tick
	virtual Uint8 cpu_mem_read(Uint16 addr);	// 16-bit addressing memory read routine
	virtual Uint8 cpu_mem_read(Uint32 addr);	// 32-bit addressing memory read routine
	virtual void cpu_mem_write(Uint16 addr, Uint8 value);		// memory write routine
	virtual void cpu_mem_write(Uint32 addr, Uint8 value);	// 32-bit addressing memory write routine
	virtual Uint8 port_read(Uint16 port);		// read from port
	virtual void port_write(Uint16 port, Uint8 value);		// write to a port
	virtual void update_pc(Uint32 new_pc);		// update the PC
	virtual void input_enable(Uint8);
	virtual void input_disable(Uint8);
	virtual void OnMouseMotion(Uint16 x, Uint16 y, Sint16 xrel, Sint16 yrel);  // Added by ScottD
	virtual void OnVblank();	// this gets called by the ldp class every vblank (since many games use vblank for their interrupt)

	// This optional function will get called 4 times by the ldv1000 driver IF the game driver has first called ldv1000_report_vsync.
	// If 'bIsStatus' is false, then this is the command strobe.
	virtual void OnLDV1000LineChange(bool bIsStatus, bool bIsEnabled);

	// generic function to initialize video
	// m_palette_color_count, m_video_overlay_width, and m_video_overlay_height should all be initialized before this function is called
	// This function called palette_initialize
	bool video_init();

	// generic function to shutdown video
	// This function calls palette_shutdown
	void video_shutdown();

	// generic function to ensure that the video buffer gets drawn to the screen, will call video_repaint()
	// this will not do anything if m_video_overlay_needs_update is false
	void video_blit();

	// forces the screen to always be redrawn (useful if you know the screen has been clobbered, such as when using the drop-down console)
	void video_force_blit();

	// game-specific function that calculates and sets the game's color palette
	// This function is called by video_init
	virtual void palette_calculate();

	// game-specific function to force the video buffer to be drawn from scratch (does not blit),
	// this function is called by video_blit
	virtual void video_repaint();

	// a way for external functions to indicate that video needs update
	// (currently the tms9128nl routines need to use this because they aren't part of the game class)
	void set_video_overlay_needs_update(bool value);
	
	// returns m_video_overlay_width
	unsigned int get_video_overlay_width();

	// returns m_video_overlay_height
	unsigned int get_video_overlay_height();

	virtual void set_prefer_samples(bool);
	virtual void set_fastboot(bool);
	virtual void set_preset(int);	// set up dip switches/rom names, etc with prepared values
	virtual void set_version(int);	// selects alternate rom revs, or whatever
	virtual bool set_bank (unsigned char, unsigned char);	// set dip switch values
	virtual bool handle_cmdline_arg(const char *arg);	// the cmd line will pass on any unknown variables to the game to see if there is a game-specific option to be parsed
	void disable_crc();  // skips CRC check on ROM load
	virtual bool load_roms();	// load roms into memory
	bool verify_required_file(const char *filename, const char *gamedir, Uint32 filecrc32);	// verifies existence of a required file (such as a readme.txt for DLE)
	virtual void patch_roms();	// do any modifications (cheats, etc) to roms after they're loaded
	int get_video_row_offset();
	int get_video_col_offset();
	unsigned get_video_visible_lines();	// returns m_uVideoOverlayVisibleLines
	SDL_Surface *get_video_overlay(int index);	// returns pointer to video overlay specified, or NULL if index is out of range
	SDL_Surface *get_active_video_overlay();	// returns the current active video overlay (that is currently being drawn)
	SDL_Surface *get_finished_video_overlay();	// returns the last complete video overlay (that isn't currently being drawn)
	bool is_overlay_size_dynamic();	// returns m_overlay_size_is_dynamic
	SDL_Surface *get_scaled_video_overlay();	// returns pointer to the video overlay which is used for scaling
	bool IsFullScaleEnabled();	// returns m_bFullScale
	void SetFullScale(bool bEnabled);	// sets m_bFullScale

	void enable_cheat();
	unsigned int get_disc_fpks();	// return # of frames per kilosecond (to avoid using gp2x-unfriendly floats)
//	double get_disc_fps();	// return FPS of the laserdisc
//	double get_disc_ms_per_frame();
	Uint8 get_game_type();
	Uint32 get_num_sounds();
	const char *get_sound_name(int);
	const char *get_issues();	// does the game have any performance issues the user should know about?
	void set_issues(const char *);
	void toggle_game_pause();	// toggles whether the game is paused or not
	const char *get_shortgamename();	// returns short game name
#ifdef CPU_DEBUG
	const char *get_address_name(unsigned int addr);	// get a potential name for a memory address (very useful for debugging)
#endif

	// returns m_bMouseEnabled
	bool getMouseEnabled();

protected:
	bool m_game_paused;	// whether the game is paused or not
	const char *m_shortgamename;	// a one-word name for this game (ie "lair" "ace" "dle", etc)
	const struct rom_def *m_rom_list;	// pointer to a null-terminated array of roms to be loaded
	Uint8 m_cpumem[CPU_MEM_SIZE];	// generic buffer that most 16-bit addressing cpu's can use
	unsigned int m_uDiscFPKS;	// frames per kilosecond of the game's laserdisc (to avoid using gp2x-unfriendly float)
	double m_disc_fps;	// frames per second of the game's laserdisc; (only used initially since it is expensive on gp2x)
//	double m_disc_ms_per_frame;	// how many ms per frame of the game's laserdisc (same value as fps, just re-arranged)
	Uint8 m_game_type;	// which game it is
	Uint32 m_num_sounds;	// how many samples the game has to load
	const char *m_sound_name[MAX_NUM_SOUNDS];	// names for each sound file
	const char *m_game_issues;	// description of any issues the game has (NULL if no issues)
	bool m_cheat_requested;	// whether user has requested any cheats to be enabled
	bool m_crc_disabled;    // set to true to disable CRC check on ROM load
	bool m_prefer_samples;
	bool m_fastboot;
	
	const char *m_nvram_filename; // filename for nvram (only for DL2/SA91 for now)
	Uint8 *m_nvram_begin;  // points to where our nvram begins
	Uint16 *m_EEPROM_9536_begin;  // points to the 9536 EEPROM for DL2/SA91
	bool m_EEPROM_9536;
	Uint32 m_nvram_size;	// how big the nvram area is (if this is 0, nvram is not loaded/saved)

	// variables relating to graphics generated by the game ROM (ie non-laserdisc video)
	bool m_game_uses_video_overlay;	// whether the game uses video overlay (most games do)
	bool m_overlay_size_is_dynamic;	// whether the game dynamically re-allocates its overlay to match the size of the mpeg (thayer's quest, seektest both do this)
	SDL_Surface *m_video_overlay[MAX_VIDEO_OVERLAY_BUFFERS];	// graphic buffers to hold ROM-generated video

	// fullscale variables
	SDL_Surface *m_video_overlay_scaled; // temporary graphic buffer which receives the scaled game graphics from m_video_overlay[...]
	long* m_video_overlay_matrix;       // the precalculated matrix used for scaling the game graphics to the target screen dimension
	Uint32 m_video_screen_width;	    // the width  of the target screen (according to the graphic mode set by Daphne)
	Uint32 m_video_screen_height;	    // the height of the target screen (according to the graphic mode set by Daphne)
	Uint32 m_video_screen_size;	        // m_video_screen_width x m_video_screen_height, just to speedup things a bit
	bool m_bFullScale;					// whether fullscale is enabled or not
	// end fullscale variables

	int m_video_overlay_count;	// how many video overlay buffers we have
	int m_active_video_overlay;	// index of the active SDL_Surface that serves as our video overlay (the one we make changes to)
	int m_finished_video_overlay;	// index of the last SDL_Surface to be completely drawn (ie finished)
	int m_palette_color_count;	// the # of colors to be allocated for the color palette, not to exceed 256 (surfaces are only 8-bit)
	
	// How many rows down to shift video (can be negative if you want to shift up)
	// IMPORTANT: The rows are relative to the ROM-generated video overlay!
	// So if the video window is 480 pixels high and the ROM overlay has 240 visible rows,
	//  then shifting 1 row would shift 2 pixels.
	int m_video_row_offset;
	
	int m_video_col_offset;	// how many pixels right to shift video (can be negative if you want to shift left)
	Uint32 m_video_overlay_width;	// the width of the ROM-generated video (should be defined in game constructor)
	Uint32 m_video_overlay_height;	// the height of the ROM-generated video (should be defined in game constructor)
	bool m_video_overlay_needs_update;	// whether the video overlay has changed
	// Sometimes the game ROM will write the same values to the video overlay buffer, and it is cheaper not to redraw
	// the video overlay buffer if nothing is being changed.  Thus, m_video_overlay_needs_update's purpose.
	// The game is responsible for setting this value to true when it knows that the video_repaint needs to be called

	// how many lines are visible in the video overlay
	// (usually 240 for almost all games, that is, half the resolution of the laserdisc player)
	// Firefox is the only current exception.
	unsigned int m_uVideoOverlayVisibleLines;

	// if the game uses the mouse, this should be set to true IN THE GAME'S CONSTRUCTOR
	bool m_bMouseEnabled;

	// logger interface (for writing to daphne_log.txt file)
	ILogger *m_pLogger;

#ifdef CPU_DEBUG
	struct addr_name *addr_names;
#endif

private:
	bool load_rom(const char *filename, Uint8 *buf, Uint32 size);
	bool load_rom(const char *filename, const char *directory, Uint8 *bif, Uint32 size);
	bool load_compressed_rom(const char *filename, unzFile opened_zip_file, Uint8 *buf, Uint32 size);

};

extern game *g_game;	// our global game class.  Instead of having every .cpp file define this, we put it here.

#endif
