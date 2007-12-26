/*  SDL_Console.c
 *  Written By: Garrett Banuk <mongoose@wpi.edu>
 *  This is free, just be sure to give me credit when using it
 *  in any of your programs.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include "SDL_DrawText.h"

#ifdef WIN32
#pragma warning (disable:4244)	// disable the warning about possible loss of data
#endif

static int		TotalFonts = 0;
/* Linked list of fonts */
static BitFont *BitFonts = NULL;



/* Loads the font into a new struct 
 * returns -1 as an error else it returns the number
 * of the font for the user to use
 */
int LoadFont(const char *BitmapName, int flags)
{
	int		FontNumber = 0;
	BitFont	**CurrentFont = &BitFonts;
	SDL_Surface	*Temp;
	
	while(*CurrentFont)
	{
		CurrentFont = &((*CurrentFont)->NextFont);
		FontNumber++;
	}
	
	/* load the font bitmap */
	if(NULL ==  (Temp = SDL_LoadBMP(BitmapName)))
	{
		printf("Error Cannot load file %s\n", BitmapName );
		return -1;
	}

	/* Add a font to the list */
	*CurrentFont = (BitFont*)malloc(sizeof(BitFont));

	(*CurrentFont)->FontSurface = SDL_DisplayFormat(Temp);
	SDL_FreeSurface(Temp);

	(*CurrentFont)->CharWidth = (*CurrentFont)->FontSurface->w/256;
	(*CurrentFont)->CharHeight = (*CurrentFont)->FontSurface->h;
	(*CurrentFont)->FontNumber = FontNumber;
	(*CurrentFont)->NextFont = NULL;

	TotalFonts++;

	/* Set font as transparent if the flag is set */
	if(flags & TRANS_FONT)
	{
		/* This line was left in in case of problems getting the pixel format */
		/* SDL_SetColorKey((*CurrentFont)->FontSurface, SDL_SRCCOLORKEY|SDL_RLEACCEL, 0xFF00FF); */
		SDL_SetColorKey((*CurrentFont)->FontSurface, SDL_SRCCOLORKEY|SDL_RLEACCEL,
						 (*CurrentFont)->FontSurface->format->Rmask|(*CurrentFont)->FontSurface->format->Bmask);
	}
	
//	printf("Loaded font \"%s\". Width:%d, Height:%d\n", BitmapName, (*CurrentFont)->CharWidth, (*CurrentFont)->CharHeight );
	return FontNumber;
}


/* Takes the font type, coords, and text to draw to the surface*/
void SDLDrawText(const char *string, SDL_Surface *surface, int FontType, int x, int y )
{
	int			loop;
	int			characters;
	SDL_Rect	SourceRect, DestRect;
	BitFont		*CurrentFont;


	CurrentFont = FontPointer(FontType);

	/* see how many characters can fit on the screen */
	if(x>surface->w || y>surface->h) return;

	if(strlen(string) < (unsigned int) (surface->w-x)/CurrentFont->CharWidth)
		characters = strlen(string);
	else
		characters = (surface->w-x)/CurrentFont->CharWidth;

	DestRect.x = x;
	DestRect.y = y;
	DestRect.w = CurrentFont->CharWidth;
	DestRect.h = CurrentFont->CharHeight;

	SourceRect.y = 0;
	SourceRect.w = CurrentFont->CharWidth;
	SourceRect.h = CurrentFont->CharHeight;

	/* Now draw it */
	for(loop=0; loop<characters; loop++)
	{
		SourceRect.x = string[loop] * CurrentFont->CharWidth;
		SDL_BlitSurface(CurrentFont->FontSurface, &SourceRect, surface, &DestRect);
		
		DestRect.x += CurrentFont->CharWidth;
	}

}


/* Returns the height of the font numbers character
 * returns 0 if the fontnumber was invalid */
int FontHeight( int FontNumber )
{
	BitFont		*CurrentFont;

	CurrentFont = FontPointer(FontNumber);
	if(CurrentFont)
		return CurrentFont->CharHeight;
	else
		return 0;
}

/* Returns the width of the font numbers charcter */
int FontWidth( int FontNumber )
{
	BitFont		*CurrentFont;

	CurrentFont = FontPointer(FontNumber);
	if(CurrentFont)
		return CurrentFont->CharWidth;
	else
		return 0;
}

/* Returns a pointer to the font struct of the number
 * returns NULL if theres an error
 */
BitFont* FontPointer(int FontNumber)
{
	int		fontamount = 0;
	BitFont	*CurrentFont = BitFonts;


	while(fontamount<TotalFonts)
	{
		if(CurrentFont->FontNumber == FontNumber)
			return CurrentFont;
		else
		{
			CurrentFont = CurrentFont->NextFont;
			fontamount++;
		}
	}
	
	return NULL;

}
