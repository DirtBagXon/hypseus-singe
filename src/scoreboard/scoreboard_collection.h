#ifndef SCOREBOARD_COLLECTION_H
#define SCOREBOARD_COLLECTION_H

#include "scoreboard_factory.h"
#include <list>

using namespace std;

class ScoreboardCollection : public IScoreboard
{
public:
	static IScoreboard *GetInstance(ILogger *pLogger, SDL_Surface *(*pFuncGetActiveOverlay)() = 0, bool bThayersQuest = false,
		bool bUsingAnnunciactor = false,
		unsigned int uWhichPort = 0);

	static bool AddType(IScoreboard *pInstance, ScoreboardFactory::ScoreboardType type);

	void DeleteInstance();

	void Invalidate();

	bool RepaintIfNeeded();

	bool ChangeVisibility(bool bVisible);

	bool is_repaint_needed();

	bool pre_set_digit(unsigned int uValue, WhichDigit which);

	bool pre_get_digit(unsigned int &uValue, WhichDigit which);

protected:
	bool set_digit(unsigned int uValue, WhichDigit which);

	bool get_digit(unsigned int &uValue, WhichDigit which);

private:
	ScoreboardCollection();

	bool AddType(ScoreboardFactory::ScoreboardType type);
	
	list <IScoreboard *> m_lScoreboards;

	ILogger *m_pLogger;

	SDL_Surface *(*m_pFuncGetActiveOverlay)();

	bool m_bThayersQuest;

	bool m_bUsingAnnunciator;

	unsigned int m_uWhichPort;
};

#endif // SCOREBOARD_COLLECTION_H
