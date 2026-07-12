/*
 * singeproxy.cpp
 *
 * Copyright (C) 2006 Scott C. Duensing - 2025 DirtBagXon
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
#include "../../video/palette.h"
#include "../../sound/sound.h"
#include "../../io/zippp.h"
#include "../../io/limits.h"
#include "../../io/homedir.h"
#include "../../io/mpo_fileio.h"
#include "sdl3_gfx/SDL3_rotozoom.h"

#include <plog/Log.h>
#include <string>
#include <vector>
#include <ctime>
#include <fstream>
#include <unordered_map>

using namespace std;
using namespace libzippp;

////////////////////////////////////////////////////////////////////////////////

// Structures used by Singe to track various internal data
typedef struct m_soundType {
	SDL_AudioSpec  audioSpec;
	Uint32         length;
	Uint8          *buffer = NULL;
	bool           load;
} m_soundT;

typedef struct m_mixerType {
	MIX_Audio   *audio = NULL;
	MIX_Track   *track = NULL;
	bool        load   = false;
} m_mixerT;

typedef struct {
	unsigned int count;
	unsigned int duration;
	bool trip;
} m_blankT;

typedef struct {
	int bpp;
	int srfbpp;
	float width;
	float height;
} m_textureT;

typedef struct m_spriteType {
	double  angle  = 0.0f;
	double  scaleX = 0.0f;
	double  scaleY = 0.0f;
	int     frames = 0;
	int     fwidth = 0;
	bool    gfx    = false;
	bool    smooth = false;
	bool    blend  = false;
	bool    rekey  = false;
	bool    nokey  = false;
	SDL_Surface *store;
	SDL_Surface *frame;
	SDL_Surface *present;
	IMG_Animation  *animation;
	int     flow = 0;
	bool    loop = false;
	bool    animating = false;
	int     last = 0;
	int     ticks = 0;
} m_spriteT;

typedef struct g_positionType {
	int     mouseX[MAX_MICE] = {0};
	int     mouseY[MAX_MICE] = {0};
	Sint16  axisvalue[MAX_GAMECONTROLLER][AXIS_COUNT];

        g_positionType() {
            for (int i = 0; i < MAX_GAMECONTROLLER; ++i)
                for (int j = 0; j < AXIS_COUNT; ++j)
                    axisvalue[i][j] = 0;
        }
} m_positionT;

typedef struct m_srtType {
        int startFrame;
        int endFrame;
        std::string text;
} m_srtT;

struct ZipMem {
    void* data;
    size_t size;
};

struct ZipFont {
    TTF_Font *font = nullptr;
    std::unique_ptr<char[]> buffer;
};

// These are pointers and values needed by the script engine to interact with Hypseus
static lua_State    *g_se_lua_context;
static SDL_Surface  *g_se_surface                  = NULL;
static SDL_Texture  *g_se_texture                  = NULL;
static int           g_se_vldp_width               = 0;
static int           g_se_vldp_height              = 0;
static int           g_se_overlay_width            = 0;
static int           g_se_overlay_height           = 0;
static double       *g_se_disc_fps;
static unsigned int *g_se_uDiscFPKS;

// used to know whether try to shutdown lua would crash
static bool g_bLuaInitialized                      = false;
static bool m_se_grunt                             = true;

// Communications from the DLL to and from Hypseus
struct       singe_out_info  g_SingeOut;
const struct singe_in_info  *g_pSingeIn            = NULL;

// Internal data to keep track of things
static SDL_Color             m_colorForeground     = {255, 255, 255, 0};
static SDL_Color             m_colorBackground     = {0, 0, 0, 0};
static vector<TTF_Font *>    m_fontList;
static vector<m_mixerT>      m_mixerList;
static vector<m_soundT>      m_soundList;
static vector<m_spriteT>     m_sprites;
static yuv_buffer            m_se_yuv_buf;
static int                   m_fontCurrent         = -1;
static int                   m_fontQuality         =  1;
static double                m_se_overlay_scale_x  =  1;
static double                m_se_overlay_scale_y  =  1;
static double                m_se_yuv_scale_x      =  1;
static double                m_se_yuv_scale_y      =  1;
static bool                  m_show_crosshair      = true;
static bool                  m_trace               = false;
static bool                  m_rom_zip             = false;
static bool                  m_firstload           = true;
static bool                  m_firstfont           = true;
static bool                  m_firstmix            = true;
static bool                  m_firstsnd            = true;
static bool                  m_pixelready          = false;
static bool                  m_colorkey            = true;
static bool                  m_zlua_arg            = false;
static uint8_t               m_upgrade_overlay     = 0;

static bool                  g_keyboard_state[SDL_SCANCODE_COUNT] = {false};
static int                   g_keyboard_down       = SDL_SCANCODE_UNKNOWN;
static int                   g_keyboard_up         = SDL_SCANCODE_UNKNOWN;
static bool                  g_pause_state         = false; // by RDG2010
const char*                  g_zipFile             = NULL;

static m_positionT           m_tract;
static std::string           m_scriptpath;
static std::string           m_altgame;
static m_textureT            m_tx                  = {0};
static m_blankT              m_blank               = {0, 0, false};
static const std::string     m_ramfiles[]          = {".cfg", ".ram"};

MIX_Mixer*                   m_mixer               = nullptr;

ZipArchive*                  g_zf                  = nullptr;
bool*                        g_zlfs                = nullptr;
static vector<ZipEntry>      m_zipList;
static vector<ZipEntry>::iterator m_iter;

static vector<m_srtT>       m_srt;
static bool                 m_srt_display          = false;
static int                  m_srt_height           = 0x50;

static std::unordered_map<TTF_Font *, std::unique_ptr<char[]>> m_fontBuffers;
static const SDL_Color m_colorTransparent = {0, 0, 0, 0};

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
    g_SingeOut.sep_datapaths           = sep_datapaths;
    g_SingeOut.sep_altgame             = sep_altgame;
    g_SingeOut.sep_minseek             = sep_minseek;
    g_SingeOut.sep_no_crosshair        = sep_no_crosshair;
    g_SingeOut.sep_rom_compressed      = sep_rom_compressed;
    g_SingeOut.sep_upgrade_overlay     = sep_upgrade_overlay;
    g_SingeOut.sep_fullalpha_overlay   = sep_fullalpha_overlay;
    g_SingeOut.sep_keyboard_set_state  = sep_keyboard_set_state;
    g_SingeOut.sep_controller_set_axis = sep_controller_set_axis;
    g_SingeOut.sep_enable_trace        = sep_enable_trace;

    result = &g_SingeOut;

    return result;
}
}

////////////////////////////////////////////////////////////////////////////////

static MIX_Track* GetCurrentlyPlayingTrack()
{
    for (auto& entry : m_mixerList)
    {
        if (entry.track && MIX_TrackPlaying(entry.track))
            return entry.track;
    }
    return NULL;
}

static inline float LegacyVolume(int value)
{
    if (value < 0 || value > 128) return 0.0f;

    return static_cast<float>(value) * (1.0f / 128.0f);
}

static bool sep_parseTS(const std::string& ts, int& h, int& m, int& s, int& ms)
{
    return sscanf(ts.c_str(), "%d:%d:%d,%d", &h, &m, &s, &ms) == 4;
}

static void UpdateSRT(const std::vector<m_srtT>& subtitles, int currentFrame)
{
    if (subtitles.empty())
        return;

    static size_t cachedIndex = 0;

    if (cachedIndex < subtitles.size())
    {
        const m_srtT& sub = subtitles[cachedIndex];

        if (currentFrame >= sub.startFrame &&
            currentFrame <= sub.endFrame)
        {
            video::draw_srt(sub.text.c_str(), 1, m_srt_height);
            return;
        }

        if (currentFrame > sub.endFrame &&
            cachedIndex + 1 < subtitles.size())
        {
            const m_srtT& next = subtitles[cachedIndex + 1];

            if (currentFrame >= next.startFrame &&
                currentFrame <= next.endFrame)
            {
                ++cachedIndex;
                video::draw_srt(next.text.c_str(), 1, m_srt_height);
                return;
            }
        }
    }

    auto it = std::upper_bound(
        subtitles.begin(),
        subtitles.end(),
        currentFrame,
        [](int frame, const m_srtT& sub)
        {
            return frame < sub.startFrame;
        });

    if (it != subtitles.begin())
        --it;

    size_t newIndex = std::distance(subtitles.begin(), it);
    cachedIndex = newIndex;

    const m_srtT& sub = subtitles[newIndex];

    if (currentFrame >= sub.startFrame &&
        currentFrame <= sub.endFrame)
    {
        video::draw_srt(sub.text.c_str(), 1, m_srt_height);
    }
}

bool net_send_enabled();

static unsigned char sep_byte_clip(int value)
{
    int result;
	
    result = value;
    if (result < 0)   result = 0;
    if (result > 255) result = 255;
	
    return (unsigned char)result;
}

static void sep_do_blank()
{
    palette::set_yuv_transparency(m_blank.count == 0);

    if (m_blank.count > 0) m_blank.count--;
    else {
        m_blank.count = m_blank.duration;
        m_blank.trip = false;
    }
}

void sep_set_espath()
{
    lua_set_espath(true);
}

void sep_datapaths(const char *data)
{
   if (data[0] != '\0') lua_set_abpath(data);
}

static SDL_GamepadButton get_button(int value)
{
    if (value >= 0 && value < static_cast<int>(SDL_GAMEPAD_BUTTON_COUNT))
        return static_cast<SDL_GamepadButton>(value);

    return SDL_GAMEPAD_BUTTON_INVALID;
}

static std::string sep_fmt(const std::string fmt_str, ...)
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

static bool audio_format(SDL_AudioSpec *sound)
{
    if (sound->format == SDL_AUDIO_S16)
        return true;

    LOGE << sep_fmt("soundLoad: Invalid audio format detected");
    return false;
}

static void sep_trace(lua_State *L)
{
    if (m_trace) {
        if (m_rom_zip) {
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

static void sep_set_rampath()
{
    lua_set_zipath(true);
    char rampath[REWRITE_MAXPATH] = {0};

    if (g_zf->isOpen()) {
        m_zipList = g_zf->getEntries();
        for (m_iter = m_zipList.begin(); m_iter != m_zipList.end(); ++m_iter) {
             ZipEntry entry = *m_iter;
             std::string name = entry.getName();

             std::string search = name;
             if (search.length() >= 3)
                 std::transform(search.end() - 3, search.end(),
                           search.end() - 3, ::tolower);

             for (const auto& ext : m_ramfiles) {
                 if (search.find(ext) != std::string::npos) {

                     void* found = entry.readAsBinary();
                     int size = entry.getSize();

                     if (!found) continue;

                     lua_rampath(name.c_str(), rampath, REWRITE_MAXPATH);

                     if (!mpo_file_exists(rampath)) {

                         g_homedir.create_dirs(rampath);
                         fstream fs(rampath, std::ios::out | std::ios::binary);

                         if (fs.is_open()) {

                             fs.write(reinterpret_cast<const char*>(found), size);
                             fs.close();

                             LOGW << sep_fmt("Copying ramfile: %s from zip", rampath);

                         } else {

                             LOGE << sep_fmt("Error copying zip ramfile: %s", rampath);
                             g_pSingeIn->set_quitflag();
                         }
                     }

                     delete[] reinterpret_cast<char*>(found);
                     break;
                 }
             }
        }
        m_zipList.clear();
    }
}

void sep_keyboard_set_state(int keycode, bool state)
{
    int scancode = SDL_GetScancodeFromKey(keycode, nullptr);

    if (state) {
        g_keyboard_down = scancode;
        g_keyboard_state[scancode] = true;
    } else {
        g_keyboard_up = scancode;
        g_keyboard_state[scancode] = false;
    }
}

void sep_controller_set_axis(Uint8 axis, Sint16 value, Uint8 id)
{
    m_tract.axisvalue[id][axis] = value;
}

static void sep_die(const char *fmt, ...)
{
    char message[2048];
    char temp[2048];

    strcpy(message, "SINGE: ");

    va_list argp;
    va_start(argp, fmt);
    vsnprintf(temp, sizeof(temp), fmt, argp);
    va_end(argp);

    strcat(message, temp);

    if (m_se_grunt) {
        sound::play_saveme();
        SDL_Delay(1000);
        m_se_grunt = false;
    }

    // tell hypseus what our last error was ...
    g_pSingeIn->set_last_error(message);

    // force (clean) shutdown
    g_pSingeIn->set_quitflag();
}

void sep_call_lua(const char *func, const char *sig, ...)
{
    va_list vl;
    int narg, nres;  /* number of arguments and results */
    int popCount;
    const int top = lua_gettop(g_se_lua_context);
    static uint8_t err = 0;

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
        if (err > 0) {
            sep_die("Multiple errors, cannot continue...");
            exit(SINGE_ERROR_RUNTIME);
        }
        err++;
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
    err = 0;
	
    if (popCount > 0)
        lua_pop(g_se_lua_context, popCount);
}

static void sep_capture_vldp()
{
    // Intercept VLDP callback
    g_original_prepare_frame = g_pSingeIn->g_local_info->prepare_frame;
    g_pSingeIn->g_local_info->prepare_frame = sep_prepare_frame_callback;
}

static bool sep_init_mixer()
{
    if (!MIX_Init()) {
        sep_die("SDL Mixer failed to initialize: %s", SDL_GetError());
        return false;
    }

    m_mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                        sound::getSpecDesired());

    MIX_SetMixerGain(m_mixer, 0.32f);

    return true;
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
	
    x1 *= m_se_overlay_scale_x;
    y1 *= m_se_overlay_scale_y;
    xr *= m_se_overlay_scale_x;
    yr *= m_se_overlay_scale_y;

    if (mID < 0) rID += 1; // SDL_MOUSE
    m_tract.mouseX[rID] = x1;
    m_tract.mouseY[rID] = y1;

    // Not sure what's wrong here. I think things are getting started before Singe is ready.
    if (!debounced) {
        debounced = true;
        return;
    }
	
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
        LOGW << sep_fmt(" %d: function '%s' at line %d %s", level, ar.name, ar.currentline, ar.short_src);
        level++;
    }
    sep_print("Trace complete.");

    return 0;
}

int sep_prepare_frame_callback(uint8_t *Yplane, uint8_t *Uplane, uint8_t *Vplane,
                           int Ypitch, int Upitch, int Vpitch)
{
    if (m_blank.trip) sep_do_blank();

    int result = (video::vid_update_yuv_overlay(Yplane, Uplane, Vplane, Ypitch, Upitch, Vpitch) == 0)
               ? VLDP_TRUE
               : VLDP_FALSE;

    if (m_pixelready)
    {
        memcpy(m_se_yuv_buf.Y.get(), Yplane, g_se_vldp_width * g_se_vldp_height);
        memcpy(m_se_yuv_buf.U.get(), Uplane, m_se_yuv_buf.UVw * m_se_yuv_buf.UVh);
        memcpy(m_se_yuv_buf.V.get(), Vplane, m_se_yuv_buf.UVw * m_se_yuv_buf.UVh);
    }

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

    g_se_overlay_width = width;
    g_se_overlay_height = height;

    g_se_vldp_width = g_pSingeIn->g_vldp_info->w;
    g_se_vldp_height = g_pSingeIn->g_vldp_info->h;
	
    if (g_se_surface == NULL)
    {
        createSurface = true;
    }
    else
    {
        if ((g_se_surface->w != g_se_overlay_width) || (g_se_surface->h != g_se_overlay_height))
        {
            SDL_DestroySurface(g_se_surface);
            createSurface = true;
        }
    }

    if (createSurface)
    {
        g_se_surface = SDL_CreateSurface(g_se_overlay_width, g_se_overlay_height, SDL_PIXELFORMAT_RGBA32);

        if (!g_se_surface)
            sep_die("sep_set_surface creation failed: %s\n", SDL_GetError());

        m_se_overlay_scale_x = (double)g_se_overlay_width  / (double)g_pSingeIn->get_video_width();
        m_se_overlay_scale_y = (double)g_se_overlay_height / (double)g_pSingeIn->get_video_height();

        g_se_texture = video::get_overlay_texture();

        if (g_se_texture)
        {

            float w, h;
            SDL_GetTextureSize(g_se_texture, &w, &h);
            m_tx.width = (int)w;
            m_tx.height = (int)h;

            SDL_PixelFormat format = (SDL_PixelFormat)SDL_GetNumberProperty(
                SDL_GetTextureProperties(g_se_texture), SDL_PROP_TEXTURE_FORMAT_NUMBER, 0);

            const SDL_PixelFormatDetails* texFmt = SDL_GetPixelFormatDetails(format);

            m_tx.bpp = texFmt->bits_per_pixel;

            const SDL_PixelFormatDetails* surfFmt = SDL_GetPixelFormatDetails(g_se_surface->format);

            m_tx.srfbpp = surfFmt->bits_per_pixel;
        }
        else
            m_tx = {0};
    }

    if (m_se_yuv_buf.width != g_se_vldp_width || m_se_yuv_buf.height != g_se_vldp_height)
    {
        size_t Ys  = static_cast<size_t>(g_se_vldp_width) *
                     static_cast<size_t>(g_se_vldp_height);
        size_t UVw = static_cast<size_t>(g_se_vldp_width)  >> 1;
        size_t UVh = static_cast<size_t>(g_se_vldp_height) >> 1;
        size_t UVs = UVw * UVh;

        try
        {
            unique_ptr<uint8_t[]> newY(new uint8_t[Ys]);
            unique_ptr<uint8_t[]> newU(new uint8_t[UVs]);
            unique_ptr<uint8_t[]> newV(new uint8_t[UVs]);

            memset(newY.get(), 0x00, Ys);
            memset(newU.get(), 0x80, UVs);
            memset(newV.get(), 0x80, UVs);

            m_se_yuv_buf.Y = std::move(newY);
            m_se_yuv_buf.U = std::move(newU);
            m_se_yuv_buf.V = std::move(newV);
        }
        catch (const std::bad_alloc&)
        {
            LOGE << "YUV buffer allocation failed";
            sep_die("YUV Buffer could not Initialize");
        }

        m_se_yuv_buf.width  = g_se_vldp_width;
        m_se_yuv_buf.height = g_se_vldp_height;

        m_se_yuv_buf.Ypitch = g_se_vldp_width;
        m_se_yuv_buf.Upitch = static_cast<int>(UVw);
        m_se_yuv_buf.Vpitch = static_cast<int>(UVw);

        m_se_yuv_buf.UVw = static_cast<int>(UVw);
        m_se_yuv_buf.UVh = static_cast<int>(UVh);
    }

    m_se_yuv_scale_x = (double)g_se_vldp_width  / (double)g_se_overlay_width;
    m_se_yuv_scale_y = (double)g_se_vldp_height / (double)g_se_overlay_height;
}

SDL_Surface *sep_copy_surface(SDL_Surface *src, SDL_Rect *rect)
{
    SDL_Surface *dst = NULL;

    int w = rect ? rect->w : src->w;
    int h = rect ? rect->h : src->h;

    const SDL_PixelFormatDetails* fmt = SDL_GetPixelFormatDetails(src->format);

    dst = SDL_CreateSurface(w, h, fmt->format);

    if (dst != NULL) {
        SDL_BlitSurface(src, rect, dst, NULL);
        SDL_SetSurfaceBlendMode(dst, SDL_BLENDMODE_NONE);
    }
    else
        sep_die("sep_copy_surface failed: %s\n", SDL_GetError());

    return dst;
}

static void sep_unload_fonts(void)
{
  if (m_fontList.size() > 0) {

      for (int x = 0; x < (int)m_fontList.size(); x++)
      {
          TTF_CloseFont(m_fontList[x]);
          m_fontBuffers.erase(m_fontList[x]);
      }

      m_fontList.clear();
  }
}

static void sep_unload_sounds(void)
{
  g_pSingeIn->samples_flush_queue();

  if (m_soundList.size() > 0) {

      for (int x = 0; x < (int)m_soundList.size(); x++)
          if (m_soundList[x].load)
              SDL_free(m_soundList[x].buffer);

      m_soundList.clear();
  }
}

static void sep_unload_mixers(void)
{

  if (m_mixerList.size() > 0) {

      for (int x = 0; x < (int)m_mixerList.size(); x++)
          if (m_mixerList[x].load) {
              MIX_DestroyTrack(m_mixerList[x].track);
              MIX_DestroyAudio(m_mixerList[x].audio);
              m_mixerList[x].track = NULL;
              m_mixerList[x].audio = NULL;
          }

      m_mixerList.clear();
  }

    MIX_DestroyMixer(m_mixer);
    m_mixer = NULL;
}

static void sep_unload_sprites(void)
{
  if (m_sprites.size() > 0) {

      for (int x = 0; x < (int)m_sprites.size(); x++)
      {
          SDL_DestroySurface(m_sprites[x].present);
          SDL_DestroySurface(m_sprites[x].store);
          SDL_DestroySurface(m_sprites[x].frame);
          m_sprites[x].present = NULL;
          m_sprites[x].store = NULL;
          m_sprites[x].frame = NULL;
          if (m_sprites[x].animation)
              IMG_FreeAnimation(m_sprites[x].animation);
          m_sprites[x].animation = NULL;
      }

      m_sprites.clear();
   }
}

void sep_shutdown(void)
{
    sep_release_vldp();

    sep_unload_mixers();
    sep_unload_fonts();
    sep_unload_sounds();
    sep_unload_sprites();

    if (g_zf)
    {
        if (*g_zlfs)
            sep_print("Unloading Zip LFS.");

        if (g_zf->isOpen())
            g_zf->close();

        delete g_zf;
        g_zf = nullptr;
    }

    delete g_zlfs;
    g_zlfs = nullptr;
	
    if (g_bLuaInitialized)
    {
        lua_close(g_se_lua_context);
        g_se_lua_context = nullptr;
        g_bLuaInitialized = false;
    }

    MIX_Quit();
}

static void sep_sprite_reset()
{
    m_sprites.clear();
    m_firstload = false;
}

static void sep_font_reset()
{
    m_fontList.clear();
    m_firstfont = false;
}

static void sep_sound_reset()
{
    m_soundList.clear();
    m_firstsnd = false;
}

static void sep_mixer_reset()
{
    m_mixerList.clear();
    m_firstmix = false;
}

template <typename T>
inline bool sep_vector_range(const std::vector<T>& vec, int index)
{
    return index >= 0 && index < static_cast<int>(vec.size());
}

inline bool sep_animation_valid(lua_State *L, int sprite, IMG_Animation *animation, const char* func)
{
    if (!sep_vector_range(m_sprites, sprite) || animation == NULL) {

        static bool die = false;
        if (!die) {
            m_trace = true; sep_trace(L);
            sep_die("Call on invalid Animation: %s()", func);
            die = true;
        }
        return false;
    }
    return true;
}

static inline bool sep_sprite_valid(lua_State *L, int sprite, SDL_Surface *surface, const char* func)
{
    if (!sep_vector_range(m_sprites, sprite) || surface == NULL) {

        static bool die = false;
        if (!die) {
            m_trace = true; sep_trace(L);
            sep_die("Call on invalid Sprite: %s()", func);
            die = true;
        }
        return false;
    }
    return true;
}

static inline bool sep_sound_valid(int sound, const char* func)
{
    if (!sep_vector_range(m_soundList, sound) ||
        !m_soundList[sound].load) {

        sep_print("Called an invalid Sound: %s()", func);
        return false;
    }
    return true;
}

static inline bool sep_mixer_valid(int mixer, const char* func)
{
    if (!sep_vector_range(m_mixerList, mixer) ||
        !m_mixerList[mixer].load) {

        sep_print("Called an invalid MIDI: %s()", func);
        return false;
    }
    return true;
}

static inline bool sep_font_valid(lua_State *L, TTF_Font *font, const char* func)
{
    if (font == NULL) {

        static bool die = false;
        if (!die) {
            m_trace = true; sep_trace(L);
            sep_die("Call on invalid Font: %s()", func);
            die = true;
        }
        return false;
    }
    return true;
}

static void sep_sound_ended(Uint8 *buffer, unsigned int slot)
{
    sep_call_lua("onSoundCompleted", "i", slot);
}

static inline void sep_draw_pixel(int x, int y, const SDL_Color *c)
{
    if ((x < 0) || (x >= g_se_surface->w) ||
        (y < 0) || (y >= g_se_surface->h))
        return;

    const SDL_Color f = m_colorkey
        ? SDL_Color{c->r, c->g, c->b, 0xff}
        : m_colorTransparent;

    Uint8 *p = (Uint8 *)g_se_surface->pixels +
               y * g_se_surface->pitch +
               x * sizeof(Uint32);

    *(Uint32 *)p = PIXEL_ABGR8888(f.r, f.g, f.b, f.a);
}

static void sep_draw_line(int x1, int y1, int x2, int y2, SDL_Color *c) {

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
    static bool debounced = false;

    // convert a 32-bit surface to an 8-bit surface
    // destination palette is predefined

    if (((dst->w == src->w) && (dst->h == src->h)) &&
        (m_tx.srfbpp == 32))
    {
        if (!debounced)
        {
            // dst must be 8-bit here - expensive SDL3 checks, so bail early
            static const SDL_PixelFormatDetails *dest =
                SDL_GetPixelFormatDetails(dst->format);

            if (dest->bits_per_pixel != 8)
            {
                sep_die("sep_srf32_to_srf8 surfaces are mismatched....");
                return bResult;
            }

            debounced = true;
        }

        SDL_LockSurface(dst);
        SDL_LockSurface(src);

        Uint8 *pSrcLine = (Uint8 *)src->pixels;
        Uint8 *pDstLine = (Uint8 *)dst->pixels;

        for (unsigned int uRowIdx = 0;
             uRowIdx < (unsigned int)src->h;
             ++uRowIdx)
        {
            Uint32 *p32SrcPix = (Uint32 *)pSrcLine;
            Uint8  *p8DstPix  = pDstLine;

            for (unsigned int uColIdx = 0;
                 uColIdx < (unsigned int)src->w;
                 ++uColIdx)
            {
                Uint32 pixel = *p32SrcPix;

                Uint8 u8Idx;

                if (pixel & 0x80000000)
                {
                    Uint8 r = (Uint8)pixel;
                    Uint8 g = (Uint8)(pixel >> 8);
                    Uint8 b = (Uint8)(pixel >> 16);

                    // 3:2:3 palette reduction
                    u8Idx = (r & 0xE0) |
                            ((g & 0xC0) >> 3) |
                            ((b & 0xE0) >> 5);

                    if (u8Idx == 0)
                        u8Idx = 1;
                }
                else
                {
                    u8Idx = 0;
                }

                *p8DstPix = u8Idx;

                ++p8DstPix;
                ++p32SrcPix;
            }

            pSrcLine += src->pitch;
            pDstLine += dst->pitch;
        }

        SDL_UnlockSurface(src);
        SDL_UnlockSurface(dst);

        bResult = true;
    }
    return bResult;
}

static bool sep_fullalpha_srf32(SDL_Surface *src, SDL_Texture *dst)
{
    bool bResult = false;
    if (
        ((m_tx.width == src->w) && (m_tx.height == src->h)) &&
        (m_tx.bpp == 32) && (m_tx.srfbpp == 32)
    )
    {
        SDL_UpdateTexture(dst, NULL, (void *)src->pixels, src->pitch);
        bResult = true;
    }
    return bResult;
}

static bool sep_format_srf32(SDL_Surface *src, SDL_Texture *dst)
{
    bool bResult = false;

    if (((m_tx.width == src->w) && (m_tx.height == src->h)) &&
        (m_tx.bpp == 32) && (m_tx.srfbpp == 32))
    {
        SDL_LockSurface(src);

        Uint8 *pSrcLine = (Uint8 *)src->pixels;

        for (unsigned int uRowIdx = 0;
             uRowIdx < (unsigned int)src->h;
             ++uRowIdx)
        {
            Uint32 *p32SrcPix = (Uint32 *)pSrcLine;

            for (unsigned int uColIdx = 0;
                 uColIdx < (unsigned int)src->w;
                 ++uColIdx)
            {
                Uint32 u32SrcPix = *p32SrcPix;

                // R=0, G=8, B=16, A=24
                Uint8 u32R = (Uint8)(u32SrcPix);
                Uint8 u32G = (Uint8)(u32SrcPix >> 8);
                Uint8 u32B = (Uint8)(u32SrcPix >> 16);
                Uint8 u32A = (Uint8)(u32SrcPix >> 24);

                Uint32 u32Idx;

                if (u32A > 0x7F)
                {
                    u32Idx = PIXEL_ABGR8888(u32R, u32G, u32B, u32A);

                    if (u32Idx == 0)
                        u32Idx = 1;
                }
                else
                {
                    u32Idx = 0;
                }

                *p32SrcPix = u32Idx;
                ++p32SrcPix;
            }

            pSrcLine += src->pitch;
        }

        SDL_UnlockSurface(src);

        SDL_UpdateTexture(dst, NULL, src->pixels, src->pitch);

        bResult = true;
    }
    return bResult;
}

static bool sep_format_monochrome(SDL_Surface *src, SDL_Texture *dst)
{
    bool bResult = false;

    if (((m_tx.width == src->w) && (m_tx.height == src->h)) &&
        (m_tx.bpp == 32) && (m_tx.srfbpp == 32))
    {
        SDL_LockSurface(src);

        Uint32 *p32Src = (Uint32 *)src->pixels;
        Uint32 *srcEnd = p32Src + (src->w * src->h);

        while (p32Src < srcEnd)
        {
            Uint32 pixel = *p32Src;

            Uint8 r = (Uint8)(pixel);
            Uint8 g = (Uint8)(pixel >> 8);
            Uint8 b = (Uint8)(pixel >> 16);
            Uint8 a = (Uint8)(pixel >> 24);

            Uint32 u32Idx;

            if (a > 0x7F)
            {
                Uint8 gray = (Uint8)((77 * r + 151 * g + 28 * b) >> 8);

                u32Idx = PIXEL_ABGR8888(gray, gray, gray, a);

                if (u32Idx == 0)
                    u32Idx = 1;
            }
            else
                u32Idx = 0;

            *p32Src++ = u32Idx;
        }

        SDL_UnlockSurface(src);

        SDL_UpdateTexture(dst, NULL, src->pixels, src->pitch);

        bResult = true;
    }
    return bResult;
}

void sep_do_blit(SDL_Surface *srfDest)
{
    if (m_srt_display)
        UpdateSRT(m_srt, g_pSingeIn->get_current_frame());

    switch (m_upgrade_overlay) {
        case SEP_OVERLAY_ALPHA:
            sep_fullalpha_srf32(g_se_surface, g_se_texture);
            break;
        case SEP_OVERLAY_MONO:
            sep_format_monochrome(g_se_surface, g_se_texture);
            break;
        case SEP_OVERLAY_FULL:
            sep_format_srf32(g_se_surface, g_se_texture);
            break;
        default:
            sep_srf32_to_srf8(g_se_surface, srfDest);
            break;
    }
}

static ZipMem sep_unzip(const std::string& s)
{
    ZipMem out{nullptr, 0};

    if (!g_zf->isOpen())
        g_zf->open(ZipArchive::ReadOnly);

    if (g_zf->isOpen()) {
        m_zipList = g_zf->getEntries();

        for (auto& entry : m_zipList) {
            if (entry.getName().find(s) != std::string::npos) {
                out.data = entry.readAsBinary();
                out.size = entry.getSize();
                break;
            }
        }

        m_zipList.clear();

        if (!*g_zlfs)
            g_zf->close();
    }

    return out;
}

static bool sep_sound_zip(std::string s, m_soundT *sound)
{
    auto zip = sep_unzip(s);

    if (!zip.data)
        return false;

    SDL_IOStream *io = SDL_IOFromConstMem(zip.data, zip.size);

    bool load = SDL_LoadWAV_IO(io, true, &sound->audioSpec,
                    &sound->buffer, &sound->length);

    delete[] static_cast<char*>(zip.data);

    return load;
}

static ZipFont sep_font_zip(std::string s, int points)
{
    ZipFont result;

    auto zip = sep_unzip(s);

    if (!zip.data)
        return result;

    result.buffer.reset(static_cast<char*>(zip.data));

    SDL_IOStream *io = SDL_IOFromConstMem(result.buffer.get(), zip.size);

    result.font = TTF_OpenFontIO(io, true, points);

    if (!result.font)
        result.buffer.reset();

    return result;
}

static bool sep_mixer_zip(std::string s, MIX_Mixer *mixer,
         MIX_Audio **audio, MIX_Track **track)
{
    if (!mixer || !audio || !track) return false;

    auto zip = sep_unzip(s);

    if (!zip.data)
        return false;

    SDL_IOStream *io = SDL_IOFromConstMem(zip.data, zip.size);

    *audio = MIX_LoadAudio_IO(mixer, io, false, true);
    *track = MIX_CreateTrack(mixer);

    bool data = (*track && *audio && MIX_SetTrackAudio(*track, *audio));

    delete[] static_cast<char*>(zip.data);

    return data;
}

static std::string sep_srt_zip(const std::string& s)
{
    auto zip = sep_unzip(s);

    if (!zip.data)
        return {};

    SDL_IOStream *rw = SDL_IOFromConstMem(zip.data, zip.size);

    if (!rw)
    {
        delete[] static_cast<char*>(zip.data);
        return {};
    }

    Sint64 size = SDL_GetIOSize(rw);
    if (size <= 0)
    {
        SDL_CloseIO(rw);
        delete[] static_cast<char*>(zip.data);
        return {};
    }

    std::string buffer(static_cast<size_t>(size), '\0');

    size_t read = SDL_ReadIO(rw, &buffer[0], size);

    SDL_CloseIO(rw);

    delete[] static_cast<char*>(zip.data);

    if (read != static_cast<size_t>(size))
        return {};

    return buffer;
}

static IMG_Animation* sep_animation_zip(std::string s)
{
    auto zip = sep_unzip(s);

    if (!zip.data)
        return nullptr;

    SDL_IOStream *io = SDL_IOFromConstMem(zip.data, zip.size);

    IMG_Animation *animation = IMG_LoadAnimation_IO(io, true);

    delete[] static_cast<char*>(zip.data);

    return animation;
}

static SDL_Surface* sep_surface_zip(std::string s)
{
    auto zip = sep_unzip(s);

    if (!zip.data)
        return nullptr;

    SDL_IOStream *io = SDL_IOFromConstMem(zip.data, zip.size);

    SDL_Surface *surface = IMG_Load_IO(io, true);

    delete[] static_cast<char*>(zip.data);

    return surface;
}

static int sep_TStoFrame(int hours, int minutes, int seconds, int milliseconds, double fps)
{
    if (fps == 0 && m_se_grunt)
        sep_die("Attemping to load SRT functions before assigning FPS: quitting...");

    double totalSeconds = hours * 3600.0 + minutes * 60.0 +
                          seconds + milliseconds / 1000.0;

    return static_cast<int>(totalSeconds * fps + 0.5);
}

static std::vector<m_srtT> LoadSRT(const std::string& filename, double fps)
{
    std::vector<m_srtT> subtitles;
    std::string content;

    if (m_rom_zip)
        content = sep_srt_zip(filename);
    else
    {
        std::ifstream file(filename, std::ios::binary);

        if (file)
            content.assign(std::istreambuf_iterator<char>(file),
                           std::istreambuf_iterator<char>());
    }

    if (content.empty())
        return subtitles;

    std::istringstream stream(content);
    std::string line;

    auto cleanWindows = [](std::string& s)
    {
        while (!s.empty() && (s.back() == '\r' || s.back() == ' ' || s.back() == '\t'))
            s.pop_back();

        size_t start = s.find_first_not_of(" \t");
        if (start != std::string::npos && start > 0)
            s = s.substr(start);
    };

    while (std::getline(stream, line))
    {
        cleanWindows(line);

        if (line.empty())
            continue;

        std::string timeLine;
        if (!std::getline(stream, timeLine))
            break;

        cleanWindows(timeLine);

        size_t arrowPos = timeLine.find("-->");
        if (arrowPos == std::string::npos)
            continue;

        std::string startStr = timeLine.substr(0, arrowPos);
        std::string endStr   = timeLine.substr(arrowPos + 3);

        cleanWindows(startStr);
        cleanWindows(endStr);

        int sh, sm, ss, sms;
        int eh, em, es, ems;

        if (!sep_parseTS(startStr, sh, sm, ss, sms))
            continue;

        if (!sep_parseTS(endStr, eh, em, es, ems))
            continue;

        m_srtT entry;
        entry.startFrame = sep_TStoFrame(sh, sm, ss, sms, fps);
        entry.endFrame = sep_TStoFrame(eh, em, es, ems, fps);

        std::stringstream text;

        while (std::getline(stream, line))
        {
            cleanWindows(line);

            if (line.empty())
                break;

            if (text.tellp() > 0)
                text << '\n';

            text << line;
        }

        entry.text = text.str();

        if (!entry.text.empty())
            subtitles.push_back(std::move(entry));
    }

    if (m_se_grunt) {
        LOGI << sep_fmt("Loaded %d subtitle entries.", subtitles.size());
    }

    return subtitles;
}

static void sep_lua_failure(lua_State* L, const char* s)
{
    sep_error("error compiling script: %s : %s", s, lua_tostring(L, -1));
    sep_die("Cannot continue, quitting...");
    g_bLuaInitialized = false;
}

void sep_altgame(const char *data)
{
    m_altgame = data;
}

void sep_minseek(unsigned int delay)
{
    m_blank.duration = m_blank.count = std::min((int)delay, 32);
}

void sep_startup(const char *data)
{
    m_mixerT mixer;
    m_soundT sound;
    m_spriteT sprite;
    mixer.load = false;
    sound.load = false;
    sprite.store = nullptr;
    sprite.frame = nullptr;
    sprite.present = nullptr;
    sprite.animation = nullptr;
    m_sprites.push_back(sprite);
    m_mixerList.push_back(mixer);
    m_soundList.push_back(sound);
    m_fontList.push_back(nullptr);

    g_zlfs = new bool(false);

    g_se_lua_context = lua_open();
    luaL_openlibs(g_se_lua_context);
    lua_atpanic(g_se_lua_context, sep_lua_error);

    lua_register(g_se_lua_context, "colorBackground",        sep_color_set_backcolor);
    lua_register(g_se_lua_context, "colorForeground",        sep_color_set_forecolor);
    lua_register(g_se_lua_context, "drawTransparent",        sep_draw_transparent);

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
    lua_register(g_se_lua_context, "overlaySetMonochrome",   sep_overlay_set_grayscale);
    lua_register(g_se_lua_context, "overlayBanner",          sep_banner);
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

    lua_register(g_se_lua_context, "srtLoad",                sep_subtitle_load);
    lua_register(g_se_lua_context, "srtClear",               sep_subtitle_clear);
    lua_register(g_se_lua_context, "srtEnable",              sep_subtitle_enable);
    lua_register(g_se_lua_context, "srtPosition",            sep_subtitle_position);

    lua_register(g_se_lua_context, "vldpGetHeight",          sep_mpeg_get_height);
    lua_register(g_se_lua_context, "vldpGetPixel",           sep_mpeg_get_pixel);
    lua_register(g_se_lua_context, "vldpGetWidth",           sep_mpeg_get_width);
    lua_register(g_se_lua_context, "vldpSetMonochrome",      sep_mpeg_set_grayscale);
    lua_register(g_se_lua_context, "vldpSetLuma",            sep_mpeg_set_luma);
    lua_register(g_se_lua_context, "vldpSetVerbose",         sep_ldp_verbose);

    // Singe 2
    lua_register(g_se_lua_context, "singeSetGameName",       sep_set_gamename);
    lua_register(g_se_lua_context, "singeGetScriptPath",     sep_get_scriptpath);
    lua_register(g_se_lua_context, "singeWantsCrosshairs",   sep_singe_wants_crosshair);
    lua_register(g_se_lua_context, "mouseHowMany",           sep_get_number_of_mice);
    lua_register(g_se_lua_context, "mouseHowManyReal",       sep_get_number_of_realmice);
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
    lua_register(g_se_lua_context, "spriteBlend",            sep_sprite_blend);
    lua_register(g_se_lua_context, "spriteUnload",           sep_sprite_unload);
    lua_register(g_se_lua_context, "soundUnload",            sep_sound_unload);
    lua_register(g_se_lua_context, "fontUnload",             sep_font_unload);
    lua_register(g_se_lua_context, "keyboardGetModifiers",   sep_keyboard_get_modifier);
    lua_register(g_se_lua_context, "keyboardGetLastDown",    sep_keyboard_get_down);
    lua_register(g_se_lua_context, "keyboardGetLastUp",      sep_keyboard_get_up);
    lua_register(g_se_lua_context, "keyboardIsDown",         sep_keyboard_is_down);
    lua_register(g_se_lua_context, "keyboardCatchQuit",      sep_keyboard_block_quit);
    lua_register(g_se_lua_context, "controllerGetAxis",      sep_controller_axis);
    lua_register(g_se_lua_context, "controllerGetButton",    sep_controller_button);
    lua_register(g_se_lua_context, "controllerSetPadding",   sep_controller_setwad);
    lua_register(g_se_lua_context, "controllerGetPadding",   sep_controller_getwad);
    lua_register(g_se_lua_context, "controllerHowMany",      sep_controller_attached);
    lua_register(g_se_lua_context, "soundGetVolume",         sep_sound_getvolume);
    lua_register(g_se_lua_context, "soundSetVolume",         sep_sound_setvolume);
    lua_register(g_se_lua_context, "videoGetVolume",         sep_vldp_getvolume);
    lua_register(g_se_lua_context, "videoSetVolume",         sep_vldp_setvolume);

    // Hypseus API
    lua_register(g_se_lua_context, "ratioGetX",              sep_get_xratio);
    lua_register(g_se_lua_context, "ratioGetY",              sep_get_yratio);
    lua_register(g_se_lua_context, "getFValue",              sep_get_fvalue);
    lua_register(g_se_lua_context, "setOverlaySize",         sep_set_overlaysize);
    lua_register(g_se_lua_context, "setOverlayFullAlpha",    sep_set_overlayfullalpha);
    lua_register(g_se_lua_context, "setOverlayOpacity",      sep_set_overlayopacity);
    lua_register(g_se_lua_context, "setOverlayResolution",   sep_set_custom_overlay);
    lua_register(g_se_lua_context, "overlaySetResolution",   sep_set_custom_overlay);
    lua_register(g_se_lua_context, "spriteLoadFrames",       sep_sprite_loadframes);
    lua_register(g_se_lua_context, "spriteGetFrames",        sep_sprite_frames);
    lua_register(g_se_lua_context, "spriteDrawFrame",        sep_sprite_animate);
    lua_register(g_se_lua_context, "spriteDrawGrid",         sep_sprite_grid);
    lua_register(g_se_lua_context, "spriteRotateFrame",      sep_sprite_rotateframe);
    lua_register(g_se_lua_context, "spriteDrawRotatedFrame", sep_sprite_animate_rotated);
    lua_register(g_se_lua_context, "spriteResetColorKey",    sep_sprite_color_rekey);
    lua_register(g_se_lua_context, "spriteLoadData",         sep_sprite_loadata);
    lua_register(g_se_lua_context, "spriteFrameHeight",      sep_sprite_height);
    lua_register(g_se_lua_context, "spriteFrameWidth",       sep_frame_width);
    lua_register(g_se_lua_context, "soundLoadData",          sep_sound_loadata);
    lua_register(g_se_lua_context, "takeScreenshot",         sep_screenshot);
    lua_register(g_se_lua_context, "rewriteStatus",          sep_lua_rewrite);
    lua_register(g_se_lua_context, "vldpGetScale",           sep_mpeg_get_scale);
    lua_register(g_se_lua_context, "vldpSetScale",           sep_mpeg_set_scale);
    lua_register(g_se_lua_context, "vldpGetRotate",          sep_mpeg_get_rotate);
    lua_register(g_se_lua_context, "vldpSetRotate",          sep_mpeg_set_rotate);
    lua_register(g_se_lua_context, "vldpFocusArea",          sep_mpeg_focus_area);
    lua_register(g_se_lua_context, "vldpResetFocus",         sep_mpeg_reset_focus);
    lua_register(g_se_lua_context, "vldpGetYUVPixel",        sep_mpeg_get_rawpixel);
    lua_register(g_se_lua_context, "getUserString",          sep_get_idstring);
    lua_register(g_se_lua_context, "allowSocketCall",        sep_get_netperm);
    lua_register(g_se_lua_context, "vldpFlash",              sep_mpeg_set_flash);
    lua_register(g_se_lua_context, "dofile",                 sep_doluafile);

    lua_register(g_se_lua_context, "discAudioSuffix",        sep_audio_suffix);
    lua_register(g_se_lua_context, "musicLoad",              sep_music_load);
    lua_register(g_se_lua_context, "musicPlay",              sep_music_play);
    lua_register(g_se_lua_context, "musicIsPlaying",         sep_music_playing);
    lua_register(g_se_lua_context, "musicPause",             sep_music_pause);
    lua_register(g_se_lua_context, "musicResume",            sep_music_resume);
    lua_register(g_se_lua_context, "musicStop",              sep_music_stop);
    lua_register(g_se_lua_context, "musicSetVolume",         sep_music_volume);
    lua_register(g_se_lua_context, "musicUnload",            sep_music_unload);

    lua_register(g_se_lua_context, "mainBezelLoaded",        sep_bezel_loaded);
    lua_register(g_se_lua_context, "scoreBezelEnable",       sep_bezel_enable);
    lua_register(g_se_lua_context, "scoreBezelClear",        sep_bezel_clear);
    lua_register(g_se_lua_context, "scoreBezelCredits",      sep_bezel_credits);
    lua_register(g_se_lua_context, "scoreBezelTwinScoreOn",  sep_bezel_second_score);
    lua_register(g_se_lua_context, "scoreBezelScore",        sep_bezel_player_score);
    lua_register(g_se_lua_context, "scoreBezelLives",        sep_bezel_player_lives);
    lua_register(g_se_lua_context, "scoreBezelGetState",     sep_bezel_is_enabled);
    lua_register(g_se_lua_context, "controllerDoRumble",     sep_controller_rumble);
    lua_register(g_se_lua_context, "controllerIsValid",      sep_controller_valid);
    lua_register(g_se_lua_context, "joyMouseEnable",         sep_joymouse_enable);

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

    lua_register(g_se_lua_context, "spriteAnimGetFrame",     sep_sprite_get_frame);
    lua_register(g_se_lua_context, "spriteAnimIsPlaying",    sep_sprite_playing);
    lua_register(g_se_lua_context, "spriteAnimLoop",         sep_sprite_loop);
    lua_register(g_se_lua_context, "spriteAnimPause",        sep_sprite_pause);
    lua_register(g_se_lua_context, "spriteAnimPlay",         sep_sprite_play);
    lua_register(g_se_lua_context, "spriteAnimSetFrame",     sep_sprite_set_frame);

    //////////////////

    if (!TTF_WasInit())
        sep_die("SDL TTF font library is not available.");

    sep_capture_vldp();
    g_bLuaInitialized = true;

    m_scriptpath = data;

    size_t pos = m_scriptpath.find_last_of(".");
    std::string ext = m_scriptpath.substr(++pos);
    int zip = ext.compare("zip");

    if (m_zlua_arg || zip == 0) { // We have a zip

         g_zf = new ZipArchive(data);
         g_zf->open(ZipArchive::ReadOnly);

         if (g_zf->isOpen()) {

             const char *init = NULL;
             int size = 0;
             m_rom_zip = true;
             m_zipList = g_zf->getEntries();
             g_zipFile = data;
             std::string startup;

             sep_print("Loading ZIP based ROM");

             for (m_iter = m_zipList.begin(); m_iter != m_zipList.end(); ++m_iter) {
                 ZipEntry entry = *m_iter;
                 std::string name = entry.getName();
                 pos = m_scriptpath.find_last_of("/");
#ifdef WIN32
                 if (pos == (size_t)-1)
                     pos = m_scriptpath.find_last_of("\\");
#endif
                 std::string s = m_scriptpath.substr(++pos);

                 if (!m_altgame.empty()) s = m_altgame + ".singe";
                 else {
                     size_t period = s.find_last_of('.');
                     if (period != std::string::npos) {
                         s.replace(period, s.length() - period, ".singe");
                     }
                 }

                 startup = s;

                 if (name.find(s) != std::string::npos) {
                     init = (const char*)entry.readAsBinary();
                     size = entry.getSize();
                     break;
                 }
             }
             m_zipList.clear();
             sep_set_rampath();
             g_zf->close();

             if (size > 0 && luaL_loadbuffer(g_se_lua_context, init, size, data) == 0) {

                if (lua_pcall(g_se_lua_context, 0, 0, 0) != 0)
                    sep_lua_failure(g_se_lua_context, startup.c_str());

             } else {
                LOGE << sep_fmt("Startup singe file (%s) not found in %s!", startup.c_str(), data);
                sep_lua_failure(g_se_lua_context, startup.c_str());
             }

             if (init) delete[] const_cast<char*>(init);

         } else {
             sep_die("Failed opening Zip file: %s", data);
             g_bLuaInitialized = false;
         }

    } else {

        if (g_pSingeIn->get_es_path()) sep_set_espath();

        if (!m_altgame.empty()) {
            LOGI << sep_fmt("'%s': -usealt is only necessary with zip ROMs.",
                                m_altgame.c_str());
        }

        if (luaL_dofile(g_se_lua_context, data) != 0)
            sep_lua_failure(g_se_lua_context, NULL);
    }
}

void sep_rom_compressed(void)
{
   m_zlua_arg = true;
}

void sep_no_crosshair(void)
{
   m_show_crosshair = false;
}

void sep_upgrade_overlay(void)
{
   m_upgrade_overlay |= (1 << 0);
}

void sep_fullalpha_overlay(void)
{
   if (g_pSingeIn->cfm_overlay_unmask(g_pSingeIn->pSingeInstance))
       m_upgrade_overlay = (1 << 2);
}

void sep_enable_trace(void)
{
   m_trace = true;
}

////////////////////////////////////////////////////////////////////////////////

// Singe API Calls

static int sep_audio_suffix(lua_State *L)
{
  int n = lua_gettop(L);
  bool result = false;

  if (n == 1) {
    if (lua_isstring(L, 1)) {
      string suffix = lua_tostring(L, 1);
      result = g_pSingeIn->switch_altaudio(suffix.c_str());
    }
  }

  lua_pushboolean(L, result);
  return 1;
}

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
  LOGW << sep_fmt("Use the discAudioSuffix() API call");
  lua_pushnumber(L, 0);

  return 1;
}

static int sep_invalid_api_call(lua_State *L)
{
  lua_Debug ar;
  int level = 0;

  while (lua_getstack(L, level++, &ar))
  {
      lua_getinfo(L, "n", &ar);
      if (ar.name) sep_die("%s() is currently unsupported", ar.name);
  }

  LOGE << sep_fmt("Use an equivalent vldp() call");
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
  
  if (n == 3 || n == 4)
      if (lua_isnumber(L, 1))
          if (lua_isnumber(L, 2))
              if (lua_isnumber(L, 3))
              {
                  m_colorBackground.r = (uint8_t)lua_tonumber(L, 1);
                  m_colorBackground.g = (uint8_t)lua_tonumber(L, 2);
                  m_colorBackground.b = (uint8_t)lua_tonumber(L, 3);
                  if (n == 4 && lua_isnumber(L, 4) ) {
                      m_colorBackground.a = (uint8_t)lua_tonumber(L, 4);
                  } else {
                      m_colorBackground.a = (uint8_t)0;
                  }
              }
  return 0;
}

static int sep_color_set_forecolor(lua_State *L)
{
  int n = lua_gettop(L);
  
  if (n == 3 || n == 4 )
      if (lua_isnumber(L, 1))
          if (lua_isnumber(L, 2))
              if (lua_isnumber(L, 3))
              {
                  m_colorForeground.r = (uint8_t)lua_tonumber(L, 1);
                  m_colorForeground.g = (uint8_t)lua_tonumber(L, 2);
                  m_colorForeground.b = (uint8_t)lua_tonumber(L, 3);
                  if (n == 4 && lua_isnumber(L, 4) ) {
                      m_colorForeground.a = (uint8_t)lua_tonumber(L, 4);
                  } else {
                      m_colorForeground.a = (uint8_t)0;
                  }
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
            if (m_fontList.size() > 0) {
                TTF_CloseFont(m_fontList[id]);
                m_fontBuffers.erase(m_fontList[id]);
                m_fontList[id] = NULL;
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

        if (m_rom_zip) {

            auto zipfont = sep_font_zip(fontpath, points);
            temp = zipfont.font;

            if (temp)
                m_fontBuffers.emplace(temp, std::move(zipfont.buffer));

        } else {

           if (g_pSingeIn->get_es_path()) {
               char filepath[REWRITE_MAXPATH] = {0};
               lua_espath(fontpath.c_str(), filepath, REWRITE_MAXPATH);
               fontpath = filepath;
           }

           temp = TTF_OpenFont(fontpath.c_str(), points);
        }

        if (temp) {
            // Make it the current font and mark it as loaded.
            if (m_firstfont) sep_font_reset();
            m_fontList.push_back(temp);
            m_fontCurrent = m_fontList.size() - 1;
            result        = m_fontCurrent;
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
      m_fontQuality = lua_tonumber(L, 1);

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
      if (fontIndex < (int)m_fontList.size())
        m_fontCurrent = fontIndex;
    }

  return 0;
}

static int sep_font_sprite(lua_State *L)
{
  int n = lua_gettop(L);
  int result = -1;

  if (n == 1)
      if (lua_isstring(L, 1))
          if (m_fontCurrent >= 0) {

              TTF_Font *font = m_fontList[m_fontCurrent];
              if (!sep_font_valid(L, font, __func__)) return 0;
              SDL_Surface *textsurface = NULL;
              const char *message = lua_tostring(L, 1);

              switch (m_fontQuality) {
                  case 2:
                      textsurface = TTF_RenderText_Shaded(font, message, strlen(message), m_colorForeground, m_colorBackground);
                      break;
                  case 3:
                      textsurface = TTF_RenderText_Blended(font, message, strlen(message), m_colorForeground);
                      break;
                  default:
                      textsurface = TTF_RenderText_Solid(font, message, strlen(message), m_colorForeground);
                      break;
              }

              if (!(textsurface)) {
                  sep_trace(L);
                  sep_die("fontToSprite: Font surface is null!");
                  return 0;
              } else {

                  if (m_firstload) sep_sprite_reset();

                  SDL_SetSurfaceRLE(textsurface, true);
                  if (m_colorkey) SDL_SetSurfaceColorKey(textsurface, true, 0x0);

                  SDL_SetSurfaceBlendMode(textsurface, SDL_BLENDMODE_NONE);

                  m_spriteT sprite;
                  sprite.scaleX = 1.0;
                  sprite.scaleY = 1.0;
                  sprite.frame = NULL;
                  sprite.store = sep_copy_surface(textsurface, NULL);
                  sprite.present = textsurface;
                  sprite.animation = NULL;
                  m_sprites.push_back(sprite);
                  result = m_sprites.size() - 1;
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
    lua_pushnumber(L, video::get_scalefactor());
    return 1;
}

static int sep_mpeg_set_scale(lua_State *L)
{
    static double throttle = 0;
    double now = (double)clock() / CLOCKS_PER_SEC;
    bool result = false;

    if ((now - throttle) < 0.015) {
        lua_pushboolean(L, result);
        return 1;
    }

    int scale = luaL_checkinteger(L, 1);
    if ((unsigned)(scale - 25) <= 75) {
        video::reset_scalefactor(scale, 0, false);
        throttle = now;
        result = true;
    }

    lua_pushboolean(L, result);
    return 1;
}

static int sep_mpeg_focus_area(lua_State *L)
{
    int n = lua_gettop(L);
    int x, y, w, h;

    if (n == 4) {
        if (lua_isnumber(L, 1)) {
            x = lua_tonumber(L, 1);
            if (lua_isnumber(L, 2)) {
                y = lua_tonumber(L, 2);
                if (lua_isnumber(L, 3)) {
                    w = lua_tonumber(L, 3);
                    if (lua_isnumber(L, 4)) {
                        h = lua_tonumber(L, 4);
                        video::set_yuv_rect(x, y, w, h);
                    }
                }
            }
        }
    }
    return 0;
}

static int sep_mpeg_set_flash(lua_State *L)
{
    video::set_yuv_flash();
    return 0;
}

static int sep_mpeg_reset_focus(lua_State *L)
{
    video::reset_yuv_rect();
    return 0;
}

static int sep_mpeg_set_grayscale(lua_State *L)
{
    int n = lua_gettop(L);

    if (n == 1)
      if (lua_isboolean(L, 1))
        {
          video::set_grayscale(lua_toboolean(L, 1));
        }

    return 0;
}

static int sep_mpeg_set_luma(lua_State *L)
{
    if (!lua_isboolean(L, 1))
        return 0;

    bool state = lua_toboolean(L, 1);
    uint8_t luma = YUV_FLAG_LUMA;

    if (lua_gettop(L) == 2) {
        if (lua_isnumber(L, 2)) {
            int v = lua_tointeger(L, 2);
            if ((unsigned)v < 9) {
                luma = (uint8_t)v;
            }
        }
    }

    video::set_luma(state, luma);
    return 0;
}

static int sep_overlay_set_grayscale(lua_State *L)
{
    int n = lua_gettop(L);

    if (n == 1)
      if (lua_isboolean(L, 1) && (m_upgrade_overlay & (1 << 0)))
        m_upgrade_overlay = (m_upgrade_overlay & ~(1 << 1)) | (lua_toboolean(L, 1) << 1);

    return 0;
}

static int sep_mpeg_get_rotate(lua_State *L)
{
    float degree = video::get_fRotateDegrees();
    lua_pushnumber(L, degree);
    return 1;
}

static int sep_mpeg_set_rotate(lua_State *L)
{
    int n = lua_gettop(L);

    if (n == 1) {
        if (lua_isnumber(L, 1)) {
            float degree = lua_tonumber(L, 1);
            if (degree >= 0.0f && degree <= 360.0f) {
                video::set_fRotateDegrees(degree, false);
            }
        }
    }

    return 0;
}

static int sep_vldp_getvolume(lua_State *L)
{
	int vol = sound::get_chip_vldp_volume();
	lua_pushnumber(L, vol);
	return 1;
}

static int sep_vldp_setvolume(lua_State *L)
{
	int n = lua_gettop(L);

	if (n == 1)
		if (lua_isnumber(L, 1))
			sound::set_chip_vldp_volume(lua_tonumber(L, 1));

	return 0;
}

static bool yuv_from_buf(lua_State* L, uint8_t& Y, uint8_t& U, uint8_t& V)
{
    int n = lua_gettop(L);

    if (n == 2) {
        if (lua_isnumber(L, 1)) {
            if (lua_isnumber(L, 2)) {

                int xpos = (int)(lua_tointeger(L, 1) * m_se_yuv_scale_x);
                int ypos = (int)(lua_tointeger(L, 2) * m_se_yuv_scale_y);

                xpos = std::min(std::max(xpos, 0), m_se_yuv_buf.width - 1);
                ypos = std::min(std::max(ypos, 0), m_se_yuv_buf.height - 1);

                int UVx = std::min(xpos >> 1, m_se_yuv_buf.UVw - 1);
                int UVy = std::min(ypos >> 1, m_se_yuv_buf.UVh - 1);

                int Y_index  = ypos * m_se_yuv_buf.Ypitch + xpos;
                int UV_index = UVy * m_se_yuv_buf.Upitch + UVx;

                Y = m_se_yuv_buf.Y[Y_index];
                U = m_se_yuv_buf.U[UV_index];
                V = m_se_yuv_buf.V[UV_index];

                if (!m_pixelready)
                    m_pixelready = true;

                return true;
            }
        }
    }

    return false;
}

static void lua_push_triplet(lua_State* L, int x, int y, int z)
{
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    lua_pushnumber(L, z);
}

static int sep_mpeg_get_rawpixel(lua_State *L)
{
    uint8_t Y, U, V;

    if (yuv_from_buf(L, Y, U, V))
    {
        lua_push_triplet(L, Y, U, V);
    }
    else
    {
        lua_push_triplet(L, -1, -1, -1);
    }

    return 3;
}

static int sep_mpeg_get_pixel(lua_State *L)
{
    uint8_t Y, U, V;

    if (yuv_from_buf(L, Y, U, V))
    {
        int C = Y - 0x10;
        int D = U - 0x80;
        int E = V - 0x80;

        uint8_t R = sep_byte_clip((298 * C           + 409 * E + 128) >> 8);
        uint8_t G = sep_byte_clip((298 * C - 100 * D - 208 * E + 128) >> 8);
        uint8_t B = sep_byte_clip((298 * C + 516 * D           + 128) >> 8);

        lua_push_triplet(L, R, G, B);
    }
    else
    {
        lua_push_triplet(L, -1, -1, -1);
    }

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
                int w = (int)lua_tonumber(L, 1);
                int h = (int)lua_tonumber(L, 2);
                struct LuaTempState {
                    lua_State* L;
                    LuaTempState() { L = luaL_newstate(); }
                    ~LuaTempState() { if (L) lua_close(L); }
                } overlay;

                lua_pushinteger(overlay.L, SINGE_OVERLAY_CUSTOM);
                lua_pushinteger(overlay.L, w);
                lua_pushinteger(overlay.L, h);

                sep_set_overlaysize(overlay.L);
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
                           int w = (int)lua_tonumber(L, 2);
                           int h = (int)lua_tonumber(L, 3);
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

static int sep_set_overlayopacity(lua_State *L)
{
    int n = lua_gettop(L);

    if (n == 1)
      if (lua_isnumber(L, 1))
          video::set_overlay_alpha((uint8_t)sep_byte_clip(lua_tonumber(L, 1)));

    return 0;
}

static int sep_set_overlayfullalpha(lua_State *L)
{
   sep_fullalpha_overlay();
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
   lua_pushboolean(L, m_show_crosshair);
   return 1;
}

static int sep_draw_transparent(lua_State *L)
{
    int n = lua_gettop(L);

    if (n == 1)
        if (lua_isboolean(L, 1))
            m_colorkey = !lua_toboolean(L, 1);

    return 0;
}

static int sep_mpeg_get_width(lua_State *L)
{
    lua_pushnumber(L, g_pSingeIn->g_vldp_info->w);
    return 1;
}

static int sep_overlay_clear(lua_State *L)
{
    SDL_FillSurfaceRect(g_se_surface, NULL, SDL_MapSurfaceRGBA(g_se_surface,
                            m_colorBackground.r, m_colorBackground.g,
                            m_colorBackground.b, m_colorBackground.a));

    return 0;
}

static int sep_lua_rewrite(lua_State *L)
{
    lua_pushboolean(L, !m_rom_zip && g_pSingeIn->get_es_path());

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

static int sep_banner(lua_State *L)
{
    const char *txt;
    int height = 0x2f;

    int n = lua_gettop(L);

    if (n == 1 || n == 2)
        if (lua_isstring(L, 1))
        {
            txt = lua_tostring(L, 1);
            if (strlen(txt) > 60)
                return 0;

            if (lua_isnumber(L, 2)) {
                int h = (int)lua_tonumber(L, 2);
                if (h > 0 && h <= 95)
                    height = h;

            }
            video::draw_srt(txt, 1, height);
         }

    return 0;
}

static int sep_say(lua_State *L)
{
    bool outline = true;
    SDL_Color rgb = {255, 255, 255, 255};

    int n = lua_gettop(L);

    if (n == 3 || n == 4)
        if (lua_isnumber(L, 1))
          if (lua_isnumber(L, 2))
            if (lua_isstring(L, 3)) {
                if (n == 4 && lua_istable(L, 4)) {
                    lua_getfield(L, 4, "color");
                    if (lua_istable(L, -1))
                    {
                        lua_rawgeti(L, -1, 1); rgb.r = (uint8_t)lua_tointeger(L,-1); lua_pop(L,1);
                        lua_rawgeti(L, -1, 2); rgb.g = (uint8_t)lua_tointeger(L,-1); lua_pop(L,1);
                        lua_rawgeti(L, -1, 3); rgb.b = (uint8_t)lua_tointeger(L,-1); lua_pop(L,1);
                    }
                    lua_pop(L, 1);

                    lua_getfield(L, 4, "outline");
                    if (lua_isboolean(L, -1))
                        outline = lua_toboolean(L, -1);

                    lua_pop(L, 1);
                }
                g_pSingeIn->draw_string((char *)lua_tostring(L, 3), lua_tonumber(L, 1), lua_tonumber(L, 2), g_se_surface, rgb, outline);
	    }
    return 0;
}

static int sep_say_font(lua_State *L)
{
  int n = lua_gettop(L);
	  
  if (n == 3)
    if (lua_isnumber(L, 1))
      if (lua_isnumber(L, 2))
        if (lua_isstring(L, 3))
          if (m_fontCurrent >= 0) {

                TTF_Font *font = m_fontList[m_fontCurrent];
                if (!sep_font_valid(L, font, __func__)) return 0;
                SDL_Surface *textsurface = NULL;
                const char *message = lua_tostring(L, 3);

                switch (m_fontQuality) {
                case 2:
                    textsurface = TTF_RenderText_Shaded(font, message, strlen(message), m_colorForeground, m_colorBackground);
                    break;
                case 3:
                    textsurface = TTF_RenderText_Blended(font, message, strlen(message), m_colorForeground);
                    break;
                default:
                    textsurface = TTF_RenderText_Solid(font, message, strlen(message), m_colorForeground);
                    break;
                }

                if (!(textsurface)) {
                    sep_trace(L);
                    sep_die("fontPrint: Font surface is null!");
                } else {
                    SDL_Rect dest;
                    dest.w = textsurface->w;
                    dest.h = textsurface->h;
                    dest.x = lua_tonumber(L, 1) + m_se_overlay_scale_x;
                    dest.y = lua_tonumber(L, 2) + m_se_overlay_scale_y;

                    if (m_colorkey) SDL_SetSurfaceColorKey(textsurface, true, 0x0);

                    SDL_SetSurfaceBlendMode(textsurface, SDL_BLENDMODE_NONE);

                    SDL_BlitSurface(textsurface, NULL, g_se_surface, &dest);
                    SDL_DestroySurface(textsurface);
                }
	}
  return 0;
}

static int sep_get_idstring(lua_State *L)
{
    lua_pushstring(L, g_pSingeIn->g_local_info->Uid);
    return 1;
}

static int sep_get_netperm(lua_State *L)
{
    lua_pushboolean(L, net_send_enabled());
    return 1;
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

static int sep_subtitle_load(lua_State *L)
{
    bool result = false;
    int n = lua_gettop(L);

    if (n == 1)
        if (lua_isstring(L, 1)) {
            m_srt.clear();
            m_srt = LoadSRT(lua_tostring(L, 1), *g_se_disc_fps);

	    if (m_srt.empty())
                m_srt_display = false;
            else
                result = true;
        }

    lua_pushboolean(L, result);
    return 1;
}

static int sep_subtitle_position(lua_State *L)
{
    int n = lua_gettop(L);

    if (n == 1)
        if (lua_isnumber(L, 1)) {
            int height = (int)lua_tonumber(L, 1);
            if (height > 0 && height <= 95)
                m_srt_height = height;
        }

    return 0;
}

static int sep_subtitle_enable(lua_State *L)
{
    int n = lua_gettop(L);

    if (n == 1)
        if (lua_isboolean(L, 1)) {
            m_srt_display = lua_toboolean(L, 1);

            if (!m_srt_display)
                video::draw_srt("-", 2, -1);
        }

    return 0;
}

static int sep_subtitle_clear(lua_State *L)
{
    // immediate clear of timer
    video::draw_srt("-", 2, -1);

    return 0;
}

static int sep_search(lua_State *L)
{
  char s[7] = { 0 };
  int n = lua_gettop(L);

  if (n == 1)
    if (lua_isnumber(L, 1))
    {
      g_pSingeIn->framenum_to_frame(lua_tonumber(L, 1), s);
      g_pSingeIn->pre_search(s, true);

      if (g_pSingeIn->g_local_info->blank_during_searches)
          m_blank.trip = true;
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
          g_pSingeIn->pre_skip_backward(lua_tonumber(L, 1));

          if (g_pSingeIn->g_local_info->blank_during_skips)
              m_blank.trip = true;
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

          if (g_pSingeIn->g_local_info->blank_during_skips)
              m_blank.trip = true;
	}
      
  return 0;
}

static int sep_skip_to_frame(lua_State *L)
{
  int n = lua_gettop(L);

  if (n == 1)
  {
      if (lua_isnumber(L, 1))
      {
          char s[7] = { 0 };

          if (g_pSingeIn->g_local_info->blank_during_skips)
              m_blank.trip = true;

          g_pSingeIn->framenum_to_frame(lua_tonumber(L, 1), s);
          g_pSingeIn->pre_search(s, true);
          g_pSingeIn->pre_play();
          g_pause_state = false; // BY RDG2010
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
          SDL_free(m_soundList[id].buffer);
          m_soundList[id].load = false;
      }
  }
  return 0;
}

static int sep_sound_loadata(lua_State *L)
{
  int n = lua_gettop(L);
  int result = -1;
  bool load = false;

  if (n == 1 && lua_type(L, 1) == LUA_TSTRING)
  {
      size_t len;
      const char *data = lua_tolstring(L, 1, &len);

      SDL_IOStream *ops = SDL_IOFromConstMem(data, len);

      if (ops != NULL)
      {
          m_soundT temp;
          load = SDL_LoadWAV_IO(ops, 1, &temp.audioSpec,
                               &temp.buffer, &temp.length);

          if (load && audio_format(&temp.audioSpec)) {
              if (m_firstsnd) sep_sound_reset();
              m_soundList.push_back(temp);
              result = m_soundList.size() - 1;
              m_soundList[result].load = true;

          } else {

              if (temp.buffer) SDL_free(temp.buffer);
              sep_die("Unable to load data as sound");
              return result;
          }
      }
  }

  lua_pushnumber(L, result);
  return 1;
}

static int sep_sound_load(lua_State *L)
{
  int n = lua_gettop(L);
  int result = -1;
  bool load = false;

  if (n == 1 && lua_type(L, 1) == LUA_TSTRING)
  {
      std::string filepath = lua_tostring(L, 1);
      m_soundT temp;

      if (m_rom_zip) {

          load = sep_sound_zip(filepath, &temp);

      } else {

          if (g_pSingeIn->get_es_path())
          {
              char tmpPath[REWRITE_MAXPATH] = {0};
              lua_espath(filepath.c_str(), tmpPath, REWRITE_MAXPATH);
              filepath = tmpPath;
          }

          load = SDL_LoadWAV(filepath.c_str(), &temp.audioSpec,
                          &temp.buffer, &temp.length);

      }

      if (load) { // check audio_format() ?
          if (m_firstsnd) sep_sound_reset();
          m_soundList.push_back(temp);
          result = m_soundList.size() - 1;
          m_soundList[result].load = true;
      } else {
          sep_trace(L);
          if (temp.buffer) SDL_free(temp.buffer);
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
          result = g_pSingeIn->samples_play_sample(m_soundList[sound].buffer, m_soundList[sound].length, m_soundList[sound].audioSpec.channels, -1, sep_sound_ended);
      }

  lua_pushnumber(L, result);
  return 1;
}

static int sep_music_unload(lua_State *L)
{
  int n = lua_gettop(L);

  if (n == 1) {
      if (lua_isnumber(L, 1)) {
          int id = lua_tonumber(L, 1);
          if (!sep_mixer_valid(id, __func__)) return 0;
          MIX_DestroyTrack(m_mixerList[id].track);
          MIX_DestroyAudio(m_mixerList[id].audio);
          m_mixerList[id].track = NULL;
          m_mixerList[id].audio = NULL;
          m_mixerList[id].load = false;
      }
  }
  return 0;
}

static int sep_music_volume(lua_State *L)
{
  int n = lua_gettop(L);

  if (n == 1) {
      if (lua_isnumber(L, 1)) {
          float vol = LegacyVolume((int)lua_tonumber(L, 1));

          MIX_SetMixerGain(m_mixer, vol);
      }
  }
  return 0;
}

static int sep_music_load(lua_State *L)
{
  int n = lua_gettop(L);
  int result = -1;

  if (n == 1 && lua_type(L, 1) == LUA_TSTRING)
  {
      if (m_firstmix) {
          if (!sep_init_mixer())
              return result;
      }

      std::string mixpath = lua_tostring(L, 1);
      MIX_Audio *tmpAud = NULL;
      MIX_Track *tmpTrk = NULL;
      bool load = false;

      if (m_rom_zip)
      {
          load = sep_mixer_zip(mixpath, m_mixer, &tmpAud, &tmpTrk);
      }
      else {
          if (g_pSingeIn->get_es_path())
          {
              char tmpPath[REWRITE_MAXPATH] = {0};
              lua_espath(mixpath.c_str(), tmpPath, REWRITE_MAXPATH);
              mixpath = tmpPath;
          }

          tmpAud = MIX_LoadAudio(m_mixer, mixpath.c_str(), false);
          tmpTrk = MIX_CreateTrack(m_mixer);

          if (tmpTrk && tmpAud)
          {
              load = MIX_SetTrackAudio(tmpTrk, tmpAud);
          }
      }

      if (load)
      {
          m_mixerT mixer;
          if (m_firstmix) sep_mixer_reset();
          mixer.audio = tmpAud;
          mixer.track = tmpTrk;
          mixer.load = true;

          m_mixerList.push_back(mixer);
          result = m_mixerList.size() - 1;

      } else {
          sep_trace(L);
          MIX_DestroyTrack(tmpTrk);
          MIX_DestroyAudio(tmpAud);
          sep_die("Could not load %s as music data: %s", mixpath.c_str(), SDL_GetError());
          return result;
      }
  }

  lua_pushnumber(L, result);
  return 1;
}

static int sep_music_play(lua_State *L)
{
  int n = lua_gettop(L);
  const uint8_t lmax = 64;
  int result = -1;
  int loop = 0;

  if (n == 1 || n == 2)
      if (lua_isnumber(L, 1))
      {
          int num = lua_tonumber(L, 1);

          if (lua_isnumber(L, 2))
              loop = lua_tonumber(L, 2);

          loop = (loop > lmax) ? lmax : (loop < -1) ? -1 : loop;

          SDL_PropertiesID props = SDL_CreateProperties();
          SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, loop);

          if (!sep_mixer_valid(num, __func__)) return 0;
          result = MIX_PlayTrack(m_mixerList[num].track, props);

          SDL_DestroyProperties(props);
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

      if (!sep_sprite_valid(L, id, m_sprites[id].present, __func__)) return 0;

      if (!m_sprites[id].nokey) m_sprites[id].rekey = r;
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
  double scalex = 0, scaley = 0;
  SDL_Rect src, dest;

  // spriteDrawFrame(x, y, f, id)           - Simple animation
  // spriteDrawFrame(x, y, s, f, id)        - Scaled animation
  // spriteDrawFrame(x, y, sx, sy, f, id)   - Axis Scaled animation

  if ((n >= 4) && (n <= 6)) {
      if (lua_isnumber(L, 1)) {
          if (lua_isnumber(L, 2)) {
              if (lua_isnumber(L, 3)) {
                  if (lua_isnumber(L, 4)) {
                      dest.x = lua_tonumber(L, 1) + m_se_overlay_scale_x;
                      dest.y = lua_tonumber(L, 2) + m_se_overlay_scale_y;
                      if (n > 4) {
                          if (lua_isnumber(L, 5)) {
                              scalex = lua_tonumber(L, 3);
                              frame = lua_tonumber(L, 4);
                              id = lua_tonumber(L, 5);
                          }
                          if (n == 6 && lua_isnumber(L, 6)) {
                              scaley = lua_tonumber(L, 4);
                              frame = lua_tonumber(L, 5);
                              id = lua_tonumber(L, 6);
                          }
                      } else {
                          frame = lua_tonumber(L, 3);
                          id = lua_tonumber(L, 4);
                      }
                  }
              }
          }

          if (!sep_sprite_valid(L, id, m_sprites[id].present, __func__)) return 0;

          frame = (frame < 1 || frame > m_sprites[id].frames) ? 1 : frame;

          src.w = m_sprites[id].fwidth;
          src.h = m_sprites[id].present->h;
          src.x = m_sprites[id].fwidth * --frame;
          src.y = 0;

          dest.w = (n == 4) ? m_sprites[id].present->w : (m_sprites[id].fwidth * scalex);
          dest.h = (n == 4) ? m_sprites[id].present->h : (m_sprites[id].present->h * ((n == 6) ? scaley : scalex));

          if (n == 4) {
              SDL_BlitSurface(m_sprites[id].present, &src, g_se_surface, &dest);

          } else {
              SDL_BlitSurfaceScaled(m_sprites[id].present, &src, g_se_surface, &dest, SDL_SCALEMODE_LINEAR);
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
  double scalex = 0, scaley = 0;
  SDL_Rect dest;

  // spriteDrawRotatedFrame(x, y, id)          - Print the rotated frame
  // spriteDrawRotatedFrame(x, y, s, id)       - Print the frame scaled
  // spriteDrawRotatedFrame(x, y, sx, sy, id)  - Print the frame (axis) scaled

  if ((n >= 3) && (n <= 5)) {
      if (lua_isnumber(L, 1)) {
          if (lua_isnumber(L, 2)) {
              if (lua_isnumber(L, 3)) {
                  dest.x = lua_tonumber(L, 1) + m_se_overlay_scale_x;
                  dest.y = lua_tonumber(L, 2) + m_se_overlay_scale_y;
                  if (n > 3) {
                      if (lua_isnumber(L, 4)) {
                          scalex = lua_tonumber(L, 3);
                          id = lua_tonumber(L, 4);
                      }
                      if (n == 5 && lua_isnumber(L, 5)) {
                          scaley = lua_tonumber(L, 4);
                          id = lua_tonumber(L, 5);
                      }
                  } else {
                      id = lua_tonumber(L, 3);
                  }
              }
          }

          if (!sep_sprite_valid(L, id, m_sprites[id].frame, __func__)) return 0;

          dest.w = m_sprites[id].fwidth * ((n == 3) ? 1 : scalex);
          dest.h = m_sprites[id].present->h * ((n == 3) ? 1 : (n == 5 ? scaley : scalex));
          dest.x -= dest.w * 0.5;
          dest.y -= dest.h * 0.5;

          if (n == 3) {
              SDL_BlitSurface(m_sprites[id].frame, NULL, g_se_surface, &dest);

          } else {
              SDL_BlitSurfaceScaled(m_sprites[id].frame, NULL, g_se_surface, &dest, SDL_SCALEMODE_LINEAR);
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
  bool change = false;

  // spriteDraw(x, y, id)            - Simple draw
  // spriteDraw(x, y, c, id)         - Centered draw
  // spriteDraw(x, y, x2, y2, id)    - Scaled draw
  // spriteDraw(x, y, x2, y2, c, id) - Centered scaled draw

  if ((n >= 3) && (n <= 6)) {
      if (lua_isnumber(L, 1)) {
          if (lua_isnumber(L, 2)) {
              dest.x = lua_tonumber(L, 1) + m_se_overlay_scale_x;
              dest.y = lua_tonumber(L, 2) + m_se_overlay_scale_y;
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

              if (!sep_sprite_valid(L, id, m_sprites[id].present, __func__)) return 0;

              if (m_sprites[id].animation != NULL && m_sprites[id].animating) {
                  m_sprites[id].ticks += SDL_GetTicks() - m_sprites[id].last;
                  m_sprites[id].last = SDL_GetTicks();
                  while (m_sprites[id].animating) {
                      if (m_sprites[id].animation->delays[m_sprites[id].flow] < m_sprites[id].ticks) {
                          m_sprites[id].ticks -= m_sprites[id].animation->delays[m_sprites[id].flow];
                          m_sprites[id].flow++;
                          change = true;
                          if (m_sprites[id].flow >= m_sprites[id].animation->count) {
                              if (m_sprites[id].loop) {
                                  m_sprites[id].flow = 0;
                              } else {
                                  m_sprites[id].flow = m_sprites[id].animation->count - 1;
                                  m_sprites[id].animating = false;
                              }
                          }
                      } else {
                          break;
                      }
                  }
                  if (change) {
                      SDL_DestroySurface(m_sprites[id].frame);
                      m_sprites[id].frame = sep_copy_surface(m_sprites[id].animation->frames[m_sprites[id].flow], NULL);
                      SDL_DestroySurface(m_sprites[id].present);
                      m_sprites[id].present = rotozoomSurfaceXY(m_sprites[id].frame, 360 - m_sprites[id].angle, m_sprites[id].scaleX, m_sprites[id].scaleY, m_sprites[id].smooth);
                      if (m_sprites[id].rekey) SDL_SetSurfaceColorKey(m_sprites[id].present, true, 0x0);
                  }
              }

              if ((n == 3) || (n == 4)) {
                  dest.w = m_sprites[id].present->w;
                  dest.h = m_sprites[id].present->h;
              }
              if (center) {
                  dest.x -= dest.w * 0.5;
                  dest.y -= dest.h * 0.5;
              }

              if (m_sprites[id].blend)
                  SDL_SetSurfaceBlendMode(m_sprites[id].present, SDL_BLENDMODE_BLEND);

              if ((n == 3) || (n == 4)) {
                  SDL_BlitSurface(m_sprites[id].present, NULL, g_se_surface, &dest);
              } else {
                  SDL_BlitSurfaceScaled(m_sprites[id].present, NULL, g_se_surface, &dest, SDL_SCALEMODE_LINEAR);
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

static int sep_sprite_grid(lua_State *L)
{
  // spriteDrawGrid(x, y, id, rectx, recty, rectw, recth)
  const int args = 7;

  if (lua_gettop(L) != args)
      return 0;

  for (int i = 1; i <= args; i++) {
      if (!lua_isnumber(L, i))
          return 0;
  }

  int x  = lua_tonumber(L, 1);
  int y  = lua_tonumber(L, 2);
  int id = lua_tonumber(L, 3);

  SDL_Rect src;
  src.x = (int)lua_tonumber(L, 4);
  src.y = (int)lua_tonumber(L, 5);
  src.w = (int)lua_tonumber(L, 6);
  src.h = (int)lua_tonumber(L, 7);

  SDL_Rect dest;
  dest.x = x + m_se_overlay_scale_x;
  dest.y = y + m_se_overlay_scale_y;
  dest.w = src.w;
  dest.h = src.h;

  if (!sep_sprite_valid(L, id, m_sprites[id].present, __func__))
      return 0;

  if (src.x + src.w > m_sprites[id].present->w || src.y + src.h > m_sprites[id].present->h)
  {
      sep_trace(L);
      sep_die("Out of bound sprite dimensions given in spriteDrawGrid");
      return 0;
  }

  if (m_sprites[id].blend)
      SDL_SetSurfaceBlendMode(m_sprites[id].present, SDL_BLENDMODE_BLEND);

  SDL_BlitSurface(m_sprites[id].present, &src, g_se_surface, &dest);

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
          if (sep_sprite_valid(L, sprite, m_sprites[sprite].store, __func__))
          {
              result = m_sprites[sprite].frames;
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
        if (sep_sprite_valid(L, sprite, m_sprites[sprite].present, __func__))
        {
            result = m_sprites[sprite].present->h;
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
            if (!sep_sprite_valid(L, id, m_sprites[id].store, __func__)) return 0;
            SDL_DestroySurface(m_sprites[id].present);
            SDL_DestroySurface(m_sprites[id].frame);
            SDL_DestroySurface(m_sprites[id].store);
            m_sprites[id].present = NULL;
            m_sprites[id].store = NULL;
            m_sprites[id].frame = NULL;
            if (m_sprites[id].animation)
                IMG_FreeAnimation(m_sprites[id].animation);
            m_sprites[id].animation = NULL;
        }
    }
    return 0;
}

static int sep_sprite_loadata(lua_State *L)
{
    int n = lua_gettop(L);
    int result = -1;
    SDL_Surface *temp = NULL;

    if (n == 1 && lua_type(L, 1) == LUA_TSTRING)
    {
        size_t len;
        const char *data = lua_tolstring(L, 1, &len);
        SDL_IOStream *ops = SDL_IOFromConstMem(data, (int)len);

        if (ops != NULL)
        {
            temp = IMG_Load_IO(ops, 1);

            if (temp) {

                m_spriteT sprite;
                if (m_firstload) sep_sprite_reset();

                SDL_SetSurfaceRLE(temp, true);
                if (m_colorkey) SDL_SetSurfaceColorKey(temp, true, 0x0);

                SDL_SetSurfaceBlendMode(temp, SDL_BLENDMODE_NONE);

                sprite.store = sep_copy_surface(temp, NULL);
                sprite.present = temp;
                sprite.scaleX = 1.0;
                sprite.scaleY = 1.0;
                sprite.gfx = true;
                sprite.frame = NULL;
                sprite.animation = NULL;
                if (!m_colorkey) sprite.nokey = true;

                m_sprites.push_back(sprite);
                result = m_sprites.size() - 1;

            } else {
                sep_trace(L);
                sep_die("Unable to load data as a sprite");
                return result;
            }
        }
    }
   lua_pushnumber(L, result);
   return 1;
}

static int sep_sprite_load(lua_State *L)
{
    int n = lua_gettop(L);
    int result = -1;

    IMG_Animation *temp = NULL;

    if (n == 1 && lua_type(L, 1) == LUA_TSTRING)
    {
        std::string filepath = lua_tostring(L, 1);

        if (m_rom_zip) {

            temp = sep_animation_zip(filepath);

        } else {

           if (g_pSingeIn->get_es_path())
           {
               char tmpPath[REWRITE_MAXPATH] = {0};
               lua_espath(filepath.c_str(), tmpPath, REWRITE_MAXPATH);
               filepath = tmpPath;
           }
           temp = IMG_LoadAnimation(filepath.c_str());
       }

       if (temp) {

           m_spriteT sprite;
           if (m_firstload) sep_sprite_reset();

           if (temp->count < 2) {

               IMG_FreeAnimation(temp);
               temp = NULL;

               SDL_Surface *image = NULL;
               if (m_rom_zip) image = sep_surface_zip(filepath);
               else image = IMG_Load(filepath.c_str());

               if (!image) {
                   sep_trace(L);
                   sep_die("Unable to reload sprite image %s!", filepath.c_str());
                   return result;
               }

               SDL_Surface* convert = NULL;

               if (g_se_surface)
                   convert = SDL_ConvertSurface(image, g_se_surface->format);

               if (!convert) {
                   SDL_DestroySurface(image);
                   sep_trace(L);
                   sep_die("Unable to convert sprite image %s!", filepath.c_str());
                   return result;
               }

               SDL_DestroySurface(image);
               image = convert;

               SDL_SetSurfaceRLE(image, true);
               if (m_colorkey) SDL_SetSurfaceColorKey(image, true, 0x0);

               SDL_SetSurfaceBlendMode(image, SDL_BLENDMODE_NONE);

               sprite.store = sep_copy_surface(image, NULL);
               sprite.present = image;
               sprite.animation = NULL;

            } else {

               for (int x = 0; x < temp->count; x++) {
                   SDL_SetSurfaceBlendMode(temp->frames[x], SDL_BLENDMODE_NONE);
                   if (m_colorkey) SDL_SetSurfaceColorKey(temp->frames[x], true, 0x0);
	       }

               sprite.present = sep_copy_surface(temp->frames[0], NULL);
               sprite.store = sep_copy_surface(sprite.present, NULL);
               sprite.animation = temp;
           }

           sprite.scaleX = 1.0;
           sprite.scaleY = 1.0;
           sprite.gfx = true;
           sprite.frame = NULL;

           if (!m_colorkey) sprite.nokey = true;

           m_sprites.push_back(sprite);
           result = m_sprites.size() - 1;

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

            if (m_rom_zip) {

               temp = sep_surface_zip(filepath);

            } else {

               if (g_pSingeIn->get_es_path())
               {
                   char tmpPath[REWRITE_MAXPATH] = {0};
                   lua_espath(filepath.c_str(), tmpPath, REWRITE_MAXPATH);
                   filepath = tmpPath;
               }

               temp = IMG_Load(filepath.c_str());
            }

            if (temp) {

                if (m_firstload) sep_sprite_reset();

                SDL_Surface* convert = NULL;

                if (g_se_surface)
                    convert = SDL_ConvertSurface(temp, g_se_surface->format);

                if (!convert) {
                    SDL_DestroySurface(temp);
                    sep_trace(L);
                    sep_die("Unable to convert frame sprites %s!", filepath.c_str());
                    return result;
                }

                SDL_DestroySurface(temp);
                temp = convert;

                SDL_SetSurfaceRLE(temp, true);
                if (m_colorkey) SDL_SetSurfaceColorKey(temp, true, 0x0);

                SDL_SetSurfaceBlendMode(temp, SDL_BLENDMODE_NONE);

                m_spriteT sprite;
                sprite.scaleX = 1.0;
                sprite.scaleY = 1.0;
                sprite.frames = frames;
                sprite.fwidth = temp->w / frames;
                sprite.frame = NULL;
                sprite.store = sep_copy_surface(temp, NULL);
                sprite.present = temp;
                sprite.animation = NULL;
                if (!m_colorkey) sprite.nokey = true;

                m_sprites.push_back(sprite);
                result = m_sprites.size() - 1;

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
        if (sep_sprite_valid(L, sprite, m_sprites[sprite].present, __func__))
        {
            result = m_sprites[sprite].present->w;
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
        if (sep_sprite_valid(L, sprite, m_sprites[sprite].present, __func__))
        {
            result = m_sprites[sprite].fwidth;
        }
    }

  lua_pushnumber(L, result);
  return 1;
}

static int sep_sprite_blend(lua_State *L)
{
  int n = lua_gettop(L);
  int id = -1;

  if (n == 2) {
      if (lua_isboolean(L, 1) && lua_isnumber(L, 2)) {
          id = lua_tonumber(L, 2);
          if (!sep_sprite_valid(L, id, m_sprites[id].present, __func__)) return 0;
          m_sprites[id].blend = lua_toboolean(L, 1);
      }
  }

  if (id < 0) {
      sep_trace(L);
      sep_die("spriteBlend Failed!");
  }

  return 0;
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

                  if (m_sprites[id].frames == 0) {
                      sep_die("spriteRotateFrame: sprite has no frames!");
                      return 0;
                  }

                  if (!sep_sprite_valid(L, id, m_sprites[id].store, __func__)) return 0;
                  frame = (frame < 1 || frame > m_sprites[id].frames) ? 1 : frame;

                  SDL_Rect src;
                  src.w = m_sprites[id].fwidth;
                  src.h = m_sprites[id].present->h;
                  src.x = m_sprites[id].fwidth * --frame;
                  src.y = 0;

                  m_sprites[id].angle = a;
                  SDL_Surface *temp = sep_copy_surface(m_sprites[id].store, &src);

                  SDL_DestroySurface(m_sprites[id].frame);
                  m_sprites[id].frame = rotozoomSurfaceXY(temp, 360 - m_sprites[id].angle, m_sprites[id].scaleX, m_sprites[id].scaleY, m_sprites[id].smooth);
                  if (m_sprites[id].rekey) SDL_SetSurfaceColorKey(m_sprites[id].frame, true, 0x0);
                  SDL_DestroySurface(temp);
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
              if (!sep_sprite_valid(L, id, m_sprites[id].store, __func__) || !m_sprites[id].gfx) return 0;
              m_sprites[id].angle = a;
              SDL_DestroySurface(m_sprites[id].present);
              m_sprites[id].present = rotozoomSurfaceXY(m_sprites[id].store, 360 - m_sprites[id].angle, m_sprites[id].scaleX, m_sprites[id].scaleY, m_sprites[id].smooth);
              if (m_sprites[id].rekey) SDL_SetSurfaceColorKey(m_sprites[id].present, true, 0x0);
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
              if (!sep_sprite_valid(L, id, m_sprites[id].store, __func__) || !m_sprites[id].gfx) return 0;
              m_sprites[id].scaleX = x;
              m_sprites[id].scaleY = y;
              SDL_DestroySurface(m_sprites[id].present);
              m_sprites[id].present = rotozoomSurfaceXY(m_sprites[id].store, 360 - m_sprites[id].angle, m_sprites[id].scaleX, m_sprites[id].scaleY, m_sprites[id].smooth);
              if (m_sprites[id].rekey) SDL_SetSurfaceColorKey(m_sprites[id].present, true, 0x0);
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
                  if (!sep_sprite_valid(L, id, m_sprites[id].store, __func__) || !m_sprites[id].gfx) return 0;
                  m_sprites[id].angle = a;
                  m_sprites[id].scaleX = x;
                  m_sprites[id].scaleY = y;
                  SDL_DestroySurface(m_sprites[id].present);
                  m_sprites[id].present = rotozoomSurfaceXY(m_sprites[id].store, 360 - m_sprites[id].angle, m_sprites[id].scaleX, m_sprites[id].scaleY, m_sprites[id].smooth);
                  if (m_sprites[id].rekey) SDL_SetSurfaceColorKey(m_sprites[id].present, true, 0x0);
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
              if (!sep_sprite_valid(L, id, m_sprites[id].store, __func__)) return 0;
              m_sprites[id].smooth = s;
              SDL_DestroySurface(m_sprites[id].present);
              m_sprites[id].present = rotozoomSurfaceXY(m_sprites[id].store, 360 - m_sprites[id].angle, m_sprites[id].scaleX, m_sprites[id].scaleY, m_sprites[id].smooth);
              if (m_sprites[id].rekey) SDL_SetSurfaceColorKey(m_sprites[id].present, true, 0x0);
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
                          sep_draw_pixel(x1, y0, &m_colorForeground);
                          sep_draw_pixel(x0, y0, &m_colorForeground);
                          sep_draw_pixel(x0, y1, &m_colorForeground);
                          sep_draw_pixel(x1, y1, &m_colorForeground);
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
                          sep_draw_pixel(x0 - 1, y0,   &m_colorForeground);
                          sep_draw_pixel(x1 + 1, y0++, &m_colorForeground);
                          sep_draw_pixel(x0 - 1, y1,   &m_colorForeground);
                          sep_draw_pixel(x1 + 1, y1--, &m_colorForeground);
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
                      sep_draw_pixel(xo + x, yo + y, &m_colorForeground);
                      sep_draw_pixel(xo + y, yo + x, &m_colorForeground);
                      sep_draw_pixel(xo - y, yo + x, &m_colorForeground);
                      sep_draw_pixel(xo - x, yo + y, &m_colorForeground);
                      sep_draw_pixel(xo - x, yo - y, &m_colorForeground);
                      sep_draw_pixel(xo - y, yo - x, &m_colorForeground);
                      sep_draw_pixel(xo + y, yo - x, &m_colorForeground);
                      sep_draw_pixel(xo + x, yo - y, &m_colorForeground);

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
                      sep_draw_line(x1, y1, x2, y2, &m_colorForeground);
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
              sep_draw_pixel(x1, y1, &m_colorForeground);
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
                      sep_draw_line(x1, y1, x2, y1, &m_colorForeground);
                      sep_draw_line(x2, y1, x2, y2, &m_colorForeground);
                      sep_draw_line(x2, y2, x1, y2, &m_colorForeground);
                      sep_draw_line(x1, y2, x1, y1, &m_colorForeground);
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

static int sep_joymouse_enable(lua_State *L)
{
    int n = lua_gettop(L);

    if (n == 1) {
        if (lua_isboolean(L, 1))
            g_pSingeIn->cfm_joymouse_enable(g_pSingeIn->pSingeInstance, lua_toboolean(L, 1));
    }

    return 0;
}

static int sep_get_number_of_mice(lua_State *L)
{
    lua_pushnumber(L, g_pSingeIn->cfm_get_number_of_mice(g_pSingeIn->pSingeInstance));
    return 1;
}

static int sep_get_number_of_realmice(lua_State *L)
{
    lua_pushinteger(L,  get_realmouse_attached());
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
            x = m_tract.mouseX[m];
            y = m_tract.mouseY[m];
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
    int c = 0;
    int v = 0;
    bool result = false;

    if (n == 1 || n == 2) {
        if (lua_isnumber(L, 1)) {

            if (n == 2 && lua_isnumber(L, 2))
            {
                c = lua_tonumber(L, 1);
                a = lua_tonumber(L, 2);
            }
            else a = lua_tonumber(L, 1);

            if (get_gamepad_id(c) == nullptr || ((a < 0) || (a > AXIS_COUNT))) {
                sep_die("Invalid controller axis specified");
                return 0;
            }
            v = m_tract.axisvalue[(uint8_t)c][a];
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
    int s = 0, l = 0, id = 0;
    bool result = false;

    if (n >= 2 && n <= 3) {
        if (lua_isnumber(L, 1) && lua_isnumber(L, 2)) {

            int t1 = 0, t2 = 0;
            if (n == 3 && lua_isnumber(L, 3))
            {
                id = lua_tonumber(L, 1);
                t1 = lua_tonumber(L, 2);
                t2 = lua_tonumber(L, 3);
            }
            else
            {
                t1 = lua_tonumber(L, 1);
                t2 = lua_tonumber(L, 2);
            }
            if (get_gamepad_id(id) != nullptr) {
                if (t1 > 0 && t1 < 5 && t2 > 0 && t2 < 5) {
                    s = t1; l = t2;
                    result = true;;
                }
            }
        }
    }

    if (!result) sep_die("Failed on controllerDoRumble");
    g_pSingeIn->cfm_set_gamepad_rumble(g_pSingeIn->pSingeInstance, s, l, id);

    return 0;
}

static int sep_controller_valid(lua_State *L)
{
    int n = lua_gettop(L);
    bool valid = false;

    if (n == 1) {
        if (lua_isnumber(L, 1)) {
            SDL_Gamepad* c = get_gamepad_id(lua_tonumber(L, 1));
            if (c != nullptr) valid = true;
        }
    }

    lua_pushboolean(L, valid);
    return 1;
}

static int sep_controller_setwad(lua_State *L)
{
    int n = lua_gettop(L);

    if (n == 1)
        if (lua_isboolean(L, 1)) {
            set_gamepad_wad(lua_toboolean(L, 1));
        }

    return 0;
}

static int sep_controller_getwad(lua_State *L)
{
    lua_pushinteger(L, get_gamepad_wad());
    return 1;
}

static int sep_controller_attached(lua_State *L)
{
    lua_pushinteger(L, get_gamepad_attached());
    return 1;
}

static int sep_controller_button(lua_State *L)
{
    int n = lua_gettop(L);
    int b = 0;
    int c = 0;
    bool d = false;
    bool result = false;

    // Default to controller 0
    // controllerGetButton(b)              - Is controller button down (SDL button enum values)
    // controllerGetButton(c, b)           - ''         ''          '' (Framework requires bool)
    // controllerGetButton(c, b, f)        - Singe2 Framework : 'f' en/(dis) Framework adjustment
    //    ''       ''        ''            - bool 'f' = false: Use the SDL button enum values direct
    //    ''       ''        ''            - bool 'f' = true: Use Singe2 Framework calculation below

    if (n > 0 && n <= 3) {
        if (lua_isnumber(L, 1)) {

            bool framework = false; // No Framework adjustment by default

            if (n >= 2 && lua_isnumber(L, 2))
            {
                c = lua_tonumber(L, 1);
                b = lua_tonumber(L, 2);

                if (n == 3 && lua_isboolean(L, 3))
                    framework = lua_toboolean(L, 3);
            }
            else b = lua_tonumber(L, 1);

            if (m_trace) {
                if (framework) {
                    LOGW << sep_fmt("Framework adjustment will be made to (%d)", b);
                }
            }

            // Singe2 has Framework adjustment
            if (framework) b = ((b - 500) - 18);

            if ((b < 0) || (b >= SDL_GAMEPAD_BUTTON_COUNT)) {
                sep_die("Invalid controller button specified");
                return 0;
            }

            SDL_Gamepad* controller = get_gamepad_id(c);

            if (controller != nullptr) {
                d = SDL_GetGamepadButton(controller, get_button(b));
                result = true;
            }
        }
    }

    if (!result) sep_die("Failed on controllerGetButton");

    lua_pushboolean(L, d);
    return 1;
}

static int sep_get_scriptpath(lua_State *L)
{
    lua_pushstring(L, m_scriptpath.c_str());
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

            if (s < 0 || s > SDL_SCANCODE_COUNT) {
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

static int sep_keyboard_block_quit(lua_State *L)
{
    int n = lua_gettop(L);

    if (n == 1) {
        if (lua_isboolean(L, 1))
            g_pSingeIn->cfm_block_quit(g_pSingeIn->pSingeInstance, lua_toboolean(L, 1));
    }
    return 0;
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

	m_se_grunt = false;
	sep_die("User requested a quit.");
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

static int sep_music_playing(lua_State *L)
{
        bool result = ((GetCurrentlyPlayingTrack()) == NULL) ? false : true;

        lua_pushboolean(L, result);
        return 1;
}


static int sep_music_pause(lua_State *L)
{
	MIX_PauseTrack(GetCurrentlyPlayingTrack());

	return 0;
}

static int sep_music_resume(lua_State *L)
{
	MIX_ResumeTrack(GetCurrentlyPlayingTrack());

	return 0;
}

static int sep_music_stop(lua_State *L)
{
	int fade = 0;
	int n = lua_gettop(L);

	if (n == 1)
	    if (lua_isnumber(L, 1))
		fade = std::min(std::max((int)lua_tonumber(L, 1), 0), 10000);

	MIX_StopTrack(GetCurrentlyPlayingTrack(), (Sint64)fade);

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

static int sep_sound_getvolume(lua_State *L)
{
	int vol = sound::get_chip_nonvldp_volume();
	lua_pushnumber(L, vol);
	return 1;
}

static int sep_sound_setvolume(lua_State *L)
{
	int n = lua_gettop(L);

	if (n == 1)
		if (lua_isnumber(L, 1))
			sound::set_chip_nonvldp_volume(lua_tonumber(L, 1));

	return 0;
}

static int sep_doluafile(lua_State *L)
{
        int n = lua_gettop(L);

        if (n == 1)
        {
            const char *fname = luaL_optstring(L, 1, NULL);

            if (!m_rom_zip) {

                if (luaL_dofile(L, fname) != 0)
                    sep_die("error compiling script: %s", lua_tostring(L, -1));

                return 0;
            }

            std::string file = fname;
            std::string found;

            if (!g_zf->isOpen()) g_zf->open(ZipArchive::ReadOnly);

            if (!g_zf->isOpen())
                return 0;

            int size = 0;
            char* entry = nullptr;

            m_zipList = g_zf->getEntries();

            for (m_iter = m_zipList.begin(); m_iter != m_zipList.end(); ++m_iter) {

                ZipEntry zipEntry = *m_iter;
                std::string name = zipEntry.getName();

                if (name.find(file) != std::string::npos) {
                    entry = static_cast<char*>(zipEntry.readAsBinary());
                    size = zipEntry.getSize();
                    found = name;
                    break;
                }
            }

            m_zipList.clear();
            if (!*g_zlfs) g_zf->close();

            if (!entry || size <= 0) {
                sep_print("Zip entry: %s", fname);
                sep_die("error loading file from Zip: %s", fname);
                return 0;
            }

            int loadStatus = luaL_loadbuffer(L, entry, size, g_zipFile);

            delete[] entry;

            if (loadStatus == 0)
            {
                if (lua_pcall(L, 0, 0, 0) != 0)
                    sep_die("error compiling script: %s: %s", found.c_str(), lua_tostring(L, -1));
            }
            else
                sep_die("error loading file from Zip: %s", lua_tostring(L, -1));
       }

       return 0;
}

static int sep_bezel_loaded(lua_State *L)
{
    lua_pushboolean(L, video::get_bezelstatus());
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

static int sep_sprite_get_frame(lua_State *L)
{
    int n = lua_gettop(L);
    int result = -1;
    int id = -1;

    if (n == 1) {
       if (lua_isstring(L, 1)) {
           id = lua_tonumber(L, 1);
           if (!sep_animation_valid(L, id, m_sprites[id].animation, __func__)) return 0;
           result = m_sprites[id].flow;
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
            if (!sep_animation_valid(L, id, m_sprites[id].animation, __func__)) return 0;
            result = m_sprites[id].animating;
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
                if (!sep_animation_valid(L, id, m_sprites[id].animation, __func__)) return 0;
                m_sprites[id].loop = loop;
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
            if (!sep_animation_valid(L, id, m_sprites[id].animation, __func__)) return 0;
            m_sprites[id].animating = false;
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
            if (!sep_animation_valid(L, id, m_sprites[id].animation, __func__)) return 0;
            if (m_sprites[id].animating == false) {
                m_sprites[id].last = SDL_GetTicks();
            }
            m_sprites[id].animating = true;
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
                if (!sep_animation_valid(L, id, m_sprites[id].animation, __func__)) return 0;
                if ((f >= 0) && (f < m_sprites[id].animation->count)) {
                    m_sprites[id].flow = f;
                    m_sprites[id].ticks = 0;
                }
            }
        }
    }

    if (id < 0) {
        sep_die("spriteAnimSetFrame Failed!");
    }
    return 0;
}
