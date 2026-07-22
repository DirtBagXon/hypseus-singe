#include "config.h"

#include "overlay_scoreboard.h"
#include "../hypseus.h"
#include "../video/video.h"	// for draw_string

#define OVERLAY_LED_WIDTH 8
#define OVERLAY_LED_HEIGHT 13

void OverlayScoreboard::DeleteInstance()
{
	delete this;
}

void OverlayScoreboard::Invalidate()
{
	m_bNeedsRepaint = true;
}

bool OverlayScoreboard::RepaintIfNeeded()
{
	if (get_scoreboard() & 0x03) m_bVisible = false;

	bool bRepainted = false;
	if (m_bNeedsRepaint)
	{
		SDL_Color rgb = {0xF0, 0xF0, 0xF0, 0xFF};
		SDL_Surface *pSurface = video::get_screen_leds();
		bool legacy = video::use_legacy_font();

		void (*decor)(const char*, int, int, SDL_Surface*, SDL_Color, bool);
		decor = video::draw_string;

		// if the overlay is visible
		if (m_bVisible)
		{
			// for Dragon's Lair/Space Ace
			if (!m_bThayers)
			{
				// Draw all DL/SA scoreboard labels.
                                if (legacy) {
                                   decor("Credits", 140, 0, pSurface, rgb, false);
                                   decor("Player 1: ", 6, 0, pSurface, rgb, false);
                                   decor("Player 2: ", 209, 0, pSurface, rgb, false);
                                   decor("Lives: ", 6, 14, pSurface, rgb, false);
                                   decor("Lives: ", 268, 14, pSurface, rgb, false);
                                }
                                else {
                                   decor("Credits", 142, 4, pSurface, rgb, true);
                                   decor("Player 1 ", 63, 4, pSurface, rgb, true);
                                   decor("Player 2 ", 210, 4, pSurface, rgb, true);
                                   decor("Lives ", 24, 18, pSurface, rgb, true);
                                   decor("Lives ", 270, 18, pSurface, rgb, true);
                                }

				// Update Player Scores
				update_player_score(pSurface, 0, 0, m_DigitValues + this->PLAYER1_0, 6);
				update_player_score(pSurface, 1, 0, m_DigitValues + this->PLAYER2_0, 6);

				// Update Player Lives
				update_player_lives(pSurface, 0, m_DigitValues[this->LIVES0]);
				update_player_lives(pSurface, 1, m_DigitValues[this->LIVES1]);
			}
			// for Thayer's Quest
			else
			{
				// Thayer's Quest only uses "Credits" portion of the DL/SA // scoreboard.
                                if (legacy)
                                   decor("Time", 150, 0, pSurface, rgb, false);
                                else
                                   decor("Time", 146, 4, pSurface, rgb, true);
			}

			// Update Credits
			update_credits(pSurface);
		}
		// else the overlay is invisible, so erase the surface
		else
		{
			SDL_FillSurfaceRect(pSurface, NULL, 0x00000000);
		}

		bRepainted = true;
		m_bNeedsRepaint = false;
	}
	return bRepainted;
}

void OverlayScoreboard::update_player_score (SDL_Surface *pSurface, int player, int start_digit, unsigned int values[], int num_digits)
{
	int x = start_digit * OVERLAY_LED_WIDTH;

	// Player1 position is static, and Player2 is NOT affected
	// by different MPEG widths (640x480, 720x480) anymore.
	if (video::use_legacy_font())
	    x += (player == 0 ? 65 : 269);
	else
	    x += (player == 0 ? 8 : 266);

	video::draw_overlay_leds(values, num_digits, x, 0);
}

void OverlayScoreboard::update_player_lives (SDL_Surface *pSurface, int player, unsigned int lives)
{
	// Value of lives was validated in caller, so charge right ahead.
	if (video::use_legacy_font())
	    video::draw_overlay_leds(&lives, 1,
		player == 0 ? 48 : 309,
		OVERLAY_LED_HEIGHT);
	else
	    video::draw_overlay_leds(&lives, 1,
		player == 0 ? 8 : 306,
		OVERLAY_LED_HEIGHT);
}

void OverlayScoreboard::update_credits(SDL_Surface *pSurface)
{
	int x;

	// need to shift a bit to look exactly centered
	if (m_bThayers)
	{
		x = (video::use_legacy_font() ? 154 : 149);
	}
	else
	{
		x = 153;
	}

	video::draw_overlay_leds(m_DigitValues + this->CREDITS1_0,
		2, x, OVERLAY_LED_HEIGHT);
}

bool OverlayScoreboard::ChangeVisibility(bool bVisible)
{
	bool bChanged = false;

	// if the visibility has changed
	if (bVisible != m_bVisible)
	{
		m_bVisible = bVisible;
		m_bNeedsRepaint = true;
		bChanged = true;
	}

	return bChanged;
}

bool OverlayScoreboard::set_digit(unsigned int uValue, WhichDigit which)
{
	return set_digit_w_sae(uValue, which);
}

bool OverlayScoreboard::is_repaint_needed()
{
	return m_bNeedsRepaint;
}

bool OverlayScoreboard::get_digit(unsigned int &uValue, WhichDigit which)
{
	bool bRes = true;
	uValue = m_DigitValues[which];
	return bRes;
}

//

OverlayScoreboard::OverlayScoreboard() :
m_pFuncGetActiveOverlay(NULL),
m_bThayers(false),
m_bVisible(true)
{
}

IScoreboard *OverlayScoreboard::GetInstance(SDL_Surface *(*pFuncGetActiveOverlay)(), bool bThayers)
{
	OverlayScoreboard *pInstance = new OverlayScoreboard();
	pInstance->m_bThayers = bThayers;
	pInstance->m_pFuncGetActiveOverlay = pFuncGetActiveOverlay;
	if (!pInstance->Init())
	{
		pInstance->DeleteInstance();
		pInstance = 0;
	}

	return pInstance;
}
