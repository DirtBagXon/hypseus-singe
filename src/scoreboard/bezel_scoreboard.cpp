#include "config.h"

#include "bezel_scoreboard.h"
#include "../video/video.h"

// Graphical coordinates of where all these LED's should go

#define LED_WIDTH 40
#define LED_HEIGHT 53
#define TITLE_HEIGHT 25

const int player_score_x = 45;
const int player1_score_y = 75;
const int player1_lives_y = player1_score_y + LED_HEIGHT + 14;
const int player_lives_x = 265 - LED_WIDTH + 15;
const int player2_score_y = player1_lives_y + LED_HEIGHT + 50;
const int player2_lives_y = player2_score_y + LED_HEIGHT + 14;
const int credits_x = player_lives_x - LED_WIDTH;
const int credits_y = player2_lives_y + LED_HEIGHT + 50;

void BezelScoreboard::DeleteInstance()
{
	delete this;
}

void BezelScoreboard::Invalidate()
{
	m_bNeedsRepaint = true;
}

bool BezelScoreboard::RepaintIfNeeded()
{
	bool bRepainted = false;

	unsigned int coord_x = 0;
	unsigned int coord_y = 0;
	unsigned int uValue = 0;

	if (m_bNeedsRepaint)
	{
		// draw all digits
		for (unsigned int which = 0; which < DIGIT_COUNT; which++)
		{
			// this is a player digit
			if ((which >= this->PLAYER1_0) && (which <= this->PLAYER2_5))
			{
				// convert 'which' to a digit ranging from 0-6
				unsigned int digit = ((which - PLAYER1_0) % 6);

				coord_x = (digit * LED_WIDTH) + player_score_x;

				// if this is player 1
				if (which <= this->PLAYER1_5)
				{
					coord_y = player1_score_y;
				}
				// else it's player 2
				else
				{
					coord_y = player2_score_y;
				}
			}
			// else if this is a lives digit
			else if ((which == LIVES0) || (which == LIVES1))
			{
				coord_x = player_lives_x;
				if (which == LIVES0)
				{
					coord_y = player1_lives_y;
				}
				else
				{
					coord_y = player2_lives_y;
				}
			}
			// else it's the credits
			else
			{
				unsigned int digit = (which - CREDITS1_0) & 1;
				coord_x = (LED_WIDTH * digit) + credits_x;
				coord_y = credits_y;
			}

			uValue = m_DigitValues[which];
			video::draw_led(uValue, coord_x, coord_y);
		}

		bRepainted = true;
		m_bNeedsRepaint = false;
	}

	return bRepainted;
}

bool BezelScoreboard::ChangeVisibility(bool bVisible)
{
	return false;
}

bool BezelScoreboard::set_digit(unsigned int uValue, WhichDigit which)
{
	return set_digit_w_sae(uValue, which);
}

bool BezelScoreboard::is_repaint_needed()
{
	return m_bNeedsRepaint;
}

bool BezelScoreboard::get_digit(unsigned int &uValue, WhichDigit which)
{
	uValue = m_DigitValues[which];
	return true;
}

BezelScoreboard::BezelScoreboard()
{
}

IScoreboard *BezelScoreboard::GetInstance()
{
	BezelScoreboard *pInstance = new BezelScoreboard();
	if (!pInstance->Init())
	{
		pInstance->DeleteInstance();
		pInstance = NULL;
	}

	return pInstance;
}
