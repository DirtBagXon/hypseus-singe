#ifndef SDL_Console_H
#define SDL_Console_H


#include "SDL_ConsoleCommands.h"


#define CHARS_PER_LINE	128

#ifdef __cplusplus
extern "C" {
#endif

void ConsoleEvents(SDL_Event *event);
void DrawConsole();
void UpdateConsole();
int ConsoleInit(const char *FontName, SDL_Surface *DisplayScreen, int lines);
void ConsoleShutdown();	// MATT added
void NewLineConsole();
void NewLineCommand();
void ConOut(const char *str, ... );
void ConOutstr(char *);	// MATT added
int SetConsoleBorder(const char *borderset);
void ConsoleAlpha( unsigned char alpha );
int ConsoleBackground(const char *image, int x, int y);


#ifdef __cplusplus
};
#endif

#endif
