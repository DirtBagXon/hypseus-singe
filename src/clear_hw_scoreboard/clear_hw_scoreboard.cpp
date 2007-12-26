
#include "../io/logger.h"
#include "../scoreboard/hw_scoreboard.h"
#include "../io/parallel.h"
#include <windows.h>

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	// by creating and deleting an instance of the HW scoreboard, this will clear the LEDs
	ILogger *pLogger = NullLogger::GetInstance();
	IScoreboard *pScoreboard = HwScoreboard::GetInstance(0, pLogger);
	if (pScoreboard)
	{
		pScoreboard->PreDeleteInstance();	// cleanup
	}
	else
	{
		MessageBox(0, "Scoreboard could not be cleared! Is par-io.dll in the path?", "Error", 0);
	}

	if (pLogger)
	{
		pLogger->DeleteInstance();	// cleanup
	}
	return 0;
}
