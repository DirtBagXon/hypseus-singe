/*
 * ssi263.cpp
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


// SSI263.CPP
// by Matt Ownby
//
// This is a _PARTIAL_ SSI263 speech chip emulator designed in order to emulate the speech used
// in the arcade game Thayer's Quest.
// At the time of this writing, we have not been able to find much documentation on how to program
// this chip, so we have had to make certain assumptions which likely won't hold true for other
// applications that used the SSI263.
//
// SSI-263 "emulation" interface
// by Garry Jordan
// Uses the (heavily modified) "rsynth" voice synthesis engine.

#include <string.h>
#include "tqsynth.h"
#include "samples.h"
#include "../daphne.h"
#include "../game/thayers.h"
#include "../io/input.h"
#include "../cpu/cpu.h"
#include "../io/conout.h"
#include "ssi263.h"

#ifdef SSI_REG_DEBUG
#include <stdio.h>
#endif

// This code attempts to emulate the SSI263 speech chip used in the game Thayer's Quest

// SSI-263 Control flag.
static bool m_ssi_control = false;  // Whe true SSI-263 is in Control Mode.

// SSI-263->rsynth phoneme conversion.
typedef struct _phoneme_xlate_table
{
    const char *ssi263_phoneme;
    const char *rsynth_phoneme;
} phoneme_xlate_table;

static bool m_speech_enabled = false;   // When true SSI-263 is ready to synthesize speech.
static thayers *m_thayers = NULL;       // Need to call into the TQ class to
                                        // subtitle the speech buffer text.

// Forward declarations of local funtions.
void ssi263_say_phones(char *phonemes, int len);

// Duration/Phoneme
// Working theory: top 2 bits are for duration, the rest is for the phoneme
// A duration of 0x0 is the slowest, 0x03 (both bits set) is the fastest
void ssi263_reg0(unsigned char value, Uint8 *irq_status)
{
    static char phones_text[SSI_PHRASE_BUF_LEN];  // Holds rsynth phonemes.
    static int phones_len = 0;

#if defined(SSI_DEBUG) || defined(SSI_REG_DEBUG)
	char s[SSI_PHRASE_BUF_LEN] = {0};
#endif
#ifdef SSI_DEBUG
    static char ssi263_phoneme_text[SSI_PHRASE_BUF_LEN]; // Phonemes sent to the SSI-263.
#endif

#define NUM_PHONEMES 64

    // SSI-263->rsynth phoneme conversion table.
    static const phoneme_xlate_table phoneme_xlate[NUM_PHONEMES] =
    {
        {"pause", " "},
        {"E", "i"},
        {"EI", "e"},
        {"Y", " "},
        {"Y1", "j"},
        {"AY", " "},
        {"IE", " "},
        {"I", "I"},
        {"A", "eI"},
        {"AI", "e@"},
        {"EH", "e"},
        {"EHI", "e"},
        {"AE", "&"},
        {"AEI", "&"},
        {"AH", "0"},
        {"AHI", "A"},
        {"AW", "O"},
        {"O", "0"},
        {"OU", "@U"},
        {"OO", "U"},
        {"IU", " "},
        {"IUI", " "},
        {"U", "u"},
        {"UI", "U@"},
        {"UH", "V"},
        {"UHI", "V"},
        {"UH2", "@"},
        {"UH3", "@"},
        {"ER", "3"},
        {"R", "r"},
        {"R1", " "},
        {"R2", " "},
        {"L", "l"},
        {"LI", "l"},
        {"LF", "l"},
        {"W", "w"},
        {"B", "b"},
        {"D", "d"},
        {"KV", "g"},
        {"P", "p"},
        {"T", "t"},
        {"K", "k"},
        {"HV", NULL},
        {"HVC", NULL},
        {"HF", "h"},
        {"HFC", NULL},
        {"HN", NULL},
        {"Z", "z"},
        {"S", "s"},
        {"J", "dZ"},
        {"SCH", "S"},
        {"V", "v"},
        {"F", "f"},
        {"THV", "T"},
        {"TH", "T"},
        {"M", "m"},
        {"N", "n"},
        {"NG", "N"},
        {"A", " "},
        {"OH", " "},
        {"U", " "},
        {"UH", " "},
        {"E2", " "},
        {"LB", " "}
    };

	// 0x80 written to reg3 sets g_ssi_control == true and puts the SSI-263
    // into control mode.
	if (m_ssi_control)
	{
		// 0xC0 starts the speech chip requesting phonemes in control mode
		if (value == 0xC0)
		{
#ifdef SSI_REG_DEBUG
            printline("ssi263_reg0: SSI263 enabled");
#endif
            if (m_speech_enabled)
            {
                // Speech synthesis option active, so reset everything so
                // phonemes can be assembled...
                if (phones_len)
                {
                    memset(phones_text, 0, phones_len);
                    phones_len = 0;
                }
            }

              *irq_status &= ~0x04;  // Enable SSI-263 IRQ (clear IRQ status bit 2)
		}
		// Zero stops the speech chip requesting phonemes (stops raising IRQs).
		else if (value == 0)
		{
#ifdef SSI_REG_DEBUG
            printline("ssi263_reg0: SSI263 disabled");
#endif
            // Call into the thayer class to display the speech text buffer,
            // as it has the game's RAM memory. Besides, it's controlling the
            // video overlay anyway...
            m_thayers->show_speech_subtitle();

            if (m_speech_enabled)
            {
                // Done concatenating phonemes, so speak if have something to say.
                if (phones_len)
                {
#ifdef SSI_DEBUG
                    sprintf(s, "SSI-263 phonemes: %s", ssi263_phoneme_text);
                    printline(s);

                    sprintf(s, "Rsynth phonemes: %s", phones_text);
                    printline(s);

                    ssi263_phoneme_text[0] = '\0';
#endif
                    // Since the speech output will likely a while,
					// the cpu timer is 'paused' here

					cpu_pause();	// MPO : this replaces flush_cpu_timers

                    // Synthesize and speak the phonemes.
                    ssi263_say_phones(phones_text, phones_len);

					cpu_unpause();

                }
            }

            *irq_status |= 0x04; // Done requesting data (set IRQ control bit 2).
		}
	}
	// We are receiving phoneme/duration data.
	else
	{
#ifdef SSI_REG_DEBUG
		unsigned char duration = static_cast<unsigned char>((value & 0xC0) >> 6);
#endif
		unsigned char phoneme = static_cast<unsigned char>(value & 0x3F);

		switch (phoneme)
		{
		case 0:
    		// Some form of pause.
#ifdef SSI_REG_DEBUG
            sprintf(s, "ssi263_reg0: Pause duration 0x%x", duration);
			printline(s);
#endif
            if (m_speech_enabled)
            {
                // The rsynth pause "phoneme" is the space character. Check to make
                // sure we're not just concatenating spaces and wasting CPU cycles.
#ifdef SSI_DEBUG
                strcat(ssi263_phoneme_text, " ");
#endif
                if (phones_len && phones_text[phones_len - 1] != ' ')
                {
                    phones_text[phones_len++] = ' ';
                    phones_text[phones_len] = '\0';
                }
            }

			break;

		default:
            if (phoneme > NUM_PHONEMES)
            {
                printline("ssi263_reg0: Phoneme code > 0x3F!");
            }
            else if (phoneme_xlate[phoneme].rsynth_phoneme)
            {
                const char *p_start;
		const char *p_end;

#ifdef SSI_DEBUG
                strcat(ssi263_phoneme_text, phoneme_xlate[phoneme].ssi263_phoneme);
#endif
                // The SSI-263 appears to have some very short duration phonemes
                // that are repeated to stretch the sound out. The rsynth
                // phonemes are of longer duration, so eliminate any duplicate
                // phonemes to help prevent a "stutter" effect.
                p_start = phoneme_xlate[phoneme].rsynth_phoneme;

                // Rsynth phonemes can be either one or two characters long.
                for (p_end = p_start; *p_end; p_end++);

                if (p_end - p_start == 2 && phones_len >= 2)
                {
                    // Check for two-character phoneme duplicate.
                    if (*(p_end - 2) == phones_text[phones_len - 2] &&
                        *(p_end - 1) == phones_text[phones_len - 1])
                    {
                        p_start = NULL;
                    }
                }
                else if (phones_len >= 1)
                {
                    // Phoneme is only a single character.
                    if (*(p_end - 1) == phones_text[phones_len - 1])
                    {
                        // Was a duplicate to it's predecessor.
                        p_start = NULL;
                    }
                    else if(*(p_end - 1) == 'g' &&
                            phones_text[phones_len - 1] == 'k')
                    {
                        // Thayer's speech rules combine "k+g" to make a "G"
                        // sound. When translated to rsynth, an annoying clicking
                        // is produced, so lose the "k" predecessor phoneme.
                        phones_len--;
                    }
                }

                if (p_start)
                {
                    // We have a phoneme to add to the word/phrase.
                    phones_len += sprintf(&phones_text[phones_len], "%s", p_start);
                }
            }

#ifdef SSI_REG_DEBUG
            sprintf(s, "ssi263_reg0: Phoneme 0x%x, duration 0x%x", phoneme, duration);
			printline(s);
#endif
			break;
		}
    }
}

// INFLECT
void ssi263_reg1(unsigned char value)
{
#ifdef SSI_REG_DEBUG
	char s[81] = { 0 };
#endif

	switch (value)
	{
	    case 0x46:
        case 0x4E:
        case 0x56:
	    case 0x5E:
        case 0x66:
        case 0x6E:
	    case 0x76:
	    case 0x8E:
#ifdef SSI_REG_DEBUG
            sprintf(s, "ssi263_reg1: Inflection byte, 0x%x", value);
            printline(s);
#endif
		    break;

    	default:
#ifdef SSI_REG_DEBUG
            sprintf(s, "ssi263_reg1: Unknown inflection byte, %x", value);
		    printline(s);
#endif
		    break;
	}
}

// Speech Rate
void ssi263_reg2(unsigned char value)
{
	switch (value)
	{
	case 0x98:
#ifdef SSI_REG_DEBUG
        printline("ssi263_reg2: Speech rate set to low");
#endif
		break;
	case 0xA8:
#ifdef SSI_REG_DEBUG
        printline("ssi263_reg2: Speech rate set to high");
#endif
		break;
	default:
#ifdef SSI_REG_DEBUG
		char s[81] = { 0 };
        sprintf(s, "ssi263_reg2: Unknown speech rate byte, 0x%x", value);
		printline(s);
#endif
		break;
	}
}

// CTTRAMP
void ssi263_reg3(unsigned char value)
{
	if (value & 0x80)
	{
        // High bit set puts SSI-263 into control mode. 
		m_ssi_control = true;
#ifdef SSI_REG_DEBUG
        printline("ssi263_reg3: Control mode enabled");
#endif
	}
	else if (value & 0x70)
	{
		m_ssi_control = false;
#ifdef SSI_REG_DEBUG
        printline("ssi263_reg3: Control mode disabled");
#endif
    }
    else
    {
		switch (value)
		{
            case 0x6C:
            case 0x6F:
            case 0x7B:
            case 0x7F:
	            break;

            default:
#ifdef SSI_REG_DEBUG
                char s[81] = { 0 };

                sprintf(s, "ssi263_reg3: Unknown amplitude code, 0x%x", value);
                printline(s);
#endif
                break;
		}
	}
}

// Filter Frequency
void ssi263_reg4(unsigned char value)
{
	switch (value)
	{
	case 0xE6:
#ifdef SSI_REG_DEBUG
        printline("ssi263_reg4: Filter frequency set to low");
#endif
		break;
	case 0xE7:
#ifdef SSI_REG_DEBUG
        printline("ssi263_reg4: Filter frequency set to high");
#endif
		break;
	default:
#ifdef SSI_REG_DEBUG
        char s[81] = {0};

        sprintf(s, "ssi263_reg4: Unknown filter frequency value, 0x%x", value);
        printline(s);
#endif
		break;
	}
}

// ***************** Thayer's Quest Speech Project ************************* //
// *  All functions from here add SSI-263->rsynth support.                 * //
// ************************************************************************* //
// Query the current audio parameters and pass them on to the synthesizer.
bool ssi263_init(bool init_speech)
{
    bool result = false;

    // Always need a TQ class pointer so subtitled speech text can be displayed.
    m_thayers = dynamic_cast<thayers *>(g_game);

    if (m_thayers)
    {
        if (init_speech)
        {
            // Request voice to have an F0 base frequency of 110Hz.
            tqsynth_init(AUDIO_FREQ, AUDIO_FORMAT, AUDIO_CHANNELS, 1100);
            m_speech_enabled = true;
        }

        result = true;
    }

    return result;
}

bool g_bSamplePlaying = false;

// Take phoneme text and ship it off to get turned into a speech wavefile. We
// request a raw waveform because it provides an opportunity exercise a little
// more control over the playback (could have done this in the tqsynth code,
// but wanted tqsynth to be somewhat independent of the Daphne code).
void ssi263_say_phones(char *phonemes, int len)
{
    sample_s the_sample;
    unsigned int channel;

	the_sample.pu8Buf = NULL;
	the_sample.uLength = 0;

	if (tqsynth_phones_to_wave(phonemes, len, &the_sample))
	{
		g_bSamplePlaying = true;	// so that we don't overlap samples (only happens at the very beginning of boot-up)
		channel = samples_play_sample(the_sample.pu8Buf, the_sample.uLength, AUDIO_CHANNELS, -1, ssi263_finished_callback);

		// Wait for sample to stop playing
		// NOTE : This is a hack and isn't proper emulation.
		// The proper fix to this is to return to the ROM some signal that our sample has finished playing.
		while ((g_bSamplePlaying) && (!get_quitflag()))
		{
			SDL_Delay(10);
			SDL_check_input();
		}

    }
	else
	{
		printline("SSI263_SAY_PHONES error : phones_to_wave procedure failed");
	}
}

// gets called when sample has finished playing
void ssi263_finished_callback(Uint8 *pu8Buf, unsigned int uSlot)
{
	g_bSamplePlaying = false;
	tqsynth_free_chunk(pu8Buf);
}
