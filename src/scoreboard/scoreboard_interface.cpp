#include "scoreboard_interface.h"

void IScoreboard::PreDeleteInstance()
{
	// Clear the scoreboard upon exit.  This is especially nice for the hardware scoreboard so it doesn't have
	//  stuff lingering on the display.
	Clear();
	DeleteInstance();
}

IScoreboard::IScoreboard() :
m_bNeedsRepaint(false),
m_bUsingAnnunciator(false),
m_bInitialized(false)
{
}

IScoreboard::~IScoreboard()
{
}

bool IScoreboard::Init()
{
	// m_bInitialized must be true for Clear to be able to work
	m_bInitialized = true;

	// but if Clear fails, we won't consider ourselves initialized
	m_bInitialized = Clear();

	return m_bInitialized;
}

bool IScoreboard::update_player_score(unsigned int digit, unsigned int value, unsigned int player)
{
	bool bRes = false;

	// range check
	if (player < 2)
	{
		// range check
		if (digit < 6)
		{
			if (player == 0)
			{
				bRes = pre_set_digit(value, (WhichDigit) (PLAYER1_0 + digit));
			}
			else
			{
				bRes = pre_set_digit(value, (WhichDigit) (PLAYER2_0 + digit));
			}
		}
	}

	return bRes;
}

bool IScoreboard::update_player_lives(unsigned int value, unsigned int player)
{
	bool bRes = false;

	// range check
	if (player < 2)
	{
		bRes = pre_set_digit(value, (WhichDigit) (LIVES0 + player));
	}

	return bRes;
}

bool IScoreboard::update_credits(unsigned int digit, unsigned int value)
{
	bool bRes = false;

	// range check
	if (digit < 2)
	{
		bRes = pre_set_digit(value, (WhichDigit) (CREDITS1_0 + digit));
	}

	return bRes;
}

bool IScoreboard::pre_set_digit(unsigned int uValue, WhichDigit which)
{
	bool bRes = false;

	// Don't let them set digits if initialization hasn't occurred
	// (this is because there was a bug in Daphne where the scoreboard wasn't getting initialized,
	//  so this forces us to initialize the scoreboard)
	if (m_bInitialized)
	{
		// range check
		if ((uValue <= 0x0F) && (which < DIGIT_COUNT))
		{
			bRes = set_digit(uValue, which);
		}
	}

	return bRes;
}

bool IScoreboard::set_digit_w_sae(unsigned int uValue, WhichDigit which)
{
	bool bAlwaysRepaint = false;

	// if we have a potential annunciator command...
	if ((m_bUsingAnnunciator) && (uValue == 0x0C))
	{
		// annunciator uses these four digits and so we must always pass a value through in these circumstances
		switch (which)
		{
		case PLAYER1_3:
		case PLAYER1_5:
		case LIVES0:
		case LIVES1:
			bAlwaysRepaint = true;
			break;
		default:
			break;
		}
	}

	// Check for special SAE case, that only happens on player 2 score
	if ((which >= this->PLAYER2_0) && (which <= this->PLAYER2_5))
	{
		// If we've had an 'A' displayed and we get a 0xC or 0xE,
		//  then keep displaying the 'A'
		if ((m_DigitValues[which] == 0x10) &&
			((uValue == 0xC) || (uValue == 0xE)))
		{
		}
		// Else if we detect the character flicker, then make it an 'A', because
		//  that's what SAE was intended to do.
		else if (
			((uValue == 0xC) && (m_DigitValues[which] == 0xE)) ||
			((uValue == 0xE) && (m_DigitValues[which] == 0xC))
			)
		{
			m_DigitValues[which] = 0x10;
			m_bNeedsRepaint = true;
		}
		// else handle it normally
		else if (m_DigitValues[which] != uValue)
		{
			m_DigitValues[which] = uValue;
			m_bNeedsRepaint = true;
		}
		// else nothing has changed
	}
	// else it's not player 2's score AND
	//  the value has changed or else we're in 'annunciator' mode and need to always repaint
	else if ((m_DigitValues[which] != uValue) || (bAlwaysRepaint))
	{
		// range checking has already been done for us
		m_DigitValues[which] = uValue;
		m_bNeedsRepaint = true;
	}
	// else nothing has changed

	return true;
}

bool IScoreboard::pre_get_digit(unsigned int &uValue, WhichDigit which)
{
	bool bRes = false;

	// don't let them get digits if initialization hasn't occurred
	if (m_bInitialized)
	{
		// make sure requested value is within range
		if (which < DIGIT_COUNT)
		{
			bRes = get_digit(uValue, which);
		}
	}

	return bRes;
}

bool IScoreboard::Clear()
{
	bool bRes = true;
	for (unsigned int u = 0; u < DIGIT_COUNT; u++)
	{
		// clear all digits to 0xF (which is blank)
		if (!pre_set_digit(0x0F, (WhichDigit) u))
		{
			bRes = false;
			break;
		}
	}

	return bRes;
}
