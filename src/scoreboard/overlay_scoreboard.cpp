#include "overlay_scoreboard.h"
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
	bool bRepainted = false;
	if (m_bNeedsRepaint)
	{
		SDL_Surface *pSurface = m_pFuncGetActiveOverlay();

		// if the overlay is visible
		if (m_bVisible)
		{
			// for Dragon's Lair/Space Ace
			if (!m_bThayers)
			{
				// Draw all DL/SA scoreboard labels.
				// Credits Label
				draw_string("Credits", pSurface->w / 12 - (pSurface->w == 360 ? 4 : 3), 0, pSurface);

				// Player labels.
				draw_string("Player 1: ", 1, 0, pSurface);
				draw_string("Player 2: ", pSurface->w / 6 - 19, 0, pSurface);

				// Lives labels.
				draw_string("Lives: ", 1, 1, pSurface);
				draw_string("Lives: ", pSurface->w / 6 - 10, 1, pSurface);

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
				// Thayer's Quest only uses "Credits" portion of the DL/SA
				// scoreboard.
				draw_string("Time", pSurface->w / 12 - 2, 0, pSurface);
			}

			// Update Credits
			update_credits(pSurface);
		}
		// else the overlay is invisible, so erase the surface
		else
		{
			SDL_FillRect(pSurface, NULL, 0);
		}

		bRepainted = true;
		m_bNeedsRepaint = false;
	}
	return bRepainted;
}

void OverlayScoreboard::update_player_score (SDL_Surface *pSurface, int player, int start_digit, unsigned int values[], int num_digits)
{
	int x = start_digit * OVERLAY_LED_WIDTH;

	// Player1 position is static, but Player2 will be affected
	// by different MPEG widths (640x480, 720x480 known so far).
	x += (player == 0 ? 65 : pSurface->w - 7 * OVERLAY_LED_WIDTH);

	draw_overlay_leds(values, num_digits, x, 0, pSurface);
}

void OverlayScoreboard::update_player_lives (SDL_Surface *pSurface, int player, unsigned int lives)
{
	// Value of lives was validated in caller, so charge right ahead.
	draw_overlay_leds(&lives, 1,
		player == 0 ? 48 : pSurface->w - 2 * OVERLAY_LED_WIDTH,
		OVERLAY_LED_HEIGHT, pSurface);
}

void OverlayScoreboard::update_credits(SDL_Surface *pSurface)
{
	int fudge;

	// need to shift a bit to look exactly centered
	if (m_bThayers)
	{
		fudge = (pSurface->w == 360 ? 4 : 3);
	}
	else
	{
		fudge = (pSurface->w == 360 ? 2 : 0);
	}

	draw_overlay_leds(m_DigitValues + this->CREDITS1_0, 2,
		pSurface->w / 2 - (OVERLAY_LED_WIDTH + fudge),
		OVERLAY_LED_HEIGHT, pSurface);
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
