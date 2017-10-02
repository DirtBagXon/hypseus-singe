/*
 * ____ DAPHNE COPYRIGHT NOTICE ____
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

#include "config.h"

// video.cpp
// Part of the DAPHNE emulator
// This code started by Matt Ownby, May 2000

#include "../game/game.h"
#include "../io/conout.h"
#include "../io/error.h"
#include "../io/mpo_fileio.h"
#include "../io/mpo_mem.h"
#include "../ldp-out/ldp.h"
#include "palette.h"
#include "video.h"
#include <SDL_syswm.h> // rdg2010
#include <plog/Log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string> // for some error messages

// MAC: sdl_video_run thread defines block
#define SDL_VIDEO_RUN_BLIT                      0
#define SDL_VIDEO_RUN_UPDATE_YUV_TEXTURE	1
#define SDL_VIDEO_RUN_CREATE_YUV_TEXTURE	2
#define SDL_VIDEO_RUN_DESTROY_TEXTURE		3
#define SDL_VIDEO_RUN_END_THREAD		4

using namespace std;

namespace video
{
int g_vid_width = 640, g_vid_height = 480; // default video dimensions

#ifdef DEBUG
const Uint16 cg_normalwidths[]  = {320, 640, 800, 1024, 1280, 1280, 1600};
const Uint16 cg_normalheights[] = {240, 480, 600, 768, 960, 1024, 1200};
#else
const Uint16 cg_normalwidths[]  = {640, 800, 1024, 1280, 1280, 1600};
const Uint16 cg_normalheights[] = {480, 600, 768, 960, 1024, 1200};
#endif // DEBUG

// the dimensions that we draw (may differ from g_vid_width/height if aspect
// ratio is enforced)
unsigned int g_draw_width = 640, g_draw_height = 480;

// the current game overlay dimensions
unsigned int g_overlay_width = 0, g_overlay_height = 0;

FC_Font *g_font                    = NULL;
SDL_Surface *g_led_bmps[LED_RANGE] = {0};
SDL_Surface *g_other_bmps[B_EMPTY] = {0};
SDL_Window *g_window               = NULL;
SDL_Renderer *g_renderer           = NULL;
SDL_Texture *g_overlay_texture     = NULL; // The OVERLAY texture, excluding LEDs wich are a special case
SDL_Texture *g_yuv_texture         = NULL; // The YUV video texture, registered from ldp-vldp.cpp
SDL_Surface *g_screen_blitter      = NULL; // The main blitter surface
SDL_Surface *g_leds_surface        = NULL;

SDL_Rect g_overlay_size_rect; 
SDL_Rect g_display_size_rect = {0, 0, g_vid_width, g_vid_height};
SDL_Rect g_leds_size_rect = {0, 0, 320, 240}; 

bool g_fullscreen = false; // whether we should initialize video in fullscreen
                           // mode or not
int g_scalefactor = 100;   // by RDG2010 -- scales the image to this percentage
                           // value (for CRT TVs with overscan problems).
int sboverlay_characterset = 1;

// whether we will try to force a 4:3 aspect ratio regardless of window size
// (this is probably a good idea as a default option)
bool g_bForceAspectRatio = true;

// the # of degrees to rotate counter-clockwise in opengl mode
float g_fRotateDegrees = 0.0;

// SDL sdl_video_run thread variables
SDL_Thread *sdl_video_run_thread;
SDL_cond *sdl_video_run_cond;
SDL_cond *yuv_pending_update_cond;
SDL_mutex *sdl_video_run_mutex;
SDL_mutex *yuv_surface_mutex;
bool sdl_video_run_loop = true;
int sdl_video_run_action = 0;
int sdl_video_run_result = 0;

// SDL sdl_video_run thread function prototypes
void sdl_video_run_rendercopy (SDL_Renderer *renderer, SDL_Texture *texture, SDL_Rect *src, SDL_Rect* dst);

// SDL Texture creation, update and destruction parameters
int yuv_texture_width;
int yuv_texture_height;
SDL_Renderer *sdl_renderer;
SDL_Texture *yuv_texture;
//SDL_Texture *sdl_texture;
void *sdl_run_param;

// SDL YUV texture update parameters
uint8_t *yuv_texture_Yplane;
uint8_t *yuv_texture_Uplane;
uint8_t *yuv_texture_Vplane;
int yuv_texture_Ypitch;
int yuv_texture_Upitch;
int yuv_texture_Vpitch;

struct {
    uint8_t *Yplane;
    uint8_t *Uplane;
    uint8_t *Vplane;
    int width, height;
    int Ysize, Usize, Vsize; // The size of each plane in bytes.
    int Ypitch, Upitch, Vpitch; // The size of each plane in bytes.
} g_yuv_surface;

// Blitting parameters. What textures need updating from their surfaces during a blitting strike?
bool g_scoreboard_needs_update = false;
bool g_overlay_needs_update    = false;
bool g_yuv_video_needs_update  = false;

////////////////////////////////////////////////////////////////////////////////////////////////////

// initializes the window in which we will draw our BMP's
// returns true if successful, false if failure
bool init_display()
{

    bool result = false; // whether video initialization is successful or not
    Uint32 sdl_flags = 0;

    sdl_flags = SDL_WINDOW_SHOWN;

    // if we were able to initialize the video properly
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) >= 0) {

        if (g_fullscreen) sdl_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

        g_draw_width  = g_vid_width;
        g_draw_height = g_vid_height;

        g_overlay_width = g_game->get_video_overlay_width();
        g_overlay_height = g_game->get_video_overlay_height();

        // if we're supposed to enforce the aspect ratio ...
        if (g_bForceAspectRatio) {
            double dCurAspectRatio = (double)g_vid_width / g_vid_height;
            const double dTARGET_ASPECT_RATIO = 4.0 / 3.0;

            // if current aspect ratio is less than 1.3333
            if (dCurAspectRatio < dTARGET_ASPECT_RATIO) {
                g_draw_height = (g_draw_width * 3) / 4;
            }
            // else if current aspect ratio is greater than 1.3333
            else if (dCurAspectRatio > dTARGET_ASPECT_RATIO) {
                g_draw_width = (g_draw_height * 4) / 3;
            }
            // else the aspect ratio is already correct, so don't change
            // anything
        }

        // if we're supposed to scale the image...
        if (g_scalefactor < 100) {
            g_draw_width  = g_draw_width * g_scalefactor / 100;
            g_draw_height = g_draw_height * g_scalefactor / 100;
        }

        // by RDG2010
        // Step 2. Create a borderless SDL window.
        // If doing fullscreen window, make the window bordeless (no title
        // bar).
        // This is achieved by adding the SDL_NOFRAME flag.
	g_window =
            SDL_CreateWindow("HYPSEUS: Multiple Arcade Laserdisc Emulator",
                             SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                             g_vid_width, g_vid_height, sdl_flags);
        if (!g_window) {
            LOGW << fmt("Could not initialize window: %s", SDL_GetError());
        } else {
            if (g_game->m_sdl_software_rendering) {
                g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE |
                                                              SDL_RENDERER_TARGETTEXTURE |
                                                              SDL_RENDERER_PRESENTVSYNC
                                                              );
            } else {
                g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED |
                                                              SDL_RENDERER_TARGETTEXTURE |
                                                              SDL_RENDERER_PRESENTVSYNC);
            }

            if (!g_renderer) {
                LOGW << fmt("Could not initialize renderer: %s", SDL_GetError());
            } else {
                // MAC: If we start in fullscreen mode, we have to set the logical
                // render size to get the desired aspect ratio.
                // Also, we set bilinear filtering and hide the mouse cursor.
                if ((sdl_flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) {
                    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
                    SDL_ShowCursor(SDL_DISABLE);
                    SDL_RenderSetLogicalSize(g_renderer, g_draw_width, g_draw_height);
                }

                g_font = FC_CreateFont();
                FC_LoadFont(g_font, g_renderer, "fonts/default.ttf", 18,
                            FC_MakeColor(0, 0, 0, 255), TTF_STYLE_NORMAL);

                // Create a 32-bit surface with alpha component. As big as an overlay can possibly be...
		    int surfacebpp;
		    Uint32 Rmask, Gmask, Bmask, Amask;              
		    SDL_PixelFormatEnumToMasks(SDL_PIXELFORMAT_RGBA8888, &surfacebpp, &Rmask, &Gmask, &Bmask, &Amask);
		    g_screen_blitter =
			SDL_CreateRGBSurface(SDL_SWSURFACE, g_overlay_width, g_overlay_height,
					    surfacebpp, Rmask, Gmask, Bmask, Amask);

		    g_leds_surface =
			SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 240,
					    surfacebpp, Rmask, Gmask, Bmask, Amask);

                // MAC: If the game uses an overlay, create a surface a texture for it.
                // The g_screen_blitter surface is used from game.cpp anyway, so we always create it, used or not.
		if (g_overlay_width && g_overlay_height) {
		    g_overlay_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGBA8888,
						 SDL_TEXTUREACCESS_TARGET,
						 g_overlay_width, g_overlay_height);
		   
		    
		    SDL_SetTextureBlendMode(g_overlay_texture, SDL_BLENDMODE_BLEND);
		    SDL_SetTextureAlphaMod(g_overlay_texture, 255);
                }

                SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
                SDL_RenderClear(g_renderer);
                SDL_RenderPresent(g_renderer);
                // NOTE: SDL Console was initialized here.
                result = true;
            }
        }
    } else {
        LOGW << fmt("Could not initialize SDL: %s", SDL_GetError());
    }

    return (result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// deinitializes the window and renderer we have used.
// returns true if successful, false if failure
bool deinit_display()
{
    SDL_FreeSurface(g_screen_blitter);
    SDL_FreeSurface(g_leds_surface);
    SDL_DestroyTexture(g_overlay_texture);

    // We invented the YUV surface and we allocated memory for it, so we free it too.
    // No need to check for NULL, freeing a NULL pointer is ok.
    free (g_yuv_surface.Yplane);
    free (g_yuv_surface.Uplane);
    free (g_yuv_surface.Vplane);
    
    SDL_DestroyRenderer(g_renderer);

    return (true);
}

// shuts down video display
void shutdown_display()
{
    LOGD << "Shutting down video display...";

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

// Clear the renderer. Good for avoiding texture mess (YUV, LEDs, Overlay...)
void vid_blank()
{
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
    SDL_RenderClear(g_renderer);
}

// redraws the proper display (Scoreboard, etc) on the screen, after first
// clearing the screen
// call this every time you want the display to return to normal
void display_repaint()
{
    vid_blank();
    //vid_flip();
    g_game->force_blit();
}

// loads all the .bmp's
// returns true if they were all successfully loaded, or a false if they weren't
bool load_bmps()
{

    bool result = true; // assume success unless we hear otherwise
    int index   = 0;
    char filename[81];

    for (; index < LED_RANGE; index++) {
        sprintf(filename, "pics/led%d.bmp", index);

        g_led_bmps[index] = load_one_bmp(filename);

        // If the bit map did not successfully load
        if (g_led_bmps[index] == 0) {
            result = false;
        }
    }

    g_other_bmps[B_DL_PLAYER1]     = load_one_bmp("pics/player1.bmp");
    g_other_bmps[B_DL_PLAYER2]     = load_one_bmp("pics/player2.bmp");
    g_other_bmps[B_DL_LIVES]       = load_one_bmp("pics/lives.bmp");
    g_other_bmps[B_DL_CREDITS]     = load_one_bmp("pics/credits.bmp");
    g_other_bmps[B_HYPSEUS_SAVEME] = load_one_bmp("pics/saveme.bmp");
    g_other_bmps[B_GAMENOWOOK]     = load_one_bmp("pics/gamenowook.bmp");

    if (sboverlay_characterset != 2)
	g_other_bmps[B_OVERLAY_LEDS] = load_one_bmp("pics/overlayleds1.bmp");
    else
	g_other_bmps[B_OVERLAY_LEDS] = load_one_bmp("pics/overlayleds2.bmp");
    
    // MAC : Strange way to detect if we're using an overlay, but well, this is a mess anyway, so...
    if (g_screen_blitter) { 
	g_other_bmps[B_OVERLAY_LEDS] = SDL_ConvertSurface(g_other_bmps[B_OVERLAY_LEDS], g_screen_blitter->format, 0);
        SDL_SetColorKey (g_other_bmps[B_OVERLAY_LEDS], 1, 0x000000ff);
    }

    g_other_bmps[B_OVERLAY_LDP1450] = load_one_bmp("pics/ldp1450font.bmp");

    // check to make sure they all loaded
    for (index = 0; index < B_EMPTY; index++) {
        if (g_other_bmps[index] == NULL) {
            result = false;
        }
    }

    return (result);
}

SDL_Surface *load_one_bmp(const char *filename)
{
    SDL_Surface *result  = SDL_LoadBMP(filename);

    if (!result)
        LOGW << fmt("Could not load bitmap: %s", SDL_GetError());
 
    return (result);
}

/*SDL_Texture *load_one_bmp(const char *filename)
{
    SDL_Texture *texture = NULL;
    SDL_Surface *result  = SDL_LoadBMP(filename);

    if (result) texture = SDL_CreateTextureFromSurface(g_renderer, result);
    
    if (!texture) {
        LOGW << fmt("Could not load bitmap: %s", SDL_GetError());
    } else {
        SDL_FreeSurface(result);
    }
    
    return (texture);
}*/

// MAC: This is for some kind of bitmap-based nice scoreboard.
// Still not implemented. 7-segments scoreboard is good enough (and also bitmap based really).
// Draw's one of our LED's to the screen
// value contains the bitmap to draw (0-9 is valid)
// x and y contain the coordinates on the screen
// This function is called from img-scoreboard.cpp
// 1 is returned on success, 0 on failure
bool draw_led(int value, int x, int y)
{
    //vid_blit(g_led_bmps[value], x, y);
    return true;
}


// Update scoreboard surface
void draw_overlay_leds(unsigned int values[], int num_digits, int start_x,
                       int y, SDL_Surface *overlay)
{
    SDL_Rect src, dest;

    dest.x = start_x;
    dest.y = y;
    dest.w = OVERLAY_LED_WIDTH;
    dest.h = OVERLAY_LED_HEIGHT;

    src.y = 0;
    src.w = OVERLAY_LED_WIDTH;
    src.h = OVERLAY_LED_HEIGHT;
    
    // The leds surface is accessed from VIDEO_RUN_BLIT, which is called from the vldp thread, so it's access
    // must be "protected". The same happens with g_scoreboard_needs_update.
    // Draw the digit(s) to the overlay surface
    for (int i = 0; i < num_digits; i++) {
        src.x = values[i] * OVERLAY_LED_WIDTH;

        // MAC : We need to call SDL_FillRect() here if we don't want our LED characters to "overlap", because
        // we set the g_other_bmps[B_OVERLAY_LEDS] color key in such a way black is not being copied
        // so segments are not clean when we go from 0 to 1, for example.
        SDL_FillRect(g_leds_surface, &dest, 0x00000000); 
        SDL_BlitSurface(g_other_bmps[B_OVERLAY_LEDS], &src, g_leds_surface, &dest);

        dest.x += OVERLAY_LED_WIDTH;
    }

    g_scoreboard_needs_update = true;

    // MAC: Even if we updated the overlay surface here, there's no need to do not-thread-safe stuff
    // like SDL_UpdateTexture(), SDL_RenderCopy(), etc... until we are going to compose a final frame
    // with the YUV texture and the overlay on top (which is issued from vldp for now) in VIDEO_RUN_BLIT.
}

// Draw LDP1450 overlay characters to the screen (added by Brad O.)
void draw_singleline_LDP1450(char *LDP1450_String, int start_x, int y, SDL_Surface *overlay)
{
    SDL_Rect src, dest;

    int i     = 0;
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

    if (!LDP1450_strlen) // if a blank line is sent, we must blank out the
                         // entire line
    {
        strcpy(LDP1450_String, "           ");
        LDP1450_strlen = strlen(LDP1450_String);
    } else {
        if (LDP1450_strlen <= 11) // pad end of string with spaces (in case
                                  // previous line was not cleared)
        {
            for (i = LDP1450_strlen; i <= 11; i++) LDP1450_String[i] = 32;
            LDP1450_strlen = strlen(LDP1450_String);
        }
    }

    for (i = 0; i < LDP1450_strlen; i++) {
        value = LDP1450_String[i];

        if (value >= 0x26 && value <= 0x39) // numbers and symbols
            value -= 0x25;
        else if (value >= 0x41 && value <= 0x5a) // alpha
            value -= 0x2a;
        else if (value == 0x13) // special LDP-1450 character (inversed space)
            value = 0x32;
        else
            value = 0x31; // if not a number, symbol, or alpha, recognize as a
                          // space

        src.x = value * OVERLAY_LDP1450_WIDTH;
        //SDL_RenderCopy(g_renderer, g_other_bmps[B_OVERLAY_LDP1450], &src, &dest);

        dest.x += OVERLAY_LDP1450_CHARACTER_SPACING;
    }
    dest.x = start_x;
    dest.w = LDP1450_strlen * OVERLAY_LDP1450_CHARACTER_SPACING;

    // MPO : calling UpdateRects probably isn't necessary and may be harmful
    // SDL_UpdateRects(overlay, 1, &dest);
}

//  used to draw non LED stuff like scoreboard text
//  'which' corresponds to enumerated values
bool draw_othergfx(int which, int x, int y, bool bSendToScreenBlitter)
{
    // NOTE : this is drawn to g_screen_blitter, not to g_screen,
    //  to be more friendly to our opengl implementation!
    /*SDL_Texture *tx = g_other_bmps[which];
    SDL_Rect dest;
    dest.x = (short)x;
    dest.y = (short)y;
    SDL_QueryTexture(tx, NULL, NULL, &dest.w, &dest.h);

    // if we should blit this to the screen blitter for later use ...
    if (bSendToScreenBlitter) {
        SDL_RenderCopy(g_renderer, tx, NULL, &dest);
    }
    // else blit it now
    else {
        //vid_blit(g_other_bmps[which], x, y);
    }*/
    return true;
}

// de-allocates all of the .bmps that we have allocated
void free_bmps()
{

    int nuke_index = 0;

    // get rid of all the LED's
    for (; nuke_index < LED_RANGE; nuke_index++) {
        free_one_bmp(g_led_bmps[nuke_index]);
    }
    for (nuke_index = 0; nuke_index < B_EMPTY; nuke_index++) {
        // check to make sure it exists before we try to free
        if (g_other_bmps[nuke_index]) {
            free_one_bmp(g_other_bmps[nuke_index]);
        }
    }
}

void free_one_bmp(SDL_Surface *candidate) { 
	SDL_FreeSurface(candidate); 
}

//////////////////////////////////////////////////////////////////////////////////////////

SDL_Renderer *get_renderer() { return g_renderer; }

SDL_Texture *get_screen() { return g_overlay_texture; }

SDL_Surface *get_screen_blitter() { return g_screen_blitter; }

bool get_fullscreen() { return g_fullscreen; }

// sets our g_fullscreen bool (determines whether will be in fullscreen mode or
// not)
void set_fullscreen(bool value) { g_fullscreen = value; }

int get_scalefactor() { return g_scalefactor; }
void set_scalefactor(int value)
{
    if (value > 100 || value < 50) // Validating in case user inputs crazy
                                   // values.
    {
        LOGW << "Invalid scale value. Ignoring -scalefactor parameter.";
        g_scalefactor = 100;

    } else {
        g_scalefactor = value;
    }
}

void set_rotate_degrees(float fDegrees) { g_fRotateDegrees = fDegrees; }

void set_sboverlay_characterset(int value) { sboverlay_characterset = value; }

// returns video width
Uint16 get_video_width() { return g_vid_width; }

// sets g_vid_width
void set_video_width(Uint16 width)
{
    // Let the user specify whatever width s/he wants (and suffer the
    // consequences)
    // We need to support arbitrary resolution to accomodate stuff like screen
    // rotation
    g_vid_width = width;
}

// returns video height
Uint16 get_video_height() { return g_vid_height; }

// sets g_vid_height
void set_video_height(Uint16 height)
{
    // Let the user specify whatever height s/he wants (and suffer the
    // consequences)
    // We need to support arbitrary resolution to accomodate stuff like screen
    // rotation
    g_vid_height = height;
}

FC_Font *get_font() { return g_font; }

void draw_string(const char *t, int col, int row, SDL_Renderer *renderer)
{
    SDL_Rect dest;

    dest.x = (short)((col * 6));
    dest.y = (short)((row * 13));
    dest.w = (unsigned short)(6 * strlen(t)); // width of rectangle area to draw
                                              // (width of font * length of
                                              // string)
    dest.h = 13;                              // height of area (height of font)

    // SDL_FillRect(overlay, &dest, 0); // erase anything at our destination
    // before
    //                                 // we print new text
    FC_Draw(g_font, renderer, dest.x, dest.y, t);
    // SDL_UpdateRects(overlay, 1, &dest); FIXME
}

// toggles fullscreen mode
void vid_toggle_fullscreen()
{
    Uint32 flags = (SDL_GetWindowFlags(g_window) ^ SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (SDL_SetWindowFullscreen(g_window, flags) < 0) {
        LOGW << fmt("Toggle fullscreen failed: %s", SDL_GetError());
        return;
    }
    if ((flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) {
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
        SDL_RenderSetLogicalSize(g_renderer, g_draw_width, g_draw_height);
        return;
    }
    SDL_SetWindowSize(g_window, g_draw_width, g_draw_height);
}

void set_force_aspect_ratio(bool bEnabled) { g_bForceAspectRatio = bEnabled; }bool get_force_aspect_ratio() { return g_bForceAspectRatio; }
unsigned int get_draw_width() { return g_draw_width; }
unsigned int get_draw_height() { return g_draw_height; }

int sdl_video_run (void *data) {
	init_display();
	while (sdl_video_run_loop) {
		// MAC: We signal upon entering the thread loop to notify
		// the hypseus thread that init_display has returned.
		// We will signal here after each loop, too, to notify that the
		// thread as completed it's task and is ready for more.
		SDL_LockMutex(sdl_video_run_mutex);
		SDL_CondSignal(sdl_video_run_cond);
		SDL_CondWait(sdl_video_run_cond, sdl_video_run_mutex);
		SDL_UnlockMutex(sdl_video_run_mutex);
		switch (sdl_video_run_action) {
			case SDL_VIDEO_RUN_BLIT:		
                                // *IF* we get to SDL_VIDEO_BLIT from game::blit(), then the access to the
                                // overlay and scoreboard textures is done from the "hypseus" thread, that blocks
                                // until all blitting operations are completed and only then loops again, SO no
                                // need to protect the access to these surfaces or their needs_update booleans.
                                // However, since we get here from game::blit(), the yuv "surface" is accessed
                                // simultaneously from vldp and from here (to update the yuv texture), so access to 
                                // that surface and it's boolean DO need to be protected with a mutex.

                                // Does YUV texture need update from the YUV "surface"?
                                SDL_LockMutex(yuv_surface_mutex);
                                if (g_yuv_video_needs_update) {
                                    SDL_UpdateYUVTexture(g_yuv_texture, NULL,
                                        g_yuv_surface.Yplane, g_yuv_surface.Ypitch,
                                        g_yuv_surface.Uplane, g_yuv_surface.Vpitch,
                                        g_yuv_surface.Vplane, g_yuv_surface.Vpitch);
                                    g_yuv_video_needs_update = false;
                                    SDL_CondSignal(yuv_pending_update_cond);
                                }
		                SDL_UnlockMutex(yuv_surface_mutex);

                                // Does OVERLAY texture need update from the scoreboard surface?
                                if(g_scoreboard_needs_update) {
				    SDL_UpdateTexture(g_overlay_texture, &g_leds_size_rect,
					(void *)g_leds_surface->pixels, g_leds_surface->pitch);
				    g_scoreboard_needs_update = false;
                                }
	       
                                // Does OVERLAY texture need update from the overlay surface?
                                if(g_overlay_needs_update) {
                                    SDL_UpdateTexture(g_overlay_texture, &g_overlay_size_rect,
                                        (void *)g_screen_blitter->pixels, g_screen_blitter->pitch);
				    g_overlay_needs_update = false;
                                }

                                // Sadly, we have to RenderCopy the YUV texture on every blitting strike, because
                                // the image on the renderer gets "dirty" with previous overlay frames on top of the yuv.
                                SDL_RenderCopy(g_renderer, g_yuv_texture, NULL, NULL); 
                                
                                // If there's an overlay texture, it means we are using some kind of overlay,
                                // be it LEDs or any other thing, so RenderCopy it to the renderer ON TOP of the YUV video.
                                // ONLY a rect of the LEDs surface size is copied for now.
                                if(g_overlay_texture) {
                                    SDL_RenderCopy(g_renderer, g_overlay_texture, &g_leds_size_rect, NULL);
                                }
                                // Issue flip.
                                SDL_RenderPresent(g_renderer);
                                break;
			case SDL_VIDEO_RUN_CREATE_YUV_TEXTURE:
				g_yuv_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_YV12,
                                	SDL_TEXTUREACCESS_TARGET, yuv_texture_width, yuv_texture_height);
                                vid_blank_yuv_texture(); 
                                break;		
			case SDL_VIDEO_RUN_UPDATE_YUV_TEXTURE:		
				sdl_video_run_result = SDL_UpdateYUVTexture(g_yuv_texture, NULL,
					yuv_texture_Yplane, yuv_texture_Ypitch, yuv_texture_Uplane,
					yuv_texture_Upitch, yuv_texture_Vplane, yuv_texture_Vpitch);
                                break;
                        case SDL_VIDEO_RUN_DESTROY_TEXTURE:
        			SDL_DestroyTexture((SDL_Texture*)sdl_run_param);
				break;
			case SDL_VIDEO_RUN_END_THREAD:
				sdl_video_run_loop = false;
				break;
			default:
				break;
		}
	}	
	deinit_display();
	return 0;
}

bool sdl_video_run_start () {
    sdl_video_run_mutex = SDL_CreateMutex();
    sdl_video_run_cond = SDL_CreateCond();
    yuv_pending_update_cond = SDL_CreateCond();
    yuv_surface_mutex = SDL_CreateMutex();

    // Create thread, then wait for it to signal. 
    // When it signals, it means init_display() has returned and 
    // the thread is ready, waiting for orders.
    sdl_video_run_thread = SDL_CreateThread (sdl_video_run, "sdl_video_run", (void*)sdl_run_param);	

    SDL_LockMutex(sdl_video_run_mutex);
    SDL_CondWait(sdl_video_run_cond, sdl_video_run_mutex);
    SDL_UnlockMutex(sdl_video_run_mutex);

    // MAC: TODO: some error handling would be good.
    return true;
}

SDL_Texture *vid_create_yuv_texture (int width, int height) {
    
    // Also prepare the YUV "surface"
    // 12 bits (1 + 0.5 bytes) per pixel, and each plane has different size. Crazy stuff.
    g_yuv_surface.Ysize = width * height;
    g_yuv_surface.Usize = g_yuv_surface.Ysize / 4;
    g_yuv_surface.Vsize = g_yuv_surface.Ysize / 4;
   
    g_yuv_surface.Yplane = (uint8_t*) malloc (g_yuv_surface.Ysize);
    g_yuv_surface.Uplane = (uint8_t*) malloc (g_yuv_surface.Usize);
    g_yuv_surface.Vplane = (uint8_t*) malloc (g_yuv_surface.Vsize);

    g_yuv_surface.width  = width;
    g_yuv_surface.height = height;

    SDL_LockMutex(sdl_video_run_mutex);
    yuv_texture_width = width;	
    yuv_texture_height = height;
    sdl_video_run_action = SDL_VIDEO_RUN_CREATE_YUV_TEXTURE;
    SDL_CondSignal(sdl_video_run_cond);
    SDL_CondWait(sdl_video_run_cond, sdl_video_run_mutex);
    SDL_UnlockMutex(sdl_video_run_mutex);
    return g_yuv_texture;
}

void vid_blank_yuv_texture () {
    // "Y 0x00, U 0x00, V 0x00" is NOT black in YUV colorspace!
    memset(g_yuv_surface.Yplane, 0x00, g_yuv_surface.Ysize);
    memset(g_yuv_surface.Uplane, 0x80, g_yuv_surface.Usize);
    memset(g_yuv_surface.Vplane, 0x80, g_yuv_surface.Vsize);
    
    SDL_UpdateYUVTexture(g_yuv_texture, NULL,
	g_yuv_surface.Yplane, g_yuv_surface.width,
        g_yuv_surface.Uplane, g_yuv_surface.width/2,
        g_yuv_surface.Vplane, g_yuv_surface.width/2);
}

int vid_update_yuv_surface ( uint8_t *Yplane, uint8_t *Uplane, uint8_t *Vplane,
	int Ypitch, int Upitch, int Vpitch)
{
    SDL_LockMutex(yuv_surface_mutex);
    // We must get sure there's no pending texture updates
    // before overwritting the surface contents with a new frame.
    if (g_yuv_video_needs_update) // We still have a surface update that has not been transferred to texture!
        SDL_CondWait(yuv_pending_update_cond, yuv_surface_mutex);

    memcpy (g_yuv_surface.Yplane, Yplane, g_yuv_surface.Ysize);	
    memcpy (g_yuv_surface.Uplane, Uplane, g_yuv_surface.Usize);	
    memcpy (g_yuv_surface.Vplane, Vplane, g_yuv_surface.Vsize);

    g_yuv_surface.Ypitch = Ypitch;
    g_yuv_surface.Upitch = Upitch;
    g_yuv_surface.Vpitch = Vpitch;

    g_yuv_video_needs_update = true;
    SDL_UnlockMutex(yuv_surface_mutex);

    return 0;
}

void vid_update_overlay_surface (SDL_Surface *tx, int x, int y) {

    // We have got here from game::blit(), which is also called when scoreboard is updated,
    // so in that case we simply return and don't do any overlay surface update. 
    // Access to g_scoreboard_needs_update is protected because it's also accessed from the sdl_video_run thread.
    if (g_scoreboard_needs_update) {
        return;
    }
    else
    
    // Remember: tx is m_video_overlay[] passed from game::blit() 
    // Careful not comment this part on testing, because this rect is used in vid_blit!
    g_overlay_size_rect.x = (short)x;
    g_overlay_size_rect.y = (short)y;
    g_overlay_size_rect.w = tx->w;
    g_overlay_size_rect.h = tx->h;

    // MAC: 8bpp to RGBA8888 conversion. Black pixels are considered totally transparent so they become 0x00000000;
    for (int i = 0; i < (tx->w * tx->h); i++){
        if (     *(  ((uint8_t*)tx->pixels)+i ) != 0x00   ) {
	    *((uint32_t*)(g_screen_blitter->pixels)+i) = //0xff0000ff;
	    (0x00000000 | tx->format->palette->colors[*((uint8_t*)(tx->pixels)+i)].r) << 24|
	    (0x00000000 | tx->format->palette->colors[*((uint8_t*)(tx->pixels)+i)].g) << 16|
	    (0x00000000 | tx->format->palette->colors[*((uint8_t*)(tx->pixels)+i)].b) << 8|
	    0x000000ff;
        }
        else *((uint32_t*)(g_screen_blitter->pixels)+i) = 0x00000000;
    }
    g_overlay_needs_update = true;
    // MAC: We update the overlay texture later, just when we are going to SDL_RenderCpy() it to the renderer.
    // SDL_UpdateTexture(g_overlay_texture, &g_overlay_size_rect, (void *)g_screen_blitter->pixels, g_screen_blitter->pitch);
}

void vid_blit () {
    SDL_LockMutex(sdl_video_run_mutex);
    sdl_video_run_action = SDL_VIDEO_RUN_BLIT;
    SDL_CondSignal(sdl_video_run_cond);
    SDL_CondWait(sdl_video_run_cond, sdl_video_run_mutex);
    SDL_UnlockMutex(sdl_video_run_mutex);
}

void sdl_video_run_end () {
    SDL_LockMutex(sdl_video_run_mutex);
    sdl_video_run_action = SDL_VIDEO_RUN_END_THREAD;
    SDL_CondSignal(sdl_video_run_cond);
    SDL_UnlockMutex(sdl_video_run_mutex);
    SDL_WaitThread(sdl_video_run_thread, (int *)NULL);

    // Destroy the mutexes and conditions used by the threading scheme.
    SDL_DestroyMutex(sdl_video_run_mutex);
    SDL_DestroyMutex(yuv_surface_mutex);
    SDL_DestroyCond(sdl_video_run_cond);
    SDL_DestroyCond(yuv_pending_update_cond);
}

void vid_destroy_texture (SDL_Texture *texture) {
    SDL_LockMutex(sdl_video_run_mutex);
    sdl_video_run_action = SDL_VIDEO_RUN_DESTROY_TEXTURE;
    sdl_run_param = (void*)texture;
    SDL_CondSignal(sdl_video_run_cond);
    SDL_CondWait(sdl_video_run_cond, sdl_video_run_mutex);
    SDL_UnlockMutex(sdl_video_run_mutex);
}

}
