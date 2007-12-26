/*
 * singe.h
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

#include "../daphne.h"           // for get_quitflag
#include "game.h"
#include "../io/conout.h"
#include "../io/mpo_fileio.h"
#include "../io/error.h"
#include "../io/dll.h"
#include "../ldp-out/ldp.h"
#include "../sound/samples.h"
#include "../video/palette.h"
#include "../video/video.h"
#include "../vldp2/vldp/vldp.h"  // to get the vldp structs

#include <string>

using namespace std;

////////////////////////////////////////////////////////////////////////////////

class singe : public game
{
public:
	singe();
	bool init();
	void start();
	void shutdown();
	void input_enable(Uint8);
	void input_disable(Uint8);
	void OnMouseMotion(Uint16 x, Uint16 y, Sint16 xrel, Sint16 yrel);
	bool handle_cmdline_arg(const char *arg);
	void palette_calculate();
	void video_repaint();

	// g_ldp function wrappers (to make function pointers out of them)
	static void enable_audio1() { g_ldp->enable_audio1(); }
	static void enable_audio2() { g_ldp->enable_audio2(); }
	static void disable_audio1() { g_ldp->disable_audio1(); }
	static void disable_audio2() { g_ldp->disable_audio2(); }
	static void request_screenshot() { g_ldp->request_screenshot(); }
	static void set_search_blanking(bool enabled) { g_ldp->set_search_blanking(enabled); }
	static void set_skip_blanking(bool enabled) { g_ldp->set_skip_blanking(enabled); }
	static bool pre_change_speed(unsigned int uNumerator, unsigned int uDenominator)
	{
		return g_ldp->pre_change_speed(uNumerator, uDenominator);
	}
	static unsigned int get_current_frame() { return g_ldp->get_current_frame(); }
	static void pre_play() { g_ldp->pre_play(); }
	static void pre_pause() { g_ldp->pre_pause(); }
	static void pre_stop() { g_ldp->pre_stop(); }
	static bool pre_search(const char *cpszFrame, bool block_until_search_finished)
	{
		return g_ldp->pre_search(cpszFrame, block_until_search_finished);
	}
	static void framenum_to_frame(Uint16 u16Frame, char *pszFrame) { g_ldp->framenum_to_frame(u16Frame, pszFrame); }
	static bool pre_skip_forward(Uint16 u16Frames) { return g_ldp->pre_skip_forward(u16Frames); }
	static bool pre_skip_backward(Uint16 u16Frames) { return g_ldp->pre_skip_backward(u16Frames); }
	static void pre_step_forward() { g_ldp->pre_step_forward(); }
	static void pre_step_backward() { g_ldp->pre_step_backward(); }

private:
	// callback function for singe to pass error messages to us
	static void set_last_error(const char *cpszErrMsg);

	string m_strName;	// name of the game	
	string m_strGameScript;	// script name for the game

	DLL_INSTANCE m_dll_instance;	// pointer to DLL we load (if we aren't statically linked)
};
