#define DZZTRUE 1
#define DZZFALSE 0

#define BUFSIZE 4096

// arbitrary commands
enum
{
	CMD_FLIP_BUF = 66
};

typedef struct SharedVariables
{
	// CPU #1 WRITABLE, CPU #2 READ-ONLY
	int g_bQuit; // 1 = quit is requested, 0 = no quit requested
	unsigned int g_uReqCmd;	// the last requested command
	unsigned int g_uCmdCount;	// the command counter
	unsigned char inBuf[BUFSIZE];

	// CPU #2 WRITABLE, CPU #1 READ-ONLY
	unsigned int g_uAckCount;	// the last acknowledged command
	unsigned char outBuf[BUFSIZE];

} SharedVariables;
