//----------------------------------------------------------------------------
//
// File:        tms9919.hpp
// Date:        23-Feb-1998
// Programmer:  Marc Rousseau
//
// Description:
//
// Copyright (c) 1998-2000 Marc Rousseau, All Rights Reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
//
// Revision History:
//
//----------------------------------------------------------------------------

#ifndef _TMS9919_HPP_
#define _TMS9919_HPP_

class cTMS5220;

class cTMS9919 {

protected:

    enum NOISE_COLOR_E { 
        NOISE_PERIODIC,
        NOISE_WHITE
    };

    cTMS5220           *m_pSpeechSynthesizer;

    int                 m_LastData;

    int                 m_Frequency [4];
    int                 m_Attenuation [4];
    NOISE_COLOR_E       m_NoiseColor;
    int                 m_NoiseType;

    virtual void SetNoise ( NOISE_COLOR_E, int );
    virtual void SetFrequency ( int, int );
    virtual void SetAttenuation ( int, int );
    Uint32 m_clock_frequency;

public:

    cTMS9919 ();
    virtual ~cTMS9919 ();

    virtual int SetSpeechSynthesizer ( cTMS5220 * );

    void WriteData ( Uint8 data );
    void set_core_frequency (Uint32);

};

#endif
