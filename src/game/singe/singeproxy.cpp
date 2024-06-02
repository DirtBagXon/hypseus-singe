/*
 * singeproxy.cpp
 *
 * Copyright (C) 2006 Scott C. Duensing - 2024 DirtBagXon
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
#include "../../io/zippp.h"
#include "../../io/homedir.h"
#include "../../io/mpo_fileio.h"
#include "SDL2_GFX/SDL2_rotozoom.h"

#include <plog/Log.h>
#include <string>
#include <vector>
#include <ctime>
#include <fstream>

using namespace std;
using namespace libzippp;

////////////////////////////////////////////////////////////////////////////////

// Structures used by Singe to track various internal data
typedef struct g_soundType {
	SDL_AudioSpec  audioSpec;
	Uint32         length;
	Uint8          *buffer;
	bool           load;
} g_soundT;

typedef struct g_spriteType {
	double  angle  = 0.0f;
	double  scaleX = 0.0f;
	double  scaleY = 0.0f;
	int     frames = 0;
	int     fwidth = 0;
	bool    gfx    = false;
	bool    smooth = false;
	bool    rekey  = false;
	SDL_Surface *store;
	SDL_Surface *frame;
	SDL_Surface *present;
#if SDL_IMAGE_VERSION_AT_LEAST(2, 6, 0)
	IMG_Animation  *animation;
	int     flow = 0;
	bool    loop = false;
	bool    animating = false;
	int     last = 0;
	int     ticks = 0;
#endif
} g_spriteT;

typedef struct g_positionType {
	int     mouseX[MAX_MICE] = {0};
	int     mouseY[MAX_MICE] = {0};
	Sint16  axisvalue[AXIS_COUNT] = {0};
} g_positionT;

// These are pointers and values needed by the script engine to interact with Hypseus
lua_State    *g_se_lua_context;
SDL_Surface  *g_se_surface        = NULL;
SDL_Renderer *g_se_renderer       = NULL;
SDL_Texture  *g_se_texture        = NULL;
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
vector<g_spriteT>     g_sprites;
int                   g_fontCurrent         = -1;
int                   g_fontQuality         =  1;
double                g_sep_overlay_scale_x =  1;
double                g_sep_overlay_scale_y =  1;
bool                  g_pause_state         = false; // by RDG2010
bool                  g_init_mute           = false;
bool                  g_upgrade_overlay     = false;
bool                  g_show_crosshair      = true;
bool                  g_blend_sprite        = false;
bool                  g_trace               = false;
bool                  g_rom_zip             = false;
bool                  g_firstload           = true;
bool                  g_firstfont           = true;
bool                  g_firstsnd            = true;

bool                  g_keyboard_state[SDL_NUM_SCANCODES] = {false};
int                   g_keyboard_down       = SDL_SCANCODE_UNKNOWN;
int                   g_keyboard_up         = SDL_SCANCODE_UNKNOWN;

g_positionT           g_tract;
std::string           g_scriptpath;
const char*           g_zipFile             = NULL;
SDL_AudioSpec*        g_sound_load          = NULL;
vector<ZipEntry>      g_zipList;
vector<ZipEntry>::iterator iter;

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
    g_SingeOut.sep_mute_vldp_init      = sep_mute_vldp_init;
    g_SingeOut.sep_no_crosshair        = sep_no_crosshair;
    g_SingeOut.sep_upgrade_overlay     = sep_upgrade_overlay;
    g_SingeOut.sep_keyboard_set_state  = sep_keyboard_set_state;
    g_SingeOut.sep_controller_set_axis = sep_controller_set_axis;
    g_SingeOut.sep_enable_trace        = sep_enable_trace;

    result = &g_SingeOut;

    return result;
}
}

////////////////////////////////////////////////////////////////////////////////

SDL_GameController* get_gamepad_id();

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

SDL_GameControllerButton get_button(int value)
{
    if (value >= 0 && value < static_cast<int>(SDL_CONTROLLER_BUTTON_MAX))
        return static_cast<SDL_GameControllerButton>(value);
    else return SDL_CONTROLLER_BUTTON_INVALID;
}

std::string sep_fmt(const std::string fmt_str, ...)
{
    va_list ap, ap_copy;
    const std::string prefix = "SINGE: ";

    va_start(ap, fmt_str);
    int size = vsnprintf(nullptr, 0, (prefix + fmt_str).c_str(), ap);
    va_end(ap);

    if (size <= static_cast<int>(prefix.size())) return "...";

    std::string formatted(static_cast<size_t>(size + 1), '\0');

    va_start(ap_copy, fmt_str);
    vsnprintf(&formatted[0], size + 1, (prefix + fmt_str).c_str(), ap_copy);
    va_end(ap_copy);

    return formatted;
}

void sep_trace(lua_State *L)
{
    if (g_trace) {
        if (g_rom_zip) {
            static bool notice = false;
            if (!notice) {
                LOGW << sep_fmt("Uncompress the ROM to enable trace debugging.");
                notice = true;
            }
            return;
        }
        lua_Debug ar;
        int level = 0;
        while (lua_getstack(L, level, &ar) != 0)
        {
            lua_getinfo(L, "nSl", &ar);
            LOGW << sep_fmt(" %d: function `%s' at line %d %s", level,
                         ar.name, ar.currentline, ar.short_src);
            level++;
        }
    }
}

void sep_set_rampath()
{
    lua_set_zipath(true);
    char rampath[RETRO_MAXPATH] = {0};
    void* found = NULL;
    int size = 0;
    homedir home;

    ZipArchive zf(g_zipFile);
    zf.open(ZipArchive::ReadOnly);

    if (zf.isOpen()) {
        g_zipList = zf.getEntries();
        for (iter = g_zipList.begin(); iter != g_zipList.end(); ++iter) {
             ZipEntry g_zipList = *iter;
             std::string name = g_zipList.getName();

             std::string search = name;
             if (search.length() >= 3)
                 std::transform(search.end() - 3, search.end(),
                           search.end() - 3, ::tolower);

             if ((int)search.find(".cfg") != -1) {
                 found = g_zipList.readAsBinary();
                 size = g_zipList.getSize();

                 int l = strlen(name.c_str());
                 lua_rampath(name.c_str(), rampath, l);

                 if (!mpo_file_exists(rampath)) {

                     home.create_dirs(rampath);
                     fstream fs(rampath,ios::out|ios::binary);

                     if (fs.is_open()) {

                         fs.write((const char*)found, size);
                         fs.close();
                         LOGW << sep_fmt("Creating ramfile: %s", rampath);

                     } else {
                         LOGE << sep_fmt("Error Creating Zip ramfile: %s", rampath);
                         g_pSingeIn->set_quitflag();
                     }
                 }
             }
        }
        g_zipList.clear();
        zf.close();
    }
}

void sep_keyboard_set_state(int keycode, bool state)
{
    int scancode = SDL_GetScancodeFromKey(keycode);

    if (state) {
        g_keyboard_down = scancode;
        g_keyboard_state[scancode] = true;
    } else {
        g_keyboard_up = scancode;
        g_keyboard_state[scancode] = false;
    }
}

void sep_controller_set_axis(Uint8 axis, Sint16 value)
{
    g_tract.axisvalue[axis] = value;
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
        sep_trace(g_se_lua_context);
        LOGE << sep_fmt("error running function '%s': %s", func, lua_tostring(g_se_lua_context, -1));
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
    va_end(argp);

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
    int8_t rID = mID;
	
    // Not sure what's wrong here.  I think things are getting started before Singe is ready.
    if (!debounced) {
        debounced = true;
        return;
    }
	
    x1 *= g_sep_overlay_scale_x;
    y1 *= g_sep_overlay_scale_y;
    xr *= g_sep_overlay_scale_x;
    yr *= g_sep_overlay_scale_y;

    if (mID < 0) rID += 1; // SDL_MOUSE
    g_tract.mouseX[rID] = x1;
    g_tract.mouseY[rID] = y1;
	
    sep_call_lua("onMouseMoved", "iiiii", x1, y1, xr, yr, mID);
}

void sep_error(const char *fmt, ...)
{
    char message[2048];
    LOGE << sep_fmt("Script Error!");
    va_list argp;
    va_start(argp, fmt);
    vsnprintf(message, sizeof(message), fmt, argp);
    va_end(argp);
    lua_close(g_se_lua_context);
    sep_die(message);
}

int sep_lua_error(lua_State *L)
{
    lua_Debug ar;
    int level = 0;

    LOGE << sep_fmt("Singe has paniced!  Very bad!");
    sep_print("Error:  %s", lua_tostring(L, -1));

    sep_print("Stack trace:");
    while (lua_getstack(L, level, &ar) != 0)
    {
        lua_getinfo(L, "nSl", &ar);
        LOGW << sep_fmt(" %d: function `%s' at line %d %s", level, ar.name, ar.currentline, ar.short_src);
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
    va_end(argp);
	
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
        g_se_surface = SDL_CreateRGBSurface(0, g_se_overlay_width, g_se_overlay_height,
                                                   32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000);
        g_sep_overlay_scale_x = (double)g_se_overlay_width / (double)g_pSingeIn->get_video_width();
        g_sep_overlay_scale_y = (double)g_se_overlay_height / (double)g_pSingeIn->get_video_height();
    }
}

SDL_Surface *sep_copy_surface(SDL_Surface *src)
{
    SDL_Surface *dst = NULL;

    dst = SDL_CreateRGBSurface(
        src->flags,
        src->w,
        src->h,
        src->format->BitsPerPixel,
        src->format->Rmask,
        src->format->Gmask,
        src->format->Bmask,
        src->format->Amask
    );

    if (dst != NULL)
        SDL_BlitSurface(src, NULL, dst, NULL);

    return dst;
}

SDL_Surface *sep_copy_surface_rect(SDL_Surface *src, SDL_Rect rect)
{
    SDL_Surface *dst = NULL;

    dst = SDL_CreateRGBSurface(
        src->flags,
        rect.w,
        rect.h,
        src->format->BitsPerPixel,
        src->format->Rmask,
        src->format->Gmask,
        src->format->Bmask,
        src->format->Amask
    );

    if (dst != NULL)
        SDL_BlitSurface(src, &rect, dst, NULL);

    return dst;
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

void sep_sprite_reset()
{
    g_sprites.clear();
    g_firstload = false;
}

void sep_font_reset()
{
    g_fontList.clear();
    g_firstfont = false;
}

void sep_sound_reset()
{
    g_soundList.clear();
    g_firstsnd = false;
}

inline bool sep_range_sprites(const vector<g_spriteT>& vec, int index)
{
    return index >= 0 && index < static_cast<int>(vec.size());
}

#if SDL_IMAGE_VERSION_AT_LEAST(2, 6, 0)
inline bool sep_animation_valid(lua_State *L, int sprite, IMG_Animation *animation, const char* func)
{
    if (!sep_range_sprites(g_sprites, sprite) || animation == NULL) {

        static bool die = false;
        if (!die) {
            g_trace = true; sep_trace(L);
            sep_die("Call on invalid Animation: %s()", func);
            die = true;
        }
        return false;
    }
    return true;
}
#endif

inline bool sep_sprite_valid(lua_State *L, int sprite, SDL_Surface *surface, const char* func)
{
    if (!sep_range_sprites(g_sprites, sprite) || surface == NULL) {

        static bool die = false;
        if (!die) {
            g_trace = true; sep_trace(L);
            sep_die("Call on invalid Sprite: %s()", func);
            die = true;
        }
        return false;
    }
    return true;
}

inline bool sep_range_sound(const vector<g_soundT>& vec, int index)
{
    return index >= 0 && index < static_cast<int>(vec.size());
}

inline bool sep_sound_valid(int sound, const char* func)
{
    if (!sep_range_sound(g_soundList, sound) ||
        !g_soundList[sound].load) {

        sep_print("Called an invalid Sound: %s()", func);
        return false;
    }
    return true;
}

inline bool sep_font_valid(lua_State *L, TTF_Font *font, const char* func)
{
    if (font == NULL) {

        static bool die = false;
        if (!die) {
            g_trace = true; sep_trace(L);
            sep_die("Call on invalid Font: %s()", func);
            die = true;
        }
        return false;
    }
    return true;
}

void sep_sound_ended(Uint8 *buffer, unsigned int slot)
{
    sep_call_lua("onSoundCompleted", "i", slot);
}

void sep_draw_pixel(int x, int y, SDL_Color *c) {

    int bpp      = g_se_surface->format->BytesPerPixel;
    Uint8 *p     = (Uint8 *)g_se_surface->pixels + y * g_se_surface->pitch + x * bpp;
    Uint32 pixel = SDL_MapRGBA(g_se_surface->format, c->r, c->g, c->b, 0xff);

    if ((x < 0) || (x >= g_se_surface->w) || (y < 0) || (y >= g_se_surface->h)) return;

    switch (bpp) {
       case 1:
               *p = (Uint8)pixel;
               break;

       case 2:
               *(Uint16 *)p = (Uint16)pixel;
               break;

       case 3:
               if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                   p[0] = (pixel >> 16) & 0xff;
                   p[1] = (pixel >>  8) & 0xff;
                   p[2] = pixel & 0xff;
               } else {
                   p[0] = pixel & 0xff;
                   p[1] = (pixel >>  8) & 0xff;
                   p[2] = (pixel >> 16) & 0xff;
               }
               break;

       case 4:
               *(Uint32 *)p = pixel;
               break;
    }
}

void sep_draw_line(int x1, int y1, int x2, int y2, SDL_Color *c) {

    int x, y, dx, dy, incX, incY, balance;
    x = y = dx = dy = incX = incY = balance = 0;

    if (x2 >= x1) {
        dx = x2 - x1;
        incX = 1;
    } else {
        dx = x1 - x2;
        incX = -1;
    }

    if (y2 >= y1) {
        dy = y2 - y1;
        incY = 1;
    } else {
        dy = y1 - y2;
        incY = -1;
    }

    x = x1;
    y = y1;

    if (dx >= dy) {
        dy <<= 1;
        balance = dy - dx;
        dx <<= 1;
        while (x != x2) {
            sep_draw_pixel(x, y, c);
            if (balance >= 0) {
                y += incY;
                balance -= dx;
            }
            balance += dy;
            x += incX;
        }
        sep_draw_pixel(x, y, c);
    } else {
         dx <<= 1;
         balance = dx - dy;
         dy <<= 1;
         while (y != y2) {
             sep_draw_pixel(x, y, c);
             if (balance >= 0) {
                 x += incX;
                 balance -= dy;
             }
             balance += dx;
             y += incY;
       }
       sep_draw_pixel(x, y, c);
    }
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
                    // if resulting index is 0, we have to change
                    // it because 0 is reserved for transparent
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

SDL_RWops* sep_unzip(std::string s)
{
    void* found = NULL;
    int size = 0;

    ZipArchive zf(g_zipFile);
    zf.open(ZipArchive::ReadOnly);

    if (zf.isOpen()) {
        g_zipList = zf.getEntries();
        for (iter = g_zipList.begin(); iter != g_zipList.end(); ++iter) {
             ZipEntry g_zipList = *iter;
             std::string name = g_zipList.getName();

             if ((int)name.find(s) != -1) {
                 found = g_zipList.readAsBinary();
                 size = g_zipList.getSize();
             }
        }
        g_zipList.clear();
        zf.close();
    }
    return SDL_RWFromConstMem(found, size);
}

g_soundT sep_sound_zip(std::string s)
{
    g_soundT sound;

    g_sound_load = SDL_LoadWAV_RW(sep_unzip(s), 1,
                      &sound.audioSpec, &sound.buffer, &sound.length);

    return sound;
}

TTF_Font* sep_font_zip(std::string s, int points)
{
    TTF_Font *font = TTF_OpenFontRW(sep_unzip(s), 1, points);

    return font;
}

#if SDL_IMAGE_VERSION_AT_LEAST(2, 6, 0)
IMG_Animation* sep_animation_zip(std::string s)
{
    IMG_Animation *animation = IMG_LoadAnimation_RW(sep_unzip(s), 1);

    return animation;
}
#endif

SDL_Surface* sep_surface_zip(std::string s)
{
    SDL_Surface *surface = IMG_Load_RW(sep_unzip(s), 1);

    return surface;
}

void sep_lua_failure(lua_State* L, const char* s)
{
    sep_error("error compiling script: %s : %s", s, lua_tostring(L, -1));
    sep_die("Cannot continue, quitting...");
    g_bLuaInitialized = false;
}

void sep_startup(const char *data)
{
    g_soundT sound;
    g_spriteT sprite;
    sound.load = false;
    sprite.store = nullptr;
    sprite.frame = nullptr;
    sprite.present = nullptr;
#if SDL_IMAGE_VERSION_AT_LEAST(2, 6, 0)
    sprite.animation = nullptr;
#endif
    g_sprites.push_back(sprite);
    g_soundList.push_back(sound);
    g_fontList.push_back(nullptr);

    g_se_lua_context = lua_open();
    luaL_openlibs(g_se_lua_context);
    lua_atpanic(g_se_lua_context, sep_lua_error);

    lua_register(g_se_lua_context, "colorBackground",        sep_color_set_backcolor);
    lua_register(g_se_lua_context, "colorForeground",        sep_color_set_forecolor);

    lua_register(g_se_lua_context, "hypseusGetHeight",       sep_hypseus_get_height);
    lua_register(g_se_lua_context, "hypseusGetWidth",        sep_hypseus_get_width);

    lua_register(g_se_lua_context, "debugPrint",             sep_debug_say);

    lua_register(g_se_lua_context, "discAudio",              sep_audio_control);
    lua_register(g_se_lua_context, "discChangeSpeed",        sep_change_speed);
    lua_register(g_se_lua_context, "discGetFrame",           sep_get_current_frame);
    lua_register(g_se_lua_context, "discPause",              sep_pause);
    lua_register(g_se_lua_context, "discPlay",               sep_play);
    lua_register(g_se_lua_context, "discSearch",             sep_search);
    lua_register(g_se_lua_context, "discSearchBlanking",     sep_search_blanking);
    lua_register(g_se_lua_context, "discSetFPS",             sep_set_disc_fps);
    lua_register(g_se_lua_context, "discSkipBackward",       sep_skip_backward);
    lua_register(g_se_lua_context, "discSkipBlanking",       sep_skip_blanking);
    lua_register(g_se_lua_context, "discSkipForward",        sep_skip_forward);
    lua_register(g_se_lua_context, "discSkipToFrame",        sep_skip_to_frame);
    lua_register(g_se_lua_context, "discStepBackward",       sep_step_backward);
    lua_register(g_se_lua_context, "discStepForward",        sep_step_forward);
    lua_register(g_se_lua_context, "discStop",               sep_stop);

    lua_register(g_se_lua_context, "fontLoad",               sep_font_load);
    lua_register(g_se_lua_context, "fontPrint",              sep_say_font);
    lua_register(g_se_lua_context, "fontQuality",            sep_font_quality);
    lua_register(g_se_lua_context, "fontSelect",             sep_font_select);
    lua_register(g_se_lua_context, "fontToSprite",           sep_font_sprite);

    lua_register(g_se_lua_context, "overlayClear",           sep_overlay_clear);
    lua_register(g_se_lua_context, "overlayGetHeight",       sep_get_overlay_height);
    lua_register(g_se_lua_context, "overlayGetWidth",        sep_get_overlay_width);
    lua_register(g_se_lua_context, "overlayPrint",           sep_say);

    lua_register(g_se_lua_context, "soundLoad",              sep_sound_load);
    lua_register(g_se_lua_context, "soundPlay",              sep_sound_play);
    lua_register(g_se_lua_context, "soundPause",             sep_sound_pause);
    lua_register(g_se_lua_context, "soundResume",            sep_sound_resume);
    lua_register(g_se_lua_context, "soundIsPlaying",         sep_sound_get_flag);
    lua_register(g_se_lua_context, "soundStop",              sep_sound_stop);
    lua_register(g_se_lua_context, "soundFullStop",          sep_sound_flush_queue);

    lua_register(g_se_lua_context, "spriteDraw",             sep_sprite_draw);
    lua_register(g_se_lua_context, "spriteGetHeight",        sep_sprite_height);
    lua_register(g_se_lua_context, "spriteGetWidth",         sep_sprite_width);
    lua_register(g_se_lua_context, "spriteLoad",             sep_sprite_load);

    lua_register(g_se_lua_context, "vldpGetHeight",          sep_mpeg_get_height);
    lua_register(g_se_lua_context, "vldpGetPixel",           sep_mpeg_get_pixel);
    lua_register(g_se_lua_context, "vldpGetWidth",           sep_mpeg_get_width);
    lua_register(g_se_lua_context, "vldpSetVerbose",         sep_ldp_verbose);

    // Singe 2
    lua_register(g_se_lua_context, "singeSetGameName",       sep_set_gamename);
    lua_register(g_se_lua_context, "singeGetScriptPath",     sep_get_scriptpath);
    lua_register(g_se_lua_context, "singeWantsCrosshairs",   sep_singe_wants_crosshair);
    lua_register(g_se_lua_context, "mouseHowMany",           sep_get_number_of_mice);
    lua_register(g_se_lua_context, "mouseGetPosition",       sep_get_mouse_position);
    lua_register(g_se_lua_context, "overlayEllipse",         sep_overlay_ellipse);
    lua_register(g_se_lua_context, "overlayCircle",          sep_overlay_circle);
    lua_register(g_se_lua_context, "overlayLine",            sep_overlay_line);
    lua_register(g_se_lua_context, "overlayPlot",            sep_overlay_plot);
    lua_register(g_se_lua_context, "overlayBox",             sep_overlay_box);
    lua_register(g_se_lua_context, "spriteRotate",           sep_sprite_rotate);
    lua_register(g_se_lua_context, "spriteScale",            sep_sprite_scale);
    lua_register(g_se_lua_context, "spriteRotateAndScale",   sep_sprite_rotatescale);
    lua_register(g_se_lua_context, "spriteQuality",          sep_sprite_quality);
    lua_register(g_se_lua_context, "spriteUnload",           sep_sprite_unload);
    lua_register(g_se_lua_context, "soundUnload",            sep_sound_unload);
    lua_register(g_se_lua_context, "fontUnload",             sep_font_unload);
    lua_register(g_se_lua_context, "keyboardGetModifiers",   sep_keyboard_get_modifier);
    lua_register(g_se_lua_context, "keyboardGetLastDown",    sep_keyboard_get_down);
    lua_register(g_se_lua_context, "keyboardGetLastUp",      sep_keyboard_get_up);
    lua_register(g_se_lua_context, "keyboardIsDown",         sep_keyboard_is_down);
    lua_register(g_se_lua_context, "controllerGetAxis",      sep_controller_axis);
    lua_register(g_se_lua_context, "controllerGetButton",    sep_controller_button);

    // Hypseus API
    lua_register(g_se_lua_context, "ratioGetX",              sep_get_xratio);
    lua_register(g_se_lua_context, "ratioGetY",              sep_get_yratio);
    lua_register(g_se_lua_context, "getFValue",              sep_get_fvalue);
    lua_register(g_se_lua_context, "setOverlaySize",         sep_set_overlaysize);
    lua_register(g_se_lua_context, "setOverlayResolution",   sep_set_custom_overlay);
    lua_register(g_se_lua_context, "overlaySetResolution",   sep_set_custom_overlay);
    lua_register(g_se_lua_context, "spriteLoadFrames",       sep_sprite_loadframes);
    lua_register(g_se_lua_context, "spriteGetFrames",        sep_sprite_frames);
    lua_register(g_se_lua_context, "spriteDrawFrame",        sep_sprite_animate);
    lua_register(g_se_lua_context, "spriteRotateFrame",      sep_sprite_rotateframe);
    lua_register(g_se_lua_context, "spriteDrawRotatedFrame", sep_sprite_animate_rotated);
    lua_register(g_se_lua_context, "spriteResetColorKey",    sep_sprite_color_rekey);
    lua_register(g_se_lua_context, "spriteFrameHeight",      sep_sprite_height);
    lua_register(g_se_lua_context, "spriteFrameWidth",       sep_frame_width);
    lua_register(g_se_lua_context, "takeScreenshot",         sep_screenshot);
    lua_register(g_se_lua_context, "rewriteStatus",          sep_lua_rewrite);
    lua_register(g_se_lua_context, "vldpGetScale",           sep_mpeg_get_scale);
    lua_register(g_se_lua_context, "dofile",                 sep_doluafile);

    lua_register(g_se_lua_context, "scoreBezelEnable",       sep_bezel_enable);
    lua_register(g_se_lua_context, "scoreBezelClear",        sep_bezel_clear);
    lua_register(g_se_lua_context, "scoreBezelCredits",      sep_bezel_credits);
    lua_register(g_se_lua_context, "scoreBezelTwinScoreOn",  sep_bezel_second_score);
    lua_register(g_se_lua_context, "scoreBezelScore",        sep_bezel_player_score);
    lua_register(g_se_lua_context, "scoreBezelLives",        sep_bezel_player_lives);
    lua_register(g_se_lua_context, "scoreBezelGetState",     sep_bezel_is_enabled);
    lua_register(g_se_lua_context, "controllerDoRumble",     sep_controller_rumble);

    // by RDG2010
    lua_register(g_se_lua_context, "keyboardGetMode",        sep_keyboard_get_mode);
    lua_register(g_se_lua_context, "keyboardSetMode",        sep_keyboard_set_mode);
    lua_register(g_se_lua_context, "discGetState",           sep_get_vldp_state);
    lua_register(g_se_lua_context, "singeGetPauseFlag",      sep_get_pause_flag);
    lua_register(g_se_lua_context, "singeSetPauseFlag",      sep_set_pause_flag);
    lua_register(g_se_lua_context, "singeQuit",              sep_singe_quit);
    lua_register(g_se_lua_context, "singeVersion",           sep_singe_version);

    // Pseudo API calls
    lua_register(g_se_lua_context, "discGetAudioTrack",      sep_pseudo_audio_call);
    lua_register(g_se_lua_context, "discGetAudioTracks",     sep_pseudo_audio_call);
    lua_register(g_se_lua_context, "discSetAudioTrack",      sep_pseudo_audio_call);
    lua_register(g_se_lua_context, "videoGetAudioTrack",     sep_pseudo_audio_call);
    lua_register(g_se_lua_context, "videoGetAudioTracks",    sep_pseudo_audio_call);
    lua_register(g_se_lua_context, "videoSetAudioTrack",     sep_pseudo_audio_call);
    lua_register(g_se_lua_context, "videoQuality",           sep_invalid_api_call);
    lua_register(g_se_lua_context, "videoRotate",            sep_invalid_api_call);
    lua_register(g_se_lua_context, "videoRotateAndScale",    sep_invalid_api_call);
    lua_register(g_se_lua_context, "videoScale",             sep_invalid_api_call);

    // These require SDL2_image version => 2.6.0
#if SDL_IMAGE_VERSION_AT_LEAST(2, 6, 0)
    lua_register(g_se_lua_context, "spriteGetAnimFrame",     sep_sprite_get_frame);
    lua_register(g_se_lua_context, "spriteAnimIsPlaying",    sep_sprite_playing);
    lua_register(g_se_lua_context, "spriteAnimLoop",         sep_sprite_loop);
    lua_register(g_se_lua_context, "spriteAnimPause",        sep_sprite_pause);
    lua_register(g_se_lua_context, "spriteAnimPlay",         sep_sprite_play);
    lua_register(g_se_lua_context, "spriteSetAnimFrame",     sep_sprite_set_frame);
#endif

    //////////////////

    if (TTF_Init() < 0)
        sep_die("Unable to initialize font library.");

    sep_capture_vldp();
    g_bLuaInitialized = true;

    g_blend_sprite = video::get_singe_blend_sprite();

    g_scriptpath = data;

    size_t pos = g_scriptpath.find_last_of(".");
    std::string ext = g_scriptpath.substr(++pos);
    int zip = ext.compare("zip");

    if (zip == 0) { // We have a zip

         ZipArchive zf(data);
         zf.open(ZipArchive::ReadOnly);

         if (zf.isOpen()) {

             const char *init = NULL;
             int size = 0;
             g_rom_zip = true;
             g_zipList = zf.getEntries();
             g_zipFile = data;
             std::string startup;

             sep_print("Loading ZIP based ROM");

             for (iter = g_zipList.begin(); iter != g_zipList.end(); ++iter) {
                 ZipEntry g_zipList = *iter;
                 std::string name = g_zipList.getName();
                 pos = g_scriptpath.find_last_of("/");
#ifdef WIN32
                 if (pos == (size_t)-1)
                     pos = g_scriptpath.find_last_of("\\");
#endif
                 std::string s = g_scriptpath.substr(++pos);
                 s.replace(s.find(".zip"), 4, ".singe");
                 startup = s;

                 if ((int)name.find(s) != -1) {
                     init = (const char*)g_zipList.readAsBinary();
                     size = g_zipList.getSize();
                 }
             }
             g_zipList.clear();
             zf.close();

             if (g_rom_zip) sep_set_rampath();

             if(size > 0 && luaL_loadbuffer(g_se_lua_context, init, size, data) == 0) {

                if (lua_pcall(g_se_lua_context, 0, 0, 0) != 0)
                    sep_lua_failure(g_se_lua_context, startup.c_str());

             } else {
                LOGE << sep_fmt("Startup singe file (%s) not found in %s!", startup.c_str(), data);
                sep_lua_failure(g_se_lua_context, startup.c_str());
             }

         } else {
             sep_die("Failed opening Zip file: %s", data);
             g_bLuaInitialized = false;
         }

    } else {

        if (g_pSingeIn->get_retro_path()) sep_set_retropath();

        if (luaL_dofile(g_se_lua_context, data) != 0)
            sep_lua_failure(g_se_lua_context, NULL);
    }
}

void sep_unload_fonts(void)
{
  if (g_fontList.size() > 0) {

      for (int x = 0; x < (int)g_fontList.size(); x++)
           TTF_CloseFont(g_fontList[x]);

      g_fontList.clear();
  }
}

void sep_unload_sounds(void)
{
  g_pSingeIn->samples_flush_queue();

  if (g_soundList.size() > 0) {

      for (int x = 0; x < (int)g_soundList.size(); x++)
          if (g_soundList[x].load)
              SDL_FreeWAV(g_soundList[x].buffer);

      g_soundList.clear();
  }
}

void sep_unload_sprites(void)
{
  if (g_sprites.size() > 0) {

      for (int x = 0; x < (int)g_sprites.size(); x++)
      {
          SDL_FreeSurface(g_sprites[x].present);
          SDL_FreeSurface(g_sprites[x].store);
          SDL_FreeSurface(g_sprites[x].frame);
          g_sprites[x].present = NULL;
          g_sprites[x].store = NULL;
          g_sprites[x].frame = NULL;
#if SDL_IMAGE_VERSION_AT_LEAST(2, 6, 0)
          if (g_sprites[x].animation)
              IMG_FreeAnimation(g_sprites[x].animation);
          g_sprites[x].animation = NULL;
#endif
      }

      g_sprites.clear();
   }
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

void sep_enable_trace(void)
{
   g_trace = true;
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

static int sep_pseudo_audio_call(lua_State *L)
{
  LOGW << sep_fmt("Use the -altaudio argument");
  lua_pushnumber(L, 0);

  return 1;
}

static int sep_invalid_api_call(lua_State *L)
{
  lua_Debug ar;
  int level = 0;

  while (lua_getstack(L, level, &ar) != 0)
  {
      lua_getinfo(L, "n", &ar);
      if (ar.name) sep_die("%s() is currently unsupported", ar.name);
      level++;
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

static int sep_font_unload(lua_State *L)
{
    int n = lua_gettop(L);

    if (n == 1) {
        if (lua_isnumber(L, 1)) {
            int id  = lua_tonumber(L, 1);
            if (g_fontList.size() > 0) {
                TTF_CloseFont(g_fontList[id]);
                g_fontList[id] = NULL;
            }
        }
    }
    return 0;
}

static int sep_font_load(lua_State *L)
{
    int n      = lua_gettop(L);
    int result = -1;

    if (n == 2 && lua_type(L, 1) == LUA_TSTRING && lua_type(L, 2) == LUA_TNUMBER) {
        std::string fontpath = lua_tostring(L, 1);

        int points = lua_tonumber(L, 2);
        TTF_Font *temp;

        if (g_rom_zip) {

            temp = sep_font_zip(fontpath, points);

        } else {

           if (g_pSingeIn->get_retro_path()) {
               char filepath[RETRO_MAXPATH] = {0};
               int len = std::min((int)fontpath.size() + RETRO_PAD, RETRO_MAXPATH);
               lua_retropath(fontpath.c_str(), filepath, len);
               fontpath = filepath;
           }

           temp = TTF_OpenFont(fontpath.c_str(), points);
        }

        if (temp) {
            // Make it the current font and mark it as loaded.
            if (g_firstfont) sep_font_reset();
            g_fontList.push_back(temp);
            g_fontCurrent = g_fontList.size() - 1;
            result        = g_fontCurrent;
        } else {
            sep_trace(L);
            sep_die("Unable to load font: %s", fontpath.c_str());
            return result;
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

              TTF_Font *font = g_fontList[g_fontCurrent];
              if (!sep_font_valid(L, font, __func__)) return 0;
              SDL_Surface *textsurface = NULL;
              textsurface = SDL_ConvertSurface(textsurface, g_se_surface->format, 0);
              const char *message = lua_tostring(L, 1);

              switch (g_fontQuality) {
                  case 2:
                      textsurface = TTF_RenderText_Shaded(font, message, g_colorForeground, g_colorBackground);
                      break;
                  case 3:
                      textsurface = TTF_RenderText_Blended(font, message, g_colorForeground);
                      break;
                  default:
                      textsurface = TTF_RenderText_Solid(font, message, g_colorForeground);
                      break;
              }

              if (!(textsurface)) {
                  sep_trace(L);
                  sep_die("fontToSprite: Font surface is null!");
                  return 0;
              } else {

                  if (g_firstload) sep_sprite_reset();
                  SDL_SetSurfaceRLE(textsurface, SDL_TRUE);
                  SDL_SetColorKey(textsurface, SDL_TRUE, 0x0);

                  g_spriteT sprite;
                  sprite.scaleX = 1.0;
                  sprite.scaleY = 1.0;
                  sprite.frame = NULL;
                  sprite.store = sep_copy_surface(textsurface);
                  sprite.present = textsurface;
#if SDL_IMAGE_VERSION_AT_LEAST(2, 6, 0)
                  sprite.animation = NULL;
#endif
                  g_sprites.push_back(sprite);
                  result = g_sprites.size() - 1;
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

static int sep_mpeg_get_scale(lua_State *L)
{
    lua_pushnumber(L, g_pSingeIn->get_scalefactor());
    return 1;
}

static int sep_mpeg_get_pixel(lua_State *L)
{
    Uint32 format;
    int n = lua_gettop(L);
    bool result = false;
    static bool ex = false;
    SDL_QueryTexture(g_se_texture, &format, NULL, NULL, NULL);
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
                rect.x = (int)((double)lua_tonumber(L, 1) * ((double)g_pSingeIn->g_vldp_info->w
                                          / (double)g_se_overlay_width));
                rect.y = (int)((double)lua_tonumber(L, 2) * ((double)g_pSingeIn->g_vldp_info->h
                                          / (double)g_se_overlay_height));
                if (g_se_renderer && g_se_texture) {
                    if (SDL_SetRenderTarget(g_se_renderer, g_se_texture) < 0) {
                        if (!ex) {
                            sep_print("get_pixel unsupported texture: Targets disabled");
                            LOGE << sep_fmt("Could not RenderTarget in get_pixel: %s", SDL_GetError());
                        }
                        lua_State* X = luaL_newstate();
                        lua_pushinteger(X, 64);
                        lua_pushinteger(X, 10);
                        lua_pushstring(X, "Targets disabled");
                        sep_say_font(X);
                    } else {
                        if (SDL_RenderReadPixels(g_se_renderer, &rect, format, pixel, SDL_BYTESPERPIXEL(format)) < 0) {
                            sep_trace(L);
                            sep_die("Could not ReadPixel in get_pixel: %s", SDL_GetError());
                        }
                    }
                    SDL_SetRenderTarget(g_se_renderer, NULL);
                } else {
                    if (!ex) {
                        g_se_renderer     = video::get_renderer();
                        g_se_texture      = video::get_yuv_screen();
                    } else sep_die("Could not initialize get_pixel");
	        }

                Y = pixel[0] - 16;
                U = (int)rand()% 6 + (-3);
                V = (int)rand()% 6 + (-3);
                R = sep_byte_clip(( 298 * Y + 409 * V + 128) >> 8);
                G = sep_byte_clip(( 298 * Y - 100 * U - 208 * V + 128) >> 8);
                B = sep_byte_clip(( 298 * Y + 516 * U + 128) >> 8);
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

static int sep_set_gamename(lua_State *L)
{
    int n = lua_gettop(L);
    bool result = false;
    char name[TITLE_LENGTH];

    if (n == 1) {
        if (lua_isstring(L, 1)) {
            snprintf(name, TITLE_LENGTH, "%.40s", lua_tostring(L, 1));
            video::set_window_title(name);
            result = true;
        }
    }

    if (!result) {
        sep_trace(L);
        sep_die("singeSetGameName Failed!");
    }

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
           case SINGE_OVERLAY_OVERSIZE:
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
                               g_pSingeIn->cfm_set_custom_overlay(g_pSingeIn->pSingeInstance, w, h);
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

static int sep_get_fvalue(lua_State *L)
{
   lua_pushnumber(L, g_pSingeIn->cfm_get_fvalue(g_pSingeIn->pSingeInstance));
   return 1;
}

static int sep_singe_wants_crosshair(lua_State *L)
{
   lua_pushboolean(L, g_show_crosshair);
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

static int sep_lua_rewrite(lua_State *L)
{
    lua_pushboolean(L, g_pSingeIn->get_retro_path());
    return 1;
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

                TTF_Font *font = g_fontList[g_fontCurrent];
                if (!sep_font_valid(L, font, __func__)) return 0;
                SDL_Surface *textsurface = NULL;
                textsurface = SDL_ConvertSurface(textsurface, g_se_surface->format, 0);
                const char *message = lua_tostring(L, 3);

                switch (g_fontQuality) {
                case 2:
                    textsurface = TTF_RenderText_Shaded(font, message, g_colorForeground, g_colorBackground);
                    break;
                case 3:
                    textsurface = TTF_RenderText_Blended(font, message, g_colorForeground);
                    break;
                default:
                    textsurface = TTF_RenderText_Solid(font, message, g_colorForeground);
                    break;
                }

                if (!(textsurface)) {
                    sep_trace(L);
                    sep_die("fontPrint: Font surface is null!");
                } else {
                    SDL_Rect dest;
                    dest.w = textsurface->w;
                    dest.h = textsurface->h;
                    dest.x = lua_tonumber(L, 1) + g_sep_overlay_scale_x;
                    dest.y = lua_tonumber(L, 2) + g_sep_overlay_scale_y;

                    SDL_SetSurfaceRLE(textsurface, SDL_TRUE);
                    SDL_SetColorKey(textsurface, SDL_TRUE, 0x0);

                    if (!g_blend_sprite)
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

    if (((int)now - last) < 5) return 0;

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
              video::set_video_blank(true);

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
              *g_se_uDiscFPKS = (unsigned int) ((*g_se_disc_fps * 1000.0) + 0.5);
              // frames per kilosecond (same precision, but an int instead of a float)
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
              video::set_video_blank(true);

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
              video::set_video_blank(true);

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
            char s[7] = { 0 };

            if (g_pSingeIn->g_local_info->blank_during_skips)
                if (debounced) video::set_video_blank(true);

            g_pSingeIn->framenum_to_frame(lua_tonumber(L, 1), s);
            g_pSingeIn->pre_search(s, true);
            g_pSingeIn->pre_play();
            g_pause_state = false; // BY RDG2010
            debounced = true;
        }
    }
    return 0;
}

static int sep_sound_unload(lua_State *L)
{
    int n = lua_gettop(L);

    if (n == 1) {
        if (lua_isnumber(L, 1)) {
            int id = lua_tonumber(L, 1);
            if (!sep_sound_valid(id, __func__)) return 0;
            SDL_FreeWAV(g_soundList[id].buffer);
            g_soundList[id].load = false;
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
        g_soundT temp;

        if (g_rom_zip) {

            temp = sep_sound_zip(filepath);

        } else {

            if (g_pSingeIn->get_retro_path())
            {
                char tmpPath[RETRO_MAXPATH] = {0};
                int len = std::min((int)filepath.size() + RETRO_PAD, RETRO_MAXPATH);
                lua_retropath(filepath.c_str(), tmpPath, len);
                filepath = tmpPath;
            }

            g_sound_load = SDL_LoadWAV(filepath.c_str(), &temp.audioSpec,
                                 &temp.buffer, &temp.length);

        }

        if (g_sound_load) {
            if (g_firstsnd) sep_sound_reset();
            g_soundList.push_back(temp);
            result = g_soundList.size() - 1;
            g_soundList[result].load = true;
        } else {
            sep_trace(L);
            sep_die("Could not open %s: %s", filepath.c_str(), SDL_GetError());
            return result;
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
          if (!sep_sound_valid(sound, __func__)) return 0;
          if (sound < (int)g_soundList.size())
          result = g_pSingeIn->samples_play_sample(g_soundList[sound].buffer, g_soundList[sound].length, g_soundList[sound].audioSpec.channels, -1, sep_sound_ended);
      }

  lua_pushnumber(L, result);
  return 1;
}

static int sep_sprite_color_rekey(lua_State *L)
{
  int n = lua_gettop(L);
  bool r = false;
  int id = -1;

  if (n == 2) {
      if (lua_isboolean(L, 1)) {
          if (lua_isnumber(L, 2)) {
              r = lua_toboolean(L, 1);
              id = lua_tonumber(L, 2);
          }
      }

      if (!sep_sprite_valid(L, id, g_sprites[id].present, __func__)) return 0;

      g_sprites[id].rekey = r;
  }

  if (id < 0) {
      sep_trace(L);
      sep_die("spriteReSetColorKey Failed!");
  }
  return 0;
}

static int sep_sprite_animate(lua_State *L)
{
  int n = lua_gettop(L);
  int frame = 0;
  int id = -1;
  double scale = 0;
  SDL_Rect src, dest;

  // spriteDrawFrame(x, y, f, id)        - Simple animation
  // spriteDrawFrame(x, y, s, f, id)     - Scaled animation

  if ((n == 4) || (n == 5)) {
      if (lua_isnumber(L, 1)) {
          if (lua_isnumber(L, 2)) {
              if (lua_isnumber(L, 3)) {
                  if (lua_isnumber(L, 4)) {
                      dest.x = lua_tonumber(L, 1) + g_sep_overlay_scale_x;
                      dest.y = lua_tonumber(L, 2) + g_sep_overlay_scale_y;
                      if (n == 5) {
                          if (lua_isnumber(L, 5)) {
                              scale = lua_tonumber(L, 3);
                              frame = lua_tonumber(L, 4);
                              id = lua_tonumber(L, 5);
                          }
                      } else {
                          frame = lua_tonumber(L, 3);
                          id = lua_tonumber(L, 4);
                      }
                  }
              }
          }

          if (!sep_sprite_valid(L, id, g_sprites[id].present, __func__)) return 0;

          if (!g_blend_sprite)
              SDL_SetSurfaceBlendMode(g_sprites[id].present, SDL_BLENDMODE_NONE);

          frame = (frame < 1 || frame > g_sprites[id].frames) ? 1 : frame;

          src.w = g_sprites[id].fwidth;
          src.h = g_sprites[id].present->h;
          src.x = g_sprites[id].fwidth * --frame;
          src.y = 0;

          if (n == 4) {
              dest.w = g_sprites[id].present->w;
              dest.h = g_sprites[id].present->h;
              SDL_BlitSurface(g_sprites[id].present, &src, g_se_surface, &dest);

          } else {
              dest.w = (g_sprites[id].fwidth * scale);
              dest.h = (g_sprites[id].present->h * scale);
              SDL_BlitScaled(g_sprites[id].present, &src, g_se_surface, &dest);
          }
      }
  }

  if (id < 0) {
      sep_trace(L);
      sep_die("spriteDrawFrame Failed!");
  }
  return 0;
}

static int sep_sprite_animate_rotated(lua_State *L)
{
  int n = lua_gettop(L);
  int id = -1;
  double scale = 0;
  SDL_Rect dest;

  // spriteDrawRotatedFrame(x, y, id)        - Print the rotated frame
  // spriteDrawRotatedFrame(x, y, s, id)     - Print the frame scaled

  if ((n == 3) || (n == 4)) {
      if (lua_isnumber(L, 1)) {
          if (lua_isnumber(L, 2)) {
              if (lua_isnumber(L, 3)) {
                  dest.x = lua_tonumber(L, 1) + g_sep_overlay_scale_x;
                  dest.y = lua_tonumber(L, 2) + g_sep_overlay_scale_y;
                  if (n == 4) {
                      if (lua_isnumber(L, 4)) {
                          scale = lua_tonumber(L, 3);
                          id = lua_tonumber(L, 4);
                      }
                  } else {
                      id = lua_tonumber(L, 3);
                  }
              }
          }

          if (!sep_sprite_valid(L, id, g_sprites[id].frame, __func__)) return 0;

          if (!g_blend_sprite)
              SDL_SetSurfaceBlendMode(g_sprites[id].frame, SDL_BLENDMODE_NONE);

          if (n == 3) {
              dest.w = g_sprites[id].fwidth;
              dest.h = g_sprites[id].present->h;
              dest.x -= dest.w * 0.5;
              dest.y -= dest.h * 0.5;
              SDL_BlitSurface(g_sprites[id].frame, NULL, g_se_surface, &dest);

          } else {
              dest.w = (g_sprites[id].fwidth * scale);
              dest.h = (g_sprites[id].present->h * scale);
              dest.x -= dest.w * 0.5;
              dest.y -= dest.h * 0.5;
              SDL_BlitScaled(g_sprites[id].frame, NULL, g_se_surface, &dest);
          }
      }
  }

  if (id < 0) {
      sep_trace(L);
      sep_die("spriteDrawRotatedFrame Failed!");
  }
  return 0;
}

static int sep_sprite_draw(lua_State *L)
{
  int n = lua_gettop(L);
  bool center = false;
  int id = -1;
  SDL_Rect dest;
#if SDL_IMAGE_VERSION_AT_LEAST(2, 6, 0)
  bool change = false;
#endif

  // spriteDraw(x, y, id)            - Simple draw
  // spriteDraw(x, y, c, id)         - Centered draw
  // spriteDraw(x, y, x2, y2, id)    - Scaled draw
  // spriteDraw(x, y, x2, y2, c, id) - Centered scaled draw

  if ((n >= 3) && (n <= 6)) {
      if (lua_isnumber(L, 1)) {
          if (lua_isnumber(L, 2)) {
              dest.x = lua_tonumber(L, 1) + g_sep_overlay_scale_x;
              dest.y = lua_tonumber(L, 2) + g_sep_overlay_scale_y;
              if ((n == 3) || (n == 4)) {
                  if (n == 4) {
                      if (lua_isboolean(L, 3) && lua_isnumber(L, 4)) {
                          center = lua_toboolean(L, 3);
                          id = lua_tonumber(L, 4);
                      }
                  } else {
                      if (lua_isnumber(L, 3)) {
                          id = lua_tonumber(L, 3);
                      }
                  }
              }
              if ((n == 5) || (n == 6)) {
                  if (lua_isnumber(L, 3), lua_isnumber(L, 4)) {
                      dest.w = lua_tonumber(L, 3) - dest.x + 1;
                      dest.h = lua_tonumber(L, 4) - dest.y + 1;
                      if (n == 6) {
                          if (lua_isboolean(L, 5) && lua_isnumber(L, 6)) {
                              center = lua_toboolean(L, 5);
                              id = lua_tonumber(L, 6);
                          }
                      } else {
                          if (lua_isnumber(L, 5)) {
                              id = lua_tonumber(L, 5);
                          }
                      }
                  }
              }

              if (!sep_sprite_valid(L, id, g_sprites[id].present, __func__)) return 0;

#if SDL_IMAGE_VERSION_AT_LEAST(2, 6, 0)
              if (g_sprites[id].animation != NULL && g_sprites[id].animating) {
                  g_sprites[id].ticks += SDL_GetTicks() - g_sprites[id].last;
                  g_sprites[id].last = SDL_GetTicks();
                  while (g_sprites[id].animating) {
                      if (g_sprites[id].animation->delays[g_sprites[id].flow] < g_sprites[id].ticks) {
                          g_sprites[id].ticks -= g_sprites[id].animation->delays[g_sprites[id].flow];
                          g_sprites[id].flow++;
                          change = true;
                          if (g_sprites[id].flow >= g_sprites[id].animation->count) {
                              if (g_sprites[id].loop) {
                                  g_sprites[id].flow = 0;
                              } else {
                                  g_sprites[id].flow = g_sprites[id].animation->count - 1;
                                  g_sprites[id].animating = false;
                              }
                          }
                      } else {
                          break;
                      }
                  }
                  if (change) {
                      SDL_FreeSurface(g_sprites[id].frame);
                      g_sprites[id].frame = sep_copy_surface(g_sprites[id].animation->frames[g_sprites[id].flow]);
                      SDL_FreeSurface(g_sprites[id].present);
                      g_sprites[id].present = rotozoomSurfaceXY(g_sprites[id].frame, 360 - g_sprites[id].angle, g_sprites[id].scaleX, g_sprites[id].scaleY, g_sprites[id].smooth);
                      if (g_sprites[id].rekey) SDL_SetColorKey(g_sprites[id].present, SDL_TRUE, 0x0);
                  }
              }
#endif
              if (!g_blend_sprite)
                  SDL_SetSurfaceBlendMode(g_sprites[id].present, SDL_BLENDMODE_NONE);

              if ((n == 3) || (n == 4)) {
                  dest.w = g_sprites[id].present->w;
                  dest.h = g_sprites[id].present->h;
              }
              if (center) {
                  dest.x -= dest.w * 0.5;
                  dest.y -= dest.h * 0.5;
              }
              if ((n == 3) || (n == 4)) {
                  SDL_BlitSurface(g_sprites[id].present, NULL, g_se_surface, &dest);
              } else {
                  SDL_BlitScaled(g_sprites[id].present, NULL, g_se_surface, &dest);
              }
          }
      }
  }

  if (id < 0) {
      sep_trace(L);
      sep_die("spriteDraw Failed!");
  }
  return 0;
}

static int sep_sprite_frames(lua_State *L)
{
  int n = lua_gettop(L);
  int result = -1;

  if (n == 1)
      if (lua_isnumber(L, 1))
      {
          int sprite = lua_tonumber(L, 1);
          if (sep_sprite_valid(L, sprite, g_sprites[sprite].store, __func__))
          {
              result = g_sprites[sprite].frames;
          }
      }

  lua_pushnumber(L, result);
  return 1;
}

static int sep_sprite_height(lua_State *L)
{
  int n = lua_gettop(L);
  int result = -1;

  if (n == 1)
    if (lua_isnumber(L, 1))
    {
        int sprite = lua_tonumber(L, 1);
        if (sep_sprite_valid(L, sprite, g_sprites[sprite].present, __func__))
        {
            result = g_sprites[sprite].present->h;
        }
    }

  lua_pushnumber(L, result);
  return 1;
}

static int sep_sprite_unload(lua_State *L)
{
    int n = lua_gettop(L);

    if (n == 1) {
        if (lua_isnumber(L, 1)) {
            int id = lua_tonumber(L, 1);
            if (!sep_sprite_valid(L, id, g_sprites[id].store, __func__)) return 0;
            SDL_FreeSurface(g_sprites[id].present);
            SDL_FreeSurface(g_sprites[id].frame);
            SDL_FreeSurface(g_sprites[id].store);
            g_sprites[id].present = NULL;
            g_sprites[id].store = NULL;
            g_sprites[id].frame = NULL;
#if SDL_IMAGE_VERSION_AT_LEAST(2, 6, 0)
            if (g_sprites[id].animation)
                IMG_FreeAnimation(g_sprites[id].animation);
            g_sprites[id].animation = NULL;
#endif
        }
    }
    return 0;
}

static int sep_sprite_load(lua_State *L)
{
    int n = lua_gettop(L);
    int result = -1;

#if SDL_IMAGE_VERSION_AT_LEAST(2, 6, 0)
    IMG_Animation *temp = NULL;
#else
    SDL_Surface *temp = NULL;
#endif

    if (n == 1 && lua_type(L, 1) == LUA_TSTRING)
    {
        std::string filepath = lua_tostring(L, 1);

        if (g_rom_zip) {

#if SDL_IMAGE_VERSION_AT_LEAST(2, 6, 0)
            temp = sep_animation_zip(filepath);
#else
            temp = sep_surface_zip(filepath);
#endif

        } else {

           if (g_pSingeIn->get_retro_path())
           {
               char tmpPath[RETRO_MAXPATH] = {0};
               int len = std::min((int)filepath.size() + RETRO_PAD, RETRO_MAXPATH);
               lua_retropath(filepath.c_str(), tmpPath, len);
               filepath = tmpPath;
           }
#if SDL_IMAGE_VERSION_AT_LEAST(2, 6, 0)
           temp = IMG_LoadAnimation(filepath.c_str());
#else
           temp = IMG_Load(filepath.c_str());
#endif
       }

       if (temp) {

           g_spriteT sprite;
           if (g_firstload) sep_sprite_reset();

#if SDL_IMAGE_VERSION_AT_LEAST(2, 6, 0)

           if (temp->count < 2) {

               IMG_FreeAnimation(temp);

               SDL_Surface *image = NULL;
               if (g_rom_zip) image = sep_surface_zip(filepath);
               else image = IMG_Load(filepath.c_str());

               image = SDL_ConvertSurface(image, g_se_surface->format, 0);
               SDL_SetSurfaceRLE(image, SDL_TRUE);
               SDL_SetColorKey(image, SDL_TRUE, 0x0);
               sprite.store = sep_copy_surface(image);
               sprite.present = image;
               sprite.animation = NULL;

            } else {

               for (int x = 0; x < temp->count; x++) {
                   SDL_SetColorKey(temp->frames[x], SDL_TRUE, 0x0);
               }

               sprite.present = sep_copy_surface(temp->frames[0]);
               sprite.store = sep_copy_surface(sprite.present);
               sprite.animation = temp;
           }
#else
           temp = SDL_ConvertSurface(temp, g_se_surface->format, 0);
           SDL_SetSurfaceRLE(temp, SDL_TRUE);
           SDL_SetColorKey(temp, SDL_TRUE, 0x0);
           sprite.store = sep_copy_surface(temp);
           sprite.present = temp;
#endif
           sprite.scaleX = 1.0;
           sprite.scaleY = 1.0;
           sprite.gfx = true;
           sprite.frame = NULL;

           g_sprites.push_back(sprite);
           result = g_sprites.size() - 1;

       } else {
           sep_trace(L);
           sep_die("Unable to load sprite %s!", filepath.c_str());
           return result;
       }
   }

   lua_pushnumber(L, result);
   return 1;
}

static int sep_sprite_loadframes(lua_State *L)
{
    // spriteLoadFrames(f, png)    - Load animation of f equal width frames

    int n = lua_gettop(L);
    int frames = 0;
    int result = -1;
    SDL_Surface *temp = NULL;

    if (n == 2 && lua_type(L, 2) == LUA_TSTRING)
    {
        std::string filepath = lua_tostring(L, 2);

        if (lua_isnumber(L, 1)) {

            frames = lua_tonumber(L, 1);

            if (frames < 0x002 || frames > 0x200) {
               sep_die("Failed loading animation strip %s with %d frames!", filepath.c_str(), frames);
               return result;
            }

            if (g_rom_zip) {

               temp = sep_surface_zip(filepath);

            } else {

               if (g_pSingeIn->get_retro_path())
               {
                   char tmpPath[RETRO_MAXPATH] = {0};
                   int len = std::min((int)filepath.size() + RETRO_PAD, RETRO_MAXPATH);
                   lua_retropath(filepath.c_str(), tmpPath, len);
                   filepath = tmpPath;
               }

               temp = IMG_Load(filepath.c_str());
            }

            if (temp) {

                if (g_firstload) sep_sprite_reset();
                temp = SDL_ConvertSurface(temp, g_se_surface->format, 0);
                SDL_SetSurfaceRLE(temp, SDL_TRUE);
                SDL_SetColorKey(temp, SDL_TRUE, 0x0);

                g_spriteT sprite;
                sprite.scaleX = 1.0;
                sprite.scaleY = 1.0;
                sprite.frames = frames;
                sprite.fwidth = temp->w / frames;
                sprite.frame = NULL;
                sprite.store = sep_copy_surface(temp);
                sprite.present = temp;
#if SDL_IMAGE_VERSION_AT_LEAST(2, 6, 0)
                sprite.animation = NULL;
#endif
                g_sprites.push_back(sprite);
                result = g_sprites.size() - 1;

            } else {
                 sep_die("Unable to load animation strip %s!", filepath.c_str());
                 return result;
            }
        }
    }
    if (result < 0) {
       sep_trace(L);
       sep_die("spriteLoadFrames Failed!");
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
        if (sep_sprite_valid(L, sprite, g_sprites[sprite].present, __func__))
        {
            result = g_sprites[sprite].present->w;
        }
    }

  lua_pushnumber(L, result);
  return 1;
}

static int sep_frame_width(lua_State *L)
{
  int n = lua_gettop(L);
  int result = -1;

  if (n == 1)
    if (lua_isnumber(L, 1))
    {
        int sprite = lua_tonumber(L, 1);
        if (sep_sprite_valid(L, sprite, g_sprites[sprite].present, __func__))
        {
            result = g_sprites[sprite].fwidth;
        }
    }

  lua_pushnumber(L, result);
  return 1;
}

static int sep_sprite_rotateframe(lua_State *L)
{
  int n = lua_gettop(L);
  int id = -1;
  int frame = 0;
  double a, d;

  if (n == 3) {
      if (lua_isnumber(L, 1)) {
          if (lua_isnumber(L, 2)) {
              if (lua_isnumber(L, 3)) {
                  d = lua_tonumber(L, 1);  a = fmod(d, 360.0);
                  frame = lua_tonumber(L, 2);
                  id = lua_tonumber(L, 3);

                  if (g_sprites[id].frames == 0) {
                      sep_die("spriteRotateFrame: sprite has no frames!");
                      return 0;
                  }

                  if (!sep_sprite_valid(L, id, g_sprites[id].store, __func__)) return 0;
                  frame = (frame < 1 || frame > g_sprites[id].frames) ? 1 : frame;

                  SDL_Rect src;
                  src.w = g_sprites[id].fwidth;
                  src.h = g_sprites[id].present->h;
                  src.x = g_sprites[id].fwidth * --frame;
                  src.y = 0;

                  g_sprites[id].angle = a;
                  SDL_Surface *temp = sep_copy_surface_rect(g_sprites[id].store, src);

                  SDL_FreeSurface(g_sprites[id].frame);
                  g_sprites[id].frame = rotozoomSurfaceXY(temp, 360 - g_sprites[id].angle, g_sprites[id].scaleX, g_sprites[id].scaleY, g_sprites[id].smooth);
                  if (g_sprites[id].rekey) SDL_SetColorKey(g_sprites[id].frame, SDL_TRUE, 0x0);
                  SDL_FreeSurface(temp);
              }
          }
      }
  }

  if (id < 0) {
      sep_die("spriteRotateFrame Failed!");
  }
  return 0;
}

static int sep_sprite_rotate(lua_State *L)
{
  int n = lua_gettop(L);
  int id = -1;
  double a, d;

  if (n == 2) {
      if (lua_isnumber(L, 1)) {
          if (lua_isnumber(L, 2)) {
              d = lua_tonumber(L, 1);  a = fmod(d, 360.0);
              id = lua_tonumber(L, 2);
              if (!sep_sprite_valid(L, id, g_sprites[id].store, __func__) || !g_sprites[id].gfx) return 0;
              g_sprites[id].angle = a;
              SDL_FreeSurface(g_sprites[id].present);
              g_sprites[id].present = rotozoomSurfaceXY(g_sprites[id].store, 360 - g_sprites[id].angle, g_sprites[id].scaleX, g_sprites[id].scaleY, g_sprites[id].smooth);
              if (g_sprites[id].rekey) SDL_SetColorKey(g_sprites[id].present, SDL_TRUE, 0x0);
          }
      }
  }

  if (id < 0) {
      sep_trace(L);
      sep_die("spriteRotate Failed!");
  }
  return 0;
}

static int sep_sprite_scale(lua_State *L)
{
  int n = lua_gettop(L);
  int id = -1;
  double x, y = 0;

  if ((n == 2) || (n == 3)) {
      if (lua_isnumber(L, 1)) {
          if (lua_isnumber(L, 2)) {
              x = lua_tonumber(L, 1);
              if (n == 2) {
                  y = x;
                  id = lua_tonumber(L, 2);
              } else {
                  if (lua_isnumber(L, 3)) {
                      y = lua_tonumber(L, 2);
                      id = lua_tonumber(L, 3);
                  }
              }
              if (!sep_sprite_valid(L, id, g_sprites[id].store, __func__) || !g_sprites[id].gfx) return 0;
              g_sprites[id].scaleX = x;
              g_sprites[id].scaleY = y;
              SDL_FreeSurface(g_sprites[id].present);
              g_sprites[id].present = rotozoomSurfaceXY(g_sprites[id].store, 360 - g_sprites[id].angle, g_sprites[id].scaleX, g_sprites[id].scaleY, g_sprites[id].smooth);
              if (g_sprites[id].rekey) SDL_SetColorKey(g_sprites[id].present, SDL_TRUE, 0x0);
          }
      }
  }

  if (id < 0) {
      sep_trace(L);
      sep_die("spriteScale Failed!");
  }
  return 0;
}

static int sep_sprite_rotatescale(lua_State *L)
{
  int n =  lua_gettop(L);
  int id = -1;
  double a, d, x, y = 0;

  if ((n == 3) || (n == 4)) {
      if (lua_isnumber(L, 1)) {
          if (lua_isnumber(L, 2)) {
              if (lua_isnumber(L, 3)) {
                  d = lua_tonumber(L, 1); a = fmod(d, 360.0);
                  x = lua_tonumber(L, 2);
                  if (n == 3) {
                      y = x;
                      id = lua_tonumber(L, 3);
                  } else {
                      if (lua_isnumber(L, 4)) {
                          y = lua_tonumber(L, 3);
                          id = lua_tonumber(L, 4);
                      }
                  }
                  if (!sep_sprite_valid(L, id, g_sprites[id].store, __func__) || !g_sprites[id].gfx) return 0;
                  g_sprites[id].angle = a;
                  g_sprites[id].scaleX = x;
                  g_sprites[id].scaleY = y;
                  SDL_FreeSurface(g_sprites[id].present);
                  g_sprites[id].present = rotozoomSurfaceXY(g_sprites[id].store, 360 - g_sprites[id].angle, g_sprites[id].scaleX, g_sprites[id].scaleY, g_sprites[id].smooth);
                  if (g_sprites[id].rekey) SDL_SetColorKey(g_sprites[id].present, SDL_TRUE, 0x0);
              }
          }
      }
  }

  if (id < 0) {
      sep_trace(L);
      sep_die("spriteRotateAndScale Failed!");
  }
  return 0;
}

static int sep_sprite_quality(lua_State *L)
{
  int n = lua_gettop(L);
  int id = -1;

  if (n == 2) {
      if (lua_isnumber(L, 1)) {
          if (lua_isnumber(L, 2)) {
              int b = lua_tonumber(L, 1);  bool s = (b != 0);
              id = lua_tonumber(L, 2);
              if (!sep_sprite_valid(L, id, g_sprites[id].store, __func__)) return 0;
              g_sprites[id].smooth = s;
              SDL_FreeSurface(g_sprites[id].present);
              g_sprites[id].present = rotozoomSurfaceXY(g_sprites[id].store, 360 - g_sprites[id].angle, g_sprites[id].scaleX, g_sprites[id].scaleY, g_sprites[id].smooth);
              if (g_sprites[id].rekey) SDL_SetColorKey(g_sprites[id].present, SDL_TRUE, 0x0);
          }
      }
  }

  if (id < 0) {
      sep_die("spriteQuality Failed!");
  }
  return 0;
}

static int sep_overlay_ellipse(lua_State *L)
{
  int n = lua_gettop(L);
  int x0, y0, x1, y1, x0o, y0o, x1o, y1o, a, b, b1, dx, dy, err, e2;
  bool result = false;

  x0 = y0 = x1 = y1 = x0o = y0o = x1o = y1o = a = b = b1 = dx = dy = err = e2 = 0;

  if (n == 4) {
      if (lua_isnumber(L, 1)) {
          if (lua_isnumber(L, 2)) {
              if (lua_isnumber(L, 3)) {
                  if (lua_isnumber(L, 4)) {
                      x0o = x0 = lua_tonumber(L, 1);
                      y0o = y0 = lua_tonumber(L, 2);
                      x1o = x1 = lua_tonumber(L, 3);
                      y1o = y1 = lua_tonumber(L, 4);

                      SDL_LockSurface(g_se_surface);

                      a  = abs(x1 - x0);
                      b  = abs(y1 - y0);
                      b1 = b & 1;
                      dx = 4 * (1 - a) * b * b;
                      dy = 4 * (b1 + 1) * a * a;
                      err = dx + dy + b1 * a * a;

                      if (x0 > x1) {
                          x0 = x1;
                          x1 += a;
                      }

                      if (y0 > y1) {
                          y0 = y1;
                      }

                      y0 += (b + 1) / 2;
                      y1 = y0 - b1;
                      a *= 8 * a;
                      b1 = 8 * b * b;

                      do {
                          sep_draw_pixel(x1, y0, &g_colorForeground);
                          sep_draw_pixel(x0, y0, &g_colorForeground);
                          sep_draw_pixel(x0, y1, &g_colorForeground);
                          sep_draw_pixel(x1, y1, &g_colorForeground);
                          e2 = 2 * err;
                          if (e2 <= dy) {
                              y0++;
                              y1--;
                              err += dy += a;
                          }
                          if (e2 >= dx || 2 * err > dy) {
                              x0++;
                              x1--;
                              err += dx += b1;
                          }
                      } while (x0 <= x1);

                      while (y0 - y1 < b) {
                          sep_draw_pixel(x0 - 1, y0,   &g_colorForeground);
                          sep_draw_pixel(x1 + 1, y0++, &g_colorForeground);
                          sep_draw_pixel(x0 - 1, y1,   &g_colorForeground);
                          sep_draw_pixel(x1 + 1, y1--, &g_colorForeground);
                      }

                      SDL_UnlockSurface(g_se_surface);
                      result = true;
                  }
              }
          }
      }
  }
  if (!result) {
      sep_trace(L);
      sep_die("Failed to draw overlayEllipse");
  }

  return 0;
}

static int sep_overlay_circle(lua_State *L)
{
  int n = lua_gettop(L);
  int r, x, y, xo, yo, dx, dy, err;
  bool result = false;

  r = x = y = xo = yo = err = 0;
  dx = dy = 1;

  if (n == 3) {
      if (lua_isnumber(L, 1)) {
          if (lua_isnumber(L, 2)) {
              if (lua_isnumber(L, 3)) {
                  xo = x = lua_tonumber(L, 1);
                  yo = lua_tonumber(L, 2);
                  r = lua_tonumber(L, 3);

                  x = r - 1;
                  err = dx - (r << 1);

                  SDL_LockSurface(g_se_surface);

                  while (x >= y) {
                      sep_draw_pixel(xo + x, yo + y, &g_colorForeground);
                      sep_draw_pixel(xo + y, yo + x, &g_colorForeground);
                      sep_draw_pixel(xo - y, yo + x, &g_colorForeground);
                      sep_draw_pixel(xo - x, yo + y, &g_colorForeground);
                      sep_draw_pixel(xo - x, yo - y, &g_colorForeground);
                      sep_draw_pixel(xo - y, yo - x, &g_colorForeground);
                      sep_draw_pixel(xo + y, yo - x, &g_colorForeground);
                      sep_draw_pixel(xo + x, yo - y, &g_colorForeground);

                      if (err <= 0) {
                          y++;
                          err += dy;
                          dy += 2;
                      }

                      if (err > 0) {
                          x--;
                          dx += 2;
                          err += dx - (r << 1);
                      }
                 }
                 SDL_UnlockSurface(g_se_surface);
                 result = true;
              }
          }
      }
  }
  if (!result) {
      sep_trace(L);
      sep_die("Failed to draw overlayCircle");
  }

  return 0;
}

static int sep_overlay_line(lua_State *L)
{
  int  n       = lua_gettop(L);
  int  x1      = 0;
  int  y1      = 0;
  int  x2      = 0;
  int  y2      = 0;
  bool result  = false;

  if (n == 4) {
      if (lua_isnumber(L, 1)) {
          if (lua_isnumber(L, 2)) {
              if (lua_isnumber(L, 3)) {
                  if (lua_isnumber(L, 4)) {
                      x1 = lua_tonumber(L, 1);
                      y1 = lua_tonumber(L, 2);
                      x2 = lua_tonumber(L, 3);
                      y2 = lua_tonumber(L, 4);

                      SDL_LockSurface(g_se_surface);
                      sep_draw_line(x1, y1, x2, y2, &g_colorForeground);
                      SDL_UnlockSurface(g_se_surface);
                      result = true;
                  }
              }
          }
      }
  }
  if (!result) {
      sep_trace(L);
      sep_die("Failed to draw overlayLine");
  }

  return 0;
}

static int sep_overlay_plot(lua_State *L)
{
  int  n       = lua_gettop(L);
  int  x1      = 0;
  int  y1      = 0;
  bool result  = false;

  if (n == 2) {
      if (lua_isnumber(L, 1)) {
          if (lua_isnumber(L, 2)) {
              x1 = lua_tonumber(L, 1);
              y1 = lua_tonumber(L, 2);

              SDL_LockSurface(g_se_surface);
              sep_draw_pixel(x1, y1, &g_colorForeground);
              SDL_UnlockSurface(g_se_surface);
              result = true;
         }
     }
 }
 if (!result) {
     sep_trace(L);
     sep_die("Failed to draw overlayPlot");
 }

 return 0;

}

static int sep_overlay_box(lua_State *L)
{
  int  n       = lua_gettop(L);
  int  x1      = 0;
  int  y1      = 0;
  int  x2      = 0;
  int  y2      = 0;
  bool result  = false;

  if (n == 4) {
      if (lua_isnumber(L, 1)) {
          if (lua_isnumber(L, 2)) {
              if (lua_isnumber(L, 3)) {
                  if (lua_isnumber(L, 4)) {
                      x1 = lua_tonumber(L, 1);
                      y1 = lua_tonumber(L, 2);
                      x2 = lua_tonumber(L, 3);
                      y2 = lua_tonumber(L, 4);

                      SDL_LockSurface(g_se_surface);
                      sep_draw_line(x1, y1, x2, y1, &g_colorForeground);
                      sep_draw_line(x2, y1, x2, y2, &g_colorForeground);
                      sep_draw_line(x2, y2, x1, y2, &g_colorForeground);
                      sep_draw_line(x1, y2, x1, y1, &g_colorForeground);
                      SDL_UnlockSurface(g_se_surface);
                      result = true;
                  }
              }
         }
     }
 }
 if (!result) {
     sep_trace(L);
     sep_die("Failed to draw overlayBox");
 }

 return 0;

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

static int sep_get_mouse_position(lua_State *L)
{
    int n = lua_gettop(L);
    int m = 0;
    int x = 0;
    int y = 0;
    bool result = false;

    if (n == 1) {
        if (lua_isnumber(L, 1)) {
            m = lua_tonumber(L, 1);
            if ((m < 0) || (m >= MAX_MICE)) {
                sep_die("Invalid mouse specified");
                return 0;
            }
            x = g_tract.mouseX[m];
            y = g_tract.mouseY[m];
            result = true;
        }
    }

    if (!result) sep_die("Failed on mouseGetPosition");

    lua_pushinteger(L, x);
    lua_pushinteger(L, y);
    return 2;
}

static int sep_controller_axis(lua_State *L)
{
    int n = lua_gettop(L);
    int a = 0;
    int v = 0;
    bool result = false;

    // We only have one controller - enabled via -gamepad
    if (n == 1 || n == 2) {
        if (lua_isnumber(L, 1)) {

            if (n == 2 && lua_isnumber(L, 2))
                a = lua_tonumber(L, 2);
            else
                a = lua_tonumber(L, 1);

            if ((a < 0) || (a > AXIS_COUNT)) {
                sep_die("Invalid controller axis specified");
                return 0;
            }
            v = g_tract.axisvalue[a];
            result = true;
        }
    }

    if (!result) sep_die("Failed on controllerGetAxis");

    lua_pushinteger(L, v);
    return 1;
}

static int sep_controller_rumble(lua_State *L)
{
    int n = lua_gettop(L);
    int s = 0, l = 0;
    bool result = false;

    if (n == 2) {
        if (lua_isnumber(L, 1) && lua_isnumber(L, 2)) {

            int t1 = lua_tonumber(L, 1);
            int t2 = lua_tonumber(L, 2);

            if (t1 > 0 && t1 < 5 && t2 > 0 && t2 < 5) {
                s = t1; l = t2;
                result = true;;
            }
        }
    }

    if (!result) sep_die("Failed on controllerDoRumble");
    g_pSingeIn->cfm_set_gamepad_rumble(g_pSingeIn->pSingeInstance, s, l);

    return 0;
}

static int sep_controller_button(lua_State *L)
{
    int n = lua_gettop(L);
    int b = 0;
    bool d = false;
    bool result = false;

    // We only have one controller (c = 0) - enabled via -gamepad
    // controllerGetButton(b)              - Is controller button down
    // controllerGetButton(c, b)           - Is controller button down (Singe2 compatibility)
    // controllerGetButton(c, b, f)        - c is unused: (bool)f (en)/dis Framework adjustment
    //    ''       ''        ''            - bool false: Adopt SDL button enum values direct

    if (n > 0 && n <= 3) {
        if (lua_isnumber(L, 1)) {

            bool framework = true;
            if (n >= 2 && lua_isnumber(L, 2)) {
                b = lua_tonumber(L, 2);

                if (n == 3 && lua_isboolean(L, 3))
                    framework = lua_toboolean(L, 3);
            }
            else b = lua_tonumber(L, 1);

            if (g_trace) {
                if (framework) {
                    LOGW << sep_fmt("Framework adjustment will be made to (%d)", b);
                }
            }

            // Singe2 has Framework adjustment
            if (framework) b = ((b - 500) - 18);

            if ((b < 0) || (b >= SDL_CONTROLLER_BUTTON_MAX)) {
                sep_die("Invalid controller button specified");
                return 0;
            }
            d = SDL_GameControllerGetButton(get_gamepad_id(), get_button(b));
            result = true;
        }
    }

    if (!result) sep_die("Failed on controllerGetButton");

    lua_pushboolean(L, d);
    return 1;
}

static int sep_get_scriptpath(lua_State *L)
{
    lua_pushstring(L, g_scriptpath.c_str());
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

static int sep_keyboard_get_modifier(lua_State *L)
{
    SDL_Keymod m = SDL_GetModState();
    lua_pushinteger(L, (int)m);
    return 1;
}

static int sep_keyboard_get_down(lua_State *L)
{
    lua_pushinteger(L, g_keyboard_down);
    g_keyboard_down = g_keyboard_up = SDL_SCANCODE_UNKNOWN;
    return 1;
}

static int sep_keyboard_get_up(lua_State *L)
{
    lua_pushinteger(L, g_keyboard_up);
    g_keyboard_down = g_keyboard_up = SDL_SCANCODE_UNKNOWN;
    return 1;
}

static int sep_keyboard_is_down(lua_State *L)
{
    int n = lua_gettop(L);
    int s = 0;
    bool r = false;
    bool result = false;

    if (n == 1) {
        if (lua_isnumber(L, 1)) {
            s = lua_tonumber(L, 1);

            if (s < 0 || s > SDL_NUM_SCANCODES) {
                sep_trace(L);
                sep_die("keyboardIsDown received invalid value");
                return 0;
            }
            result = true;
            r = g_keyboard_state[s];
        }
    }

    if (!result) {
        sep_trace(L);
        sep_die("Failed on keyboardIsDown");
    }

    lua_pushboolean(L, r);
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

static int sep_doluafile(lua_State *L)
{
	int n = lua_gettop(L);

	if (n == 1)
	{
            const char *fname = luaL_optstring(L, 1, NULL);

            if (g_rom_zip) {

                std::string file = fname;
                std::string found;

                ZipArchive zf(g_zipFile);
                zf.open(ZipArchive::ReadOnly);

                if (zf.isOpen()) {

                    int size = 0;
                    const char *entry = NULL;
                    g_zipList = zf.getEntries();

                    for (iter = g_zipList.begin(); iter != g_zipList.end(); ++iter) {
                        ZipEntry g_zipList = *iter;
                        std::string name = g_zipList.getName();

                        if ((int)name.find(file) != -1) {
                            entry = (const char*)g_zipList.readAsBinary();
                            size = g_zipList.getSize();
                            found = name;
                        }
                    }
                    g_zipList.clear();
                    zf.close();

                    if (size > 0 && luaL_loadbuffer(L, entry, size, g_zipFile) == 0) {

                        if (lua_pcall(L, 0, 0, 0) != 0)
                            sep_die("error compiling script: %s: %s", found.c_str(), lua_tostring(L, -1));

                    } else {
                        sep_print("Zip entry: %s", fname);
                        sep_die("error loading file from Zip: %s", lua_tostring(L, -1));
                    }
                }
            } else if (luaL_dofile(L, fname) != 0)
                       sep_die("error compiling script: %s", lua_tostring(L, -1));
	}

	return 0;
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

static int sep_bezel_is_enabled(lua_State *L)
{
    lua_pushboolean(L, g_pSingeIn->cfm_bezel_is_enabled(g_pSingeIn->pSingeInstance));

    return 1;
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

#if SDL_IMAGE_VERSION_AT_LEAST(2, 6, 0)

static int sep_sprite_get_frame(lua_State *L)
{
    int n = lua_gettop(L);
    int result = -1;
    int id = -1;

    if (n == 1) {
       if (lua_isstring(L, 1)) {
           id = lua_tonumber(L, 1);
           if (!sep_animation_valid(L, id, g_sprites[id].animation, __func__)) return 0;
           result = g_sprites[id].flow;
       }
    }

    if (id < 0) {
        sep_die("spriteAnimGetFrame Failed!");
    }

    lua_pushinteger(L, result);
    return 1;
}

static int sep_sprite_playing(lua_State *L)
{
    int n = lua_gettop(L);
    int result = -1;
    int id = -1;

    if (n == 1) {
        if (lua_isstring(L, 1)) {
            id = lua_tonumber(L, 1);
            if (!sep_animation_valid(L, id, g_sprites[id].animation, __func__)) return 0;
            result = g_sprites[id].animating;
        }
     }

     if (id < 0) {
         sep_die("spriteAnimIsPlaying Failed!");
     }

     lua_pushboolean(L, result);
     return 1;
}

static int sep_sprite_loop(lua_State *L)
{
    int n = lua_gettop(L);
    int id = -1;
    bool loop = false;

    if (n == 2) {
        if (lua_isboolean(L, 1)) {
            if (lua_isnumber(L, 2)) {
                loop = lua_toboolean(L, 1);
                id = lua_tonumber(L, 2);
                if (!sep_animation_valid(L, id, g_sprites[id].animation, __func__)) return 0;
                g_sprites[id].loop = loop;
            }
        }
    }

    if (id < 0) {
        sep_die("spriteAnimLoop Failed!");
    }
    return 0;
}

static int sep_sprite_pause(lua_State *L)
{
    int n = lua_gettop(L);
    int id = -1;

    if (n == 1) {
        if (lua_isnumber(L, 1)) {
            id = lua_tonumber(L, 1);
            if (!sep_animation_valid(L, id, g_sprites[id].animation, __func__)) return 0;
            g_sprites[id].animating = false;
       }
    }

    if (id < 0) {
        sep_die("spriteAnimPause Failed!");
    }
    return 0;
}

static int sep_sprite_play(lua_State *L)
{
    int n = lua_gettop(L);
    int id = -1;

    if (n == 1) {
        if (lua_isnumber(L, 1)) {
            id = lua_tonumber(L, 1);
            if (!sep_animation_valid(L, id, g_sprites[id].animation, __func__)) return 0;
            if (g_sprites[id].animating == false) {
                g_sprites[id].last = SDL_GetTicks();
            }
            g_sprites[id].animating = true;
       }
    }

    if (id < 0) {
        sep_die("spriteAnimPlay Failed!");
    }
    return 0;
}

static int sep_sprite_set_frame(lua_State *L)
{
    int n = lua_gettop(L);
    int id = -1;
    int f;

    if (n == 2) {
        if (lua_isnumber(L, 1)) {
            if (lua_isnumber(L, 2)) {
                f = lua_tonumber(L, 1);
                id = lua_tonumber(L, 2);
                if (!sep_animation_valid(L, id, g_sprites[id].animation, __func__)) return 0;
                if ((f >= 0) && (f < g_sprites[id].animation->count)) {
                    g_sprites[id].flow = f;
                    g_sprites[id].ticks = 0;
                }
            }
        }
    }

    if (id < 0) {
        sep_die("spriteSetAnimFrame Failed!");
    }
    return 0;
}

#endif
