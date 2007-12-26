#include "stdafx.h"

void test_scoreboard_helper(IScoreboard *pScoreboard)
{
	bool bRes = false;
	unsigned int uVal = 0;

	// Test to make sure that scoreboard has been initialized to all 0's
	for (unsigned int u = 0; u < IScoreboard::DIGIT_COUNT; u++)
	{
		bRes = pScoreboard->pre_get_digit(uVal, (IScoreboard::WhichDigit) u);
		TEST_REQUIRE(bRes);
		TEST_CHECK_EQUAL(uVal, 0x0F);
	}

	// try a value that's out of range
	bRes = pScoreboard->pre_get_digit(uVal, IScoreboard::DIGIT_COUNT);
	TEST_CHECK(!bRes);

	// now try the convenience functions

	// CREDITS
	// out of range
	bRes = pScoreboard->update_credits(2, 0);	// try a value out of range
	TEST_CHECK(!bRes);

	// out of range 2
	bRes = pScoreboard->update_credits(0, 17);	// try a value out of range
	TEST_CHECK(!bRes);

	// in range
	bRes = pScoreboard->update_credits(1, 5);
	TEST_CHECK(bRes);
	bRes = pScoreboard->pre_get_digit(uVal, IScoreboard::CREDITS1_1);
	TEST_CHECK_EQUAL(uVal, 5);

	// PLAYER LIVES
	// out of range
	bRes = pScoreboard->update_player_lives(0, 2);
	TEST_CHECK(!bRes);

	// within range
	bRes = pScoreboard->update_player_lives(9, 1);
	TEST_CHECK(bRes);

	// make sure value is correct
	bRes = pScoreboard->pre_get_digit(uVal, IScoreboard::LIVES1);
	TEST_CHECK_EQUAL(uVal, 9);

	// test annunciator value
	bRes = pScoreboard->update_player_lives(0x0C, 1);
	TEST_CHECK(bRes);

	// PLAYER SCORE
	// out of range on digit
	bRes = pScoreboard->update_player_score(6, 0, 0);
	TEST_CHECK(!bRes);

	// out of range on player
	bRes = pScoreboard->update_player_score(5, 0, 2);
	TEST_CHECK(!bRes);

	// should pass
	bRes = pScoreboard->update_player_score(5, 9, 1);
	TEST_CHECK(bRes);

	bRes = pScoreboard->pre_get_digit(uVal, IScoreboard::PLAYER2_5);
	TEST_CHECK_EQUAL(uVal, 9);

	// make sure that player1's digit hasn't changed
	bRes = pScoreboard->pre_get_digit(uVal, IScoreboard::PLAYER1_5);
	TEST_CHECK_EQUAL(uVal, 0xF);

	// test annunciator values
	bRes = pScoreboard->update_player_score(4, 0x0C, 0);
	TEST_CHECK(bRes);

	// now test player 1's score
	bRes = pScoreboard->update_player_score(1, 6, 0);
	TEST_CHECK(bRes);

	bRes = pScoreboard->pre_get_digit(uVal, IScoreboard::PLAYER1_1);
	TEST_CHECK_EQUAL(uVal, 6);
}

// check for the 'A' used in Space Ace Enhanced (SAE)
void test_scoreboard_for_a(IScoreboard *pScoreboard)
{
	bool bRes = false;
	unsigned int uVal = 0;

	bRes = pScoreboard->pre_set_digit(0xC, IScoreboard::PLAYER2_0);
	TEST_CHECK(bRes);

	// a repaint should be needed at this point
	bRes = pScoreboard->is_repaint_needed();
	TEST_CHECK(bRes);

	bRes = pScoreboard->pre_set_digit(0xE, IScoreboard::PLAYER2_0);
	TEST_CHECK(bRes);

	// repaint should happen here too, digit should now be set to 'A'
	bRes = pScoreboard->is_repaint_needed();
	TEST_CHECK(bRes);

	// when cycling 0xC and 0xE on the scoreboard, that means we should have an 'A'
	pScoreboard->pre_get_digit(uVal, IScoreboard::PLAYER2_0);
	TEST_CHECK_EQUAL(uVal, 0x10);

	// clear repaint_needed flag for next test
	pScoreboard->RepaintIfNeeded();

	// back to 0xC, we should still have an 'A'
	bRes = pScoreboard->pre_set_digit(0xC, IScoreboard::PLAYER2_0);
	TEST_CHECK(bRes);
	pScoreboard->pre_get_digit(uVal, IScoreboard::PLAYER2_0);
	TEST_CHECK_EQUAL(uVal, 0x10);

	// we should still be at 'A', so a repaint should not occur
	bRes = pScoreboard->is_repaint_needed();
	TEST_CHECK(!bRes);

	// repaint should occur if we set digit to something else
	pScoreboard->pre_set_digit(0, IScoreboard::PLAYER2_0);
	bRes = pScoreboard->is_repaint_needed();
	TEST_CHECK(bRes);

	// clear repaint_needed flag for next test
	pScoreboard->RepaintIfNeeded();

	// but if we set it again, repaint should not occur
	pScoreboard->pre_set_digit(0, IScoreboard::PLAYER2_0);
	bRes = pScoreboard->is_repaint_needed();
	TEST_CHECK(!bRes);
}

void visibility_helper(IScoreboard *pScoreboard, bool bExpectedResult)
{
	bool bRes = false;

	// visibility probably starts as on by default, so disable it
	bRes = pScoreboard->ChangeVisibility(false);
	TEST_CHECK_EQUAL(bRes, bExpectedResult);

	bRes = pScoreboard->ChangeVisibility(true);
	TEST_CHECK_EQUAL(bRes, bExpectedResult);

	// if we expect to be able to change the visibility, then make sure that repaint is now needed
	if (bExpectedResult == true)
	{
		bRes = pScoreboard->is_repaint_needed();
		TEST_CHECK(bRes);

		// trying to change to the current state should return false
		bRes = pScoreboard->ChangeVisibility(true);
		TEST_CHECK(!bRes);
	}
}

void repaint_helper(IScoreboard *pScoreboard, bool bRepaintUsed)
{
	bool bRes = false;

	// Test Invalidate()
	// if repaint is supported, then invalidating should cause 'repaint needed' to be true,
	//  otherwise it shouldn't have any effect
	pScoreboard->Invalidate();
	bRes = pScoreboard->is_repaint_needed();
	TEST_CHECK_EQUAL(bRes, bRepaintUsed);

	// clear 'repaint needed' flag
	bRes = pScoreboard->RepaintIfNeeded();
	TEST_CHECK_EQUAL(bRes, bRepaintUsed);
	bRes = pScoreboard->is_repaint_needed();
	TEST_CHECK(!bRes);
	bRes = pScoreboard->RepaintIfNeeded();
	TEST_CHECK(!bRes);

	// force a repaint
	pScoreboard->pre_set_digit(0, IScoreboard::CREDITS1_0);
	pScoreboard->pre_set_digit(1, IScoreboard::CREDITS1_0);
	bRes = pScoreboard->is_repaint_needed();
	TEST_CHECK_EQUAL(bRes, bRepaintUsed);

}

TEST_CASE(scoreboard_img)
{
	ILogger *pLog = NullLogger::GetInstance();
	IScoreboard *pScoreboard = ScoreboardFactory::GetInstance(ScoreboardFactory::NULLTYPE, pLog, 0);
	TEST_REQUIRE(pScoreboard);

	test_scoreboard_helper(pScoreboard);
	visibility_helper(pScoreboard, false);	// can't change visibility on null
	repaint_helper(pScoreboard, false);	// repaint not used with null

	pScoreboard->PreDeleteInstance();

	// image scoreboard
	pScoreboard = ScoreboardFactory::GetInstance(ScoreboardFactory::IMAGE, pLog, 0);
	TEST_REQUIRE(pScoreboard);

	test_scoreboard_helper(pScoreboard);
	test_scoreboard_for_a(pScoreboard);
	visibility_helper(pScoreboard, false);	// can't change visibility on image
	repaint_helper(pScoreboard, true);	// repaint is used with image

	pScoreboard->PreDeleteInstance();
	pLog->DeleteInstance();
}

SDL_Surface *g_stub_surface = NULL;

SDL_Surface *stub_function()
{
	return g_stub_surface;
}

TEST_CASE(scoreboard_overlay)
{
	bool bRes = false;
	ILogger *pLog = NullLogger::GetInstance();
	IScoreboard *pScoreboard = ScoreboardFactory::GetInstance(ScoreboardFactory::NULLTYPE, pLog, 0);
	TEST_REQUIRE(pScoreboard);

	// overlay scoreboard
	SDL_Surface *pSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, 
                                        360, 
                                        240, 8, 0, 0, 0, 0); // create an 8-bit surface

	g_stub_surface = pSurface;

	pScoreboard = ScoreboardFactory::GetInstance(ScoreboardFactory::OVERLAY, pLog, stub_function);
	TEST_REQUIRE(pScoreboard);

	test_scoreboard_helper(pScoreboard);
	test_scoreboard_for_a(pScoreboard);
	visibility_helper(pScoreboard, true);	// can change visibility for overlay
	repaint_helper(pScoreboard, true);	// repaint is used with overlay

	// now test to make sure that we can indeed turn off visibility
	bRes = pScoreboard->ChangeVisibility(false);
	TEST_CHECK(bRes);
	bRes = pScoreboard->RepaintIfNeeded();
	TEST_CHECK(bRes);

	Uint8 *pPixel = (Uint8 *) pSurface->pixels;

	// make sure all pixels are black
	for (unsigned int u = 0; u < (unsigned int) (pSurface->w * pSurface->h); u++)
	{
		TEST_CHECK_EQUAL((int) pPixel[u], 0);
		if (pPixel[u] != 0)
		{
			break;
		}
	}

	pScoreboard->PreDeleteInstance();

	// now do the same visibility test, except with Thayer's Quest
	pScoreboard = ScoreboardFactory::GetInstance(ScoreboardFactory::OVERLAY, pLog, stub_function, true);
	TEST_REQUIRE(pScoreboard);

	bRes = pScoreboard->ChangeVisibility(false);
	TEST_CHECK(bRes);
	bRes = pScoreboard->RepaintIfNeeded();
	TEST_CHECK(bRes);

	pPixel = (Uint8 *) pSurface->pixels;

	// make sure all pixels are black
	for (unsigned int u = 0; u < (unsigned int) (pSurface->w * pSurface->h); u++)
	{
		TEST_CHECK_EQUAL((int) pPixel[u], 0);
		if (pPixel[u] != 0)
		{
			break;
		}
	}

	pScoreboard->PreDeleteInstance();

	SDL_FreeSurface(pSurface);

	pLog->DeleteInstance();
}

TEST_CASE(scoreboard_hw)
{
	ILogger *pLog = NullLogger::GetInstance();
	IScoreboard *pScoreboard = ScoreboardFactory::GetInstance(ScoreboardFactory::HARDWARE, pLog, 0);
	TEST_REQUIRE(pScoreboard);

	test_scoreboard_helper(pScoreboard);
	visibility_helper(pScoreboard, false);	// can't change visibility
	repaint_helper(pScoreboard, false);

	pScoreboard->PreDeleteInstance();
	pLog->DeleteInstance();
}

TEST_CASE(scoreboard_collection)
{
	ILogger *pLog = NullLogger::GetInstance();
	IScoreboard *pScoreboard = ScoreboardCollection::GetInstance(pLog);
	TEST_REQUIRE(pScoreboard);

	// image + hardware is a valid configuration
	ScoreboardCollection::AddType(pScoreboard, ScoreboardFactory::IMAGE);
	ScoreboardCollection::AddType(pScoreboard, ScoreboardFactory::HARDWARE);

	test_scoreboard_helper(pScoreboard);
	visibility_helper(pScoreboard, false);	// can't change visibility
	repaint_helper(pScoreboard, true);	// should be able to repaint due to the image

	pScoreboard->PreDeleteInstance();

	// overlay scoreboard
	SDL_Surface *pSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, 
                                        360, 
                                        240, 8, 0, 0, 0, 0); // create an 8-bit surface
	g_stub_surface = pSurface;

	// now try another valid configuration
	pScoreboard = ScoreboardCollection::GetInstance(pLog, stub_function);
	TEST_REQUIRE(pScoreboard);

	// overlay + hardware is a valid configuration
	ScoreboardCollection::AddType(pScoreboard, ScoreboardFactory::OVERLAY);
	ScoreboardCollection::AddType(pScoreboard, ScoreboardFactory::HARDWARE);

	test_scoreboard_helper(pScoreboard);
	visibility_helper(pScoreboard, true);	// can change visibility
	repaint_helper(pScoreboard, true);	// should be able to repaint due to the overlay

	pScoreboard->PreDeleteInstance();
	SDL_FreeSurface(pSurface);
	pLog->DeleteInstance();
}

void annunciator_helper(IScoreboard *pScoreboard, IScoreboard::WhichDigit digit, bool bRepaintExpected)
{
	bool bRes = false;

	bRes = pScoreboard->pre_set_digit(0xC, digit);
	TEST_CHECK(bRes);
	bRes = pScoreboard->is_repaint_needed();
	TEST_CHECK(bRes);
	pScoreboard->RepaintIfNeeded();

	// now set the digit again, we should not do any repaint
	bRes = pScoreboard->pre_set_digit(0xC, digit);
	TEST_CHECK(bRes);
	bRes = pScoreboard->is_repaint_needed();
	TEST_CHECK_EQUAL(bRes, bRepaintExpected);
	pScoreboard->RepaintIfNeeded();
}

TEST_CASE(annunciator)
{
	bool bRes = false;
	ILogger *pLog = NullLogger::GetInstance();
	IScoreboard *pScoreboard = ScoreboardFactory::GetInstance(ScoreboardFactory::IMAGE, pLog, 0, false, false);
	TEST_REQUIRE(pScoreboard);

	annunciator_helper(pScoreboard, IScoreboard::PLAYER1_3, false);
	annunciator_helper(pScoreboard, IScoreboard::PLAYER1_5, false);
	annunciator_helper(pScoreboard, IScoreboard::PLAYER2_0, false);
	annunciator_helper(pScoreboard, IScoreboard::LIVES0, false);
	annunciator_helper(pScoreboard, IScoreboard::LIVES1, false);
	annunciator_helper(pScoreboard, IScoreboard::CREDITS1_0, false);

	pScoreboard->PreDeleteInstance();

	// now re-create with annunciator mode enabled
	pScoreboard = ScoreboardFactory::GetInstance(ScoreboardFactory::IMAGE, pLog, 0, false, true);

	annunciator_helper(pScoreboard, IScoreboard::PLAYER1_0, false);
	annunciator_helper(pScoreboard, IScoreboard::PLAYER1_1, false);
	annunciator_helper(pScoreboard, IScoreboard::PLAYER1_2, false);
	annunciator_helper(pScoreboard, IScoreboard::PLAYER1_3, true);
	annunciator_helper(pScoreboard, IScoreboard::PLAYER1_4, false);
	annunciator_helper(pScoreboard, IScoreboard::PLAYER1_5, true);
	annunciator_helper(pScoreboard, IScoreboard::PLAYER2_0, false);
	annunciator_helper(pScoreboard, IScoreboard::LIVES0, true);
	annunciator_helper(pScoreboard, IScoreboard::LIVES1, true);
	annunciator_helper(pScoreboard, IScoreboard::CREDITS1_0, false);
	annunciator_helper(pScoreboard, IScoreboard::CREDITS1_1, false);

	pLog->DeleteInstance();
}
