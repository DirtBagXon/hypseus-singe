/*  SDL_Console.c
 *  Written By: Garrett Banuk <mongoose@wpi.edu>
 *  This is free, just be sure to give me credit when using it
 *  in any of your programs.
 */


#include <stdio.h>
#include <string.h>

#include <stdlib.h>
// Mac OSX doesn't have a malloc.h
//#include <malloc.h>

#include <stdarg.h>
#include <SDL.h>
//#include "sdl_input.h"	// MATT added
//#include "bitmap.h"	// MATT added
#include "SDL_Console.h"
#include "SDL_DrawText.h"

#ifdef WIN32
#pragma warning (disable:4244)	// disable the warning about possible loss of data
#endif

//extern SDL_Surface *G_screen;	// MATT added, functions aren't working
//extern int G_consoledown;	// MATT added, functions aren't working

static char				**ConsoleLines = NULL;
static char				**CommandLines = NULL;
static int				TotalConsoleLines = 0;			/* Total number of lines in the console */
static int				ConsoleScrollBack = 0;			/* How much the users scrolled back in the console */
static int				TotalCommands = 0;				/* Number of commands in the Back Commands */
static int				FontNumber;						/* This is the number of the font for the console */
static int				Line_Buffer;					/* The number of lines in the console */
static int				BackX, BackY;					/* Background images x and y coords */
static SDL_Surface		*ConsoleSurface;				/* Surface for the console text */
static SDL_Surface		*OutputScreen;					/* This is the screen to draw to */
static SDL_Surface		*Border = NULL;					/* Sides, bottom and corners of the console border */
static SDL_Surface		*BackgroundImage = NULL;		/* Background image for the console */
static SDL_Surface		*InputBackground;				/* Dirty rectangle to draw over behind the users background */


/* Takes keys from the keyboard and inputs them to the console */
void ConsoleEvents(SDL_Event *event)
{
	
	static int	StringLocation = 0;		/* Current character location in the current string */
	static int	CommandScrollBack = 0;	/* How much the users scrolled back in the command lines */
	SDL_Rect	inputbackground;

	
	switch(event->type)
	{
		case SDL_KEYDOWN:
			switch(event->key.keysym.sym)
			{
				case SDLK_PAGEUP:
					if(ConsoleScrollBack < TotalConsoleLines  && ConsoleScrollBack < Line_Buffer)
					{
						ConsoleScrollBack++;
						UpdateConsole();
					}
					break;
				case SDLK_PAGEDOWN:
					if(ConsoleScrollBack>0)
					{
						ConsoleScrollBack--;
						UpdateConsole();
					}
					break;
				case SDLK_END:
					ConsoleScrollBack = 0;
					UpdateConsole();
					break;
				case SDLK_UP:
					if(CommandScrollBack < TotalCommands )
					{
						/* move back a line in the command strings and copy the command to the current input string */
						CommandScrollBack++;
						memset(ConsoleLines[0], 0, CHARS_PER_LINE);
						strcpy(ConsoleLines[0], CommandLines[CommandScrollBack]);
						StringLocation = strlen( CommandLines[CommandScrollBack]);
						UpdateConsole();
					}
					break;
				case SDLK_DOWN:
					if(CommandScrollBack > 0)
					{
						/* move forward a line in the command strings and copy the command to the current input string*/
						CommandScrollBack--;
						memset(ConsoleLines[0], 0, CHARS_PER_LINE);
						strcpy(ConsoleLines[0], CommandLines[CommandScrollBack]);
						StringLocation = strlen(ConsoleLines[CommandScrollBack]);
						UpdateConsole();	
					}
					break;
				case SDLK_BACKSPACE:
					if(StringLocation > 0) 
					{
						ConsoleLines[0][StringLocation-1] = '\0';
						StringLocation--;
						inputbackground.x = 0;
						inputbackground.y = ConsoleSurface->h-FontHeight(FontNumber);
						inputbackground.w = ConsoleSurface->w;
						inputbackground.h = FontHeight(FontNumber);
						SDL_BlitSurface(InputBackground, NULL, ConsoleSurface, &inputbackground);
						SDLDrawText(ConsoleLines[0], ConsoleSurface, FontNumber, 4, ConsoleSurface->h-FontHeight(FontNumber));
					}
					break;
				case SDLK_TAB:
					TabCompletion( ConsoleLines[0], &StringLocation );
					break;
				case SDLK_RETURN:
					NewLineCommand();
					/* copy the input into the past commands strings */
					strcpy(CommandLines[0], ConsoleLines[0]);
					strcpy(ConsoleLines[1], ConsoleLines[0]);
					CommandExecute(ConsoleLines[0]);
					/* zero out the current string and get it ready for new input */
					memset(ConsoleLines[0], 0, CHARS_PER_LINE);
					CommandScrollBack = -1;
					StringLocation = 0;
					UpdateConsole();
					break;
				default:
					if(StringLocation < CHARS_PER_LINE-1 && event->key.keysym.unicode)
					{
						ConsoleLines[0][StringLocation] = event->key.keysym.unicode;
						StringLocation++;
						inputbackground.x = 0;
						inputbackground.y = ConsoleSurface->h-FontHeight(FontNumber);
						inputbackground.w = ConsoleSurface->w;
						inputbackground.h = FontHeight(FontNumber);
						SDL_BlitSurface(InputBackground, NULL, ConsoleSurface, &inputbackground);
						SDLDrawText(ConsoleLines[0], ConsoleSurface, FontNumber, 4, ConsoleSurface->h-FontHeight(FontNumber));
					}
					break;
			}
			break;
		default:
			break;
	}
}


/* Updates the console buffer */
void UpdateConsole()
{
	int loop;
	int	Screenline = OutputScreen->h/2/FontHeight(FontNumber);
	SDL_Rect DestRect;
	

	SDL_FillRect(ConsoleSurface, NULL, 0);

	/* draw the background image if there is one */
	if(BackgroundImage)
	{	
		DestRect.x = BackX;
		DestRect.y = BackY;
		DestRect.w = BackgroundImage->w;
		DestRect.h = BackgroundImage->h;
		SDL_BlitSurface(BackgroundImage, NULL, ConsoleSurface, &DestRect);
	}

	/* Draw the text */
	for( loop=0; loop<Screenline; loop++)
		SDLDrawText(ConsoleLines[Screenline-loop+ConsoleScrollBack], ConsoleSurface, FontNumber, 4, loop*FontHeight(FontNumber));
		
	if( ConsoleScrollBack > 0 )
		for( loop=4; loop<OutputScreen->w-FontWidth(FontNumber)*2; loop+=FontWidth(FontNumber)*2)
			SDLDrawText("^ ", ConsoleSurface, FontNumber, loop, ConsoleSurface->h-FontHeight(FontNumber));
	else
		SDLDrawText(ConsoleLines[0], ConsoleSurface, FontNumber, 4, ConsoleSurface->h-FontHeight(FontNumber));

}

/* Draws the console buffer to the screen */
void DrawConsole()
{
	int loop;
	SDL_Rect DestRect = {0, 0, ConsoleSurface->w, ConsoleSurface->h};
	

	SDL_BlitSurface(ConsoleSurface, NULL, OutputScreen, &DestRect);

	/* Now draw a border */
	if(Border)
	{
		DestRect.x = 0;
		DestRect.y = ConsoleSurface->h;
		DestRect.w = Border->w;
		DestRect.h = Border->h;

		for(loop=0; loop<=OutputScreen->w/Border->w; loop++)
		{
			DestRect.x = Border->w*loop;
			SDL_BlitSurface(Border, NULL, OutputScreen, &DestRect);
		}
	}

	
}


/* Initializes the console strings */
int ConsoleInit(const char *FontName, SDL_Surface *DisplayScreen, int lines)
{
	int	loop;
	SDL_Surface	*Temp;

	OutputScreen = DisplayScreen;
	Line_Buffer = lines;

	/* malloc memory for the console lines. */
	ConsoleLines = (char **)malloc(sizeof(char *)*Line_Buffer);
	CommandLines = (char **)malloc(sizeof(char *)*Line_Buffer);

	for(loop=0; loop<=Line_Buffer-1; loop++)
	{
		ConsoleLines[loop] = (char*)calloc( CHARS_PER_LINE, sizeof(char));
		CommandLines[loop] = (char*)calloc( CHARS_PER_LINE, sizeof(char));
	}

	/* load the console surface */
	Temp = SDL_AllocSurface(SDL_SWSURFACE, OutputScreen->w, OutputScreen->h/2, OutputScreen->format->BitsPerPixel, 0, 0, 0, 0);
	if(Temp == NULL)
	{
		printf("Error Console.c:ConsoleInit()\n\tCouldn't create the ConsoleSurface\n");
		return 1;
	}
	ConsoleSurface = SDL_DisplayFormat(Temp);
	SDL_FreeSurface(Temp);
	SDL_FillRect(ConsoleSurface, NULL, 0);
	
	/* Load the consoles font */
	if( -1 == (FontNumber = LoadFont(FontName, TRANS_FONT)))
	{
		printf("Could not load the font \"%s\" for the console!\n", FontName);
		return 1;
	}
	
	/* Load the dirty rectangle for user input */
	Temp = SDL_AllocSurface(SDL_SWSURFACE, OutputScreen->w, FontHeight(FontNumber), OutputScreen->format->BitsPerPixel, 0, 0, 0, 0);
	if(Temp == NULL)
	{
		printf("Error Console.c:ConsoleInit()\n\tCouldn't create the input background\n");
		return 1;
	}
	InputBackground = SDL_DisplayFormat(Temp);
	SDL_FreeSurface(Temp);
	SDL_FillRect(InputBackground, NULL, 0);

	
	ConOut("Console initialised." );
	ListCommands();

	

	return 0;
}

// MATT added this to fix the memory leaks
void ConsoleShutdown()
{

	int loop = 0;

	for(loop=0; loop<=Line_Buffer-1; loop++)
	{
		if (ConsoleLines && ConsoleLines[loop])
		{
			free(ConsoleLines[loop]);
			ConsoleLines[loop] = 0;
		}
		if (CommandLines && CommandLines[loop])
		{
			free(CommandLines[loop]);
			ConsoleLines[loop] = 0;
		}
	}

	if (ConsoleLines)
	{
		free(ConsoleLines);
		ConsoleLines = 0;
	}
	if (CommandLines)
	{
		free(CommandLines);
		CommandLines = 0;
	}
}

/* Increments the console line */
void NewLineConsole()
{
	int		loop;
	char	*temp = ConsoleLines[Line_Buffer-1];

	for(loop=Line_Buffer-1; loop>1; loop--)
		ConsoleLines[loop] = ConsoleLines[loop-1];

	ConsoleLines[1] = temp;
	
	memset( ConsoleLines[1], 0, CHARS_PER_LINE);
	TotalConsoleLines++;
}


/* Increments the command lines */
void NewLineCommand()
{
	int		loop;
	char	*temp = CommandLines[Line_Buffer-1];

	for(loop=Line_Buffer-1; loop>0; loop--)
		CommandLines[loop] = CommandLines[loop-1];

	CommandLines[0] = temp;

	memset( CommandLines[0], 0, CHARS_PER_LINE);
	TotalCommands++;
}


/* Outputs text to the console (in game and stdout), up to 256 chars can be entered
 * '\n' characters only take effect on stdout.
 */
void ConOut(const char *str, ... )
{
	va_list			marker;
	char			temp[256];
		

	va_start(marker, str);
	vsprintf(temp, str, marker);
	va_end(marker);

	if(ConsoleLines)
	{
		strncpy(ConsoleLines[1], temp, CHARS_PER_LINE);
		ConsoleLines[1][CHARS_PER_LINE-1] = '\0';
		NewLineConsole();
		UpdateConsole();
	}

	/* And print to stdout */
	// MATT : we don't want stdout.txt file createdin windows
#ifdef UNIX
	// MPO : updated, printing to stdout will be done in conout.cpp insteead
//	printf("%s\n", temp);
#endif
}

// MATT added this function to print a string but not add a newline to the end
// It's the same as ConOut with small modifications
void ConOutstr(char *s)
{
	if(ConsoleLines)
	{
		strncpy(ConsoleLines[1], s, CHARS_PER_LINE);
		ConsoleLines[1][CHARS_PER_LINE-1] = '\0';
		UpdateConsole();
	}

// don't print until we line feed or else we will get multiple prints
//	printf("%s", s);
}

/* Sets a border for the console, the parameter is NULL the border will 
 * be unloaded */
int SetConsoleBorder(const char *border)
{
	SDL_Surface *temp;


	if(border == NULL && Border != NULL)
	{
		SDL_FreeSurface(Border);
		Border = NULL;
		return 0;
	}

	if(NULL == (temp = SDL_LoadBMP(border)))
	{
		ConOut("Cannot load border %s.", border);
		return 1;
	}
	if(Border != NULL)
		SDL_FreeSurface(Border);
		
	Border = SDL_DisplayFormat(temp);
	SDL_FreeSurface(temp);
	
	SDL_SetColorKey(Border, SDL_SRCCOLORKEY, Border->format->Rmask|Border->format->Bmask);

	if(ConsoleSurface->format->alpha)
		SDL_SetAlpha(Border, SDL_SRCALPHA, ConsoleSurface->format->alpha);
	

	return 0;
}


/* Sets the alpha level of the console, 0 turns off alpha blending */
void ConsoleAlpha( unsigned char alpha )
{

	if(alpha > 0 && alpha <= 200)
	{
		SDL_SetAlpha(ConsoleSurface, SDL_SRCALPHA, alpha);
		if(Border) SDL_SetAlpha(Border, SDL_SRCALPHA, alpha);
	}
	else if(alpha == 0)
	{
		SDL_SetAlpha(ConsoleSurface, 0, alpha);
		if(Border) SDL_SetAlpha(Border, 0, alpha);
	}
	UpdateConsole();
}


/* Adds  background image to the console */
int ConsoleBackground(const char *image, int x, int y)
{
	SDL_Surface *temp;
	SDL_Rect backgroundsrc, backgrounddest;


	if(image == NULL && BackgroundImage != NULL)
	{
		SDL_FreeSurface(BackgroundImage);
		BackgroundImage = NULL;
		SDL_FillRect(InputBackground, NULL, 0);
		return 0;
	}

	if(NULL == (temp = SDL_LoadBMP(image)))
	{
		ConOut("Cannot load background %s.", image);
		return 1;
	}
	if(BackgroundImage != NULL)
		SDL_FreeSurface(BackgroundImage);
		
	BackgroundImage = SDL_DisplayFormat(temp);
	SDL_FreeSurface(temp);
	BackX = x;
	BackY = y;

	backgroundsrc.x = 0;
	backgroundsrc.y = ConsoleSurface->h-FontHeight(FontNumber)-BackY;
	backgroundsrc.w = BackgroundImage->w;
	backgroundsrc.h = InputBackground->h;

	backgrounddest.x = BackX;
	backgrounddest.y = 0;
	backgrounddest.w = BackgroundImage->w;
	backgrounddest.h = FontHeight(FontNumber);

	SDL_FillRect(InputBackground, NULL, 0);
	SDL_BlitSurface(BackgroundImage, &backgroundsrc, InputBackground, &backgrounddest);

	return 0;
}
