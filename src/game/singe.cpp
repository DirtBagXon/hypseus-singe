/*
* singe.cpp
*
* Copyright (C) 2006 Scott C. Duensing
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

/*
* This is SINGE - the Somewhat Interactive Nostalgic Game Engine!
*/

#include "singe.h"
#include "singe/singe_interface.h"

// Win32 doesn't use strcasecmp, it uses stricmp (lame)
#ifdef WIN32
#define strcasecmp stricmp
#endif

////////////////////////////////////////////////////////////////////////////////

// For intercepting the VLDP MPEG data

extern       struct vldp_in_info   g_local_info;
extern const struct vldp_out_info *g_vldp_info;

////////////////////////////////////////////////////////////////////////////////

typedef const struct singe_out_info *(*singeinitproc)(const struct singe_in_info *in_info);

// pointer to all info SINGE PROXY gives to us
const struct singe_out_info *g_pSingeOut = NULL;

// info that we provide to the SINGE PROXY DLL
struct singe_in_info g_SingeIn;

////////////////////////////////////////////////////////////////////////////////

singe::singe()
{
	m_strGameScript = "";
	m_shortgamename = "singe";
	m_strName = "[Undefined scripted game]";
	m_video_overlay_width = 320;	// sensible default
	m_video_overlay_height = 240;	// " " "
	m_palette_color_count = 256;
	m_overlay_size_is_dynamic = true;	// this 'game' does reallocate the size of its overlay
	m_bMouseEnabled = true;
	m_dll_instance = NULL;
}

bool singe::init()
{
	bool bSuccess = false;
	singeinitproc pSingeInit;	// pointer to the init proc ...

#ifndef STATIC_SINGE	// if we're loading SINGE dynamically
#ifndef DEBUG
	m_dll_instance = M_LOAD_LIB(singe);	// load SINGE.DLL
#else
	m_dll_instance = M_LOAD_LIB(singed);	// load SINGED.DLL (debug version)
#endif

	// If the handle is valid, try to get the function address. 
	if (m_dll_instance)
	{
		pSingeInit = (singeinitproc) M_GET_SYM(m_dll_instance, "singeproxy_init");

		// if init function was found
		if (pSingeInit)
		{
			bSuccess = true;
		}
		else
		{
			printerror("SINGE LOAD ERROR : singeproxy_init could not be loaded");
		}
	}
	else
	{
		printerror("ERROR: could not open the SINGE dynamic library (file not found maybe?)");
	}

#else // else if we're loading SINGE statically
	pSingeInit = singeproxy_init;
	bSuccess = true;
#endif // STATIC_SINGE

	// if pSingeInit is valid ...
	if (bSuccess)
	{
		g_SingeIn.uVersion = SINGE_INTERFACE_API_VERSION;
		g_SingeIn.printline = printline;
		g_SingeIn.set_quitflag = set_quitflag;
		g_SingeIn.disable_audio1 = disable_audio1;
		g_SingeIn.disable_audio2 = disable_audio2;
		g_SingeIn.enable_audio1 = enable_audio1;
		g_SingeIn.enable_audio2 = enable_audio2;
		g_SingeIn.framenum_to_frame = framenum_to_frame;
		g_SingeIn.get_current_frame = get_current_frame;
		g_SingeIn.pre_change_speed = pre_change_speed;
		g_SingeIn.pre_pause = pre_pause;
		g_SingeIn.pre_play = pre_play;
		g_SingeIn.pre_search = pre_search;
		g_SingeIn.pre_skip_backward = pre_skip_backward;
		g_SingeIn.pre_skip_forward = pre_skip_forward;
		g_SingeIn.pre_step_backward = pre_step_backward;
		g_SingeIn.pre_step_forward = pre_step_forward;
		g_SingeIn.pre_stop = pre_stop;
		g_SingeIn.set_search_blanking = set_search_blanking;
		g_SingeIn.set_skip_blanking = set_skip_blanking;
		g_SingeIn.g_local_info = &g_local_info;
		g_SingeIn.g_vldp_info = g_vldp_info;
		g_SingeIn.get_video_height = get_video_height;
		g_SingeIn.get_video_width = get_video_width;
		g_SingeIn.draw_string = draw_string;
		g_SingeIn.samples_play_sample = samples_play_sample;
		g_SingeIn.set_last_error = set_last_error;

		// establish link betwixt singe proxy and us
		g_pSingeOut = pSingeInit(&g_SingeIn);

		// version check
		if (g_pSingeOut->uVersion != SINGE_INTERFACE_API_VERSION)
		{
			printline("Singe API version mismatch!  Something needs to be recompiled...");
			bSuccess = false;
		}
	}

	// if we're not using VLDP, then singe will segfault, so abort ...
	if (g_vldp_info == NULL)
	{
		printerror("You must use VLDP when using Singe.");
		bSuccess = false;
	}

	if (!bSuccess)
	{
#ifndef STATIC_SINGE
		M_FREE_LIB(m_dll_instance);
#endif // STATIC_SINGE
	}

	return bSuccess;
}

void singe::start()
{
	int intReturn = 0;

	printline("Starting Singe.");
	g_pSingeOut->sep_set_surface(m_video_overlay_width, m_video_overlay_height);
	g_pSingeOut->sep_set_static_pointers(&m_disc_fps, &m_uDiscFPKS);
	g_pSingeOut->sep_startup(m_strGameScript.c_str());

	// if singe didn't get an error during startup...
	if (!get_quitflag())
	{

		while (!get_quitflag())
		{
			g_pSingeOut->sep_call_lua("onOverlayUpdate", ">i", &intReturn);
			if (intReturn == 1)
			{
				m_video_overlay_needs_update = true;
			}
			video_blit();
			SDL_check_input();
			g_ldp->think_delay(10);	// don't hog cpu, and advance timer
		}

		g_pSingeOut->sep_call_lua("onShutdown", "");
	} // end if there was no startup error

	// always call sep_shutdown just to make sure everything gets cleaned up
	g_pSingeOut->sep_shutdown();
}

void singe::shutdown()
{
#ifndef STATIC_SINGE
	// if a DLL is loaded, then free it
	if (m_dll_instance)
	{
		M_FREE_LIB(m_dll_instance);
		m_dll_instance = NULL;
	}
	// else do nothing ...
#endif // STATIC_SINGE
}

void singe::input_enable(Uint8 input)
{
	g_pSingeOut->sep_call_lua("onInputPressed", "i", input);
}

void singe::input_disable(Uint8 input)
{
	g_pSingeOut->sep_call_lua("onInputReleased", "i", input);
}

void singe::OnMouseMotion(Uint16 x, Uint16 y, Sint16 xrel, Sint16 yrel)
{
	if (g_pSingeOut)
	{
		g_pSingeOut->sep_do_mouse_move(x, y, xrel, yrel);
	}
}

// game-specific command line arguments handled here
bool singe::handle_cmdline_arg(const char *arg)
{
	bool bResult = false;
	static bool scriptLoaded = false;

	if (mpo_file_exists(arg))
	{
		if (!scriptLoaded)
		{
			bResult = scriptLoaded = true;
			m_strGameScript = arg;
		}
		else
		{
			printline("Only one game script may be loaded at a time!");
			bResult = false;
		}
	}
	else
	{
		string strErrMsg = "Script ";
		strErrMsg += arg;
		strErrMsg += " does not exist.";
		printline(strErrMsg.c_str());
	}

	return bResult;
}


void singe::palette_calculate()
{
	SDL_Color temp_color;

	temp_color.unused = 0; // Eliminates a warning.

	// go through all colors and compute the palette
	// (start at 2 because 0 and 1 are a special case)
	for (unsigned int i = 2; i < 256; i++)
	{
		temp_color.r = i & 0xE0;					// Top 3 bits for red
		temp_color.g = (i << 3) & 0xC0;		// Middle 2 bits for green
		temp_color.b = (i << 5) & 0xE0;		// Bottom 3 bits for blue
		palette_set_color(i, temp_color);
	}

	// special case: 00 is reserved for transparency, so 01 becomes fully black
	temp_color.r = temp_color.g = temp_color.b = 0;
	palette_set_color(1, temp_color);

	// safety : 00 should never be visible so we'll make it a bright color to help us
	//  catch errors
	temp_color.r = temp_color.g = temp_color.b = 0xFF;
	palette_set_color(0, temp_color);
}


// redraws video
void singe::video_repaint()
{
	Uint32 cur_w = g_ldp->get_discvideo_width() >> 1;	// width overlay should be
	Uint32 cur_h = g_ldp->get_discvideo_height() >> 1;	// height overlay should be

	// if the width or height of the mpeg video has changed since we last were here (ie, opening a new mpeg)
	// then reallocate the video overlay buffer
	if ((cur_w != m_video_overlay_width) || (cur_h != m_video_overlay_height))
	{
		if (g_ldp->lock_overlay(1000))
		{
			m_video_overlay_width = cur_w;
			m_video_overlay_height = cur_h;

			g_pSingeOut->sep_set_surface(m_video_overlay_width, m_video_overlay_height);

			video_shutdown();
			if (!video_init())
			{
				printline("Fatal Error, trying to re-create the surface failed!");
				set_quitflag();
			}
			g_ldp->unlock_overlay(1000);	// unblock game video overlay
		}
		else
		{
			g_pSingeOut->sep_print("Timed out trying to get a lock on the yuv overlay");
			return;
		}
	} // end if dimensions are incorrect

	g_pSingeOut->sep_do_blit(m_video_overlay[m_active_video_overlay]);
}

void singe::set_last_error(const char *cpszErrMsg)
{
	// TODO : figure out reliable way to call printerror (maybe there isn't one?)
	printline(cpszErrMsg);
}
