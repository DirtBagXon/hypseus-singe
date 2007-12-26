#ifndef TEST_SB_H
#define TEST_SB_H

#include "game.h"
#include "../scoreboard/scoreboard_factory.h"

class test_sb : public game
{
public:
	test_sb();
	void start();
};

#endif // TEST_SB_H
