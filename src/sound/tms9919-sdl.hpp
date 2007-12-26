//----------------------------------------------------------------------------
//
// File:        tms9919-sdl.hpp
// Date:        20-Jun-2000
// Programmer:  Marc Rousseau
//
// Description: 
//
// Copyright (c) 2000-2001 Marc Rousseau, All Rights Reserved.
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

#ifndef _TMS9919_SDL_HPP_
#define _TMS9919_SDL_HPP_

#if ! defined ( _TMS9919_HPP_ )
    #error You must include tms9919.hpp before tms9919-sdl.hpp
#endif

#define SIZE sizeof

class cSdlTMS9919 : public cTMS9919 {

    struct sVoiceInfo {
        float  period;
        float  toggle;
        int    setting;
        Uint8 *buffer;
    };	     

    int                 m_VolumeTable [16];
    
    bool                m_Initialized;
    int                 m_MasterVolume;
	
    SDL_AudioSpec       m_AudioSpec;
    sVoiceInfo          m_Info [4];
    int                 m_ShiftRegister;
    int                 m_NoiseGenerator;
    Uint8              *m_MixBuffer;

    static void _AudioCallback ( void *, Uint8 *, int );
 
    virtual void SetNoise ( NOISE_COLOR_E, int );
    virtual void SetFrequency ( int, int );
    virtual void SetAttenuation ( int, int );
   			 
public:

    cSdlTMS9919 ();
    ~cSdlTMS9919 ();

    virtual int SetSpeechSynthesizer ( cTMS5220 * );

    void AudioCallback ( Uint8 *, int );
    int  GetMasterVolume () const		{ return m_MasterVolume; }
    void SetMasterVolume ( int );

};

#endif
