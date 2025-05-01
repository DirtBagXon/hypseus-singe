#include "config.h"

#include "img_scoreboard.h"
#include "../video/video.h"

// Graphical coordinates of where all these LED's should go

#define LED_WIDTH 40
#define LED_HEIGHT 53
#define TITLE_HEIGHT 20

const int player_title_x = 65;
const int player_score_x = 45;
const int player1_score_y = 75;
const int player1_title_y = player1_score_y - 27;
const int player1_lives_y = player1_score_y + LED_HEIGHT + 14;
const int player_lives_x = 166 - LED_WIDTH + 15;
const int player1_lives_title_y = player1_lives_y + (LED_HEIGHT/2) - (TITLE_HEIGHT/2);	// centered with LED
const int player_lives_title_x = player_lives_x + LED_WIDTH + 21;
const int player2_score_y = player1_lives_y + LED_HEIGHT + 45;
const int player2_title_y = player2_score_y - 27;
const int player2_lives_y = player2_score_y + LED_HEIGHT + 14;
const int player2_lives_title_y = player2_lives_y + (LED_HEIGHT/2) - (TITLE_HEIGHT/2);
const int credits_x = player_lives_x - LED_WIDTH;
const int credits_y = player2_lives_y + LED_HEIGHT + 23;
const int credits_title_y = credits_y + 18;

const int thayers_time_x = 130;
const int thayers_time_y = 18;
const int time_title_x = thayers_time_x + (LED_WIDTH * 2.6);
const int time_title_y = thayers_time_y + (LED_HEIGHT / 2.5);

void ImgScoreboard::DeleteInstance()
{
	delete this;
}

void ImgScoreboard::Invalidate()
{
	m_bNeedsRepaint = true;
}

bool ImgScoreboard::RepaintIfNeeded()
{
	bool bRepainted = false;

	unsigned int coord_x = 0;
	unsigned int coord_y = 0;
	unsigned int uValue = 0;

	if (m_bNeedsRepaint)
	{
		bool (*decor)(int, int, int);
		decor = video::draw_othergfx;

		if (m_bThayers)
		{
			decor(video::B_TQ_TIME, time_title_x, time_title_y);
		}
		else
		{
			// draw all scoreboard decorations
			decor(video::B_DL_PLAYER1, player_title_x, player1_title_y);
			decor(video::B_DL_PLAYER2, player_title_x, player2_title_y);
			decor(video::B_DL_LIVES, player_lives_title_x, player1_lives_title_y);
			decor(video::B_DL_LIVES, player_lives_title_x, player2_lives_title_y);
			decor(video::B_DL_CREDITS, player_lives_title_x, credits_title_y);
		}

		// draw all digits
		for (unsigned int which = 0; which < DIGIT_COUNT; which++)
		{
			if (!m_bThayers)
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
				video::draw_led(uValue, coord_x, coord_y, 0xf);
			}
			else
			{
				if ((which == CREDITS1_0) || (which == CREDITS1_1)) {
					unsigned int digit = (which - CREDITS1_0) & 1;
					coord_x = (LED_WIDTH * digit) + thayers_time_x;
					video::draw_led(m_DigitValues[which], coord_x,
							thayers_time_y, 0x1);
				}
			}
		}

		bRepainted = true;
		m_bNeedsRepaint = false;
	}

	return bRepainted;
}

bool ImgScoreboard::ChangeVisibility(bool bVisible)
{
	// we don't support changing visibility with an image scoreboard
	return false;
}

bool ImgScoreboard::set_digit(unsigned int uValue, WhichDigit which)
{
	return set_digit_w_sae(uValue, which);
}

bool ImgScoreboard::is_repaint_needed()
{
	return m_bNeedsRepaint;
}

bool ImgScoreboard::get_digit(unsigned int &uValue, WhichDigit which)
{
	uValue = m_DigitValues[which];
	return true;
}

ImgScoreboard::ImgScoreboard() :
m_bThayers(false)
{
}

IScoreboard *ImgScoreboard::GetInstance(bool bThayers)
{
	ImgScoreboard *pInstance = new ImgScoreboard();
	pInstance->m_bThayers = bThayers;
	if (!pInstance->Init())
	{
		pInstance->DeleteInstance();
		pInstance = NULL;
	}

	return pInstance;
}
