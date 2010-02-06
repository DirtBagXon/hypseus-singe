/*
 * singeproxy.cpp
 *
 * Copyright (C) 2006 Scott C. Duensing
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

#include "singeproxy.h"
#include "singe_interface.h"

#include <vector>

using namespace std;

////////////////////////////////////////////////////////////////////////////////

// Structures used by Singe to track various internal data
typedef struct g_soundType {
	SDL_AudioSpec  audioSpec;
	Uint32         length;
	Uint8         *buffer;
} g_soundT;

// These are pointers and values needed by the script engine to interact with Daphne
lua_State    *g_se_lua_context;
SDL_Surface  *g_se_surface        = NULL;
int           g_se_overlay_width;
int           g_se_overlay_height;
double       *g_se_disc_fps;
unsigned int *g_se_uDiscFPKS;

// used to know whether try to shutdown lua would crash
bool g_bLuaInitialized = false;

// Communications from the DLL to and from Daphne
struct       singe_out_info  g_SingeOut;
const struct singe_in_info  *g_pSingeIn = NULL;

// Internal data to keep track of things
SDL_Color             g_colorForeground     = {255, 255, 255, 0};
SDL_Color             g_colorBackground     = {0, 0, 0, 0};
vector<TTF_Font *>    g_fontList;
vector<g_soundT>      g_soundList;
vector<SDL_Surface *> g_spriteList;
struct yuv_buf        g_sep_yuv_buf;
int                   g_fontCurrent         = -1;
int                   g_fontQuality         =  1;
double                g_sep_overlay_scale_x =  1;
double                g_sep_overlay_scale_y =  1;

int (*g_original_prepare_frame)(struct yuv_buf *buf);

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

void sep_call_lua(const char *func, const char *sig, ...)
{
	va_list vl;
	int narg, nres;  /* number of arguments and results */
	int popCount;
	const int top = lua_gettop(g_se_lua_context);
	
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
	if (lua_pcall(g_se_lua_context, narg, nres, 0) != 0)  /* do the call */
		sep_error("error running function '%s': %s", func, lua_tostring(g_se_lua_context, -1));
	
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
	vsprintf(temp, fmt, argp);
	
	strcat(message, temp);

	// tell daphne what our last error was ...
	g_pSingeIn->set_last_error(message);

	// force (clean) shutdown
	g_pSingeIn->set_quitflag();
}

void sep_do_blit(SDL_Surface *srfDest)
{
	sep_srf32_to_srf8(g_se_surface, srfDest);
}

void sep_do_mouse_move(Uint16 x, Uint16 y, Sint16 xrel, Sint16 yrel)
{
	static bool debounced = false;
	int x1 = (int)x;
	int y1 = (int)y;
	int xr = (int)xrel;
	int yr = (int)yrel;
	
	// Not sure what's wrong here.  I think things are getting started before Singe is ready.
	if (!debounced) {
		debounced = true;
		return;
	}
	
	x1 *= g_sep_overlay_scale_x;
	y1 *= g_sep_overlay_scale_y;
	xr *= g_sep_overlay_scale_x;
	yr *= g_sep_overlay_scale_y;
	
	sep_call_lua("onMouseMoved", "iiii", x1, y1, xr, yr);
}

void sep_error(const char *fmt, ...)
{
	char message[2048];
	sep_print("Script Error!");
	va_list argp;
	va_start(argp, fmt);
	vsprintf(message, fmt, argp);
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

int sep_prepare_frame_callback(struct yuv_buf *src)
{
	// Is our buffer the correct size?
	if ((g_sep_yuv_buf.Y_size != src->Y_size) || (g_sep_yuv_buf.UV_size != src->UV_size)) {
		if (g_sep_yuv_buf.Y != NULL) free(g_sep_yuv_buf.Y);
		if (g_sep_yuv_buf.U != NULL) free(g_sep_yuv_buf.U);
		if (g_sep_yuv_buf.V != NULL) free(g_sep_yuv_buf.V);
		g_sep_yuv_buf.Y_size = 0;
		g_sep_yuv_buf.UV_size = 0;
	}

	// Is our buffer allocated?
	if ((g_sep_yuv_buf.Y_size == 0) || (g_sep_yuv_buf.UV_size == 0)) {
		g_sep_yuv_buf.Y = (unsigned char *)malloc(src->Y_size * sizeof(unsigned char));
		g_sep_yuv_buf.U = (unsigned char *)malloc(src->UV_size * sizeof(unsigned char));
		g_sep_yuv_buf.V = (unsigned char *)malloc(src->UV_size * sizeof(unsigned char));
		g_sep_yuv_buf.Y_size = src->Y_size;
		g_sep_yuv_buf.UV_size = src->UV_size;
	}

	// Make a copy of the passed YUV buffer for our own use later
	memcpy(g_sep_yuv_buf.Y, src->Y, src->Y_size * sizeof(unsigned char));
	memcpy(g_sep_yuv_buf.U, src->U, src->UV_size * sizeof(unsigned char));
	memcpy(g_sep_yuv_buf.V, src->V, src->UV_size * sizeof(unsigned char));
	
	// Pass callback along
	return g_original_prepare_frame(src);
}

void sep_print(const char *fmt, ...)
{
	char message[2048];
	char temp[2048];
	
	strcpy(message, "SINGE: ");

	va_list argp;
	va_start(argp, fmt);
	vsprintf(temp, fmt, argp);
	
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
		g_se_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, g_se_overlay_width, g_se_overlay_height, 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000);
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
	
	if (g_sep_yuv_buf.Y != NULL) free(g_sep_yuv_buf.Y);
	if (g_sep_yuv_buf.U != NULL) free(g_sep_yuv_buf.U);
	if (g_sep_yuv_buf.V != NULL) free(g_sep_yuv_buf.V);
	
	g_sep_yuv_buf.Y_size = 0;
	g_sep_yuv_buf.UV_size = 0;

  TTF_Quit();

  if (g_bLuaInitialized)
  {
	lua_close(g_se_lua_context);
	g_bLuaInitialized = false;
  }
}

void sep_sound_ended(Uint8 *buffer, unsigned int slot)
{
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

void sep_startup(const char *script)
{
  g_se_lua_context = lua_open();
  luaL_openlibs(g_se_lua_context);
	lua_atpanic(g_se_lua_context, sep_lua_error);

  lua_register(g_se_lua_context, "colorBackground",    sep_color_set_backcolor);
  lua_register(g_se_lua_context, "colorForeground",    sep_color_set_forecolor);

  lua_register(g_se_lua_context, "daphneGetHeight",    sep_daphne_get_height);
  lua_register(g_se_lua_context, "daphneGetWidth",     sep_daphne_get_width);
  lua_register(g_se_lua_context, "daphneScreenshot",   sep_screenshot);

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
	
	lua_register(g_se_lua_context, "soundLoad",          sep_sound_load);
	lua_register(g_se_lua_context, "soundPlay",          sep_sound_play);

	lua_register(g_se_lua_context, "spriteDraw",         sep_sprite_draw);
	lua_register(g_se_lua_context, "spriteGetHeight",    sep_sprite_height);
	lua_register(g_se_lua_context, "spriteGetWidth",     sep_sprite_width);
	lua_register(g_se_lua_context, "spriteLoad",         sep_sprite_load);

  lua_register(g_se_lua_context, "vldpGetHeight",      sep_mpeg_get_height);
  lua_register(g_se_lua_context, "vldpGetPixel",       sep_mpeg_get_pixel);
  lua_register(g_se_lua_context, "vldpGetWidth",       sep_mpeg_get_width);

  if (TTF_Init() < 0)
  {
    sep_die("Unable to initialize font library.");
  }

	g_sep_yuv_buf.Y = NULL;
	g_sep_yuv_buf.U = NULL;
	g_sep_yuv_buf.V = NULL;
	g_sep_yuv_buf.Y_size = 0;
	g_sep_yuv_buf.UV_size = 0;
	
	sep_capture_vldp();

	g_bLuaInitialized = true;

  if (luaL_dofile(g_se_lua_context, script) != 0)
  {
	sep_error("error compiling script: %s", lua_tostring(g_se_lua_context, -1));
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
					g_colorBackground.unused = (char)0;
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
					g_colorForeground.unused = (char)0;
				}

	return 0;
}

static int sep_daphne_get_height(lua_State *L)
{
  lua_pushnumber(L, g_pSingeIn->get_video_height());
  return 1;
}

static int sep_daphne_get_width(lua_State *L)
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
  int n = lua_gettop(L);
  int result = -1;
  const char *font = NULL;
  int points = 0;
  
  if (n == 2)
    if (lua_isstring(L, 1))
      if (lua_isnumber(L, 2))
      {
        font = lua_tostring(L, 1);
        points = lua_tonumber(L, 2);
				TTF_Font *temp = NULL;
        // Load this font.
				temp = TTF_OpenFont(font, points);
				if (temp == NULL)
          sep_die("Unable to load font.");
        // Make it the current font and mark it as loaded.
				g_fontList.push_back(temp);
        g_fontCurrent = g_fontList.size() - 1;
				result = g_fontCurrent;
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

					// the colorkey is only 0 when using Solid (quick n' dirty) mode.
					// In shaded mode, color 0 refers to the background color.
					if (g_fontQuality == 1)
					{
						SDL_SetAlpha(textsurface, SDL_RLEACCEL, 0);

						// by definition, the transparent index is 0
						SDL_SetColorKey(textsurface, SDL_SRCCOLORKEY|SDL_RLEACCEL, 0);
					}
					// alpha must be set for 32-bit surface when blitting or else alpha channel will be ignored
					else if (g_fontQuality == 3)
					{
						SDL_SetAlpha(textsurface, SDL_SRCALPHA | SDL_RLEACCEL, 0);
					}

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
  int n = lua_gettop(L);
	bool result = false;
	int xpos;
	int ypos;
	unsigned int Y_index;
	unsigned int UV_index;
	unsigned char Y;
	unsigned char U;
	unsigned char V;
	unsigned char R;
	unsigned char G;
	unsigned char B;
	int C;
	int D;
	int E;
	
  if (n == 2)
		if (lua_isnumber(L, 1))
			if (lua_isnumber(L, 2)) {
				xpos = (int)((double)lua_tonumber(L, 1) * ((double)g_pSingeIn->g_vldp_info->w / (double)g_se_overlay_width));
				ypos = (int)((double)lua_tonumber(L, 2) * ((double)g_pSingeIn->g_vldp_info->h / (double)g_se_overlay_height));
				Y_index = (g_pSingeIn->g_vldp_info->w * ypos) + xpos;
				UV_index = (g_pSingeIn->g_vldp_info->w * ypos / 4) + (xpos / 2);
				Y = g_sep_yuv_buf.Y[Y_index];
				U = g_sep_yuv_buf.U[UV_index];
				V = g_sep_yuv_buf.V[UV_index];
				C = Y - 16;
				D = U - 128;
				E = V - 128;
				R = sep_byte_clip(( 298 * C           + 409 * E + 128) >> 8);
				G = sep_byte_clip(( 298 * C - 100 * D - 208 * E + 128) >> 8);
				B = sep_byte_clip(( 298 * C + 516 * D           + 128) >> 8);
				result = true;
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
	return 3;
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

  return 0;
}

static int sep_play(lua_State *L)
{
  g_pSingeIn->pre_play();

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
							dest.x = lua_tonumber(L, 1);
							dest.y = lua_tonumber(L, 2);
							dest.w = textsurface->w;
							dest.h = textsurface->h;

							// the colorkey is only 0 when using Solid (quick n' dirty) mode.
							// In shaded mode, color 0 refers to the background color.
							if (g_fontQuality == 1)
							{
								SDL_SetAlpha(textsurface, SDL_RLEACCEL, 0);

								// by definition, the transparent index is 0
								SDL_SetColorKey(textsurface, SDL_SRCCOLORKEY|SDL_RLEACCEL, 0);
							}
							// alpha must be set for 32-bit surface when blitting or else alpha channel will be ignored
							else if (g_fontQuality == 3)
							{
								SDL_SetAlpha(textsurface, SDL_SRCALPHA | SDL_RLEACCEL, 0);
							}

//							SDL_SaveBMP(textsurface, "nukeme.bmp");

							SDL_BlitSurface(textsurface, NULL, g_se_surface, &dest);

//							SDL_SaveBMP(g_se_surface, "nukeme2.bmp");

							SDL_FreeSurface(textsurface);
						}
          }

  return 0;
}

static int sep_screenshot(lua_State *L)
{
  int n = lua_gettop(L);
  
  if (n == 0)
  {
    g_pSingeIn->request_screenshot();
  }
	
	//SDL_SaveBMP(g_se_surface, "/Users/scott/source/daphne/singe/g_se_surface.bmp");

  return 0;
}

static int sep_search(lua_State *L)
{
  char s[6] = { 0 };
  int n = lua_gettop(L);
  
  if (n == 1)
    if (lua_isnumber(L, 1))
    {
      g_pSingeIn->framenum_to_frame(lua_tonumber(L, 1), s);
      g_pSingeIn->pre_search(s, true);
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
      g_pSingeIn->pre_skip_forward(lua_tonumber(L, 1));
	}
      
  return 0;
}

/*
static int sep_search(lua_State *L)
{
  char s[6] = { 0 };
  int n = lua_gettop(L);
  
  if (n == 1)
    if (lua_isnumber(L, 1))
    {
      g_pSingeIn->framenum_to_frame(lua_tonumber(L, 1), s);
      g_pSingeIn->pre_search(s, true);
    }

  return 0;
}
*/

static int sep_skip_to_frame(lua_State *L)
{
	int n = lua_gettop(L);

	if (n == 1)
	{
		if (lua_isnumber(L, 1))
		{
			// TODO : implement this for real on the daphne side of things instead of having to do a search/play hack
			char s[6] = { 0 };
			g_pSingeIn->framenum_to_frame(lua_tonumber(L, 1), s);
			g_pSingeIn->pre_search(s, true);
			g_pSingeIn->pre_play();
		}
	}

	return 0;
}

static int sep_sound_load(lua_State *L)
{
  int n = lua_gettop(L);
	int result = -1;

  if (n == 1)
    if (lua_isstring(L, 1))
		{
			const char *file = lua_tostring(L, 1);
			g_soundT temp;
			if (SDL_LoadWAV(file, &temp.audioSpec, &temp.buffer, &temp.length) == NULL)
			{
				sep_die("Could not open %s: %s", file, SDL_GetError());
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

  if (n == 3)
    if (lua_isnumber(L, 1))
			if (lua_isnumber(L, 2))
				if (lua_isnumber(L, 3))
				{
					int sprite = lua_tonumber(L, 3);
					if (sprite < (int)g_spriteList.size())
					{
						SDL_Rect dest;
						dest.x = lua_tonumber(L, 1);
						dest.y = lua_tonumber(L, 2);
						dest.w = g_spriteList[sprite]->w;
						dest.h = g_spriteList[sprite]->h;
						
						SDL_BlitSurface(g_spriteList[sprite], NULL, g_se_surface, &dest);
					}
				}
	
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
	
  if (n == 1)
    if (lua_isstring(L, 1))
		{
			const char *sprite = lua_tostring(L, 1);
			SDL_Surface *temp = IMG_Load(sprite);
			if (temp != NULL)
			{
				SDL_SetAlpha(temp, SDL_RLEACCEL, 0);
				g_spriteList.push_back(temp);
				result = g_spriteList.size() - 1;
			} else
				sep_die("Unable to load sprite %s!", sprite);
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
