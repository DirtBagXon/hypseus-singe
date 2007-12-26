#include <stdio.h>
#include <string.h>	// for memcpy
#include <SDL.h>
#include <unistd.h>	// for usleep
#include <assert.h>
#include "interface_920.h"

unsigned int g_uFrameCount = 0;
unsigned int g_uTesting = 0;
SDL_Overlay *g_pOverlay = NULL;
SDL_Surface *g_pScreen = NULL;

#define USE_YUY2

#ifdef USE_YUY2
unsigned int g_uUseYUY2 = 1;
#else
unsigned int g_uUseYUY2 = 0;
#endif // USE_YUY2

void print_rgb(unsigned char Y, unsigned char U, unsigned char V)
{
    int B = (1.164*(Y - 16))                   + (2.018*(U - 128));
    int G = (1.164*(Y - 16)) - (0.813*(V - 128)) - (0.391*(U - 128));
    int R = (1.164*(Y - 16)) + (1.596*(V - 128));
	printf("R: %d, G: %d, B: %d\n", R, G, B);
}

void free_overlay()
{
	if (g_pOverlay)
	{
		SDL_FreeYUVOverlay(g_pOverlay);
		g_pOverlay = NULL;
	}
}

void ResolutionCallback(unsigned int X, unsigned int Y)
{
	printf("Resolution Callback : %u x %u\n", X, Y);

	free_overlay();	// just to be safe

#ifdef USE_YUY2
	g_pOverlay = SDL_CreateYUVOverlay(X, Y, SDL_YUY2_OVERLAY, g_pScreen);
#else
	g_pOverlay = SDL_CreateYUVOverlay(X, Y, SDL_YV12_OVERLAY, g_pScreen);
#endif // USE_YUY2

}

struct sdl_gp2x_yuvhwdata
{
	// the first few fields in this struct
	Uint8 *YUVBuf[2];
	unsigned int GP2XAddr[2];
	unsigned int uBackBuf;
};

void YUY2Callback(const struct yuy2_s *cpYUY2)
{
	unsigned char *YUY2 = cpYUY2->YUY2;

	assert(g_uUseYUY2 != 0);

	// if we're doing self testing ...
	if (g_uTesting)
	{
		printf("Received frame %u (%u %u %u) ", g_uFrameCount, YUY2[0], YUY2[1], YUY2[3]);
		print_rgb(YUY2[0], YUY2[1], YUY2[3]);

		switch (g_uFrameCount)
		{
		case 0:
			if (YUY2[0] != 235)
			{
				printf("*** Unexpected value for frame 0! (%u)\n", YUY2[0]);
			}
			printf("Stalling for a while to see if CPU2 stays with us...\n");
			usleep(1000 * 1000);
			break;
		case 1:
			if (YUY2[0] != 16)
			{
				printf("*** Unexpected value for frame 1! (%u)\n", YUY2[0]);
			}
			break;
		case 2:
			if (YUY2[0] != 81)
			{
				printf("*** Unexpected value for frame 2! (%u)\n", YUY2[0]);
			}
			break;
		}
	}

	// ie "if (g_uFrameCount % 16) == 0", but 'mod' is slow on the gp2x
	if (((g_uFrameCount >> 4) << 4) == g_uFrameCount)
	{
		printf("Got frame %u\n", g_uFrameCount);
	}

	g_uFrameCount++;

#if 1
	// display the frame
	if (SDL_LockYUVOverlay(g_pOverlay) == 0)
	{
#ifdef GP2X
		struct sdl_gp2x_yuvhwdata *pData = g_pOverlay->hwdata;
//		pData->GP2XAddr[pData->uBackBuf] = mp2_920_get_yuy2_addr();	// redirect sdl's buffer to the buffer that's already finished
		
		unsigned int uDstAddr = pData->GP2XAddr[pData->uBackBuf];
		unsigned char *ptr = (unsigned char *) g_pOverlay->pixels[0];

		mp2_920_start_fast_yuy2_blit(uDstAddr);	// do the blit
		
#else
		// NOTE : this won't work if we have an odd pitch, but ohwell :)
		memcpy(g_pOverlay->pixels[0], YUY2, cpYUY2->uSize);
#endif // GP2X
		SDL_UnlockYUVOverlay(g_pOverlay);

#ifdef GP2X
		
		// wait for the blitter to finish the copy
		while (mp2_920_is_blitter_busy())
		{
			printf("blitter is busy..\n");
			usleep(0);
		}

		if (g_uTesting && (((unsigned char *) g_pOverlay->pixels[0])[0] != YUY2[0]))
		{
			printf("*** ERROR BLIT FAILED! first 4 bytes written are %u %u %u %u\n",
				   ptr[0], ptr[1], ptr[2], ptr[3]);
		}

		
#endif // GP2X
		SDL_DisplayYUVOverlay(g_pOverlay, &g_pScreen->clip_rect);
		//usleep(250 * 1000);
	}
	else
	{
		fprintf(stderr, "SDL_LockYUVOverlay failed\n");
	}
#endif // GP2X

}

void YV12Callback(const struct yv12_s *cpYV12)
{
	unsigned char *Y = cpYV12->Y;
	unsigned char *U = cpYV12->U;
	unsigned char *V = cpYV12->V;

	assert(g_uUseYUY2 == 0);

	// if we're doing self testing ...
	if (g_uTesting)
	{
		printf("Received frame %u (%u %u %u) ", g_uFrameCount, Y[0], U[0], V[0]);
		print_rgb(Y[0], U[0], V[0]);

		switch (g_uFrameCount)
		{
		case 0:
			if (Y[0] != 235)
			{
				printf("*** Unexpected value for frame 0! (%u)\n", Y[0]);
			}
			printf("Stalling for a while to see if CPU2 stays with us...\n");
			usleep(1000 * 1000);
			break;
		case 1:
			if (Y[0] != 16)
			{
				printf("*** Unexpected value for frame 1! (%u)\n", Y[0]);
			}
			break;
		case 2:
			if (Y[0] != 81)
			{
				printf("*** Unexpected value for frame 2! (%u)\n", Y[0]);
			}
			break;
		}
	}

	// ie "if (g_uFrameCount % 16) == 0", but 'mod' is slow on the gp2x
	if (((g_uFrameCount >> 4) << 4) == g_uFrameCount)
	{
		printf("Got frame %u\n", g_uFrameCount);
	}

	g_uFrameCount++;

#ifndef GP2X
	// display the frame
	if (SDL_LockYUVOverlay(g_pOverlay) == 0)
	{
		memcpy(g_pOverlay->pixels[0], Y, cpYV12->uYSize);
		memcpy(g_pOverlay->pixels[2], U, cpYV12->uUVSize);
		memcpy(g_pOverlay->pixels[1], V, cpYV12->uUVSize);
		SDL_UnlockYUVOverlay(g_pOverlay);
		SDL_DisplayYUVOverlay(g_pOverlay, &g_pScreen->clip_rect);
	}
	else
	{
		fprintf(stderr, "SDL_LockYUVOverlay failed\n");
	}
#endif // GP2X

}

void LogCallback(const char *cpszMsg)
{
	printf("%s\n", cpszMsg);
}

#define BUFSIZE 16384
unsigned int g_uUseBufSize = BUFSIZE;	// this can be overriden for testing

void do_it(const char *filename)
{
	FILE *F = fopen(filename, "rb");
	if (F)
	{
		unsigned char buf[BUFSIZE];

		for (;;)
		{
			size_t uBytesRead = fread(buf, 1, g_uUseBufSize, F);

			mp2_920_decode_mpeg(buf, buf + uBytesRead);

			// we're done!
			if (uBytesRead < g_uUseBufSize)
			{
				break;
			}
		}
		fclose(F);

		if (g_uTesting)
		{
			printf("Mpeg file has been fully streamed, sleeping to make sure we get it all\n");
			mp2_920_sleep_ms(1000);
		}
	}
	else
	{
		printf("File %s could not be opened\n", filename);
	}
}

int main(int argc, char **argv)
{
	struct interface_920_callbacks_s callbacks;

	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		printf("SDL_Init failed: %s\n", SDL_GetError());
	}
	g_pScreen = SDL_SetVideoMode(320, 240, 0, SDL_HWSURFACE);

	if (!g_pScreen)
	{
		printf("SDL_SetVideoMode failed! : %s\n", SDL_GetError());
		return 1;
	}

	callbacks.LogCallback = LogCallback;
	callbacks.YV12Callback = YV12Callback;
	callbacks.YUY2Callback = YUY2Callback;
	callbacks.ResolutionCallback = ResolutionCallback;

	mp2_920_init(&callbacks, g_uUseYUY2);

	if (argc == 2)
	{
		unsigned int uStartTicks = SDL_GetTicks();
		unsigned int uEndTicks = 0;
		g_uFrameCount = 0;
		do_it(argv[1]);
		uEndTicks = SDL_GetTicks();

		// avoid divide by 0
		if (uEndTicks != uStartTicks)
		{
			printf("FPS: %u (%u frames in %u milliseconds)\n", (g_uFrameCount * 1000) / (uEndTicks - uStartTicks),
			   g_uFrameCount, (uEndTicks - uStartTicks));
		}
		else
		{
			printf("FPS : error (divide by 0 avoided)\n");
		}
	}
	else
	{
		g_uTesting = 1;
		g_uUseBufSize = 2000;	// something ridiculously small to ensure that we call decode multiple times

		printf("TEST: just basic decode\n");
		do_it("test320.m2v");

		printf("TEST: reset followed by basic decode\n");
		// test the wait and reset ability
		mp2_920_reset();
		g_uFrameCount = 0;

		mp2_920_SetDelay(1);		
		do_it("test320.m2v");
		if (mp2_920_GetDelayTest1Result())
		{
			printf("Delay Test Passed!\n");
		}
		else
		{
			printf("*** DELAY TEST FAILED ***\n");
		}
	}

	mp2_920_shutdown();

	// test to see if we can init and shutdown again (tests memory allocation)
	if (g_uTesting)
	{
		mp2_920_init(&callbacks, g_uUseYUY2);
		mp2_920_shutdown();
		mp2_920_init(&callbacks, g_uUseYUY2);
		mp2_920_shutdown();
		mp2_920_init(&callbacks, g_uUseYUY2);
		mp2_920_shutdown();
		mp2_920_init(&callbacks, g_uUseYUY2);
		g_uFrameCount = 0;
		do_it("test320.m2v");
		mp2_920_shutdown();

		// test to see if we can issue commands after being shut down
		//do_it("test320.m2v");
	}

	free_overlay();

#ifndef GP2X
	{
		void *result;
		printf("Testing mpo_memalign with a too-big number...");
		result = (void *) mpo_memalign(1, 10000 * 1024);
		if (result == 0)
		{
			printf("passed.\n");
		}
		else
		{
			printf("failed!\n");
		}
	}

	{
		unsigned char s[3] = { 1, 1, 1 };
		unsigned char t[3] = { 2, 2, 1 };
		mpo_memset(s, 2, 2);	// test memset...
		if (mpo_memcmp(s, t, 3) == memcmp(s, t, 3))
		{
			printf("1: mpo_memcmp and mpo_memset PASS\n");
		}
		else
		{
			printf("1: FAIL memset or memcmp\n");
		}
		if (mpo_memcmp(s, s, 3) == memcmp(s, s, 3))
		{
			printf("2: mpo_memcmp and mpo_memset PASS\n");
		}
		else
		{
			printf("2: FAIL memset or memcmp\n");
		}
	}
#endif // GP2X

	printf("Main returning\n");
	usleep(1000 * 1000);	// give time for thread to shutdown

	SDL_Quit();

	return 0;
}

