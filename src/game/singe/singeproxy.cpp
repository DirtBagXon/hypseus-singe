/*
 * singeproxy.cpp
 *
 * Copyright (C) 2006 Scott C. Duensing
 *
 * This file is part of HYPSEUS, a laserdisc arcade game emulator
 *
 * HYPSEUS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HYPSEUS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "singeproxy.h"
#include "singe_interface.h"

#include "../../video/video.h"
#include "../../sound/sound.h"

#include <string>
#include <vector>

using namespace std;

////////////////////////////////////////////////////////////////////////////////

// Structures used by Singe to track various internal data
typedef struct g_soundType {
	SDL_AudioSpec  audioSpec;
	Uint32         length;
	Uint8         *buffer;
} g_soundT;

// These are pointers and values needed by the script engine to interact with Hypseus
lua_State    *g_se_lua_context;
SDL_Surface  *g_se_surface        = NULL;
int           g_se_overlay_width;
int           g_se_overlay_height;
double       *g_se_disc_fps;
unsigned int *g_se_uDiscFPKS;

// used to know whether try to shutdown lua would crash
bool g_bLuaInitialized = false;

bool g_se_saveme = true;

// Communications from the DLL to and from Hypseus
struct       singe_out_info  g_SingeOut;
const struct singe_in_info  *g_pSingeIn = NULL;

// Internal data to keep track of things
SDL_Color             g_colorForeground     = {255, 255, 255, 0};
SDL_Color             g_colorBackground     = {0, 0, 0, 0};
vector<TTF_Font *>    g_fontList;
vector<g_soundT>      g_soundList;
vector<SDL_Surface *> g_spriteList;
int                   g_fontCurrent         = -1;
int                   g_fontQuality         =  1;
double                g_sep_overlay_scale_x =  1;
double                g_sep_overlay_scale_y =  1;
bool                  g_pause_state         = false; // by RDG2010
bool                  g_init_mute           = false;
bool                  g_upgrade_overlay     = false;
bool                  g_fullsize_overlay    = false;
bool                  g_show_crosshair      = true;
bool                  g_not_cursor          = true;

int (*g_original_prepare_frame)(uint8_t *Yplane, uint8_t *Uplane, uint8_t *Vplane,
               int Ypitch, int Upitch, int Vpitch);

////////////////////////////////////////////////////////////////////////////////

extern "C"
{
SINGE_EXPORT const struct singe_out_info *singeproxy_init(const struct singe_in_info *in_info)
{
	const struct singe_out_info *result = NULL;

	g_pSingeIn = in_info;
	
	g_SingeOut.uVersion = SINGE_INTERFACE_API_VERSION;

	g_SingeOut.sep_call_lua            = sep_call_lua;
	g_SingeOut.sep_do_blit             = sep_do_blit;
	g_SingeOut.sep_do_mouse_move       = sep_do_mouse_move;
	g_SingeOut.sep_error               = sep_error;
	g_SingeOut.sep_print               = sep_print;
	g_SingeOut.sep_set_static_pointers = sep_set_static_pointers;
	g_SingeOut.sep_set_surface         = sep_set_surface;
	g_SingeOut.sep_shutdown            = sep_shutdown;
	g_SingeOut.sep_startup             = sep_startup;
	g_SingeOut.sep_alter_lua_clock     = sep_alter_lua_clock;
	g_SingeOut.sep_mute_vldp_init      = sep_mute_vldp_init;
	g_SingeOut.sep_no_crosshair        = sep_no_crosshair;
	g_SingeOut.sep_upgrade_overlay     = sep_upgrade_overlay;
	g_SingeOut.sep_overlay_resize      = sep_overlay_resize;
	
	result = &g_SingeOut;
	
	return result;
}
}

////////////////////////////////////////////////////////////////////////////////

unsigned char sep_byte_clip(int value)
{
	int result;
	
	result = value;
	if (result < 0)   result = 0;
	if (result > 255) result = 255;
	
	return (unsigned char)result;
}

void sep_set_retropath()
{
       lua_set_retropath(true);
}

void sep_call_lua(const char *func, const char *sig, ...)
{
	va_list vl;
	int narg, nres;  /* number of arguments and results */
	int popCount;
	const int top = lua_gettop(g_se_lua_context);
	static unsigned char e;
	
	va_start(vl, sig);
	
	/* get function */
	lua_getglobal(g_se_lua_context, func);
	if (!lua_isfunction(g_se_lua_context, -1)) {
		// Function does not exist.  Bail.
		lua_settop(g_se_lua_context, top);
		return;
	}

	/* push arguments */
	narg = 0;
	while (*sig) {  /* push arguments */
		switch (*sig++) {
		case 'd':  /* double argument */
			lua_pushnumber(g_se_lua_context, va_arg(vl, double));
			break;
		case 'i':  /* int argument */
			lua_pushnumber(g_se_lua_context, va_arg(vl, int));
			break;
		case 's':  /* string argument */
			lua_pushstring(g_se_lua_context, va_arg(vl, char *));
			break;
		case '>':
			goto endwhile;
		default:
			sep_error("invalid option (%c)", *(sig - 1));
		}
		narg++;
		luaL_checkstack(g_se_lua_context, 1, "too many arguments");
	} endwhile:
    
	/* do the call */
	popCount = nres = strlen(sig);  /* number of expected results */
	if (lua_pcall(g_se_lua_context, narg, nres, 0) != 0) { /* do the call */
		sep_print("error running function '%s': %s", func, lua_tostring(g_se_lua_context, -1));
		if (e) { sep_die("Multiple errors, cannot continue..."); exit(SINGE_ERROR_RUNTIME); }
		e++;
		return;
	}
	
	/* retrieve results */
	nres = -nres;  /* stack index of first result */
	while (*sig) {  /* get results */
		switch (*sig++) {

		case 'd':  /* double result */
			if (!lua_isnumber(g_se_lua_context, nres))
				sep_error("wrong result type");
			*va_arg(vl, double *) = lua_tonumber(g_se_lua_context, nres);
			break;
		case 'i':  /* int result */
			if (!lua_isnumber(g_se_lua_context, nres))
				sep_error("wrong result type");
			*va_arg(vl, int *) = (int)lua_tonumber(g_se_lua_context, nres);
			break;
		case 's':  /* string result */
			if (!lua_isstring(g_se_lua_context, nres))
				sep_error("wrong result type");
			*va_arg(vl, const char **) = lua_tostring(g_se_lua_context, nres);
			break;
		default:
			sep_error("invalid option (%c)", *(sig - 1));
		}
		nres++;
	}
	va_end(vl);
	e = 0;
	
	if (popCount > 0)
		lua_pop(g_se_lua_context, popCount);
}

void sep_capture_vldp()
{
	// Intercept VLDP callback
	g_original_prepare_frame = g_pSingeIn->g_local_info->prepare_frame;
	g_pSingeIn->g_local_info->prepare_frame = sep_prepare_frame_callback;
}

void sep_die(const char *fmt, ...)
{
	char message[2048];
	char temp[2048];
	
	strcpy(message, "SINGE: ");

	va_list argp;
	va_start(argp, fmt);
	vsnprintf(temp, sizeof(temp), fmt, argp);

	strcat(message, temp);

	if (g_se_saveme) {
		sound::play_saveme();
		SDL_Delay(1000);
		g_se_saveme = false;
		g_pSingeIn->set_singe_errors(SINGE_ERROR_RUNTIME);
	}

	// tell hypseus what our last error was ...
	g_pSingeIn->set_last_error(message);

	// force (clean) shutdown
	g_pSingeIn->set_quitflag();
}

void sep_do_blit(SDL_Surface *srfDest)
{
	if (g_upgrade_overlay)
	    sep_format_srf32(g_se_surface, srfDest);
	else
	    sep_srf32_to_srf8(g_se_surface, srfDest);
}

void sep_do_mouse_move(Uint16 x, Uint16 y, Sint16 xrel, Sint16 yrel, Sint8 mouseID)
{
	static bool debounced = false;
	int x1 = (int)x;
	int y1 = (int)y;
	int xr = (int)xrel;
	int yr = (int)yrel;
	int mID = (int) mouseID;
	
	// Not sure what's wrong here.  I think things are getting started before Singe is ready.
	if (!debounced) {
		debounced = true;
		return;
	}
	
	x1 *= g_sep_overlay_scale_x;
	y1 *= g_sep_overlay_scale_y;
	xr *= g_sep_overlay_scale_x;
	yr *= g_sep_overlay_scale_y;
	
	sep_call_lua("onMouseMoved", "iiiii", x1, y1, xr, yr, mID);
}

void sep_error(const char *fmt, ...)
{
	char message[2048];
	sep_print("Script Error!");
	va_list argp;
	va_start(argp, fmt);
	vsnprintf(message, sizeof(message), fmt, argp);
	lua_close(g_se_lua_context);
	sep_die(message);
}

int sep_lua_error(lua_State *L)
{
	lua_Debug ar;
	int level = 0;

	sep_print("Singe has paniced!  Very bad!");
	sep_print("Error:  %s", lua_tostring(L, -1));

	sep_print("Stack trace:");
	while (lua_getstack(L, level, &ar) != 0)
	{
		lua_getinfo(L, "nSl", &ar);
		sep_print(" %d: function `%s' at line %d %s", level, ar.name, ar.currentline, ar.short_src);
		level++;
	}
	sep_print("Trace complete.");

	return 0;
}

int sep_prepare_frame_callback(uint8_t *Yplane, uint8_t *Uplane, uint8_t *Vplane,
                           int Ypitch, int Upitch, int Vpitch)
{
	int result = VLDP_FALSE;

	result = (video::vid_update_yuv_overlay(Yplane, Uplane, Vplane, Ypitch, Upitch, Vpitch) == 0)
		? VLDP_TRUE
		: VLDP_FALSE;

	return result;
}

void sep_print(const char *fmt, ...)
{
	char message[2048];
	char temp[2048];
	
	strcpy(message, "SINGE: ");

	va_list argp;
	va_start(argp, fmt);
	vsnprintf(temp, sizeof(temp), fmt, argp);
	
	strcat(message, temp);
	
	// Send all our output through Matt's code.
	g_pSingeIn->printline(message);
}

void sep_release_vldp()
{
	g_pSingeIn->g_local_info->prepare_frame = g_original_prepare_frame;
}

void sep_set_static_pointers(double *m_disc_fps, unsigned int *m_uDiscFPKS)
{
  g_se_disc_fps = m_disc_fps;
  g_se_uDiscFPKS = m_uDiscFPKS;
}

void sep_set_surface(int width, int height)
{
	bool createSurface = false;
	
	g_se_overlay_height = height;
	g_se_overlay_width = width;
	
	if (g_se_surface == NULL) {
		createSurface = true;
	} else {
		if ((g_se_surface->w != g_se_overlay_width) || (g_se_surface->h != g_se_overlay_height))
		{
			SDL_FreeSurface(g_se_surface);
			createSurface = true;
		}
	}
	
	if (createSurface) {
		g_se_surface = SDL_CreateRGBSurface(0, g_se_overlay_width, g_se_overlay_height, 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000);
		g_sep_overlay_scale_x = (double)g_se_overlay_width / (double)g_pSingeIn->get_video_width();
		g_sep_overlay_scale_y = (double)g_se_overlay_height / (double)g_pSingeIn->get_video_height();
	}
}

void sep_shutdown(void)
{
	sep_release_vldp();
	
	sep_unload_fonts();
	sep_unload_sounds();
	sep_unload_sprites();
	
  TTF_Quit();

  if (g_bLuaInitialized)
  {
	lua_close(g_se_lua_context);
	g_bLuaInitialized = false;
  }
}

void sep_sound_ended(Uint8 *buffer, unsigned int slot)
{
	/*
	* by RDG2010
	* This function is triggered by a callback when a sound finishes playing
	
	// Following lines are for debug purposes:

	char s[100];
	snprintf(s, sizeof(s), "sep_sound_ended:  slot numer is %u", slot);
	sep_print(s);
	
	///////////////////////////
	*/

	sep_call_lua("onSoundCompleted", "i", slot);
	
}

bool sep_srf32_to_srf8(SDL_Surface *src, SDL_Surface *dst)
{
	bool bResult = false;
	
	// convert a 32-bit surface to an 8-bit surface (where the palette is one we've defined)

	// safety check
	if (
		// if source and destination surfaces are the same dimensions
		((dst->w == src->w) && (dst->h == src->h)) && 
		// and destination is 8-bit
		(dst->format->BitsPerPixel == 8) &&
		// and source is 32-bit
		(src->format->BitsPerPixel == 32)
	)
	{
		SDL_LockSurface(dst);
		SDL_LockSurface(src);

		void *pSrcLine = src->pixels;
		void *pDstLine = dst->pixels;
		for (unsigned int uRowIdx = 0; uRowIdx < (unsigned int) src->h; ++uRowIdx)
		{
			Uint32 *p32SrcPix = (Uint32 *) pSrcLine;
			Uint8 *p8DstPix = (Uint8 *) pDstLine;

			// do one line
			for (unsigned int uColIdx = 0; uColIdx < (unsigned int) src->w; ++uColIdx)
			{
				// get source pixel ...
				Uint32 u32SrcPix = *p32SrcPix;

				Uint8 u8B = (u32SrcPix & src->format->Bmask) >> src->format->Bshift;
				Uint8 u8G = (u32SrcPix & src->format->Gmask) >> src->format->Gshift;
				Uint8 u8R = (u32SrcPix & src->format->Rmask) >> src->format->Rshift;
				Uint8 u8A = (u32SrcPix & src->format->Amask) >> src->format->Ashift;

				u8B &= 0xE0;  // blue has 3 bits (8 shades)
				u8G &= 0xC0;  // green has 2 bits
				u8R &= 0xE0;  // red has 3 bits

				// compute 8-bit index
				Uint8 u8Idx = u8R | (u8G >> 3) | (u8B >> 5);

				if (u8Idx > 0xFE) u8Idx--;

				// if alpha channel is more opaque, then make it fully opaque
				if (u8A > 0x7F)
				{
					// if resulting index is 0, we have to change it because 0 is reserved for transparent
					if (u8Idx == 0)
					{
						// 1 becomes the replacement (and will be black)
						u8Idx = 1;
					}
					// else leave it alone
				}
				// else make it fully transparent
				else
				{
					u8Idx = 0;
				}

				// store computed value
				*p8DstPix = u8Idx;

				++p8DstPix;	// go to the next one ...
				++p32SrcPix;	// go to the next one ...
			} // end doing current line

			pSrcLine = ((Uint8 *) pSrcLine) + src->pitch;	// go to the next line
			pDstLine = ((Uint8 *) pDstLine) + dst->pitch;	// " " "
		} // end doing all rows

		SDL_UnlockSurface(src);
		SDL_UnlockSurface(dst);

		bResult = true;
	}

	return bResult;
}

bool sep_format_srf32(SDL_Surface *src, SDL_Surface *dst)
{
	bool bResult = false;

	// cleanup 32-bit surface

	if (
		((dst->w == src->w) && (dst->h == src->h)) &&
		(dst->format->BitsPerPixel == 32) &&
		(src->format->BitsPerPixel == 32)
	)
	{
		SDL_LockSurface(dst);
		SDL_LockSurface(src);

		void *pSrcLine = src->pixels;
		void *pDstLine = dst->pixels;
		for (unsigned int uRowIdx = 0; uRowIdx < (unsigned int) src->h; ++uRowIdx)
		{
			Uint32 *p32SrcPix = (Uint32 *) pSrcLine;
			Uint32 *p32DstPix = (Uint32 *) pDstLine;

			for (unsigned int uColIdx = 0; uColIdx < (unsigned int) src->w; ++uColIdx)
			{
				Uint32 u32SrcPix = *p32SrcPix;

				Uint8 u32A = (u32SrcPix & src->format->Amask) >> src->format->Ashift;
				Uint8 u32R = (u32SrcPix & src->format->Rmask) >> src->format->Rshift;
				Uint8 u32G = (u32SrcPix & src->format->Gmask) >> src->format->Gshift;
				Uint8 u32B = (u32SrcPix & src->format->Bmask) >> src->format->Bshift;

				Uint32 u32Idx = (u32A << 24) | (u32R << 16) | (u32G << 8) | u32B;

				if (u32A > 0x7F)
				{
					if (u32Idx == 0)
						u32Idx = 1;
				}
				else
				{
					u32Idx = 0;
				}

				*p32DstPix = u32Idx;
				++p32DstPix;
				++p32SrcPix;
			}
			pSrcLine = ((Uint8 *) pSrcLine) + src->pitch;
			pDstLine = ((Uint8 *) pDstLine) + dst->pitch;
		}
		SDL_UnlockSurface(src);
		SDL_UnlockSurface(dst);

		bResult = true;
	}
	return bResult;
}

void sep_startup(const char *script)
{
  g_se_lua_context = lua_open();
  luaL_openlibs(g_se_lua_context);
  lua_atpanic(g_se_lua_context, sep_lua_error);

  lua_register(g_se_lua_context, "colorBackground",    sep_color_set_backcolor);
  lua_register(g_se_lua_context, "colorForeground",    sep_color_set_forecolor);

  lua_register(g_se_lua_context, "hypseusGetHeight",   sep_hypseus_get_height);
  lua_register(g_se_lua_context, "hypseusGetWidth",    sep_hypseus_get_width);

  lua_register(g_se_lua_context, "debugPrint",         sep_debug_say);

  lua_register(g_se_lua_context, "discAudio",          sep_audio_control);
  lua_register(g_se_lua_context, "discChangeSpeed",    sep_change_speed);
  lua_register(g_se_lua_context, "discGetFrame",       sep_get_current_frame);
  lua_register(g_se_lua_context, "discPause",          sep_pause);
  lua_register(g_se_lua_context, "discPlay",           sep_play);
  lua_register(g_se_lua_context, "discSearch",         sep_search);
  lua_register(g_se_lua_context, "discSearchBlanking", sep_search_blanking);
  lua_register(g_se_lua_context, "discSetFPS",         sep_set_disc_fps);
  lua_register(g_se_lua_context, "discSkipBackward",   sep_skip_backward);
  lua_register(g_se_lua_context, "discSkipBlanking",   sep_skip_blanking);
  lua_register(g_se_lua_context, "discSkipForward",    sep_skip_forward);
  lua_register(g_se_lua_context, "discSkipToFrame",    sep_skip_to_frame);
  lua_register(g_se_lua_context, "discStepBackward",   sep_step_backward);
  lua_register(g_se_lua_context, "discStepForward",    sep_step_forward);
  lua_register(g_se_lua_context, "discStop",           sep_stop);

  lua_register(g_se_lua_context, "fontLoad",           sep_font_load);
  lua_register(g_se_lua_context, "fontPrint",          sep_say_font);
  lua_register(g_se_lua_context, "fontQuality",        sep_font_quality);
  lua_register(g_se_lua_context, "fontSelect",         sep_font_select);
  lua_register(g_se_lua_context, "fontToSprite",       sep_font_sprite);

  lua_register(g_se_lua_context, "overlayClear",       sep_overlay_clear);
  lua_register(g_se_lua_context, "overlayGetHeight",   sep_get_overlay_height);
  lua_register(g_se_lua_context, "overlayGetWidth",    sep_get_overlay_width);
  lua_register(g_se_lua_context, "overlayPrint",       sep_say);
	
  lua_register(g_se_lua_context, "soundLoad",        sep_sound_load);
  lua_register(g_se_lua_context, "soundPlay",        sep_sound_play);
  lua_register(g_se_lua_context, "soundPause",       sep_sound_pause);
  lua_register(g_se_lua_context, "soundResume",      sep_sound_resume);
  lua_register(g_se_lua_context, "soundIsPlaying",   sep_sound_get_flag);
  lua_register(g_se_lua_context, "soundStop",        sep_sound_stop);
  lua_register(g_se_lua_context, "soundFullStop",    sep_sound_flush_queue);

  lua_register(g_se_lua_context, "spriteDraw",       sep_sprite_draw);
  lua_register(g_se_lua_context, "spriteGetHeight",  sep_sprite_height);
  lua_register(g_se_lua_context, "spriteGetWidth",   sep_sprite_width);
  lua_register(g_se_lua_context, "spriteLoad",       sep_sprite_load);

  lua_register(g_se_lua_context, "vldpGetHeight",      sep_mpeg_get_height);
  lua_register(g_se_lua_context, "vldpGetPixel",       sep_mpeg_get_pixel);
  lua_register(g_se_lua_context, "vldpGetWidth",       sep_mpeg_get_width);
  lua_register(g_se_lua_context, "vldpSetVerbose",     sep_ldp_verbose);  

  // Singe 2
  lua_register(g_se_lua_context, "overlaySetResolution",   sep_singe_two_pseudo_call_true);
  lua_register(g_se_lua_context, "singeSetGameName",       sep_singe_two_pseudo_call_true);
  lua_register(g_se_lua_context, "onOverlayUpdate",        sep_singe_two_pseudo_call_true);
  lua_register(g_se_lua_context, "singeWantsCrosshairs",   sep_singe_wants_crosshair);
  lua_register(g_se_lua_context, "mouseHowMany",           sep_get_number_of_mice);

  // Hypseus API
  lua_register(g_se_lua_context, "ratioGetX",              sep_get_xratio);
  lua_register(g_se_lua_context, "ratioGetY",              sep_get_yratio);
  lua_register(g_se_lua_context, "setOverlaySize",         sep_set_overlaysize);
  lua_register(g_se_lua_context, "setOverlayResolution",   sep_set_custom_overlay);
  lua_register(g_se_lua_context, "takeScreenshot",         sep_screenshot);

  lua_register(g_se_lua_context, "scoreBezelEnable",       sep_bezel_enable);
  lua_register(g_se_lua_context, "scoreBezelClear",        sep_bezel_clear);
  lua_register(g_se_lua_context, "scoreBezelCredits",      sep_bezel_credits);
  lua_register(g_se_lua_context, "scoreBezelTwinScoreOn",  sep_bezel_second_score);
  lua_register(g_se_lua_context, "scoreBezelScore",        sep_bezel_player_score);
  lua_register(g_se_lua_context, "scoreBezelLives",        sep_bezel_player_lives);

  // by RDG2010
  lua_register(g_se_lua_context, "keyboardGetMode",    sep_keyboard_get_mode); 
  lua_register(g_se_lua_context, "keyboardSetMode",    sep_keyboard_set_mode);
  lua_register(g_se_lua_context, "discGetState",       sep_get_vldp_state);  
  lua_register(g_se_lua_context, "singeGetPauseFlag",  sep_get_pause_flag);
  lua_register(g_se_lua_context, "singeSetPauseFlag",  sep_set_pause_flag);
  lua_register(g_se_lua_context, "singeQuit",          sep_singe_quit);
  lua_register(g_se_lua_context, "singeVersion",       sep_singe_version);  
  
  //////////////////

  if (TTF_Init() < 0)
  {
    sep_die("Unable to initialize font library.");
  }
	sep_capture_vldp();

	g_bLuaInitialized = true;

  if (g_pSingeIn->get_retro_path()) sep_set_retropath();

  if (luaL_dofile(g_se_lua_context, script) != 0)
  {
	sep_error("error compiling script: %s", lua_tostring(g_se_lua_context, -1));
	sep_die("Cannot continue, quitting...");
	g_bLuaInitialized = false;
  }
}


void sep_unload_fonts(void)
{
  int x;

  if (g_fontList.size() > 0)
	{
    for (x=0; x<(int)g_fontList.size(); x++)
      TTF_CloseFont(g_fontList[x]);
		g_fontList.clear();
	}
}

void sep_unload_sounds(void)
{
  int x;

  g_pSingeIn->samples_flush_queue();

  if (g_soundList.size() > 0)
	{
    for (x=0; x<(int)g_soundList.size(); x++)
			SDL_FreeWAV(g_soundList[x].buffer);
		g_soundList.clear();
	}
}

void sep_unload_sprites(void)
{
  int x;

  if (g_spriteList.size() > 0)
	{
    for (x=0; x<(int)g_spriteList.size(); x++)
			SDL_FreeSurface(g_spriteList[x]);
		g_spriteList.clear();
	}
}

void sep_alter_lua_clock(bool s)
{
   if (s)
       os_alter_clocker(LUA_HI);
   else
       os_alter_clocker(LUA_LO);
}

void sep_mute_vldp_init(void)
{
   g_init_mute = true;
   sep_print("Booting initVLDP() silently");
}

void sep_no_crosshair(void)
{
  g_show_crosshair = false;
}

void sep_upgrade_overlay(void)
{
  g_upgrade_overlay = true;
}

void sep_overlay_resize(void)
{
  g_fullsize_overlay = true;
}

////////////////////////////////////////////////////////////////////////////////

// Singe API Calls

static int sep_audio_control(lua_State *L)
{
  int n = lua_gettop(L);
  int channel = 0;
  bool onOff = false;
  
  if (n == 2)
    if (lua_isnumber(L, 1))
      if (lua_isboolean(L, 2))
      {
        channel = lua_tonumber(L, 1);
        onOff = lua_toboolean(L, 2);
        if (onOff)
          if (channel == 1)
		  {
            g_pSingeIn->enable_audio1();
		  }
          else
		  {
           g_pSingeIn->enable_audio2();
		  }
        else
          if (channel == 1)
		  {
           g_pSingeIn->disable_audio1();
		  }
          else
		  {
           g_pSingeIn->disable_audio2();
		  }
      }

  return 0;
}

static int sep_change_speed(lua_State *L)
{
  int n = lua_gettop(L);
  
  if (n == 2)
    if (lua_isnumber(L, 1))
      if (lua_isnumber(L, 2))
	  {
        g_pSingeIn->pre_change_speed(lua_tonumber(L, 1), lua_tonumber(L, 2));
	  }

  return 0;
}

static int sep_color_set_backcolor(lua_State *L)
{
  int n = lua_gettop(L);
  
  if (n == 3)
    if (lua_isnumber(L, 1))
      if (lua_isnumber(L, 2))
		if (lua_isnumber(L, 3))
		{
			g_colorBackground.r = (char)lua_tonumber(L, 1);
			g_colorBackground.g = (char)lua_tonumber(L, 2);
			g_colorBackground.b = (char)lua_tonumber(L, 3);
			g_colorBackground.a = (char)0;
		}
					
	return 0;
}

static int sep_color_set_forecolor(lua_State *L)
{
  int n = lua_gettop(L);
  
  if (n == 3)
    if (lua_isnumber(L, 1))
      if (lua_isnumber(L, 2))
		if (lua_isnumber(L, 3))
		{
			g_colorForeground.r = (char)lua_tonumber(L, 1);
			g_colorForeground.g = (char)lua_tonumber(L, 2);
			g_colorForeground.b = (char)lua_tonumber(L, 3);
			g_colorForeground.a = (char)0;
		}

	return 0;
}

static int sep_hypseus_get_height(lua_State *L)
{
  lua_pushnumber(L, g_pSingeIn->get_video_height());
  return 1;
}

static int sep_hypseus_get_width(lua_State *L)
{
  lua_pushnumber(L, g_pSingeIn->get_video_width());
  return 1;
}

static int sep_debug_say(lua_State *L)
{
  int n = lua_gettop(L);
	
  if (n == 1)
    if (lua_isstring(L, 1))
			sep_print("%s", lua_tostring(L, 1));
			
	return 0;
}

static int sep_font_load(lua_State *L)
{
    int n      = lua_gettop(L);
    int result = -1;

    if (n == 2 && lua_type(L, 1) == LUA_TSTRING && lua_type(L, 2) == LUA_TNUMBER) {
        std::string fontpath = lua_tostring(L, 1);

        if (g_pSingeIn->get_retro_path()) {
            char filepath[RETRO_MAXPATH] = {0};
            int len = std::min((int)fontpath.size() + RETRO_PAD, RETRO_MAXPATH);
            lua_retropath(fontpath.c_str(), filepath, len);
            fontpath = filepath;
        }

        int points = lua_tonumber(L, 2);

        // Load this font.
        TTF_Font *temp = TTF_OpenFont(fontpath.c_str(), points);
        if (temp) {
            // Make it the current font and mark it as loaded.
            g_fontList.push_back(temp);
            g_fontCurrent = g_fontList.size() - 1;
            result        = g_fontCurrent;
        } else {
            sep_die("Unable to load font: %s", fontpath.c_str());
        }
    }

    lua_pushnumber(L, result);
    return 1;
}

static int sep_font_quality(lua_State *L)
{
  int n = lua_gettop(L);

  if (n == 1)
    if (lua_isnumber(L, 1))
      g_fontQuality = lua_tonumber(L, 1);

  return 0;
}

static int sep_font_select(lua_State *L)
{
  int n = lua_gettop(L);
  int fontIndex = -1;

  if (n == 1)
    if (lua_isnumber(L, 1))
    {
      fontIndex = lua_tonumber(L, 1);
      if (fontIndex < (int)g_fontList.size())
        g_fontCurrent = fontIndex;
    }

  return 0;
}

static int sep_font_sprite(lua_State *L)
{
  int n = lua_gettop(L);
	int result = -1;
	
  if (n == 1)
	if (lua_isstring(L, 1))
		if (g_fontCurrent >= 0) {
			SDL_Surface *textsurface = NULL;
			textsurface = SDL_ConvertSurface(textsurface, g_se_surface->format, 0);
			const char *message = lua_tostring(L, 1);
			TTF_Font *font = g_fontList[g_fontCurrent];

			switch (g_fontQuality) {
			case 1:
				textsurface = TTF_RenderText_Solid(font, message, g_colorForeground);
				break;
			case 2:
				textsurface = TTF_RenderText_Shaded(font, message, g_colorForeground, g_colorBackground);
				break;
			case 3:
				textsurface = TTF_RenderText_Blended(font, message, g_colorForeground);
				break;
			}

			if (!(textsurface)) {
				sep_die("Font surface is null!");
			} else {

				SDL_SetSurfaceRLE(textsurface, SDL_TRUE);
				SDL_SetColorKey(textsurface, SDL_TRUE, 0x0);

				g_spriteList.push_back(textsurface);
				result = g_spriteList.size() - 1;
			}
		}
  lua_pushnumber(L, result);
  return 1;
}

static int sep_get_current_frame(lua_State *L)
{
  lua_pushnumber(L, g_pSingeIn->get_current_frame());
  return 1;
}

static int sep_get_overlay_height(lua_State *L)
{
  lua_pushnumber(L, g_se_overlay_height);
  return 1;
}

static int sep_get_overlay_width(lua_State *L)
{
  lua_pushnumber(L, g_se_overlay_width);
  return 1;
}

static int sep_mpeg_get_height(lua_State *L)
{
  lua_pushnumber(L, g_pSingeIn->g_vldp_info->h);
  return 1;
}

static int sep_mpeg_get_pixel(lua_State *L)
{
        Uint32 format;
        int32_t  n                   = lua_gettop(L);
        bool     result              = false;
        static bool ex               = false;
        SDL_Renderer *g_renderer     = video::get_renderer();
        SDL_Texture  *g_texture      = video::get_yuv_screen();
        SDL_QueryTexture(g_texture, &format, NULL, NULL, NULL);
        unsigned char pixel[SDL_BYTESPERPIXEL(format)];
        unsigned char R;
        unsigned char G;
        unsigned char B;
        SDL_Rect rect;
        int Y;
        int U;
        int V;

        if (n == 2) {
                if (lua_isnumber(L, 1)) {
                        if (lua_isnumber(L, 2)) {

				rect.h = 1;
				rect.w = 1;
				if (!ex) sep_print("sep_mpeg_get_pixel()");
				rect.x = (int)((double)lua_tonumber(L, 1) * ((double)g_pSingeIn->g_vldp_info->w
                                              / (double)g_se_overlay_width));
				rect.y = (int)((double)lua_tonumber(L, 2) * ((double)g_pSingeIn->g_vldp_info->h
                                              / (double)g_se_overlay_height));
				if (g_renderer && g_texture) {
					if (SDL_SetRenderTarget(g_renderer, g_texture) < 0) {
                                            if (!ex) {
						sep_print("get_pixel unsupported texture: Targets disabled");
						sep_print("Could not RenderTarget in get_pixel: %s", SDL_GetError());
                                            }
                                            lua_State* X = luaL_newstate();
                                            lua_pushinteger(X, 70);
                                            lua_pushinteger(X, 10);
                                            lua_pushstring(X, "Targets disabled");
                                            sep_say_font(X);
					} else {
                                            if (SDL_RenderReadPixels(g_renderer, &rect, format,
                                                              pixel, SDL_BYTESPERPIXEL(format)) < 0)
						  sep_die("Could not ReadPixel in get_pixel: %s", SDL_GetError());
					}
					SDL_SetRenderTarget(g_renderer, NULL);
				} else {
					sep_die("Could not initialize get_pixel");
				}
				Y = pixel[0] - 16;
				U = (int)rand()% 6 + (-3);
				V = (int)rand()% 6 + (-3);
				R = sep_byte_clip(( 298 * Y           + 409 * V + 128) >> 8);
				G = sep_byte_clip(( 298 * Y - 100 * U - 208 * V + 128) >> 8);
				B = sep_byte_clip(( 298 * Y + 516 * U           + 128) >> 8);
				result = true;
			}
		}
	}

	if (result) {
		lua_pushnumber(L, (int)R);
		lua_pushnumber(L, (int)G);
		lua_pushnumber(L, (int)B);
	} else {
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
	}
	ex = true;
	return 3;
}


static int sep_singe_two_pseudo_call_true(lua_State *L)
{
   return 0;
}

static int sep_set_custom_overlay(lua_State *L)
{
    int n = lua_gettop(L);

    if (n == 2) {
        if (lua_isnumber(L, 1)) {
            if (lua_isnumber(L, 2)) {

                double f = 0;
                f = lua_tonumber(L, 1);
                int w = (int)f;
                f = lua_tonumber(L, 2);
                int h = (int)f;

                lua_State* R = luaL_newstate();
                lua_pushinteger(R, SINGE_OVERLAY_CUSTOM);
                lua_pushinteger(R, w);
                lua_pushinteger(R, h);

                sep_set_overlaysize(R);
            }
        }
    }
    return 0;
}

static int sep_set_overlaysize(lua_State *L)
{
    int n = lua_gettop(L);

    if (n > 0) {
        if (lua_isnumber(L, 1))
        {
           int size = lua_tonumber(L, 1);

           switch (size) {
           case SINGE_OVERLAY_FULL:
           case SINGE_OVERLAY_HALF:
               g_fullsize_overlay = true;
               g_pSingeIn->cfm_set_upgradeoverlay(g_pSingeIn->pSingeInstance, true);
               break;
           case SINGE_OVERLAY_OVERSIZE:
               g_fullsize_overlay = false;
               g_pSingeIn->cfm_set_upgradeoverlay(g_pSingeIn->pSingeInstance, false);
               break;
           case SINGE_OVERLAY_CUSTOM:
               if (n == 3) {
                   if (lua_isnumber(L, 2)) {
                       if (lua_isnumber(L, 3)) {
                           double f = 0;
                           f = lua_tonumber(L, 2);
                           int w = (int)f;
                           f = lua_tonumber(L, 3);
                           int h = (int)f;
                           if (w && h) {
                               g_fullsize_overlay = true;
                               g_pSingeIn->cfm_set_custom_overlay(g_pSingeIn->pSingeInstance, w, h);
                               g_pSingeIn->cfm_set_upgradeoverlay(g_pSingeIn->pSingeInstance, true);
                           } else size = SINGE_OVERLAY_EMPTY;
                       }
                   }
               } else size = SINGE_OVERLAY_EMPTY;
               break;
           }

           if (size > 0 && size < SINGE_OVERLAY_EMPTY) {
               g_pSingeIn->cfm_set_overlaysize(g_pSingeIn->pSingeInstance, size);
           }
       }
   }
   return 0;
}

static int sep_get_xratio(lua_State *L)
{
   lua_pushnumber(L, g_pSingeIn->cfm_get_xratio(g_pSingeIn->pSingeInstance));
   return 1;
}

static int sep_get_yratio(lua_State *L)
{
   lua_pushnumber(L, g_pSingeIn->cfm_get_yratio(g_pSingeIn->pSingeInstance));
   return 1;
}

static int sep_singe_wants_crosshair(lua_State *L)
{
   lua_pushboolean(L, g_show_crosshair);
   if (g_show_crosshair) g_not_cursor = false;
   return 1;
}

static int sep_mpeg_get_width(lua_State *L)
{
  lua_pushnumber(L, g_pSingeIn->g_vldp_info->w);
  return 1;
}

static int sep_overlay_clear(lua_State *L)
{
	SDL_FillRect(g_se_surface, NULL, 0);
	return 0;
}

static int sep_pause(lua_State *L)
{
  g_pSingeIn->pre_pause();
  g_pause_state = true; // by RDG2010

  return 0;
}

static int sep_play(lua_State *L)
{
  g_pSingeIn->pre_play();
  g_pause_state = false; // by RDG2010

  return 0;
}

static int sep_say(lua_State *L)
{
  int n = lua_gettop(L);
	  
  if (n == 3)
    if (lua_isnumber(L, 1))
      if (lua_isnumber(L, 2))
        if (lua_isstring(L, 3))
		g_pSingeIn->draw_string((char *)lua_tostring(L, 3), lua_tonumber(L, 1), lua_tonumber(L, 2), g_se_surface);

  return 0;
}

static int sep_say_font(lua_State *L)
{
  int n = lua_gettop(L);
	  
  if (n == 3)
    if (lua_isnumber(L, 1))
      if (lua_isnumber(L, 2))
        if (lua_isstring(L, 3))
	  if (g_fontCurrent >= 0) {
		SDL_Surface *textsurface = NULL;
		textsurface = SDL_ConvertSurface(textsurface, g_se_surface->format, 0);
		const char *message = lua_tostring(L, 3);
		TTF_Font *font = g_fontList[g_fontCurrent];

		switch (g_fontQuality) {
		case 1:
		    textsurface = TTF_RenderText_Solid(font, message, g_colorForeground);
		    break;
		case 2:
		    textsurface = TTF_RenderText_Shaded(font, message, g_colorForeground, g_colorBackground);
		    break;
		case 3:
		    textsurface = TTF_RenderText_Blended(font, message, g_colorForeground);
		    break;
		}

		if (!(textsurface)) {
			sep_die("Font surface is null!");
		} else {
			SDL_Rect dest;
			dest.w = textsurface->w;
			dest.h = textsurface->h;

			if (g_fullsize_overlay) {

			    dest.x = lua_tonumber(L, 1) + g_sep_overlay_scale_x;
			    dest.y = lua_tonumber(L, 2) + g_sep_overlay_scale_y;

			} else { // Deal with legacy stuff - TODO: remove

			    dest.x = lua_tonumber(L, 1);
			    dest.y = lua_tonumber(L, 2);

			    if (dest.x == 0x05 && dest.y == 0x05 && dest.h == 0x17) // AM SCORE SHUNT
				dest.x+=20;

			    if (g_se_overlay_width > SINGE_OVERLAY_STD_W) {
				if (dest.h == 0x16 && dest.y == 0xcf) { // JR SCOREBOARD
                                           dest.x = dest.x - (double)((g_se_overlay_width + dest.x + dest.w) / 22);
                                           if (dest.x <(SINGE_OVERLAY_STD_W>>2)) dest.x-=4;
                                               if (dest.x >(SINGE_OVERLAY_STD_W>>1)) dest.x+=4;
			    }
				else
				    dest.x = dest.x - (double)(((g_se_overlay_width) + (dest.x * 32)
                                                              + (dest.w * 26)) / SINGE_OVERLAY_STD_W);
			    }
			}

			SDL_SetSurfaceRLE(textsurface, SDL_TRUE);
			SDL_SetColorKey(textsurface, SDL_TRUE, 0x0);
			if (!video::get_singe_blend_sprite())
				SDL_SetSurfaceBlendMode(textsurface, SDL_BLENDMODE_NONE);

			SDL_BlitSurface(textsurface, NULL, g_se_surface, &dest);
			SDL_FreeSurface(textsurface);
		}
	}
  return 0;
}

static int sep_screenshot(lua_State *L)
{
  static int last = 0;
  double now = clock() / (double)CLOCKS_PER_SEC;

  if (((int)now - last) < 5) return 1;

  int n = lua_gettop(L);

  if (n == 0)
  {
    g_pSingeIn->request_screenshot();
    last = (int)now;
  }
	
  return 0;
}

static int sep_search(lua_State *L)
{
  static bool debounced = false;
  char s[7] = { 0 };
  int n = lua_gettop(L);

  if (n == 1)
    if (lua_isnumber(L, 1))
    {
      g_pSingeIn->framenum_to_frame(lua_tonumber(L, 1), s);
      g_pSingeIn->pre_search(s, true);

      if (g_pSingeIn->g_local_info->blank_during_searches)
          if (debounced)
              video::set_video_timer_blank(true);

      debounced = true;
    }

  return 0;
}

static int sep_search_blanking(lua_State *L)
{
  int n = lua_gettop(L);
  
  if (n == 1)
    if (lua_isboolean(L, 1))
	{
          g_pSingeIn->set_search_blanking(lua_toboolean(L, 1));
	}

  return 0;
}

static int sep_set_disc_fps(lua_State *L)
{
  int n = lua_gettop(L);

  if (g_init_mute) {
      g_pSingeIn->disable_audio1();
      g_pSingeIn->disable_audio2();
  }

  if (n == 1)
    if (lua_isnumber(L, 1))
    {
      *g_se_disc_fps = lua_tonumber(L, 1);
	    if (*g_se_disc_fps != 0.0)
		    *g_se_uDiscFPKS = (unsigned int) ((*g_se_disc_fps * 1000.0) + 0.5);	// frames per kilosecond (same precision, but an int instead of a float)
    }

  return 0;
}

static int sep_skip_backward(lua_State *L)
{
  int n = lua_gettop(L);

  if (n == 1)
    if (lua_isnumber(L, 1))
	{
          if (g_pSingeIn->g_local_info->blank_during_skips)
              video::set_video_timer_blank(true);

          g_pSingeIn->pre_skip_backward(lua_tonumber(L, 1));
	}
      
  return 0;
}

static int sep_skip_blanking(lua_State *L)
{
  int n = lua_gettop(L);
  
  if (n == 1)
    if (lua_isboolean(L, 1))
	{
           g_pSingeIn->set_skip_blanking(lua_toboolean(L, 1));
	}

  return 0;
}

static int sep_skip_forward(lua_State *L)
{
  int n = lua_gettop(L);

  if (n == 1)
    if (lua_isnumber(L, 1))
	{
          if (g_pSingeIn->g_local_info->blank_during_skips)
              video::set_video_timer_blank(true);

          g_pSingeIn->pre_skip_forward(lua_tonumber(L, 1));
	}
      
  return 0;
}

static int sep_skip_to_frame(lua_State *L)
{
	int n = lua_gettop(L);
	static bool debounced = false;

	if (g_init_mute && debounced) {
            g_pSingeIn->enable_audio1();
            g_pSingeIn->enable_audio2();
            g_init_mute = false;
	}

	if (n == 1)
	{
		if (lua_isnumber(L, 1))
		{
			// TODO : implement this for real on the hypseus side of things instead of having to do a search/play hack
			char s[7] = { 0 };

			if (g_pSingeIn->g_local_info->blank_during_skips)
			    if (debounced)
			        video::set_video_timer_blank(true);

			g_pSingeIn->framenum_to_frame(lua_tonumber(L, 1), s);
			g_pSingeIn->pre_search(s, true);
			g_pSingeIn->pre_play();
			g_pause_state = false; // BY RDG2010
			debounced = true;
		}
	}

	return 0;
}

static int sep_sound_load(lua_State *L)
{
    int n = lua_gettop(L);
    int result = -1;

    if (n == 1 && lua_type(L, 1) == LUA_TSTRING)
    {
        std::string filepath = lua_tostring(L, 1);

        if (g_pSingeIn->get_retro_path())
        {
            char tmpPath[RETRO_MAXPATH] = {0};
            int len = std::min((int)filepath.size() + RETRO_PAD, RETRO_MAXPATH);
            lua_retropath(filepath.c_str(), tmpPath, len);
            filepath = tmpPath;
        }

        g_soundT temp;
        if (SDL_LoadWAV(filepath.c_str(), &temp.audioSpec, &temp.buffer, &temp.length) == NULL) {
            sep_die("Could not open %s: %s", filepath.c_str(), SDL_GetError());
        } else {
            g_soundList.push_back(temp);
            result = g_soundList.size() - 1;
        }
    }

    lua_pushnumber(L, result);
    return 1;
}

static int sep_sound_play(lua_State *L)
{
  int n = lua_gettop(L);
	int result = -1;

  if (n == 1)
    if (lua_isnumber(L, 1))
	{
		int sound = lua_tonumber(L, 1);
		if (sound < (int)g_soundList.size())
		    result = g_pSingeIn->samples_play_sample(g_soundList[sound].buffer, g_soundList[sound].length, g_soundList[sound].audioSpec.channels, -1, sep_sound_ended);
	}
		
	lua_pushnumber(L, result);

  return 1;
}

static int sep_sprite_draw(lua_State *L)
{
  int n = lua_gettop(L);

  if (n == 3) {
    if (lua_isnumber(L, 1)) {
	if (lua_isnumber(L, 2)) {
	    if (lua_isnumber(L, 3))
	    {
		int sprite = lua_tonumber(L, 3);
		if (sprite < (int)g_spriteList.size())
		{
			SDL_Rect dest;
			dest.w = g_spriteList[sprite]->w;
			dest.h = g_spriteList[sprite]->h;

			if (g_fullsize_overlay) {

			    dest.x = lua_tonumber(L, 1) + g_sep_overlay_scale_x;
			    dest.y = lua_tonumber(L, 2) + g_sep_overlay_scale_y;

			} else { // Deal with legacy stuff - TODO: remove

			    dest.x = lua_tonumber(L, 1);
			    dest.y = lua_tonumber(L, 2);

			    if (g_se_overlay_width > SINGE_OVERLAY_STD_W) {
				if (g_not_cursor && dest.y > 0xbe && dest.y <= 0xde)
				    dest.x = dest.x - (double)((g_se_overlay_width + dest.x + dest.w) / 26);
				else
				    dest.x = dest.x - (double)((g_se_overlay_width + (dest.x * 32)
						    + (dest.w * 26)) / SINGE_OVERLAY_STD_W);
			    }
			}

			if (dest.w == 0x89 && dest.h == 0x1c) { // SP
				SDL_SetColorKey(g_spriteList[sprite], SDL_TRUE, 0x000000ff);
				dest.x+=3;
			}

			if ((!video::get_singe_blend_sprite()) &&
				(dest.w != 0xcc && dest.h != 0x15) &&
				    (dest.w != 0x0b && dest.h != 0x0b)) // JR / AM
				SDL_SetSurfaceBlendMode(g_spriteList[sprite], SDL_BLENDMODE_NONE);

			SDL_BlitSurface(g_spriteList[sprite], NULL, g_se_surface, &dest);
                  }
              }
          }
      }
  }
  g_not_cursor = true;
  return 0;
}

static int sep_sprite_height(lua_State *L)
{
  int n = lua_gettop(L);
	int result = -1;

  if (n == 1)
    if (lua_isnumber(L, 1))
		{
			int sprite = lua_tonumber(L, 1);
			if ((sprite < (int)g_spriteList.size()) && (sprite >= 0))
			{
				result = g_spriteList[sprite]->h;
			}
		}
	
	lua_pushnumber(L, result);
  return 1;
}

static int sep_sprite_load(lua_State *L)
{
    int n = lua_gettop(L);
    int result = -1;

    if (n == 1 && lua_type(L, 1) == LUA_TSTRING)
    {
        std::string filepath = lua_tostring(L, 1);

        if (g_pSingeIn->get_retro_path())
        {
            char tmpPath[RETRO_MAXPATH] = {0};
            int len = std::min((int)filepath.size() + RETRO_PAD, RETRO_MAXPATH);
            lua_retropath(filepath.c_str(), tmpPath, len);
            filepath = tmpPath;
        }

        SDL_Surface *temp = IMG_Load(filepath.c_str());

        if (temp) {
            temp = SDL_ConvertSurface(temp, g_se_surface->format, 0);
            SDL_SetSurfaceRLE(temp, SDL_TRUE);
            SDL_SetColorKey(temp, SDL_TRUE, 0x0);
            g_spriteList.push_back(temp);
            result = g_spriteList.size() - 1;
        } else {
            sep_die("Unable to load sprite %s!", filepath.c_str());
        }
    }

    lua_pushnumber(L, result);
    return 1;
}

static int sep_sprite_width(lua_State *L)
{
  int n = lua_gettop(L);
	int result = -1;
	
  if (n == 1)
    if (lua_isnumber(L, 1))
		{
			int sprite = lua_tonumber(L, 1);
			if ((sprite < (int)g_spriteList.size()) && (sprite >= 0))
			{
				result = g_spriteList[sprite]->w;
			}
		}

	lua_pushnumber(L, result);
  return 1;
}

static int sep_step_backward(lua_State *L)
{
	g_pSingeIn->pre_step_backward();
	return 0;
}

static int sep_step_forward(lua_State *L)
{
	g_pSingeIn->pre_step_forward();
	return 0;
}

static int sep_stop(lua_State *L)
{
	g_pSingeIn->pre_stop();
	return 0;
}

static int sep_get_number_of_mice(lua_State *L)
{
	lua_pushnumber(L, g_pSingeIn->cfm_get_number_of_mice(g_pSingeIn->pSingeInstance));
	return 1;
}

// by RDG2010
static int sep_keyboard_set_mode(lua_State *L)
{

	/*
	* Singe can scan keyboard input in two ways:
	*
	* MODE_NORMAL - Singe will only check for keys defined
	* in hypinput.ini. This is the default behavior.
	* 
	* MODE_FULL   - Singe will scan the keyboard for most keypresses.
	* 
	* This function allows the mode to be set through a lua scrip command.
	* Take a look at singe::set_keyboard_mode on singe.cpp for more info.
	*
	*/

	int n = lua_gettop(L);
	int q = 0;
		
	if (n == 1)
	{		
		if (lua_isnumber(L, 1))
		{	
			q = lua_tonumber(L, 1);
			g_pSingeIn->cfm_set_keyboard_mode(g_pSingeIn->pSingeInstance, q);
			
		}
	}
	return 0;
}

// by RDG2010
static int sep_keyboard_get_mode(lua_State *L)
{
	/*
	* Singe can scan keyboard input in two ways:
	*
	* MODE_NORMAL - Singe will only check for keys defined
	* in hypinput.ini. This is the default behavior.
	* 
	* MODE_FULL   - Singe will scan the keyboard for most keypresses.
	* 
	* This function returns the current scan mode for
	* programming code on the lua script.
	*
	* Take a look at singe::get_keyboard_mode on singe.cpp for more info.
	*
	*/

	lua_pushnumber(L, g_pSingeIn->cfm_get_keyboard_mode(g_pSingeIn->pSingeInstance));	
	return 1;
}

// by RDG2010
static int sep_singe_quit(lua_State *L)
{
   /*
	* This function allows a programmer to end program execution
	* through a simple command in the lua script.
	*
	* Useful for cases where the game design calls for
	* an 'Exit' option in a user menu.
	* 
	*/

	g_se_saveme = false;
	sep_die("User decided to quit early.");
	return 0;
}
// by RDG2010
static int sep_get_vldp_state(lua_State *L)
{

   /*
	* Returns the status of the vldp
	* Values returned are 
	* based on the following enumeration (found in ldp.h).
	*
	* LDP_ERROR = 0
	* LDP_SEARCHING = 1
	* LDP_STOPPED = 2
	* LDP_PLAYING = 3
	* LDP_PAUSE = 4
	*
	*/

	lua_pushnumber(L, g_pSingeIn->get_status());	
	return 1;

}

// by RDG2010
static int sep_get_pause_flag(lua_State *L)
{
   /*
	* This function returns g_pause_state's value to the lua script.
	*
	* Sometimes game logic pauses the game (which implies pausing video playback).
	* When implementing a pause state it is possible for the player
	* to resume playblack at moments where the game is not intended to.
	* Boolean g_pause state is an internal variable that keeps track
	* of this. It's set to true whenever sep_pre_pause is called.
	* It's set to false whenever sep_pre_play or sep_skip_to_frame is called.
	* 
	* A lua programmer can use this to prevent resuming playback accidentally.
	*/
	lua_pushboolean(L, g_pause_state);
	return 1;

}

static int sep_set_pause_flag(lua_State *L)
{
	int n = lua_gettop(L);
	bool b1 = false;
		
	if (n == 1)
	{		
		if (lua_isboolean(L, 1))
		{	
			b1 = lua_toboolean(L, 1);
			g_pause_state = b1;
			
		}
	}	
	return 0;
}

static int sep_singe_version(lua_State *L)
{
   /*
	* Returns Singe engine version to the lua script.
	* For validation purposes.
	*
	*/
	
	lua_pushnumber(L, g_pSingeIn->get_singe_version());
	return 1;
	
}
static int sep_ldp_verbose(lua_State *L)
{
	/*
	 * Enables/Disables writing of VLDP playback activity to hypseus.log
	 * Enabled by default.
	 */

	int n = lua_gettop(L);
	bool b1 = false;
		
	if (n == 1)
	{		
		if (lua_isboolean(L, 1))
		{	
			b1 = lua_toboolean(L, 1);
			g_pSingeIn->set_ldp_verbose(b1);
			
		}
	}	
	
	return 0;
}

static int sep_sound_pause(lua_State *L)
{
	int n = lua_gettop(L);
	int result = -1;

	if (n == 1)
		if (lua_isnumber(L, 1))
		{
			int sound = lua_tonumber(L, 1);
			result = g_pSingeIn->samples_set_state(sound, false);
		}

	lua_pushboolean(L, result);
	return 1;
}

static int sep_sound_resume(lua_State *L)
{
	int n = lua_gettop(L);
	int result = -1;

	if (n == 1)
		if (lua_isnumber(L, 1))
		{
			int sound = lua_tonumber(L, 1);
			result = g_pSingeIn->samples_set_state(sound, true);
		}

	lua_pushboolean(L, result);
	return 1;
}

static int sep_sound_stop(lua_State *L)
{
	int n = lua_gettop(L);
	int result = -1;

	if (n == 1)
		if (lua_isnumber(L, 1))
		{
			int sound = lua_tonumber(L, 1);
			result = g_pSingeIn->samples_end_early(sound);
		}

	lua_pushboolean(L, result);
	return 1;
}

static int sep_sound_flush_queue(lua_State *L)
{
	g_pSingeIn->samples_flush_queue();
	return 0;
}

static int sep_sound_get_flag(lua_State *L)
{
	int n = lua_gettop(L);
	bool result = false;

	if (n == 1)
		if (lua_isnumber(L, 1))
		{
			int sound = lua_tonumber(L, 1);
			result = g_pSingeIn->samples_is_playing(sound);
		}

	lua_pushboolean(L, result);
	return 1;
}

static int sep_bezel_enable(lua_State *L)
{
    int n = lua_gettop(L);

    if (n > 0)
        if (lua_isboolean(L, 1)) {
            bool result = lua_toboolean(L, 1);
            g_pSingeIn->cfm_bezel_enable(g_pSingeIn->pSingeInstance, result);

            if (n == 2)
                if (lua_isnumber(L, 2)) {
                    g_pSingeIn->cfm_bezel_type(g_pSingeIn->pSingeInstance, lua_tonumber(L, 2));
                }
        }

    return 0;
}

static int sep_bezel_clear(lua_State *L)
{
    g_pSingeIn->cfm_bezel_clear(g_pSingeIn->pSingeInstance, true);

    return 0;
}

static int sep_bezel_second_score(lua_State *L)
{
    int n = lua_gettop(L);
    bool result;

    if (n == 1)
        if (lua_isboolean(L, 1)) {
            result = lua_toboolean(L, 1);
            g_pSingeIn->cfm_second_score(g_pSingeIn->pSingeInstance, result);
        }

    return 0;
}

static int sep_bezel_credits(lua_State *L)
{
    int n = lua_gettop(L);

    if (n == 1)
        if (lua_isnumber(L, 1))
            g_pSingeIn->cfm_bezel_credits(g_pSingeIn->pSingeInstance, lua_tonumber(L, 1));

    return 0;
}


static int sep_bezel_player_score(lua_State *L)
{
    int n = lua_gettop(L);
    int player;

    if (n == 2)
        if (lua_isnumber(L, 1))
            if (lua_isnumber(L, 2)) {
                player = lua_tonumber(L, 1);
                if (player == 1)
                    g_pSingeIn->cfm_player1_score(g_pSingeIn->pSingeInstance, lua_tonumber(L, 2));
                if (player == 2)
                    g_pSingeIn->cfm_player2_score(g_pSingeIn->pSingeInstance, lua_tonumber(L, 2));
            }

    return 0;
}

static int sep_bezel_player_lives(lua_State *L)
{
    int n = lua_gettop(L);
    int player;

    if (n == 2)
        if (lua_isnumber(L, 1))
            if (lua_isnumber(L, 2)) {
                player = lua_tonumber(L, 1);
                if (player == 1)
                    g_pSingeIn->cfm_player1_lives(g_pSingeIn->pSingeInstance, lua_tonumber(L, 2));
                if (player == 2)
                    g_pSingeIn->cfm_player2_lives(g_pSingeIn->pSingeInstance, lua_tonumber(L, 2));
            }

    return 0;
}
