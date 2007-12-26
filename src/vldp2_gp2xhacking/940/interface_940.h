#ifndef VLDP_940_INTERFACE_H
#define VLDP_940_INTERFACE_H

// the size of the input buffers
#define INBUFSIZE 16384
#define INBUFCOUNT 2

// # of output buffers (must be a power of 2 because we use AND to do a fast MOD function!!!)
#define OUTBUFCOUNT 4

// this value is AND'd by buf count to do a fast MOD
#define OUTBUFMASK (OUTBUFCOUNT-1)

// The biggest buffer we expect to ever use (512x240 for Y surface)
// Increase this if you need more space.
#define OUTBUFSIZE (0x20000)

// the maximum size of the null-terminated log string returned by CPU2
#define LOG_STRING_SIZE 320

#ifndef NULL
#define NULL 0
#endif // NULL

// commands from CPU1 to CPU2
enum
{
	// no command is waiting
	C940_NO_COMMAND = 0,

	// tells program to shutdown (only really used when we're testing this code with threads)
	C940_QUIT,

	// No-Operation command, just used to PING cpu2 from cpu1 to see if it's there
	C940_NOP,

	// resets libmpeg2 to start decoding a new mpeg stream
	C940_LIBMPEG2_RESET
};

struct libmpeg2_shared_vars_s
{
	// where the YUV frame will be stored (read-only by cpu1, read/write by cpu2)
	// (put at the beginning to debug blit problem)
	unsigned char outBuf[OUTBUFCOUNT][3][OUTBUFSIZE];

	// VARIABLES READ/WRITE BY CPU1, READ-ONLY BY CPU2

	// the amount of bytes used in the input buffer (doesn't have to take up full size)
	unsigned int uInBufSize[INBUFCOUNT];

	// which input buffer CPU2 should use
	//unsigned int uReqInBuf;
	// UPDATE : uReqInBuf will always equal the (current buf & 1) so it will be implicitly 0 or 1.
	// For example, if uAckBufCount is 1, then the next buf cpu2 will use will be 0 because 2 & 1 is 0.

	// requested command
	unsigned int uReqCmd;

	// request counter (so cpu2 knows when cpu1 has sent a command)
	unsigned int uReqCount;

	// Used to notify CPU2 that we have a new buffer to decode.
	unsigned int uReqBufCount;

	// cpu1's frame counter (so cpu2 knows when cpu1 has received the frame)
	unsigned int uReqFrameCount;

	// cpu1's log counter (so cpu2 knows when cpu1 has received log message)
	unsigned int uReqLogCount;

	// if cpu1 wants is to return a YUY2 surface instead of a YV12 one
	unsigned int uReqYUY2;

	// cpu1 requests cpu2 to perform slowly to test cpu1's ability to wait for cpu2
	unsigned int uReqTestDelay;

	// whether one of the delay tests passed
	unsigned int uReqTestDelay1Passed;

	// diagnostics stuff... buf stalls may be good, no stalls may be bad
	unsigned int uReqBufStalls;
	unsigned int uReqBufNoStalls;

	/////////////////////////////////////////////////////////
	// VARIABLES READ/WRITE BY CPU2, READ-ONLY BY CPU1

	// acknowledgement counter (so cpu1 knows when cpu2 has received the command)
	unsigned int uAckCount;

	// to tell cpu1 when we've finished with a buffer so cpu1 can overwrite that buffer.
	unsigned int uAckBufCount;

	// the output resolution of the frame
	unsigned int uOutX;
	unsigned int uOutY;

	// the size of the Y, U, and V buffers (for convenience)
	unsigned int uOutYSize;
	unsigned int uOutUVSize;

	// which output buffer is 'live'
	unsigned int uAckOutBuf;

	// # of frames we've displayed (so CPU1 knows when a new frame is available)
	unsigned int uAckFrameCount;

	// log counter, so CPU1 knows when a new log string from CPU2 is available
	unsigned int uAckLogCount;

	// diagnostics stuff... stalls are bad, nostalls are good
	unsigned int uAckFrameStalls;
	unsigned int uAckFrameNoStalls;

	// set by CPU2 to indicate that it's at least running
	unsigned int uCPU2Running;

	// null-terminated log string
	char logStr[LOG_STRING_SIZE];

	///////////////////////
	// BUFFERS (put at the end so we don't have to make memset slower by clearing these out)

	// input buffers (read/write by cpu1, read-only by cpu2)
	unsigned char inBuf[INBUFCOUNT][INBUFSIZE];

	unsigned int uCorruptTest;
};

#endif // VLDP_940_INTERFACE_H

