#ifndef SCOREBOARD_FACTORY_H
#define SCOREBOARD_FACTORY_H

#include "scoreboard_interface.h"
#include "../io/logger.h"
#include <SDL.h>	// for SDL_Surface

class HwScoreboard;

class ScoreboardFactory
{
public:
	typedef enum
	{
		NULLTYPE,		// null scoreboard (used for testing)
		IMAGE,	// graphics drawn on the screen (no VLDP)
		OVERLAY,	// overlay graphics drawn over VLDP video
		HARDWARE,	// hardware scoreboard controlled via parallel port
	} ScoreboardType;

	// call this to get a new scoreboard instance
	// A logger interface must be provided because some logging takes place (when using the parellel port for example)
	// 'pFuncGetActiveOverlay' is used by the overlay scoreboard and should point to g_game->get_active_video_overlay()).
	//		(if not using overlay scoreboard, this can be NULL)
	// 'bThayersQuest' should be set to true if the game using the scoreboard is Thayer's Quest.
	//		(if not using overlay scoreboard, this value is ignored)
	// 'bUsingAnnunciator' will cause digit values of 0xC to always be sent to the scoreboard even if the existing digit is a 0xC.
	//		This is only used with real hardware.
	// 'uWhichPort' is only used on hardware scoreboards and refers to which parallel port to plug into
	static IScoreboard *GetInstance(ScoreboardType type,
		ILogger *pLogger, SDL_Surface *(*pFuncGetActiveOverlay)() = 0, bool bThayersQuest = false,
		bool bUsingAnnunciactor = false,
		unsigned int uWhichPort = 0);
};

#endif // SCOREBOARD_FACTORY_H
