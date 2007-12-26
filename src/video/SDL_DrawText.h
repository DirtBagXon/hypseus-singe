#ifndef SDL_DrawText_h
#define SDL_DrawText_h


#define TRANS_FONT 1


typedef struct BitFont_td
{
	SDL_Surface			*FontSurface;
	int					CharWidth;
	int					CharHeight;
	int					FontNumber;
	struct BitFont_td	*NextFont;
} BitFont;

#ifdef __cplusplus
extern "C" {
#endif

void SDLDrawText(const char *string, SDL_Surface *surface, int FontType, int x, int y );
int LoadFont(const char *BitmapName, int flags );
int FontHeight( int FontNumber );
int FontWidth( int FontNumber );
BitFont* FontPointer(int FontNumber );

#ifdef __cplusplus
};
#endif

#endif
