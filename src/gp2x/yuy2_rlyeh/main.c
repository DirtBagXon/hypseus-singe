#include <stdio.h>
#include "minimal.h"

const unsigned int uWIDTH = 320;
const unsigned int uHEIGHT = 240;

typedef unsigned char Uint8;
typedef unsigned int Uint32;

void do_it()
{
	unsigned int uRow = 0, uCol = 0;
	unsigned int uOdd = 0;
	unsigned int uCount = 0;

	// go until user wants us to quit ...
	while (uCount++ < 20)
	{
		Uint8 *ptrDst = (Uint8 *) gp2x_video_YUV[0].screen32;
		unsigned int uPitch = uWIDTH * 2;
		
			for (uRow = 0; uRow < uHEIGHT; uRow++)
			{
				unsigned int uVal = 0x7F007F00, uVal2 = 0x7FFF7FFF;
				Uint32 *ptrLine = (Uint32 *) ptrDst;
	
				// if we're supposed to try a dotted line
				if ((uRow % 2) == uOdd)
				{
					uVal = 0x7FFF7FFF;
					uVal2 = 0x7F007F00;
				}
	
				// 4 pixels at a time
				for (uCol = 0; uCol < uWIDTH; uCol += 4)
				{
					*ptrLine = uVal;
					++ptrLine;
					*ptrLine = uVal2;
					++ptrLine;
				}
	
				ptrDst += uPitch;
			}

			usleep(250 * 1000);
			// wait until vsync before flipping
		gp2x_video_waitvsync();
		gp2x_video_YUV_flip(0);

		uOdd = uOdd ^ 1;
	}
}

void do_it_image()
{
	FILE *F = fopen("surface.YUY2", "rb");
	if (F)
	{
		Uint8 *ptrDst = (Uint8 *) gp2x_video_YUV[0].screen32;
		unsigned int uPitch = uWIDTH * 2;
		unsigned int uRow = 0;

		for (uRow = 0; uRow < uHEIGHT; uRow++)
		{
			fread(ptrDst, 1, uWIDTH * 2, F);
			ptrDst += uPitch;
		}

		// wait until vsync before flipping
		gp2x_video_waitvsync();
		gp2x_video_YUV_flip(0);
	}
	else
	{
		printf("Could not open file\n");
	}

	fclose(F);

	sleep(3);
}

int main(int argc, char **argv)
{
	gp2x_init(1000,
		16,	// bpp
		44100,	// sound freq
		16,	// 16-bit sound
		1,	// stereo
		60,	// unused (must not be zero!)
		1	// solid font?
		);
	gp2x_video_YUV_setparts(0,-1,-1,-1,319,239);
	gp2x_video_RGB_setwindows(0x10,0x10,0x10,0x10,319,239);
	do_it();
	do_it_image();
	return 0;
}

void gp2x_sound_frame(void *blah, void *buff, int samples) {}

