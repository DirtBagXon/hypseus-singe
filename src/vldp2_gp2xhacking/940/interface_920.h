
#ifdef __cplusplus
extern "C" {
#endif

struct yv12_s
{
	// pointers to the 3 layers of the surface
	// (Y will be (width*height) bytes big, U and V will be (width*height)/4 bytes big)
	void *Y;
	void *U;
	void *V;

	unsigned int uYSize;	// size of Y buffer in bytes
	unsigned int uUVSize;	// size of U and V buffers (in bytes)
};

struct yuy2_s
{
	// pointer to YUY2 buffer
	void *YUY2;

	// the size of the buffer (will always be width * height * 2)
	unsigned int uSize;
};

struct interface_920_callbacks_s
{
	// when an mpeg has begun decoding, announces the resolution of the mpeg before the first frame is sent
	void (*ResolutionCallback)(unsigned int uX, unsigned int uY);

	// when cpu2 has a new YV12 image decoded, it will be in here
	void (*YV12Callback)(const struct yv12_s *cpYV12);

	// when cpu2 has a new YUY2 image decoded, it will be in here
	void (*YUY2Callback)(const struct yuy2_s *cpYUY2);

	// when cpu2 has something to say ...
	void (*LogCallback)(const char *cpszMessage);
};

// returns absolute (not virtual) address of current yuy2 buffer that is waiting to be displayed.
// (this should only be called from the YUY2Callback function)
unsigned int mp2_920_get_yuy2_addr();

// Copies current YUY2 frame to the _absolute_ (not virtual) address indicated by uDestAddr
// (this should be called from the YUY2Callback function to ensure proper syncronization with the CPU2)
void mp2_920_start_fast_yuy2_blit(unsigned int uDestAddr);

// returns non-zero if blitter is busy or 0 if blitter is idle/ready
unsigned int mp2_920_is_blitter_busy();

unsigned int mp2_920_think();

unsigned int mp2_920_sleep_ms(unsigned int uMs);

void mp2_920_decode_mpeg(unsigned char *pBufStart, unsigned char *pBufEnd);

void mp2_920_reset();

void mp2_920_SetDelay(unsigned int uEnabled);

int mp2_920_init(const struct interface_920_callbacks_s *pCallbacks, unsigned int uUseYUY2);

void mp2_920_shutdown();

#ifdef __cplusplus
}
#endif // c++
