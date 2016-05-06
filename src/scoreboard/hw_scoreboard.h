#ifndef HW_SCOREBOARD_H
#define HW_SCOREBOARD_H

#include "scoreboard_interface.h"

class ScoreboardFactory;

class HwScoreboard : public IScoreboard
{
	// allow ScoreboardFactory to call our GetInstance
	friend class ScoreboardFactory;

public:
	// I made GetInstance public so my 'clear_hw_scoreboard' program could access this
	//  directly without linking against the other scoreboard types.
	static IScoreboard *GetInstance(unsigned int uParallelPort);

	void DeleteInstance();

	void Invalidate();

	bool RepaintIfNeeded();

	bool ChangeVisibility(bool bVisible);

	bool set_digit(unsigned int uValue, WhichDigit which);

	bool is_repaint_needed();

	bool get_digit(unsigned int &uValue, WhichDigit which);

protected:
private:
	// initialize the parallel port
	bool ParInit();

	// shutdown the parallel port
	void ParShutdown();

	HwScoreboard(unsigned int uParallelPort);
	virtual ~HwScoreboard();

	unsigned int m_uParallelPort;
};

#endif // HW_SCOREBOARD_H
