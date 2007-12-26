#include <stdio.h>
#include <SDL.h>
#include <string.h>	// for memcpy
#include <stdlib.h>	// for malloc

const unsigned int uWIDTH = 320;
const unsigned int uHEIGHT = 240;

void do_display(SDL_Overlay *pOverlay, SDL_Rect *pRect, Uint8 *pBuf, unsigned int uBufSize)
{
	if (SDL_LockYUVOverlay(pOverlay) == 0)
	{
		Uint8 *ptrDst = (Uint8 *) pOverlay->pixels[0];
		unsigned int uPitch = pOverlay->pitches[0];
		unsigned int uRow = 0;
		unsigned int uRowSize = pOverlay->w << 1;

		for (uRow = 0; uRow < pOverlay->h; uRow++)
		{
			memcpy(ptrDst, pBuf, uRowSize);
			ptrDst += uPitch;
			pBuf += uRowSize;
		}

		SDL_UnlockYUVOverlay(pOverlay);
		SDL_DisplayYUVOverlay(pOverlay, pRect);
	}
	else
	{
		printf("Lock failed\n");
	}
}

void animate(SDL_Overlay *pOverlay, SDL_Rect *pRect, void *pBuf, unsigned int uSize)
{
	SDL_Rect rect = *pRect;
	int i = 0;

	// shrink
	do
	{
		// display
		do_display(pOverlay, &rect, pBuf, uSize);
		rect.x++;
		rect.y++;
		rect.w -= 2;
		rect.h -= 2;
		i++;
	} while (i < 25);

	sleep(1);	// let user look at this for a second ...

	// restore rectangle ...
	rect = *pRect;

	// let them see regular size
	do_display(pOverlay, &rect, pBuf, uSize);

	sleep(1);

	i = 0;
	// shrink
	do
	{
		// display
		do_display(pOverlay, &rect, pBuf, uSize);
		rect.w += 2;
		rect.h += 2;
		i++;
	} while (i < 25);

	sleep(1);
}

void prep(const char *cpszFileName, SDL_Overlay *pOverlay, SDL_Rect *rect)
{
	unsigned int uBufSize = pOverlay->w * pOverlay->h * 2;
	void *pBuf = malloc(uBufSize);
	FILE *F = fopen(cpszFileName, "rb");
	if (F)
	{
		fread(pBuf, 1, uBufSize, F);
		fclose(F);
		printf("%u hw accel is %d\n", pOverlay->w, pOverlay->hw_overlay);
		animate(pOverlay, rect, pBuf, uBufSize);
	}
	else
	{
		printf("ERROR : File %s could not be opened\n", cpszFileName);
	}
	free(pBuf);
}

void do_it_image320(SDL_Surface *pScreen)
{
	SDL_Overlay *pOverlay = SDL_CreateYUVOverlay(320, 240, SDL_YUY2_OVERLAY, pScreen);
	if (pOverlay)
	{
		prep("surface320x240.YUY2", pOverlay, &pScreen->clip_rect);
		SDL_FreeYUVOverlay(pOverlay);
	}
	else
	{
		printf("320: SDL_CreateYUVOverlay failed\n");
	}	
}

void do_it_image512(SDL_Surface *pScreen)
{
	SDL_Overlay *pOverlay = SDL_CreateYUVOverlay(512, 480, SDL_YUY2_OVERLAY, pScreen);
	if (pOverlay)
	{
		prep("surface512x480.YUY2", pOverlay, &pScreen->clip_rect);
		SDL_FreeYUVOverlay(pOverlay);
	}
	else
	{
		printf("512: SDL_CreateYUVOverlay failed\n");
	}	
}

int main(int argc, char **argv)
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Surface *pScreen = SDL_SetVideoMode(uWIDTH, uHEIGHT, 0, SDL_HWSURFACE);
	if (pScreen)
	{
		do_it_image320(pScreen);
		do_it_image512(pScreen);
	}
	SDL_Quit();
	
	return 0;
}

