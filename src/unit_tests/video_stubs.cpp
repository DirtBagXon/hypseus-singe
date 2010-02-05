// Purpose:
// To test the video scoreboard without linking into all the real video schlop.

#include <SDL.h>	// for draw_string

void vid_flip()
{
}

bool draw_led(int, int, int)
{
	return true;
}

bool draw_othergfx(int, int, int, bool)
{
	return true;
}

void vid_blank()
{
}

void draw_string(const char *t, int col, int row, SDL_Surface *overlay)
{
	SDL_FillRect(overlay, NULL, 0xFFFFFFFF);
}

void draw_overlay_leds(unsigned int values[], int num_digits, int start_x, int y, SDL_Surface *overlay)
{
}
