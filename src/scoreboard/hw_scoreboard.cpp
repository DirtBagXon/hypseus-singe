#include "hw_scoreboard.h"
#include "../io/parallel.h"

void HwScoreboard::DeleteInstance()
{
	delete this;
}

IScoreboard *HwScoreboard::GetInstance(unsigned int uParallelPort, ILogger *pLogger)
{
	HwScoreboard *pRes = 0;

	pRes = new HwScoreboard(uParallelPort, pLogger);

	// try to init, if this fails, we fail
	// (NOTE: ParInit must be called first because Init calls set_digit)
	if (!pRes->ParInit() || !pRes->Init())
	{
		pRes->DeleteInstance();
		pRes = 0;
	}

	return pRes;
}

bool HwScoreboard::ParInit()
{
	bool bRes = par::init(m_uParallelPort, m_pLogger);
	return bRes;
}

void HwScoreboard::ParShutdown()
{
	par::close(m_pLogger);
}

HwScoreboard::HwScoreboard(unsigned int uParallelPort, ILogger *pLogger)
{
	m_uParallelPort = uParallelPort;
	m_pLogger = pLogger;
}

HwScoreboard::~HwScoreboard()
{
	ParShutdown();
}

void HwScoreboard::Invalidate()
{
}

bool HwScoreboard::RepaintIfNeeded()
{
	return false;
}

bool HwScoreboard::set_digit(unsigned int uValue, WhichDigit which)
{
	// NOTE : the 'rsb' prefix stands for Real Score Board

	m_DigitValues[which] = uValue;
	unsigned char rsb_value = 0;
//	int rsb_port = m_uParallelPort;
	unsigned int digit = 0;

	// this is a player digit
	if ((which >= this->PLAYER1_0) && (which <= this->PLAYER2_5))
	{
		digit = ((which - PLAYER1_0) % 6);
		rsb_value = (unsigned char) ((digit << 4) | uValue);
		par::base2(0);
		par::base0(rsb_value);

		// if it's player 1
		if (which <= this->PLAYER1_5)
		{
			par::base2(2);
		}
		// else player 2
		else
		{
			par::base2(8);
		}
	}
	// else if this is a lives digit
	else if ((which == LIVES0) || (which == LIVES1))
	{
		// player 1
		if (which == LIVES0)
		{
			rsb_value = (unsigned char ) (0x60 | uValue);
		}
		// player 2
		else
		{
			rsb_value = (unsigned char) (0x70 | uValue);
		}

		par::base2(0);
		par::base0(rsb_value);
		par::base2(2);

	}
	// else it's the credits
	else
	{
		digit = (which - CREDITS1_0) & 1;

		// first credit digit
		if (digit == 0)
		{
			rsb_value = (unsigned char ) (0x60 | uValue);
		}
		// second credit digit
		else
		{
			rsb_value = (unsigned char) (0x70 | uValue);
		}
		par::base2(0);
		par::base0(rsb_value);
		par::base2(8);
	}

	return true;
}

bool HwScoreboard::is_repaint_needed()
{
	return false;
}

bool HwScoreboard::get_digit(unsigned int &uValue, WhichDigit which)
{
	uValue = m_DigitValues[which];
	return true;
}

bool HwScoreboard::ChangeVisibility(bool bDontCare)
{
	// visibility can't be changed on the HW scoreboard
	return false;
}

