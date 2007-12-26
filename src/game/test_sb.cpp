#include "test_sb.h"
#include "../io/logger_factory.h"
#include "../daphne.h"	// for get_quitflag
#include "../io/input.h"
#include "../ldp-out/ldp.h"
#include "../scoreboard/scoreboard_collection.h"

test_sb::test_sb()
{
	m_shortgamename = "test_sb";
	m_game_uses_video_overlay = false;
}

void test_sb::start()
{
	ILogger *pLogger = LoggerFactory::GetInstance(LoggerFactory::NULLTYPE);
	IScoreboard *pScoreboard = ScoreboardCollection::GetInstance(pLogger);
	unsigned int which = IScoreboard::PLAYER1_0;

	// this is mainly used to test the hardware scoreboard, so we'll add an image and hardware
	ScoreboardCollection::AddType(pScoreboard, ScoreboardFactory::IMAGE);
	ScoreboardCollection::AddType(pScoreboard, ScoreboardFactory::HARDWARE);

	while (!get_quitflag())
	{
		// clear all digits
		for (unsigned int u = IScoreboard::PLAYER1_0; u < IScoreboard::DIGIT_COUNT; u = u + 1)
		{
			pScoreboard->pre_set_digit(0, (IScoreboard::WhichDigit) u);
		}
		// and set the current digit to 1
		pScoreboard->pre_set_digit(1, (IScoreboard::WhichDigit) which);

		which++;
		if (which >= IScoreboard::DIGIT_COUNT)
		{
			which = IScoreboard::PLAYER1_0;
		}

		pScoreboard->RepaintIfNeeded();

		SDL_check_input();
		g_ldp->think_delay(500);
	}

	pScoreboard->PreDeleteInstance();
	pLogger->DeleteInstance();
}
