#ifndef NULL_SCOREBOARD_H
#define NULL_SCOREBOARD_H

#include "scoreboard_interface.h"

// fake scoreboard, used for testing
class NullScoreboard : public IScoreboard
{
	friend class ScoreboardFactory;
public:

	void DeleteInstance();

	void Invalidate();

	bool RepaintIfNeeded();

	bool ChangeVisibility(bool bVisible);

	bool set_digit(unsigned int uValue, WhichDigit which);

	bool is_repaint_needed();

	bool get_digit(unsigned int &uValue, WhichDigit which);

	// public so we can test without including other scoreboard instances
	static IScoreboard *GetInstance();

private:
	NullScoreboard();
};

#endif // NULL_SCOREBOARD_H
