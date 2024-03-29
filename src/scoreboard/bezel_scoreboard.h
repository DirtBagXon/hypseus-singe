#ifndef BEZEL_SCOREBOARD_H
#define BEZEL_SCOREBOARD_H

#include "scoreboard_interface.h"

// A scoreboard for bezels
class BezelScoreboard : public IScoreboard
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

private:
	BezelScoreboard();

	static IScoreboard *GetInstance();

};

#endif // BEZEL_SCOREBOARD_H
