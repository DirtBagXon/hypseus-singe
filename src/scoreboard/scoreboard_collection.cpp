#include "scoreboard_collection.h"

IScoreboard *ScoreboardCollection::GetInstance(ILogger *pLogger, SDL_Surface *(*pFuncGetActiveOverlay)(), bool bThayersQuest,
		bool bUsingAnnunciactor,
		unsigned int uWhichPort)
{
	ScoreboardCollection *pInstance = new ScoreboardCollection();
	if (pInstance)
	{
		pInstance->m_pLogger = pLogger;
		pInstance->m_pFuncGetActiveOverlay = pFuncGetActiveOverlay;
		pInstance->m_bThayersQuest = bThayersQuest;
		pInstance->m_bUsingAnnunciator = bUsingAnnunciactor;
		pInstance->m_uWhichPort = uWhichPort;
	}
	return pInstance;
}

bool ScoreboardCollection::AddType(IScoreboard *pInstanceSC, ScoreboardFactory::ScoreboardType type)
{
	bool bRes = false;
	ScoreboardCollection *pInstance = dynamic_cast<ScoreboardCollection *>(pInstanceSC);

	// if this instance is a scoreboard collection ...
	if (pInstance)
	{
		bRes = pInstance->AddType(type);
	}

	return bRes;
}

// This function isn't designed to be called directly.
// (Use the static version of this function instead)
bool ScoreboardCollection::AddType(ScoreboardFactory::ScoreboardType type)
{
	bool bRes = false;

	IScoreboard *pScoreboard = ScoreboardFactory::GetInstance(type, m_pLogger, m_pFuncGetActiveOverlay, m_bThayersQuest,
		m_bUsingAnnunciator, m_uWhichPort);

	if (pScoreboard)
	{
		m_lScoreboards.push_back(pScoreboard);
		bRes = true;
	}

	return bRes;
}

// this is used repeatedly
#define LIST_LOOP for (list<IScoreboard *>::iterator li = m_lScoreboards.begin(); li != m_lScoreboards.end(); ++li)

void ScoreboardCollection::DeleteInstance()
{
	LIST_LOOP
	{
		(*li)->PreDeleteInstance();
	}
	m_lScoreboards.clear();
	delete this;
}

void ScoreboardCollection::Invalidate()
{
	LIST_LOOP
	{
		(*li)->Invalidate();
	}
}

bool ScoreboardCollection::RepaintIfNeeded()
{
	bool bRes = false;

	LIST_LOOP
	{
		bRes = bRes || (*li)->RepaintIfNeeded();
	}
	return bRes;
}

bool ScoreboardCollection::ChangeVisibility(bool bVisible)
{
	bool bRes = false;

	LIST_LOOP
	{
		bRes = bRes || (*li)->ChangeVisibility(bVisible);
	}
	return bRes;
}

bool ScoreboardCollection::is_repaint_needed()
{
	bool bRes = false;

	LIST_LOOP
	{
		// if any of the collection needs repaint, then return true
		bRes = bRes || (*li)->is_repaint_needed();
	}
	return bRes;
}

bool ScoreboardCollection::pre_set_digit(unsigned int uValue, WhichDigit which)
{
	bool bRes = false;

	if (!m_lScoreboards.empty()) bRes = true;

	LIST_LOOP
	{
		bRes = bRes && (*li)->pre_set_digit(uValue, which);
	}
	return bRes;
}

bool ScoreboardCollection::pre_get_digit(unsigned int &uValue, WhichDigit which)
{
	bool bRes = false;

	if (!m_lScoreboards.empty()) bRes = true;

	LIST_LOOP
	{
		bRes = bRes && (*li)->pre_get_digit(uValue, which);
	}
	return bRes;
}

// this function should never get called because we've overriden the pre_* function
bool ScoreboardCollection::set_digit(unsigned int uValue, WhichDigit which)
{
	return false;
}

// this function should never get called because we've overriden the pre_* function
bool ScoreboardCollection::get_digit(unsigned int &uValue, WhichDigit which)
{
	return false;
}


ScoreboardCollection::ScoreboardCollection() :
m_pLogger(NULL),
m_pFuncGetActiveOverlay(NULL),
m_bThayersQuest(false),
m_bUsingAnnunciator(false),
m_uWhichPort(0)
{
}
