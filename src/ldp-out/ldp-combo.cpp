/*
 * ldp-combo.cpp
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


// combo.cpp
// by Matt Ownby

// a combo laserdisc player class that controls both smpeg and a real laserdisc player

#include "../timer/timer.h"
#include "ldp-combo.h"

combo::combo()
{
	blitting_allowed = false;
	need_serial = true;
}

bool combo::init_player()
{
	return (m_rldp.init_player() && m_vldp.init_player());
}

void combo::shutdown_player()
{
	m_rldp.shutdown_player();
	m_vldp.shutdown_player();
}

bool combo::search(char *frame)
{
	// real ldp should seek first since it takes longer
//	return (m_rldp.search(frame) && m_vldp.search(frame));
	// the search function is going away and combo really isn't being used anymore, so I am not maintaining it
	return false;
}

unsigned int combo::play()
{
	m_vldp.play();	// virtual ldp should play first since it takes longer
	m_rldp.play();
	
	return (refresh_ms_time());
}

void combo::pause()
{
	m_rldp.pause();
	m_vldp.pause();
}
