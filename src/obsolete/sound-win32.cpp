// DAPHNE
// The win32 sound code is contained in this file
// The win32 DirectSound code was contributed by Gregory Hermann and
//  I believe he got at least some of it from MSDN

#include <stdio.h>
#include <stdlib.h>
#include "sound.h"
#include "conout.h"
#include "lair.h"

#include <windows.h>
#include <dsound.h>
#include <mmreg.h>
#include <mmsystem.h>

LPDIRECTSOUND g_pds;
LPDIRECTSOUNDBUFFER g_pdsbPrimary;

LPDIRECTSOUNDBUFFER g_pdsbBeeps[3]; // Our Three Beeps

//-----------------------------------------------------------------------------
// Name: ReadMMIO()
// Desc: Support function for reading from a multimedia I/O stream
//-----------------------------------------------------------------------------
HRESULT ReadMMIO( HMMIO hmmioIn, MMCKINFO* pckInRIFF, WAVEFORMATEX** ppwfxInfo )
{
    MMCKINFO        ckIn;           // chunk info. for general use.
    PCMWAVEFORMAT   pcmWaveFormat;  // Temp PCM structure to load in.       

    *ppwfxInfo = NULL;

    if( ( 0 != mmioDescend( hmmioIn, pckInRIFF, NULL, 0 ) ) )
        return E_FAIL;

    if( (pckInRIFF->ckid != FOURCC_RIFF) ||
        (pckInRIFF->fccType != mmioFOURCC('W', 'A', 'V', 'E') ) )
        return E_FAIL;

    // Search the input file for for the 'fmt ' chunk.
    ckIn.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if( 0 != mmioDescend(hmmioIn, &ckIn, pckInRIFF, MMIO_FINDCHUNK) )
        return E_FAIL;

    // Expect the 'fmt' chunk to be at least as large as <PCMWAVEFORMAT>;
    // if there are extra parameters at the end, we'll ignore them
       if( ckIn.cksize < (LONG) sizeof(PCMWAVEFORMAT) )
           return E_FAIL;

    // Read the 'fmt ' chunk into <pcmWaveFormat>.
    if( mmioRead( hmmioIn, (HPSTR) &pcmWaveFormat, 
                  sizeof(pcmWaveFormat)) != sizeof(pcmWaveFormat) )
        return E_FAIL;

    // Allocate the waveformatex, but if its not pcm format, read the next
    // word, and thats how many extra bytes to allocate.
    if( pcmWaveFormat.wf.wFormatTag == WAVE_FORMAT_PCM )
    {
        if( NULL == ( *ppwfxInfo = new WAVEFORMATEX ) )
            return E_FAIL;

        // Copy the bytes from the pcm structure to the waveformatex structure
        memcpy( *ppwfxInfo, &pcmWaveFormat, sizeof(pcmWaveFormat) );
        (*ppwfxInfo)->cbSize = 0;
    }
    else
    {
        // Read in length of extra bytes.
        WORD cbExtraBytes = 0L;
        if( mmioRead( hmmioIn, (CHAR*)&cbExtraBytes, sizeof(WORD)) != sizeof(WORD) )
            return E_FAIL;

        *ppwfxInfo = (WAVEFORMATEX*)new CHAR[ sizeof(WAVEFORMATEX) + cbExtraBytes ];
        if( NULL == *ppwfxInfo )
            return E_FAIL;

        // Copy the bytes from the pcm structure to the waveformatex structure
        memcpy( *ppwfxInfo, &pcmWaveFormat, sizeof(pcmWaveFormat) );
        (*ppwfxInfo)->cbSize = cbExtraBytes;

        // Now, read those extra bytes into the structure, if cbExtraAlloc != 0.
        if( mmioRead( hmmioIn, (CHAR*)(((BYTE*)&((*ppwfxInfo)->cbSize))+sizeof(WORD)),
                      cbExtraBytes ) != cbExtraBytes )
        {
            delete *ppwfxInfo;
            *ppwfxInfo = NULL;
            return E_FAIL;
        }
    }

    // Ascend the input file out of the 'fmt ' chunk.
    if( 0 != mmioAscend( hmmioIn, &ckIn, 0 ) )
    {
        delete *ppwfxInfo;
        *ppwfxInfo = NULL;
        return E_FAIL;
    }

    return S_OK;
}


HRESULT WaveOpenFile( CHAR* strFileName, HMMIO* phmmioIn, WAVEFORMATEX** ppwfxInfo,
                  MMCKINFO* pckInRIFF )
{
    HRESULT hr;
    HMMIO   hmmioIn = NULL;
    
    if( NULL == ( hmmioIn = mmioOpen( strFileName, NULL, MMIO_ALLOCBUF|MMIO_READ ) ) )
        return E_FAIL;

    if( FAILED( hr = ReadMMIO( hmmioIn, pckInRIFF, ppwfxInfo ) ) )
    {
        mmioClose( hmmioIn, 0 );
        return hr;
    }

    *phmmioIn = hmmioIn;

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: WaveStartDataRead()
// Desc: Routine has to be called before WaveReadFile as it searches for the
//       chunk to descend into for reading, that is, the 'data' chunk.  For
//       simplicity, this used to be in the open routine, but was taken out and
//       moved to a separate routine so there was more control on the chunks
//       that are before the data chunk, such as 'fact', etc...
//-----------------------------------------------------------------------------
HRESULT WaveStartDataRead( HMMIO* phmmioIn, MMCKINFO* pckIn,
                           MMCKINFO* pckInRIFF )
{
    // Seek to the data
    if( -1 == mmioSeek( *phmmioIn, pckInRIFF->dwDataOffset + sizeof(FOURCC),
                        SEEK_SET ) )
        return E_FAIL;

    // Search the input file for for the 'data' chunk.
    pckIn->ckid = mmioFOURCC('d', 'a', 't', 'a');
    if( 0 != mmioDescend( *phmmioIn, pckIn, pckInRIFF, MMIO_FINDCHUNK ) )
        return E_FAIL;

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: WaveReadFile()
// Desc: Reads wave data from the wave file. Make sure we're descended into
//       the data chunk before calling this function.
//          hmmioIn      - Handle to mmio.
//          cbRead       - # of bytes to read.   
//          pbDest       - Destination buffer to put bytes.
//          cbActualRead - # of bytes actually read.
//-----------------------------------------------------------------------------
HRESULT WaveReadFile( HMMIO hmmioIn, UINT cbRead, BYTE* pbDest,
                      MMCKINFO* pckIn, UINT* cbActualRead )
{
    MMIOINFO mmioinfoIn;         // current status of <hmmioIn>

    *cbActualRead = 0;

    if( 0 != mmioGetInfo( hmmioIn, &mmioinfoIn, 0 ) )
        return E_FAIL;
                
    UINT cbDataIn = cbRead;
    if( cbDataIn > pckIn->cksize ) 
        cbDataIn = pckIn->cksize;       

    pckIn->cksize -= cbDataIn;
    
    for( DWORD cT = 0; cT < cbDataIn; cT++ )
    {
        // Copy the bytes from the io to the buffer.
        if( mmioinfoIn.pchNext == mmioinfoIn.pchEndRead )
        {
            if( 0 != mmioAdvance( hmmioIn, &mmioinfoIn, MMIO_READ ) )
                return E_FAIL;

            if( mmioinfoIn.pchNext == mmioinfoIn.pchEndRead )
                return E_FAIL;
        }

        // Actual copy.
        *((BYTE*)pbDest+cT) = *((BYTE*)mmioinfoIn.pchNext);
        mmioinfoIn.pchNext++;
    }

    if( 0 != mmioSetInfo( hmmioIn, &mmioinfoIn, 0 ) )
        return E_FAIL;

    *cbActualRead = cbDataIn;
    return S_OK;
}


HRESULT sound_dsb_from_wav(LPLPDIRECTSOUNDBUFFER ppdsb, char *szFile)
{
	DSBUFFERDESC dsbd;
	HMMIO hmmio;
	WAVEFORMATEX *pwfmt;
	MMCKINFO mmckinfoParent, mmckinfoData;
	VOID *pv, *pvT;
	DWORD cb, cbT, cbTT;
	DWORD cbData;
	HRESULT hr;

	hr = WaveOpenFile(szFile, &hmmio, &pwfmt, &mmckinfoParent);
	hr = WaveStartDataRead(&hmmio, &mmckinfoData, &mmckinfoParent);



	ZeroMemory(&dsbd, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_STATIC | DSBCAPS_GLOBALFOCUS;
	dsbd.dwBufferBytes = mmckinfoData.cksize;
	dsbd.lpwfxFormat = pwfmt;
	
	hr = g_pds->CreateSoundBuffer(&dsbd, ppdsb, NULL);

	cbData = mmckinfoData.cksize;

	(*ppdsb)->Lock(0, cbData, &pv, &cb, &pvT, &cbT,  0);

	hr = WaveReadFile(hmmio, cbData, (BYTE *) pv, &mmckinfoData, (unsigned int *) &cbTT);
	hr = (*ppdsb)->Unlock(pv, cb, pvT, cbT);

	free(pwfmt);
	mmioClose(hmmio, 0);
	
	return S_OK;
}

extern HWND Our_Hwnd;
int sound_init()
{

	HRESULT hr;
	DSBUFFERDESC dsbd;

	// only initialize if sound is enabled
	if (get_sound_enabled())
	{

		CoInitialize(NULL);
		hr = DirectSoundCreate(NULL, &g_pds, NULL);

		hr = g_pds->SetCooperativeLevel(GetForegroundWindow(), DSSCL_PRIORITY);

		ZeroMemory(&dsbd, sizeof(dsbd));
		dsbd.dwSize = sizeof(dsbd);
		dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
		g_pds->CreateSoundBuffer(&dsbd, &g_pdsbPrimary, NULL);
		g_pdsbPrimary->Release();

		sound_dsb_from_wav( &(g_pdsbBeeps[0]), "beep0.wav");
		sound_dsb_from_wav( &(g_pdsbBeeps[1]), "beep1.wav");
		sound_dsb_from_wav( &(g_pdsbBeeps[2]), "beep2.wav");

	}

	return TRUE;
}


int sound_beep(int whichone)
{
	HRESULT hr;

	if (get_sound_enabled())
	{
		hr = g_pdsbBeeps[whichone]->Play(0, 0, 0);
	}
	return TRUE;
}

int sound_resume() { return TRUE; }


void sound_suspend() { }
int sound_shutdown()
{

	if (get_sound_enabled())
	{
		g_pdsbBeeps[0]->Release();
		g_pdsbBeeps[1]->Release();
		g_pdsbBeeps[2]->Release();
		g_pds->Release();
	}

	return TRUE;
}
