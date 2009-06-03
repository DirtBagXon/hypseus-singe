/*
 * game.cpp
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

// game.cpp
// by Matt Ownby

#ifdef WIN32
#pragma warning (disable:4100) // disable warning about unreferenced parameter

// this doesn't disable the warnings I want disabled :(
#pragma warning (disable:4244) // disable stupid warnings in MSVC's include files
#pragma warning (disable:4511)
#pragma warning (disable:4512)
#pragma warning (disable:4663)
#endif

#include <zlib.h>	// for CRC checking
#include <string>	// STL strings, useful to prevent buffer overrun
#include "../daphne.h"	// for get_quitflag()
#include "../io/homedir.h"
#include "../io/conout.h"
#include "../io/error.h"
#include "../io/unzip.h"
#include "../io/mpo_fileio.h"
#include "../io/mpo_mem.h"	// for better malloc
#include "../io/numstr.h"
#include "../ldp-out/ldp.h"
#include "../cpu/cpu-debug.h"	// for set_cpu_trace
#include "../timer/timer.h"
#include "../io/input.h"
#include "../io/sram.h"
#include "../io/logger_console.h"	// for writing to daphne_log.txt file
#include "../video/video.h"	// for get_screen
#include "../video/palette.h"
#include "game.h"

#ifdef USE_OPENGL
#ifdef MAC_OSX
#include <glew.h>
#else
#include <GL/glew.h>
#endif
#endif

using namespace std;	// for STL string to compile without problems ...

///////////////////////////////////////////////////////////

#ifdef CPU_DEBUG


// a generic example of how to setup debug names
struct addr_name game_addr_names[] =
{
	{ "Start", 0x0000 },
	{ NULL, 0 }
};
#endif

// breaks cpu execution into trace mode from whichever point the program is at
// This function is executed by typing "break" at the daphne console
// This can't be part of the class because I can't figure out how to make it work =]
void g_cpu_break (char *s)
{

	// I put this here simply to get rid of warning messages
	if (s)
	{
	}

#ifdef CPU_DEBUG
	set_cpu_trace(1);
#else
	printline("You have to compile with CPU_DEBUG defined to use the debugger!");
#endif

}


///////////////////////////////////////////////////////////

game::game() :
	m_game_paused(false),
	m_game_uses_video_overlay(true),	// since most games do use video overlay, we'll default this to true
	m_overlay_size_is_dynamic(false),	// the overlay size is usually static
	m_video_overlay_scaled(0),  // " " "
	m_video_overlay_matrix(0),  //
	m_video_screen_width(0),	// 
	m_video_screen_height(0),	// 
	m_video_screen_size(0),	    // 
	m_bFullScale(false),	// full-scale is disabled by default
	m_video_overlay_count(2),	// default to double buffering because it is conservative
	m_active_video_overlay(0),	// the first overlay (0) starts out as the active one
	m_finished_video_overlay(0),
	m_palette_color_count(0),	// force game to specify this
	m_video_row_offset(0),	// most games will want this to be 0
	m_video_col_offset(0),	// " " "
	m_video_overlay_width(0),	// " " "
	m_video_overlay_height(0),	// " " "
	m_video_overlay_needs_update(true),	// it always needs to be updated the first time
	m_uVideoOverlayVisibleLines(240),	// (480/2) for almost all games with overlay
	m_bMouseEnabled(false)	// mouse is disabled for most games
{
	memset(m_video_overlay, 0, sizeof(m_video_overlay));	// clear this structure so we can easily detect whether we are using video overlay or not
	m_uDiscFPKS = 0;
	m_disc_fps = 0.0;
//	m_disc_ms_per_frame = 0.0;
	m_game_type = GAME_UNDEFINED;
	m_game_issues = NULL;	// assume game has no issues unless we specify some
#ifdef CPU_DEBUG
	addr_names = game_addr_names;
#endif
	
	m_num_sounds = 0;
	m_cheat_requested = false;	// no cheats until specifically requested
	m_shortgamename = "game";	// change this to match your game
	
	m_nvram_begin = NULL;
	m_nvram_size = 0;	// no nvram by default
	m_EEPROM_9536 = false;

	m_rom_list = NULL;
	m_crc_disabled = false;
	m_prefer_samples = false;    // default to emulated sound
	m_fastboot = false;

	m_pLogger = ConsoleLogger::GetInstance();
}

game::~game()
{
	// cleanup logger
	m_pLogger->DeleteInstance();
}

// call this instead of init() directly to ensure that some universal stuff gets taken care of
bool game::pre_init()
{
	// compute integer values that we will actually use in practice
	// (floating point is too expensive on GP2X so we're eliminating it for recurring computations)
	if (m_disc_fps != 0.0)
	{
		m_uDiscFPKS = (unsigned int) ((m_disc_fps * 1000.0) + 0.5);	// frames per kilosecond (same precision, but an int instead of a float)
	}

	// if we have nvram that we need to load
	if (m_nvram_size > 0)
	{	
		if (m_EEPROM_9536)
		{
			string filename = m_nvram_filename;
			filename += ".gz";      // trail w/ gz since it'll be compressed 
			sram_load(filename.c_str(), (unsigned char *) m_EEPROM_9536_begin, m_nvram_size);
		}
		else
		{
			string filename = m_shortgamename;
			filename += ".gz";      // trail w/ gz since it'll be compressed 
			sram_load(filename.c_str(), m_nvram_begin, m_nvram_size);
		}
	}
	
	return init();
}

// generic game initialization
bool game::init()
{
	bool result = true;
	
	cpu_init();
	return result;
}

// generic game start function.  Starts the game playing (usually just begins executing the cpu(s) )
void game::start()
{
	cpu_execute();
}

// call this instead of shutdown directly
void game::pre_shutdown()
{
	save_sram();
	shutdown();
}

// called to ensure sram is saved even if Daphne is terminated improperly
void game::save_sram()
{
	// if we have nvram that we need to save to disk
	if (m_nvram_size > 0)
	{
		if (m_EEPROM_9536)
		{
			string filename = m_nvram_filename;                 
            filename += ".gz";      // trail w/ gz since it'll be compressed 
			sram_save(filename.c_str(), (unsigned char *) m_EEPROM_9536_begin, m_nvram_size);
		}
		else
		{
			string filename = m_shortgamename;                 
            filename += ".gz";      // trail w/ gz since it'll be compressed 
			sram_save(filename.c_str(), m_nvram_begin, m_nvram_size);
		}
	}
}

// generic game shutdown function
void game::shutdown()
{
	cpu_shutdown();
}

// generic game reset function
void game::reset()
{
	cpu_reset();
}

// does anything special needed to send an IRQ
void game::do_irq(unsigned int which_irq)
{
	// get rid of warnings
	if (which_irq)
	{
	}

	printline("WARNING : Unhandled IRQ in generic game class!  This is probably not what you want!");
}

// does anything special needed to send an NMI
void game::do_nmi()
{
	printline("WARNING : Unhandled NMI in generic game class!  This is probably not what you want!");
}

// reads a byte from a 16-bit address space
Uint8 game::cpu_mem_read(Uint16 addr)
{
	return m_cpumem[addr];
}

// reads a byte from a 32-bit address space
Uint8 game::cpu_mem_read(Uint32 addr)
{
	return m_cpumem[addr];
}

// writes a byte to a 16-bit addresss space
void game::cpu_mem_write(Uint16 addr, Uint8 value)
{
	m_cpumem[addr] = value;
}

// writes a byte to a 32-bit address space
void game::cpu_mem_write(Uint32 addr, Uint8 value)
{
	m_cpumem[addr] = value;
}

// reads a byte from the cpu's port
Uint8 game::port_read(Uint16 port)
{
	char s[81] = { 0 };

	port &= 0xFF;	
	sprintf(s, "ERROR: CPU port %x read requested, but this function is unimplemented!", port);
	printline(s);

	return(0);
}

// writes a byte to the cpu's port
void game::port_write(Uint16 port, Uint8 value)
{
	char s[81] = { 0 };

	port &= 0xFF;
	sprintf(s, "ERROR: CPU port %x write requested (value %x) but this function is unimplemented!", port, value);
	printline(s);

}

// notifies us of the new Program Counter (which most games usually don't care about)
void game::update_pc(Uint32 new_pc)
{
	// I put this here to get rid of warnings
	if (new_pc)
	{
	}
}

void game::input_enable(Uint8 input)
{
	// get rid of warnings
	if (input)
	{
	}

	printline("Warning: generic input_enable function called, does nothing");
}

void game::input_disable(Uint8 input)
{
	// get rid of warnings
	if (input)
	{
	}

	printline("Warning: generic input_disable function called, does nothing");
}

// Added by ScottD
void game::OnMouseMotion(Uint16 x, Uint16 y, Sint16 xrel, Sint16 yrel)
{
	// get rid of warnings
	if (x || y || xrel || yrel)
	{
	}

	printline("Warning: generic mouse_motion function called, does nothing");
}

// by default, this is ignored, but it should be used by specific game drivers to stay in sync with the laserdisc
void game::OnVblank()
{
}

void game::OnLDV1000LineChange(bool bIsStatus, bool bIsEnabled)
{
	// get rid of warnings
	if (bIsStatus || bIsEnabled)
	{
	}
}

// does any video initialization we might need
// returns 'true' if the initialization was successful, false if it failed
// It is good to use generic video init and shutdown functions because then we minimize the possibility of errors such as forgetting to call palette_shutdown
bool game::video_init()
{
	bool result = false;
	int index = 0;
	int w;
	int h;
	int x;
	int y;
	float dx;
	float dy;
	float srcx;
	float srcy;

    // set instance variables and local variables to the actual screen (or window) dimension
    m_video_screen_width = w = get_screen_blitter()->w;
    m_video_screen_height = h = get_screen_blitter()->h;
	
    // if this particular game uses video overlay (most do)
	if (m_game_uses_video_overlay)
	{
		// safety check, make sure variables are initialized like we expect them to be ...
		if ((m_video_overlay_width != 0) && (m_video_overlay_height != 0) && (m_palette_color_count != 0))
		{
			result = true;	// it's easier to assume true here and find out false, than the reverse

            if (m_bFullScale)
			{
                m_video_overlay_scaled = 
                    SDL_CreateRGBSurface(SDL_SWSURFACE, 
                                        w, 
                                        h, 8, 0, 0, 0, 0); // create an 8-bit surface

                // create matrix for scaling the overlay pixmap to the physical surface
                // - size of overlay pixmap m_video_overlay_width x m_video_overlay_height
                // - size of physical surface is get_screen()->w x get_screen()->h
                // - matrix is created as 2dimensional array of "long" values, with size of 
                //   the physical surface
                // each entry in the matrix holds an offset into the overlay pixmap array from
                // which the pixel is copied into to the corresponding surface pixmap
                if ( (m_video_overlay_matrix = (long*) MPO_MALLOC(sizeof(long)*w*h)) == NULL) {
                        printline("MEM ERROR : malloc failed in video_init!");
                        return false;
                } /*endif*/

                dx=(float)m_video_overlay_width/(float)w;	// 256/640=0.4
                dy=(float)m_video_overlay_height/(float)h;	// 256/480=0.5333

                srcx=0;
                srcy=0;

                for (y=0; y<h; y++) {
                    srcx=0;
                    for (x=0; x<w; x++) {
                        m_video_overlay_matrix[x+(y*w)]=(long)srcx+((long)srcy*m_video_overlay_width);
                        srcx+=dx;
                    } /*endfor*/
                    srcy+=dy;
                } /*endfor*/
            } // end if fullscale is enabled

			// create each buffer
			for (index = 0; index < m_video_overlay_count; index++)
			{
				m_video_overlay[index] = SDL_CreateRGBSurface(SDL_SWSURFACE, 
					m_video_overlay_width, m_video_overlay_height, 8, 0, 0, 0, 0); // create an 8-bit surface

				// check to see if we got an error (this should never happen)
				if (!m_video_overlay[index])
				{
					printline("ODD ERROR : SDL_CreateRGBSurface failed in video_init!");
					result = false;
				}
			}

			// if we created the surfaces alright, then allocate space for the color palette
			if (result)
			{
				result = palette_initialize(m_palette_color_count);
				if (result)
				{
					palette_calculate();
					palette_finalize();
				}
			}
		} // end if video overlay is used

		// if the game has not explicitely specified those variables that we need ...
		else
		{
			printerror("See video_init() inside game.cpp for what you need to do to fix a problem");
			// If your game doesn't use video overlay, set m_game_uses_video_overlay to false ...
		}
	} // end if game uses video overlay

	// else game doesn't use video overlay, so we always return true here
	else
	{
		result = true;
	}

	return(result);
}

// shuts down any video we might have initialized
void game::video_shutdown()
{
	int index = 0;

	palette_shutdown();	// de-allocate memory in color palette routines

	for (index = 0; index < m_video_overlay_count; index++)
	{
		// only free surface if it has been allocated (if we get an error in video_init, some surfaces may not be allocated)
		if (m_video_overlay[index] != NULL)
		{
			SDL_FreeSurface(m_video_overlay[index]);
			m_video_overlay[index] = NULL;
		}
	}

	if (m_video_overlay_scaled != NULL)
	{
		SDL_FreeSurface(m_video_overlay_scaled);
		m_video_overlay_scaled = NULL;
	}

	MPO_FREE(m_video_overlay_matrix);

}

// Note: The way it is implemented works only for UPscaling the game-graphics.
//       If game graphic needs to be DOWNscaled, multiple pixels would have to be
//       combined into one pixel of the resulting surface.
void Scale(SDL_Surface* src, SDL_Surface* dst, long* matrix)
{
	long i;
	long m=dst->w*dst->h;

	Uint8* srcpixmap=(Uint8*)src->pixels;
	Uint8* dstpixmap=(Uint8*)dst->pixels;

	for (i=0; i<m; i++) {			
		dstpixmap[i] = srcpixmap[matrix[i]];
	} /*endfor*/
}
// end-add

// generic function to ensure that the video buffer gets drawn to the screen, will call video_repaint()
void game::video_blit()
{
	// if something has actually changed in the game's video (video_blit() will probably get called regularly on each screen refresh,
	// and we don't want to call the potentially expensive video_repaint() unless we have to)
	if (m_video_overlay_needs_update)
	{
		m_active_video_overlay++;	// move to the next video buffer (in case we are supporting more than one buffer)

		// check for wraparound ... (the count will be the last index+1, which is why we do >= instead of >)
		if (m_active_video_overlay >= m_video_overlay_count)
		{
			m_active_video_overlay = 0;
		}
		video_repaint();	// call game-specific function to get palette refreshed
		m_video_overlay_needs_update = false;	// game will need to set this value to true next time it becomes needful for us to redraw the screen

		// if we are in non-VLDP mode, then we can blit to the main surface right here,
		// otherwise we do nothing because the yuv_callback in ldp-vldp.cpp will take care of it
		if (!g_ldp->is_vldp())
		{
#ifdef USE_OPENGL
			// if we're not in OpenGL mode
			if (!get_use_opengl())
			{
#endif
				// If we're not scaling the video
				if (!m_bFullScale)
				{                
					vid_blit(m_video_overlay[m_active_video_overlay], 0, 0);
				}
				else
				{
					// scale game graphics to the screen dimensions
					Scale(m_video_overlay[m_active_video_overlay], 
						m_video_overlay_scaled, 
						m_video_overlay_matrix);
					vid_blit(m_video_overlay_scaled, 0, 0);
				} /*endelse*/
#ifdef USE_OPENGL
			}
			// else we're using OpenGL
			else
			{
				vid_blank();	// openGL requires this

				if (!m_bFullScale)
				{
					SDL_Surface *srf = m_video_overlay[m_active_video_overlay];

					// blit in the center of the screen
					vid_blit(srf,
						(m_video_screen_width >> 1) - (srf->w >> 1),
						(m_video_screen_height >> 1) - (srf->h >> 1));
				}

				// else if 'fullscale' is enabled
				else
				{
					GLfloat fXScale = (GLfloat) m_video_screen_width / m_video_overlay_width;
					GLfloat fYScale = (GLfloat) m_video_screen_height / m_video_overlay_height;

					glPushMatrix();
					glScalef(fXScale, fYScale, 1.0);

					// blit in the center of the screen
					SDL_Surface *srf = m_video_overlay[m_active_video_overlay];
					vid_blit(srf,
						(m_video_screen_width >> 1) - (srf->w >> 1),
						(m_video_screen_height >> 1) - (srf->h >> 1));
					glPopMatrix();
				}
			} // end if using opengl

#endif // USE_OPENGL
			vid_flip();
		} // end if this isn't VLDP

		m_finished_video_overlay = m_active_video_overlay;
	}
}

// forces the video overlay to be redrawn to the screen
// This is necessary when the screen has been clobbered (such as when the debug console is down)
void game::video_force_blit()
{
	// if the game uses a video overlay, we have to go through this video_blit routine to update a bunch of variables
	if (m_game_uses_video_overlay)
	{
		m_video_overlay_needs_update = true;
		video_blit();
	}

	// otherwise we can just call video repaint directly
	else
	{
		video_repaint();
	}
}

// sets up the game's palette, this is a game-specific function
void game::palette_calculate()
{
	SDL_Color temp_color = { 0 };
	
	// fill color palette with sensible grey defaults
	for (int i = 0; i < m_palette_color_count; i++)
	{
		temp_color.r = (unsigned char) i;
		temp_color.g = (unsigned char) i;
		temp_color.b = (unsigned char) i;
		palette_set_color(i, temp_color);
	}
}

// redraws the current active video overlay buffer (but doesn't blit anything to the screen)
// This is a game-specific function
void game::video_repaint()
{
}

void game::set_video_overlay_needs_update(bool value)
{
	m_video_overlay_needs_update = value;
}

unsigned int game::get_video_overlay_height()
{
	return m_video_overlay_height;
}

unsigned int game::get_video_overlay_width()
{
	return m_video_overlay_width;
}

void game::set_prefer_samples(bool value)
{
	m_prefer_samples = value;
}

void game::set_fastboot(bool value)
{
	m_fastboot = value;
}

// generic preset function, does nothing
void game::set_preset(int preset)
{
	printline("NOTE: There are no presets defined for the game you have chosen!");
}

// generic version function, does nothing
void game::set_version(int version)
{
	printline("NOTE: There are no alternate versions defined for the game you have chosen!");
}

// returns false if there was an error trying to set the bank value (ie if the user requested an illegal bank)
bool game::set_bank(unsigned char which_bank, unsigned char value)
{
	bool result = false;	// give them an error to help them troubleshoot any problem they are having with their command line
	
	printline("ERROR: The ability to set bank values is not supported in this game.");
	
	return result;
}

// this is called by cmdline.cpp if it gets any cmdline parameters that it doesn't recognize
// returns true if this argument was recognized and processed,
// or false is this argument wasn't recognized (and therefore the command line is bad on a whole)
bool game::handle_cmdline_arg(const char *arg)
{
	return false;	// no extra arguments supported by default
}

// prevents daphne from verifying that the ROM images' CRC values are correct, useful when testing new ROMs
void game::disable_crc()
{
	m_crc_disabled = true;
}

// routine to load roms
// returns true if there were no errors
bool game::load_roms()
{
	bool result = true;	// we must assume result is true in case the game has no roms to load at all

	// if a rom list has been defined, then begin loading roms
	if (m_rom_list)
	{
		int index = 0;
		const struct rom_def *rom = &m_rom_list[0];
		string opened_zip_name = "";	// the name of the zip file that we currently have open (if we have one open)
		unzFile zip_file = NULL;	// pointer to open zip file (NULL if file is closed)

		// go until we get an error or we run out of roms to load
		do
		{
			string path, zip_path = "";
			unsigned int crc = crc32(0L, Z_NULL, 0);

			// if this game explicitely specifies a subdirectory
			if (rom->dir)
			{
				path = rom->dir;
				zip_path = path + ".zip"; // append zip extension ...
			}

			// otherwise use shortgamename by default
			else
			{
				path = m_shortgamename;
				zip_path = path + ".zip"; // append zip extension ...
			}
			
			//Use homedir to locate the compressed rom
			zip_path = g_homedir.get_romfile(zip_path);

			// if we have not opened a ZIP file, or if we need to open a new zip file ...
			if ((!zip_file) || (zip_path.compare(opened_zip_name) != 0))
			{
				// if we need to open a zip file ...
				if (!zip_file)
				{
					zip_file = unzOpen(zip_path.c_str());
				}

				// we need to open a different zip file ...
				else
				{
//					string s = "Closing " + opened_zip_name + " and attempting to open " + zip_path;	// just for debugging to make sure this is behaving properly
					unzClose(zip_file);
					zip_file = unzOpen(zip_path.c_str());
				}

				// if we successfully opened the file ...
				if (zip_file)
				{
					opened_zip_name = zip_path;
				}
			}

			result = false;
			// if we have a zip file open, try to load the ROM from this file first ...
			if (zip_file)
			{
				result = load_compressed_rom(rom->filename, zip_file, rom->buf, rom->size);
			}

			// if we were unable to open the rom from a zip file, try to open it as an uncompressed file
			if (!result)
			{
				result = load_rom(rom->filename, path.c_str(), rom->buf, rom->size);
			}

			// if file was loaded and was proper length, check CRC
			if (result) 
			{
				if (!m_crc_disabled)  //skip if user doesn't care
				{
					crc = crc32(crc, rom->buf, rom->size);
					if (rom->crc32 == 0)  //skip if crc is set to 0 (game driver doesn't care)
					{
						crc=0;  
					}
					// if CRC's don't match
					if (crc != rom->crc32)
					{
						char s[160];
						sprintf(s, "ROM CRC checked failed for %s, expected %x, got %x", rom->filename, rom->crc32, crc);
						printerror(s);	// warn them about this disaster :)
						printline(s);
					}
				}
			}

			// if ROM could not be loaded
			else
			{
				string s = "ROM ";
				s += rom->filename;
				s += " couldn't be found in roms/";
				s += path;
				s += "/, or in ";
				s += zip_path;
				printerror(s.c_str());
				// if this game borrows roms from another game, point that out to the user to help
				// them troubleshoot
				if (rom->dir)
				{
					s = "NOTE : this ROM comes from the folder '";
					s += rom->dir;
					s += "', which belongs to another game.";
					printline(s.c_str());
					s = "You also NEED to get the ROMs for the game that uses the directory '";
					s += rom->dir;
					s += "'.";
					printline(s.c_str());
				}
			}

			index++;
			rom = &m_rom_list[index];	// move to next entry
		} while (result && rom->filename);

		// if we had a zip file open, close it now
		if (zip_file)
		{
			unzClose(zip_file);
			zip_file = NULL;
		}
		
		patch_roms();
	}

	return(result);
}

// returns true if the file in question exists, and has the proper CRC32
// 'gamedir' is which rom directory (or .ZIP file) this file is expected to be in
bool game::verify_required_file(const char *filename, const char *gamedir, Uint32 filecrc32)
{
	Uint8 *readme_test = NULL;
	bool passed_test = false;
	string path = gamedir;
	path += "/";
	path += filename;
	
	// TRY UNCOMPRESSED FIRST
	string uncompressed_path = g_homedir.get_romfile(path);

	struct mpo_io *io;

	io = mpo_open(uncompressed_path.c_str(), MPO_OPEN_READONLY);
	// if file is opened successfully
	if (io)
	{

		readme_test = new Uint8[io->size];	// allocate file buffer

		if (readme_test)
		{
			unsigned int crc = crc32(0L, Z_NULL, 0);	// zlib crc32

			// if we are able to read in
			mpo_read(readme_test, io->size, NULL, io);

			crc = crc32(crc, readme_test, io->size);
			
			// if the required file has been unaltered, allow user to continue
			if (crc == filecrc32)
			{
				passed_test = true;
			}

			delete [] readme_test;	// free allocated mem
			readme_test = NULL;
		}

		mpo_close(io);
	}

	// IF UNCOMPRESSED TEST FAILED, TRY COMPRESSED TEST ...
	if (!passed_test)
	{
		string zip_path = gamedir;
		zip_path += ".zip";  // we now have "/gamename.zip"
		zip_path = g_homedir.get_romfile(zip_path);

		unzFile zip_file = NULL;	// pointer to open zip file (NULL if file is closed)
		zip_file = unzOpen(zip_path.c_str());
		if (zip_file)
		{
			if (unzLocateFile(zip_file, filename, 2) == UNZ_OK)
			{
				unz_file_info info;

				// get CRC
				unzGetCurrentFileInfo OF((zip_file,
					     &info,
					     NULL,
					     0,
					     NULL,
					     0,
					     NULL,
					     0));

				if (info.crc == filecrc32)
				{
					passed_test = true;
				}
			}
			unzClose(zip_file);
		}
	}

	return passed_test;
}

// loads size # of bytes from filename into buf
// returns true if successful, or false if there was an error
bool game::load_rom(const char *filename, Uint8 *buf, Uint32 size)
{
	struct mpo_io *F;
	MPO_BYTES_READ bytes_read = 0;
	bool result = false;
	string fullpath = g_homedir.get_romfile(filename); // pathname to roms directory
	string s = "";

	outstr("Loading ");
	outstr(fullpath.c_str());
	outstr(" ... ");
	
	F = mpo_open(fullpath.c_str(), MPO_OPEN_READONLY);

	// if file was opened successfully
	if (F)
	{
		mpo_read(buf, size, &bytes_read, F);
		
		// if we read in the # of bytes we expected to
		if (bytes_read == size)
		{
			result = true;
		}
		// notify the user what the problem is
		else
		{
			s = "error in rom_load: expected " + numstr::ToStr(size) + " but only read " + numstr::ToStr((unsigned int) bytes_read);
			printline(s.c_str());
		}
		mpo_close(F);
	}

	s = numstr::ToStr((unsigned int) bytes_read) + " bytes read into memory";
	printline(s.c_str());
	
	return(result);
	
}

// transition function ...
bool game::load_rom(const char *filename, const char *directory, Uint8 *buf, Uint32 size)
{
	string full_path = directory;
	full_path += "/";
	full_path += filename;
	return load_rom(full_path.c_str(), buf, size);
}

// similar to load_rom except this function loads a rom image from a .zip file
// the previously-opened zip file is indicated by 'opened_zip_file'
// true is returned only if the rom was loaded, and it was the expected length
bool game::load_compressed_rom(const char *filename, unzFile opened_zip_file, Uint8 *buf, Uint32 size)
{
	bool result = false;

	outstr("Loading compressed ROM image ");
	outstr(filename);
	outstr("...");

	// try to locate requested rom file inside .ZIP archive and proceed if successful
	// (the '2' indicates case-insensitivity)
	if (unzLocateFile(opened_zip_file, filename, 2) == UNZ_OK)
	{
		// try to open the current file that we've located
		if (unzOpenCurrentFile(opened_zip_file) == UNZ_OK)
		{
			// read this file
			Uint32 bytes_read = (Uint32) unzReadCurrentFile(opened_zip_file, buf, size);
			unzCloseCurrentFile(opened_zip_file);

			// if we read what we expected to read ...
			if (bytes_read == size)
			{
				char s[81];
				sprintf(s, "%d bytes read.", bytes_read);
				printline(s);
				result = true;
			}
			else
			{
				printline("unexpected read result!");
			}
		}
		else
		{
			printline("could not open current file!");
		}
	}
	else
	{
		printline("file not found in .ZIP archive!");
	}

	return result;
}


// modify roms (apply cheats, for example) after they are loaded
// this can also be used for any post-rom-loading procedure, such as verifying the existence of readme files for DLE/SAE
void game::patch_roms()
{
}

// how many pixels down to shift video overlay
int game::get_video_row_offset()
{
	return m_video_row_offset;
}

// how many pixels to the right to shift video overlay
int game::get_video_col_offset()
{
	return m_video_col_offset;
}

unsigned game::get_video_visible_lines()
{
	return m_uVideoOverlayVisibleLines;
}

SDL_Surface *game::get_video_overlay(int index)
{
	SDL_Surface *result = NULL;

	// safety check
	if (index < m_video_overlay_count)
	{
		result = m_video_overlay[index];
	}

	return result;
}

// gets surface that's being drawn
SDL_Surface *game::get_active_video_overlay()
{
	return m_video_overlay[m_active_video_overlay];
}

// gets last surface to be completely drawn (so it can be displayed without worrying about tearing or flickering)
SDL_Surface *game::get_finished_video_overlay()
{
	return m_video_overlay[m_finished_video_overlay];
}

// mainly used by ldp-vldp.cpp so it doesn't print a huge warning message if the overlay's size is dynamic
bool game::is_overlay_size_dynamic()
{
	return m_overlay_size_is_dynamic;
}

SDL_Surface *game::get_scaled_video_overlay()
{
	return m_video_overlay_scaled;
}

bool game::IsFullScaleEnabled()
{
	return m_bFullScale;
}

void game::SetFullScale(bool bEnabled)
{
	m_bFullScale = bEnabled;
}
// end-add

// enables a cheat (any cheat) in the current game.  Unlimited lives is probably the most common.
void game::enable_cheat()
{
	m_cheat_requested = true;
}

unsigned int game::get_disc_fpks()
{
	return m_uDiscFPKS;
}

// UPDATE : it's too expensive to use floating point on GP2X, so we are phasing this function out in
//  favor of integer operations.
// returns how many ms per frame the disc runs at
//double game::get_disc_ms_per_frame()
//{
//	return m_disc_ms_per_frame;
//}

Uint8 game::get_game_type()
{
	return m_game_type;
}

Uint32 game::get_num_sounds()
{
	return m_num_sounds;
}

const char *game::get_sound_name(int whichone)
{
	return m_sound_name[whichone];
}

// returns a string of text that explains the problems with the game OR else null if the game has no problems :)
const char *game::get_issues()
{
	return m_game_issues;
}

void game::set_issues(const char *issues)
{
	m_game_issues = issues;
}

void game::toggle_game_pause()
{
	// if the game is already paused ...
	if (m_game_paused)
	{
		cpu_unpause();
		g_ldp->pre_play();
		m_game_paused = false;
	}

	// for now we will only support game pausing if the disc is playing
	else if (g_ldp->get_status() == LDP_PLAYING)
	{
//		char frame[6];
//		Uint16 cur_frame = g_ldp->get_current_frame();

		cpu_pause();
		g_ldp->pre_pause();

		// If seek delay is enabled, we can't search here because the seek delay depends
		//  upon the cpu running (ie pre_think getting called regularly)
		/*
		g_ldp->framenum_to_frame(cur_frame, frame);
		g_ldp->pre_search(frame, false);	// pause the disc on the frame we think we're on (not the frame we're actually on)

		// wait for seek to complete
		// (using non-blocking seeking to avoid an extra cpu_pause)
		while (g_ldp->get_status() == LDP_SEARCHING)
		{
			make_delay(1);
		}
		*/

		m_game_paused = true;		
	}
}

const char *game::get_shortgamename()
{
	return m_shortgamename;
}

#ifdef CPU_DEBUG
// returns either a name for the address or else NULL if no name exists
// Used for debugging to improve readability (to assign function and variable names)
const char *game::get_address_name(unsigned int addr)
{

	const char *name = NULL;
	int i = 0;

	// loop until we've hit the end of our memory names or until we've found a match
	for (;;)
	{
		// if we're at the end of the list
		if (addr_names[i].name == NULL)
		{
			break;
		}
		else if (addr_names[i].address == addr)
		{
			name = addr_names[i].name;
			break;
		}
		i++;
	}

	return(name);
}

#endif	// CPU_DEBUG

bool game::getMouseEnabled()
{
	return m_bMouseEnabled;
}
