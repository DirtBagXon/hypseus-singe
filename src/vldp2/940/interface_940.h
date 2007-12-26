#ifndef VLDP_940_INTERFACE_H
#define VLDP_940_INTERFACE_H

// the size of the input buffers
#define INBUFSIZE 4096
#define INBUFCOUNT 2

// libmpeg2 needs 3 output buffers!
#define OUTBUFCOUNT 3

// The biggest buffer we expect to ever use (Y surface, 720x480 resolution, 8 bits per pixel)
// Increase this if you need more space.
#define OUTBUFSIZE (720*480)

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

	// decodes from inBuf1
	C940_LIBMPEG2_DECODE,

	// resets libmpeg2 to start decoding a new mpeg stream
	C940_LIBMPEG2_RESET
};

// status of CPU2 for CPU1
enum
{
	S940_BUSY = 200,
	S940_READY,
	S940_ERR
};

struct libmpeg2_shared_vars_s
{
	// VARIABLES READ/WRITE BY CPU1, READ-ONLY BY CPU2

	// input buffers
	unsigned char inBuf[INBUFCOUNT][INBUFSIZE];

	// the amount of bytes used in the input buffer (doesn't have to take up full size)
	unsigned int uInBufSize[INBUFCOUNT];

	// which input buffer CPU2 should use
	unsigned int uReqInBuf;

	// requested command
	unsigned int uReqCmd;

	// request counter (so cpu2 knows when cpu1 has sent a command)
	unsigned int uReqCount;

	// counter to tell whether cpu2 is keeping in sync with us so we don't fill the buffers too quickly
	unsigned int uReqBufCount;

	// cpu1's frame counter (so cpu2 knows when cpu1 has received the frame)
	unsigned int uReqFrameCount;

	// cpu1's log counter (so cpu2 knows when cpu1 has received log message)
	unsigned int uReqLogCount;

	// VARIABLES READ/WRITE BY CPU2, READ-ONLY BY CPU1

	// acknowledgement counter (so cpu1 knows when cpu2 has received the command)
	unsigned int uAckCount;

	// to tell cpu1 that we've finished processing the inbuf
	unsigned int uAckBufCount;

	// the output resolution of the frame
	unsigned int uOutX;
	unsigned int uOutY;

	// which output buffer is 'live'
	unsigned int uAckOutBuf;

	// # of frames we've displayed (so CPU1 knows when a new frame is available)
	unsigned int uAckFrameCount;

	// log counter, so CPU1 knows when a new log string from CPU2 is available
	unsigned int uAckLogCount;

	// null-terminated log string
	char logStr[LOG_STRING_SIZE];

	// where the YUV frame will be stored
	unsigned char outBuf[OUTBUFCOUNT][3][OUTBUFSIZE];

	unsigned int uCorruptTest;
};

#endif // VLDP_940_INTERFACE_H

