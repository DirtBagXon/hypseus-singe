#include "null_scoreboard.h"
#include <stdio.h>	// for NULL

void NullScoreboard::DeleteInstance()
{
	delete this;
}

void NullScoreboard::Invalidate()
{
}

bool NullScoreboard::RepaintIfNeeded()
{
	return false;
}

bool NullScoreboard::ChangeVisibility(bool bVisible)
{
	return false;
}

bool NullScoreboard::set_digit(unsigned int uValue, WhichDigit which)
{
	m_DigitValues[which] = uValue;
	return true;
}

bool NullScoreboard::is_repaint_needed()
{
	return false;
}

bool NullScoreboard::get_digit(unsigned int &uValue, WhichDigit which)
{
	uValue = m_DigitValues[which];
	return true;
}

IScoreboard *NullScoreboard::GetInstance()
{
	NullScoreboard *pInstance = new NullScoreboard();
	if (!pInstance->Init())
	{
		delete pInstance;
		pInstance = NULL;
	}
	return pInstance;
}

NullScoreboard::NullScoreboard()
{
}
