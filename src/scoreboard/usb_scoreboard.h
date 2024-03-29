/*
 * ____ DAPHNE COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2022 DirtBagXon
 *
 * This file is part of HYPSEUS, a laserdisc arcade game emulator
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

#ifndef USB_SCOREBOARD_H
#define USB_SCOREBOARD_H

#include "scoreboard_interface.h"
#include "usb_util.h"

class ScoreboardFactory;

class USBScoreboard : public IScoreboard
{
	// allow ScoreboardFactory GetInstance
	friend class ScoreboardFactory;

public:
	static IScoreboard *GetInstance();

	void DeleteInstance();

	void Invalidate();

	bool RepaintIfNeeded();

	bool ChangeVisibility(bool bVisible);

	bool set_digit(unsigned int uValue, WhichDigit which);

	bool is_repaint_needed();

	bool get_digit(unsigned int &uValue, WhichDigit which);

protected:
private:
	// initialize the USB port
	bool USBInit();

	// shutdown the USB port
	void USBShutdown();

	USBScoreboard();
	virtual ~USBScoreboard();

	DigitStruct ds;
};

#endif
