/*
 * releasetest.h
 *
 * Copyright (C) 2005 Matt Ownby
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

// releasetest.h

#ifndef RELEASETEST_H
#define RELEASETEST_H

#ifdef MAC_OSX
#include "mmxdefs.h"
#endif

#include <vector>
#include <string>
#include "game.h"

using namespace std;

class releasetest : public game
{
public:
	releasetest();
	virtual ~releasetest();
	void start();
	void video_repaint();
private:
	// after a test, call this function to indicate whether test passed or failed
	void logtest(bool passed, const string &testname);
	
	// log of tests that passed
	vector <string> m_log_passed;
	
	// log of tests that failed
	vector <string> m_log_failed;

	// returns true if b is true OR if m_test_all is true (convenience function)
	bool dotest(bool b);
	
	// since some calculations have precision problems, this function is used to determine if results are close enough
	// (for integer comparisons)
	bool i_close_enuf(int a, int b, int prec);
	
	//////////// TESTS /////////////////
	bool m_test_all;	// if true, ignores individual test bools and runs all tests
	
	// test line parsing
	void test_line_parse();
	bool m_test_line_parse;

	// test framefile parsing
	void test_framefile_parse();
	bool m_test_framefile_parse;
	
	// tests RGB2YUV conversion functions
	void test_rgb2yuv();
	bool m_test_rgb2yuv;	// runs rgb2yuv test if true
	
	// tests the ability to enable/disable hardware acceleration
	void test_yuv_hwaccel();
	bool m_test_yuv_hwaccel;

	// tests think_delay function
	void test_think_delay();
	bool m_test_think_delay;

	// does an exhaustive test of all VLDP functionality (requires test .m2v files)
	void test_vldp();
	bool m_test_vldp;

	// tests VLDP rendering functions
	void test_vldp_render();
	bool m_test_vldp_render;

	// test blend MMX function
	void test_blend();
	bool m_test_blend;

	// test mix MMX function
	void test_mix();
	bool m_test_mix;

#ifdef USE_OPENGL
	// test OpenGL vertical offset
	void test_gl_offset();
	bool m_test_gl_offset;
#endif

	void test_samples();
	bool m_test_samples;

	void test_sound_mixing();
	bool m_test_sound_mixing;

#ifdef GP2X
	void test_gp2x_timer();
	bool m_test_gp2x_timer;
#endif // GP2X

};

#endif
