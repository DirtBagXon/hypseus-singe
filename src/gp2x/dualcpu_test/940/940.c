#include "940.h"

SharedVariables *g_pShared;

void do_flip()
{
	int i = 0;
	for (i = 0; i < BUFSIZE; i++)
	{
		g_pShared->outBuf[i] = g_pShared->inBuf[i] ^ 0xFF;
	}
}

void Main940()
{
	SharedVariables *pVars = (SharedVariables *) 0x200000;
	g_pShared = pVars;

	// go while we haven't been told to quit ...
	while(!g_pShared->g_bQuit)
	{
		// if a new command has come in ...
		if (g_pShared->g_uCmdCount != g_pShared->g_uAckCount)
		{
			switch (g_pShared->g_uReqCmd)
			{
			case CMD_FLIP_BUF:
				do_flip();
				g_pShared->g_uAckCount = g_pShared->g_uCmdCount;	// acknowledge command as being completed
				break;
			default:
				// unknown command, ignore ...
				break;
			}
		}
	}
}

