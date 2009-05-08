/*
 * video.cpp
 *
 * Copyright (C) 2001 Matt Ownby
 *
 * This file is part of DAPHNE, a laserdisc arcade game emulator
 *
 * DAPHNE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * DAPHNE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// video.cpp
// Part of the DAPHNE emulator
// This code started by Matt Ownby, May 2000

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>	// for some error messages
#include "video.h"
#include "palette.h"
#include "SDL_DrawText.h"
#include "../io/conout.h"
#include "../io/error.h"
#include "../io/mpo_fileio.h"
#include "../io/mpo_mem.h"
#include "SDL_Console.h"
#include "../game/game.h"
#include "../ldp-out/ldp.h"
#include "../ldp-out/ldp-vldp-gl.h"

#ifdef USE_OPENGL
#ifdef MAC_OSX
#include <glew.h>
#else
#include <GL/glew.h>
#endif


// This is the max width and height that any .BMP will be, so that we can fit it into
//  an opengl texture.  If you need to display a bigger .BMP, increase this number.
// NOTE : this number must be a power of 2!!!
#define GL_TEX_SIZE 1024

// pointer to the buffer that we allocate for bitmap blits
Uint8 *g_pVidTex = NULL;

#endif

using namespace std;

#ifndef GP2X
unsigned int g_vid_width = 640, g_vid_height = 480;	// default video width and video height
#ifdef DEBUG
const Uint16 cg_normalwidths[] = { 320, 640, 800, 1024, 1280, 1280, 1600 };
const Uint16 cg_normalheights[]= { 240, 480, 600, 768, 960, 1024, 1200 };
#else
const Uint16 cg_normalwidths[] = { 640, 800, 1024, 1280, 1280, 1600 };
const Uint16 cg_normalheights[]= { 480, 600, 768, 960, 1024, 1200 };
#endif // DEBUG
#else
unsigned int g_vid_width = 320, g_vid_height = 240;	// default for gp2x
const Uint16 cg_normalwidths[] = { 320 };
const Uint16 cg_normalheights[]= { 240 };
#endif

// the dimensions that we draw (may differ from g_vid_width/height if aspect ratio is enforced)
unsigned int g_draw_width = 640, g_draw_height = 480;

SDL_Surface *g_led_bmps[LED_RANGE] = { 0 };
SDL_Surface *g_other_bmps[B_EMPTY] = { 0 };
SDL_Surface *g_screen = NULL;	// our primary display
SDL_Surface *g_screen_blitter = NULL;	// the surface we blit to (we don't blit directly to g_screen because opengl doesn't like that)
bool g_console_initialized = false;	// 1 once console is initialized
bool g_fullscreen = false;	// whether we should initialize video in fullscreen mode or not
int sboverlay_characterset = 1;

// whether we will try to force a 4:3 aspect ratio regardless of window size
// (this is probably a good idea as a default option)
bool g_bForceAspectRatio = true;

bool g_bUseOpenGL = false;	// whether user has requested we use OpenGL

// the # of degrees to rotate counter-clockwise in opengl mode
float g_fRotateDegrees = 0.0;

#ifdef USE_OPENGL
GLuint g_texture_id = 0;	// for any blits we have to do in opengl ...
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

// initializes the window in which we will draw our BMP's
// returns true if successful, false if failure
bool init_display()
{

	bool result = false;	// whether video initialization is successful or not
	bool abnormalscreensize = true; // assume abnormal
	const SDL_VideoInfo *vidinfo = NULL;
	Uint8 suggested_bpp = 0;
	Uint32 sdl_flags = 0;	// the default for this depends on whether we are using HW accelerated YUV overlays or not

	char *hw_env = getenv("SDL_VIDEO_YUV_HWACCEL");

	// if HW acceleration has been disabled, we need to use a SW surface due to some oddities with crashing and fullscreen
	if (hw_env && (hw_env[0] == '0'))
	{
		sdl_flags = SDL_SWSURFACE;
	}

	// else if HW acceleration hasn't been explicitely disabled ...
	else
	{

		sdl_flags = SDL_HWSURFACE;
		// Win32 notes (may not apply to linux) :
		// After digging around in the SDL source code, I've discovered a few things ...
		// When using fullscreen mode, you should always use SDL_HWSURFACE because otherwise
		// you can't use YUV hardware overlays due to SDL creating an extra surface without
		// your consent (which seems retarded to me hehe).
		// SDL_SWSURFACE + hw accelerated YUV overlays will work in windowed mode, but might
		// as well just use SDL_HWSURFACE for everything.
	}

	char s[250] = { 0 };
	Uint32 x = 0;	// temporary index

	// if we were able to initialize the video properly
	if ( SDL_InitSubSystem(SDL_INIT_VIDEO) >=0 )
	{

		vidinfo = SDL_GetVideoInfo();
		suggested_bpp = vidinfo->vfmt->BitsPerPixel;

		// if we were in 8-bit mode, try to get at least 16-bit color
		// because we definitely use more than 256 colors in daphne
		if (suggested_bpp < 16)
		{
			suggested_bpp = 32;
		}

		if (g_fullscreen)
		{
			SDL_ShowCursor(SDL_DISABLE);	// hide mouse in fullscreen mode
			sdl_flags |= SDL_FULLSCREEN;
		}

		// go through each standard resolution size to see if we are using a standard resolution
		for (x=0; x < (sizeof(cg_normalwidths) / sizeof(Uint16)); x++)
		{
			// if we find a match, we know we're using a standard res
			if ((g_vid_width == cg_normalwidths[x]) && (g_vid_height == cg_normalheights[x]))
			{
				abnormalscreensize = false;
			}
		}

		// if we're using a non-standard resolution
		if (abnormalscreensize)
		{
			printline("WARNING : You have specified an abnormal screen resolution! Normal screen resolutions are:");
			// print each standard resolution
			for (x=0; x < (sizeof(cg_normalwidths) / sizeof(Uint16)); x++)
			{	
				sprintf(s,"%d x %d", cg_normalwidths[x], cg_normalheights[x]);
				printline(s);
			}
			newline();
		}

		g_draw_width = g_vid_width;
		g_draw_height = g_vid_height;

		// if we're supposed to enforce the aspect ratio ...
		if (g_bForceAspectRatio)
		{
			double dCurAspectRatio = (double) g_vid_width / g_vid_height;
			const double dTARGET_ASPECT_RATIO = 4.0/3.0;

			// if current aspect ratio is less than 1.3333
			if (dCurAspectRatio < dTARGET_ASPECT_RATIO)
			{
				g_draw_height = (g_draw_width * 3) / 4;
			}
			// else if current aspect ratio is greater than 1.3333
			else if (dCurAspectRatio > dTARGET_ASPECT_RATIO)
			{
				g_draw_width = (g_draw_height * 4) / 3;
			}
			// else the aspect ratio is already correct, so don't change anything
		}

#ifndef GP2X
		if (!g_bUseOpenGL)
		{
			g_screen = SDL_SetVideoMode(g_vid_width, g_vid_height, suggested_bpp, sdl_flags);
			SDL_WM_SetCaption("DAPHNE: First Ever Multiple Arcade Laserdisc Emulator =]", "daphne");
		}
		else
		{
#ifdef USE_OPENGL
			init_opengl();
			glGenTextures(1, &g_texture_id);	// generate texture buffer for use in this file
			g_pVidTex = MPO_MALLOC(GL_TEX_SIZE * GL_TEX_SIZE * 4);	// 32-bit bits per pixel, width and height the same
#endif
		}
#else
		SDL_ShowCursor(SDL_DISABLE);	// always hide mouse for gp2x
		g_screen = SDL_SetVideoMode(320, 240, 0, SDL_HWSURFACE);
#endif

		// create a 32-bit surface
		g_screen_blitter = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                        g_vid_width,
                                        g_vid_height,
										32,
										0xff, 0xFF00, 0xFF0000, 0xFF000000);

		if (g_screen && g_screen_blitter)
		{
			sprintf(s, "Set %dx%d at %d bpp with flags: %x", g_screen->w, g_screen->h, g_screen->format->BitsPerPixel, g_screen->flags);
			printline(s);

			// initialize SDL console in the background
			if (ConsoleInit("pics/ConsoleFont.bmp", g_screen_blitter, 100)==0)
			{
				AddCommand(g_cpu_break, "break");
				g_console_initialized = true;
				result = true;
			}
			else
			{
				printerror("Error initializing SDL console =(");			
			}

			// sometimes the screen initializes to white, so this attempts to prevent that
			vid_blank();
			vid_flip();
			vid_blank();
			vid_flip();
		}
	}

	if (result == 0)
	{
		sprintf(s, "Could not initialize video display: %s", SDL_GetError());
		printerror(s);
	}

	return(result);

}

#ifdef USE_OPENGL
bool init_opengl()
{
	bool bResult = false;

	Uint32 sdl_flags = SDL_SWSURFACE | SDL_OPENGL;

	// fullscreen is broken
	if (get_fullscreen()) sdl_flags |= SDL_FULLSCREEN;

	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	g_screen = SDL_SetVideoMode(g_vid_width, g_vid_height, 0, sdl_flags);

	// if SDL_SetVideoMode worked ...
	if (g_screen)
	{
		SDL_WM_SetCaption("DAPHNE: Now in experimental OpenGL mode :)", "daphne");

		//glShadeModel(GL_SMOOTH);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_NORMALIZE);	// since we are doing scaling, we need this ...
		glFrontFace(GL_CCW);
		glCullFace(GL_BACK);
		glEnable(GL_CULL_FACE);	// this should give us some optimization for free	
		glDisable(GL_LIGHTING);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		/* Set the clear color. */
		glClearColor( 0, 0.0, 0, 1 );

		/* Setup our viewport. */
		glViewport( 0, 0, g_screen->w, g_screen->h );

		// ENABLE 2D MODE (ORTHO)
		glMatrixMode (GL_PROJECTION);
		glLoadIdentity();

		// The idea is to have the center of the image be at 0,0 to make rotation, scaling, etc
		//  easier.
		glOrtho(
			   -g_screen->w / 2,   // left
			   g_screen->w / 2,	   // right
			   -g_screen->h / 2,   // bottom
			   g_screen->h / 2,	   // top
			   -10, 10);

		glMatrixMode (GL_MODELVIEW);

		glEnable(GL_TEXTURE_2D);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		// handle optional rotation
		// if we're to one side or the other, scale the image so it will fit
		if ((g_fRotateDegrees == 90.0) || (g_fRotateDegrees == 270.0))
		{
			// NOTE : this can either shrink or enlarge depending on whether the width or height is bigger
			GLfloat fFactor = (GLfloat) g_vid_height / g_vid_width;
			glScalef(fFactor, fFactor, 1.0);
		}

		// only rotate if we have a need to
		if (g_fRotateDegrees != 0)
		{
			glRotatef(g_fRotateDegrees, 0, 0, 1.0);
		}

		bResult = true;
	}
	// else SetVideoMode failed ...

	return bResult;
}
#endif // USE_OPENGL

// shuts down video display
void shutdown_display()
{
	printline("Shutting down video display...");

	if (g_console_initialized)
	{
		ConsoleShutdown();
		g_console_initialized = false;
	}

#ifdef USE_OPENGL
	// free texture buffer from memory
	MPO_FREE(g_pVidTex);
#endif

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void vid_flip()
{
	// if we're not using OpenGL, then just use the regular SDL Flip ...
	if (!g_bUseOpenGL)
	{
		SDL_Flip(g_screen);
	}
	else
	{
		SDL_GL_SwapBuffers();
	}
}

void vid_blank()
{
	if (!g_bUseOpenGL)
	{
		SDL_FillRect(g_screen, NULL, 0);
	}
	else
	{
#ifdef USE_OPENGL
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
	}
}

#ifdef USE_OPENGL
// converts an SDL surface to an opengl texture
void vid_srf2tex(SDL_Surface *srf, GLuint uTexID)
{
	Uint8 *ptrPixelRow = (Uint8 *) srf->pixels;
	Uint32 *RGBARow = (Uint32 *) g_pVidTex;
	Uint32 *g_puRGBAPalette = get_rgba_palette();
	for (unsigned int uRow = 0; (uRow < (unsigned) srf->h) && (uRow < GL_TEX_SIZE); ++uRow)
	{
		Uint8 *ptrPixel = ptrPixelRow;
		Uint32 *RGBA = RGBARow;

		for (unsigned int uCol = 0; (uCol < (unsigned) srf->w) && (uCol < GL_TEX_SIZE); ++uCol)
		{
			Uint8 R, G, B;

			// if this is an 8-bit surface
			if (srf->format->BitsPerPixel == 8)
			{
				// retrieve precalculated RGBA value
				*RGBA = g_puRGBAPalette[*ptrPixel];
			}
			// else it doesn't use a color palette
			else
			{
				Uint32 uVal = *((Uint32*) ptrPixel);
				R = ((uVal & srf->format->Rmask) >> srf->format->Rshift) << srf->format->Rloss;
				G = ((uVal & srf->format->Gmask) >> srf->format->Gshift) << srf->format->Gloss;
				B = ((uVal & srf->format->Bmask) >> srf->format->Bshift) << srf->format->Bloss;
				// this may not work for big endian, but we won't know until we try
				*RGBA = R | (G << 8) | (B << 16) | 0xFF000000;
			}

			// move to the next pixel ...
			ptrPixel += srf->format->BytesPerPixel;
			++RGBA;
		} // end this line

		// move to the next row
		ptrPixelRow += srf->pitch;
		RGBARow += GL_TEX_SIZE;	// move down one row in the texture memory
	}

	glBindTexture(GL_TEXTURE_2D, uTexID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	// apply the texture
	glTexImage2D(GL_TEXTURE_2D,
		0,	// level is 0
		GL_RGBA, GL_TEX_SIZE, GL_TEX_SIZE,
		0,	// border is 0
		GL_RGBA, GL_UNSIGNED_BYTE, g_pVidTex);

}
#endif

void vid_blit(SDL_Surface *srf, int x, int y)
{
	if (g_ldp->is_blitting_allowed())
	{
		if (!g_bUseOpenGL)
		{
			SDL_Rect dest;
			dest.x = (short) x;
			dest.y = (short) y;
			dest.w = (unsigned short) srf->w;
			dest.h = (unsigned short) srf->h;
			SDL_BlitSurface(srf, NULL, g_screen, &dest);
		}

		// else, OpenGL mode for blitting ...
		else
		{
#ifdef USE_OPENGL

			// convert surface to a texture
			vid_srf2tex(srf, g_texture_id);

			// draw the textured rectangle
			GLfloat fWidth = (GLfloat) srf->w / GL_TEX_SIZE;
			GLfloat fHeight = (GLfloat) srf->h / GL_TEX_SIZE;

			// convert y from top-to-bottom to bottom-to-top (SDL -> openGL)
			y = g_vid_height - y;

			// adjust coordinates so they match up with glOrtho projection
			x -= (g_vid_width >> 1);
			y -= (g_vid_height >> 1);

			glBegin(GL_QUADS);
			glTexCoord2f(0, 0); glVertex3i(x, y, 0); // top left
			glTexCoord2f(0, fHeight); glVertex3i(x, y - srf->h, 0); // bottom left
			glTexCoord2f(fWidth, fHeight); glVertex3i(x + srf->w, y - srf->h, 0); // bottom right
			glTexCoord2f(fWidth, 0); glVertex3i(x + srf->w, y, 0); // top right
			glEnd();

#endif // USE_OPENGL
		}
	}
	// else blitting isn't allowed, so just ignore
}

// redraws the proper display (Scoreboard, etc) on the screen, after first clearing the screen
// call this every time you want the display to return to normal
void display_repaint()
{
	vid_blank();
	vid_flip();
	g_game->video_force_blit();
}

// loads all the .bmp's used by DAPHNE
// returns true if they were all successfully loaded, or a false if they weren't
bool load_bmps()
{

	bool result = true;	// assume success unless we hear otherwise
	int index = 0;
	char filename[81];

	for (; index < LED_RANGE; index++)
	{
		sprintf(filename, "pics/led%d.bmp", index);

		g_led_bmps[index] = load_one_bmp(filename);

		// If the bit map did not successfully load
		if (g_led_bmps[index] == 0)
		{
			result = false;
		}
	}

	g_other_bmps[B_DL_PLAYER1] = load_one_bmp("pics/player1.bmp");
	g_other_bmps[B_DL_PLAYER2] = load_one_bmp("pics/player2.bmp");
	g_other_bmps[B_DL_LIVES] = load_one_bmp("pics/lives.bmp");
	g_other_bmps[B_DL_CREDITS] = load_one_bmp("pics/credits.bmp");
	g_other_bmps[B_DAPHNE_SAVEME] = load_one_bmp("pics/saveme.bmp");
	g_other_bmps[B_GAMENOWOOK] = load_one_bmp("pics/gamenowook.bmp");

    if (sboverlay_characterset != 2)
		g_other_bmps[B_OVERLAY_LEDS] = load_one_bmp("pics/overlayleds1.bmp");
	else
        g_other_bmps[B_OVERLAY_LEDS] = load_one_bmp("pics/overlayleds2.bmp");

	g_other_bmps[B_OVERLAY_LDP1450] = load_one_bmp("pics/ldp1450font.bmp");

	// check to make sure they all loaded
	for (index = 0; index < B_EMPTY; index++)
	{
		if (g_other_bmps[index] == NULL)
		{
			result = false;
		}
	}

	return(result);
}


SDL_Surface *load_one_bmp(const char *filename)
{
	SDL_Surface *result = SDL_LoadBMP(filename);

	if (!result)
	{
		string err = "Could not load bitmap : ";
		err = err + filename;
		printerror(err.c_str());
	}

	return(result);
}

// Draw's one of our LED's to the screen
// value contains the bitmap to draw (0-9 is valid)
// x and y contain the coordinates on the screen
// This function is called from scoreboard.cpp
// 1 is returned on success, 0 on failure
bool draw_led(int value, int x, int y)
{
	vid_blit(g_led_bmps[value], x, y);
	return true;
}

// Draw overlay digits to the screen
void draw_overlay_leds(unsigned int values[], int num_digits, int start_x, int y, SDL_Surface *overlay)
{
	SDL_Rect src, dest;

	dest.x = start_x;
	dest.y = y;
	dest.w = OVERLAY_LED_WIDTH;
	dest.h = OVERLAY_LED_HEIGHT;

    src.y = 0;
    src.w = OVERLAY_LED_WIDTH;
    src.h = OVERLAY_LED_HEIGHT;
		
	/* Draw the digit(s) */
	for(int i = 0; i < num_digits; i++)
	{
		src.x = values[i] * OVERLAY_LED_WIDTH;
		SDL_BlitSurface(g_other_bmps[B_OVERLAY_LEDS], &src, overlay, &dest);
	    dest.x += OVERLAY_LED_WIDTH;
	}

    dest.x = start_x;
    dest.w = num_digits * OVERLAY_LED_WIDTH;
    SDL_UpdateRects(overlay, 1, &dest);
}

// Draw LDP1450 overlay characters to the screen (added by Brad O.)
void draw_singleline_LDP1450(char *LDP1450_String, int start_x, int y, SDL_Surface *overlay)
{
	SDL_Rect src, dest;

	int i = 0;
	int value = 0;
	int LDP1450_strlen;
//	char s[81]="";

	dest.x = start_x;
	dest.y = y;
	dest.w = OVERLAY_LDP1450_WIDTH;
	dest.h = OVERLAY_LDP1450_HEIGHT;

	src.y = 0;
	src.w = OVERLAY_LDP1450_WIDTH;
	src.h = OVERLAY_LDP1450_WIDTH;

	LDP1450_strlen = strlen(LDP1450_String);

	if (!LDP1450_strlen)              // if a blank line is sent, we must blank out the entire line
	{
		strcpy(LDP1450_String,"           ");
		LDP1450_strlen = strlen(LDP1450_String);
	}
	else 
	{
		if (LDP1450_strlen <= 11)	 // pad end of string with spaces (in case previous line was not cleared)
		{
			for (i=LDP1450_strlen; i<=11; i++)
				LDP1450_String[i] = 32;
			LDP1450_strlen = strlen(LDP1450_String);
		}
	}

	for (i=0; i<LDP1450_strlen; i++)
	{
		value = LDP1450_String[i];

		if (value >= 0x26 && value <= 0x39)       // numbers and symbols
			value -= 0x25;
		else if (value >= 0x41 && value <= 0x5a)  // alpha
			value -= 0x2a;
		else if (value == 0x13)                   // special LDP-1450 character (inversed space)
			value = 0x32;
		else
			value = 0x31;			              // if not a number, symbol, or alpha, recognize as a space

		src.x = value * OVERLAY_LDP1450_WIDTH;
		SDL_BlitSurface(g_other_bmps[B_OVERLAY_LDP1450], &src, overlay, &dest);
		
		dest.x += OVERLAY_LDP1450_CHARACTER_SPACING;
	}
	dest.x = start_x;
	dest.w = LDP1450_strlen * OVERLAY_LDP1450_CHARACTER_SPACING;

	// MPO : calling UpdateRects probably isn't necessary and may be harmful
	//SDL_UpdateRects(overlay, 1, &dest);
}

//  used to draw non LED stuff like scoreboard text
//  'which' corresponds to enumerated values
bool draw_othergfx(int which, int x, int y, bool bSendToScreenBlitter)
{
	// NOTE : this is drawn to g_screen_blitter, not to g_screen,
	//  to be more friendly to our opengl implementation!
	SDL_Surface *srf = g_other_bmps[which];
	SDL_Rect dest;
	dest.x = (short) x;
	dest.y = (short) y;
	dest.w = (unsigned short) srf->w;
	dest.h = (unsigned short) srf->h;

	// if we should blit this to the screen blitter for later use ...
	if (bSendToScreenBlitter)
	{
		SDL_BlitSurface(srf, NULL, g_screen_blitter, &dest);
	}
	// else blit it now
	else
	{
		vid_blit(g_other_bmps[which], x, y);
	}
	return true;
}

// de-allocates all of the .bmps that we have allocated
void free_bmps()
{

	int nuke_index = 0;

	// get rid of all the LED's
	for (; nuke_index < LED_RANGE; nuke_index++)
	{
		free_one_bmp(g_led_bmps[nuke_index]);
	}
	for (nuke_index = 0; nuke_index < B_EMPTY; nuke_index++)
	{
		// check to make sure it exists before we try to free
		if (g_other_bmps[nuke_index])
		{
			free_one_bmp(g_other_bmps[nuke_index]);
		}
	}
}

void free_one_bmp(SDL_Surface *candidate)
{

	SDL_FreeSurface(candidate);

}

//////////////////////////////////////////////////////////////////////////////////////////

SDL_Surface *get_screen()
{
	return g_screen;
}

SDL_Surface *get_screen_blitter()
{
	return g_screen_blitter;
}

int get_console_initialized()
{
	return g_console_initialized;
}

bool get_fullscreen()
{
	return g_fullscreen;
}

// sets our g_fullscreen bool (determines whether will be in fullscreen mode or not)
void set_fullscreen(bool value)
{
	g_fullscreen = value;
}

void set_rotate_degrees(float fDegrees)
{
	g_fRotateDegrees = fDegrees;
}

void set_sboverlay_characterset(int value)
{
	sboverlay_characterset = value;
}

// returns video width
Uint16 get_video_width()
{
	return g_vid_width;
}

// sets g_vid_width
void set_video_width(Uint16 width)
{
	// Let the user specify whatever width s/he wants (and suffer the consequences)
	// We need to support arbitrary resolution to accomodate stuff like screen rotation
	g_vid_width = width;
}

// returns video height
Uint16 get_video_height()
{
	return g_vid_height;
}

// sets g_vid_height
void set_video_height(Uint16 height)
{
	// Let the user specify whatever height s/he wants (and suffer the consequences)
	// We need to support arbitrary resolution to accomodate stuff like screen rotation
	g_vid_height = height;
}

///////////////////////////////////////////////////////////////////////////////////////////

// converts the SDL_Overlay to a BMP file
// ASSUMES OVERLAY IS ALREADY LOCKED!!!
void take_screenshot(SDL_Overlay *yuvimage)
{
	SDL_Color cur_color = { 0 };
	bool bSaveYUV = false;

#ifdef DEBUG
	// in debug mode, save the YUV stuff too
	bSaveYUV = true;
#endif // DEBUG

	SDL_Surface *rgbimage = SDL_CreateRGBSurface(SDL_SWSURFACE, yuvimage->w, yuvimage->h, 32, 0xFF, 0xFF00, 0xFF0000, 0);
	if (rgbimage)
	{
		Uint32 *cur_pixel = (Uint32 *) rgbimage->pixels;

		// if we're in YV12 format (old)
		if (yuvimage->format == SDL_YV12_OVERLAY)
		{
			Uint8 *Y = yuvimage->pixels[0];
			Uint8 *V = yuvimage->pixels[1];
			Uint8 *U = yuvimage->pixels[2];
			int y_extra = yuvimage->pitches[0] - yuvimage->w;
			int v_extra = yuvimage->pitches[1] - (yuvimage->w / 2);
			int u_extra = yuvimage->pitches[2] - (yuvimage->w / 2);

			// advance by 2 since the U and V channels are half-sized
			for (int row = 0; row < yuvimage->h; row += 2)
			{
				// advance by 2 since the U and V channels are half-sized
				for (int col = 0; col < yuvimage->w; col += 2)
				{
					// get a 2x2 block of pixels and store them due to YUV structure
					// upper left
					yuv2rgb(&cur_color, *Y, *U, *V);
					cur_pixel[0] = SDL_MapRGB(rgbimage->format, cur_color.r, cur_color.g, cur_color.b);

					// upper right
					yuv2rgb(&cur_color, *(Y+1), *U, *V);
					cur_pixel[1] = SDL_MapRGB(rgbimage->format, cur_color.r, cur_color.g, cur_color.b);

					// lower left
					yuv2rgb(&cur_color, *(Y + yuvimage->pitches[0]), *U, *V);
					cur_pixel[0 + rgbimage->w] = SDL_MapRGB(rgbimage->format, cur_color.r, cur_color.g, cur_color.b);

					// lower right
					yuv2rgb(&cur_color, *(Y + yuvimage->pitches[0] + 1), *U, *V);
					cur_pixel[1 + rgbimage->w] = SDL_MapRGB(rgbimage->format, cur_color.r, cur_color.g, cur_color.b);

					cur_pixel += 2;	// move two pixels over
					Y += 2;	// move two Y values over
					U++;	// but just 1 U and V
					V++;
				}
				Y += y_extra;
				V += v_extra;
				U += u_extra;
				cur_pixel += rgbimage->w;	// move down one row (FIXME: I think this should use pitch)
				// WARNING: the above assumes that the pitch is the width * 4

				Y += yuvimage->pitches[0];	// move down one line
			}
		}
		else if (yuvimage->format == SDL_YUY2_OVERLAY)
		{
			mpo_io *io = NULL;
			if (bSaveYUV)
			{
				io = mpo_open("surface.YUY2", MPO_OPEN_CREATE);
			}

			Uint8 *line_ptr = yuvimage->pixels[0];
			for (int row = 0; row < yuvimage->h; row ++)
			{
				// if we're saving to a file, then write the YUY2 line
				if (bSaveYUV)
				{
					mpo_write(line_ptr, yuvimage->w * 2, NULL, io);
				}

				Uint8 *pixel_ptr = line_ptr;
				// advance by 2 because we're doing 2 pixels at a time
				for (int col = 0; col < yuvimage->w; col += 2)
				{
					// left
					yuv2rgb(&cur_color, *(pixel_ptr), *(pixel_ptr+1), *(pixel_ptr+3));
					cur_pixel[0] = SDL_MapRGB(rgbimage->format, cur_color.r, cur_color.g, cur_color.b);

					// right
					yuv2rgb(&cur_color, *(pixel_ptr+2), *(pixel_ptr+1), *(pixel_ptr+3));
					cur_pixel[1] = SDL_MapRGB(rgbimage->format, (cur_color.r & 0xFE), (cur_color.g & 0xFE), (cur_color.b & 0xFE));

					cur_pixel += 2;	// move 2 pixels over
					pixel_ptr += 4;	// move 4 bytes over
				}
				line_ptr += yuvimage->pitches[0];
				cur_pixel += (rgbimage->pitch - (rgbimage->w*4)) / 4;	// this look complicated, but the pitch should be the width * 4 so it's just a precaution
				// NOTE : if the pitch is not a multiple of 4, then we've got problems..
			}

			if (io)
			{
				mpo_close(io);
			}
		}
		else
		{
			printline ("ERROR : unknown YUV format!");
		}

		// now the RGB image has been created, time to save
		save_screenshot(rgbimage);
		SDL_FreeSurface(rgbimage);	// de-allocate surface, as we no longer need it

	} // end if RGB image
	else
	{
		printline("ERROR: Could not create RGB image to take screenshot with");
	}

	// if we're in YV12 mode and we should save the YUV stuff, then do so
	if (bSaveYUV && (yuvimage->format == SDL_YV12_OVERLAY))
	{
		// NOW save the Y, U and V channels in separate files
		// Most people won't need or want these files, but I have use of them for testing
		Uint8 *Y = yuvimage->pixels[0];
		Uint8 *V = yuvimage->pixels[1];
		Uint8 *U = yuvimage->pixels[2];

		struct mpo_io *io  = mpo_open("surface.Y", MPO_OPEN_CREATE);
		if (io)
		{
			for (int row = 0; row < yuvimage->h; row++)
			{
				mpo_write(Y, yuvimage->w, NULL, io);
				Y += yuvimage->pitches[0];
			}
			mpo_close(io);
		}

		io = mpo_open("surface.V", MPO_OPEN_CREATE);
		if (io)
		{
			for (int row = 0; row < (yuvimage->h / 2); row++)
			{
				mpo_write(V, yuvimage->w / 2, NULL, io);
				V += yuvimage->pitches[1];
			}
			mpo_close(io);
		}

		io = mpo_open("surface.U", MPO_OPEN_CREATE);
		if (io)
		{
			for (int row = 0; row < (yuvimage->h / 2); row++)
			{
				mpo_write(U, yuvimage->w / 2, NULL, io);
				U += yuvimage->pitches[2];
			}
			mpo_close(io);
		}
	}
}

#ifdef USE_OPENGL
void take_screenshot_GL()
{
	SDL_Surface *rgbimage = SDL_CreateRGBSurface(SDL_SWSURFACE,
		g_vid_width, g_vid_height, 32, 0xFF, 0xFF00, 0xFF0000, 0);

	if (rgbimage)
	{
		unsigned char *bufDst = (unsigned char *) rgbimage->pixels;
		unsigned int uLineLength = g_vid_width << 2;

		unsigned char *bufTmp = new unsigned char [uLineLength * g_vid_height];
		if (bufTmp)
		{
			// grab current screen
			glReadPixels(0, 0, g_vid_width, g_vid_height, GL_RGBA, GL_UNSIGNED_BYTE, bufTmp);

			// OpenGL goes bottom to top, and the SDL Surface goes top to bottom,
			//  so we have to flip the image.
			for (unsigned int uLine = 0; uLine < g_vid_height; ++uLine)
			{
				unsigned char *u8Dst = bufDst + (uLine * uLineLength);
				unsigned char *u8Src = bufTmp + ((g_vid_height - uLine - 1) * uLineLength);

				memcpy(u8Dst, u8Src, (g_vid_width << 2));
			}

			// now the RGB image has been created, time to save
			save_screenshot(rgbimage);

			delete [] bufTmp;
		}
		else printline("ERROR: take_screenshot_GL memory allocation failed");

		SDL_FreeSurface(rgbimage);	// de-allocate surface, as we no longer need it

	} // end if RGB image
	else
	{
		printline("ERROR: Could not create RGB image to take screenshot with");
	}
}
#endif // USE_OPENGL

// saves an SDL surface to a .BMP file in the screenshots directory
void save_screenshot(SDL_Surface *shot)
{
	int screenshot_num = 0;
	char filename[81] = { 0 };

	// search for a filename that does not exist
	for (;;)
	{
		screenshot_num++;
		sprintf(filename, "screen%d.bmp", screenshot_num);

		// if file does not exist, we'll save a screenshot to that filename
		if (!mpo_file_exists(filename))
		{
			break;
		}
	}

	if (SDL_SaveBMP(shot, filename) == 0)
	{
		outstr("NOTE: Wrote screenshot to file ");
		printline(filename);
	}
	else
	{
		outstr("ERROR: Could not write screenshot to file ");
		printline(filename);
	}
}

// converts YUV to RGB
// Use this only when you don't care about speed =]
// NOTE : it is important for y, u, and v to be signed
void yuv2rgb(SDL_Color *result, int y, int u, int v)
{
	// NOTE : Visual C++ 7.0 apparently has a bug
	// in its floating point optimizations because this function
	// will return incorrect results when compiled as a Release
	// Possible workaround: use integer math instead of float? or don't use VC++ 7? hehe

	int b = (int) (1.164*(y - 16)                   + 2.018*(u - 128));
	int g = (int) (1.164*(y - 16) - 0.813*(v - 128) - 0.391*(u - 128));
	int r = (int) (1.164*(y - 16) + 1.596*(v - 128));

	// clamp values (not sure if this is necessary, but we aren't worried about speed)
	if (b > 255) b = 255;
	if (b < 0) b = 0;
	if (g > 255) g = 255;
	if (g < 0) g = 0;
	if (r > 255) r = 255;
	if (r < 0) r = 0;
	
	result->r = (unsigned char) r;
	result->g = (unsigned char) g;
	result->b = (unsigned char) b;
}

void draw_string(const char* t, int col, int row, SDL_Surface* overlay)
{
	SDL_Rect dest;

	dest.x = (short) ((col*6));
	dest.y = (short) ((row*13));
	dest.w = (unsigned short) (6 * strlen(t)); // width of rectangle area to draw (width of font * length of string)
	dest.h = 13;	// height of area (height of font)
		
	SDL_FillRect(overlay, &dest, 0); // erase anything at our destination before we print new text
	SDLDrawText(t,  overlay, FONT_SMALL, dest.x, dest.y);
	SDL_UpdateRects(overlay, 1, &dest);	
}

// toggles fullscreen mode
void vid_toggle_fullscreen()
{
	// Commented out because it creates major problems with YUV overlays and causing segfaults..
	// The real way to toggle fullscreen is to kill overlay, kill surface, make surface, make overlay.
	/*
	g_screen = SDL_SetVideoMode(g_screen->w,
		g_screen->h,
		g_screen->format->BitsPerPixel,
		g_screen->flags ^ SDL_FULLSCREEN);
		*/
}

// NOTE : put into a separate function to make autotesting easier
void set_yuv_hwaccel(bool enabled)
{
	const char *val = "0";
	if (enabled) val = "1";
#ifdef WIN32
	SetEnvironmentVariable("SDL_VIDEO_YUV_HWACCEL", val);
	string sEnv = "SDL_VIDEO_YUV_HWACCEL=";
	sEnv += val;
	putenv(sEnv.c_str());
#else
	setenv("SDL_VIDEO_YUV_HWACCEL", val, 1);
#endif
}

bool get_yuv_hwaccel()
{
	bool result = true;	// it is enabled by default
#ifdef WIN32
	char buf[30];
	ZeroMemory(buf, sizeof(buf));
	DWORD res = GetEnvironmentVariable("SDL_VIDEO_YUV_HWACCEL", buf, sizeof(buf));
	if (buf[0] == '0') result = false;
#else
	char *hw_env = getenv("SDL_VIDEO_YUV_HWACCEL");

	// if HW acceleration has been disabled
	if (hw_env && (hw_env[0] == '0')) result = false;
#endif
	return result;
}

void set_force_aspect_ratio(bool bEnabled)
{
	g_bForceAspectRatio = bEnabled;
}

bool get_force_aspect_ratio()
{
	return g_bForceAspectRatio;
}

#ifdef USE_OPENGL
void set_use_opengl(bool enabled)
{
	g_bUseOpenGL = enabled;
}

bool get_use_opengl()
{
	return g_bUseOpenGL;
}

#endif // USE_OPENGL

