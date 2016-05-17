#include <SDL.h>

#include "video/SDL_FontCache.h"
#include "vldp/vldp.h"     // VLDP stuff

#include <unistd.h> // for chdir / sleep

#ifndef WIN32
#include <string.h>
#include <stdio.h>  // for stderr
#include <stdlib.h> // for srand
#endif

#define MPO_MALLOC(bytes) malloc(bytes)

// only frees memory if it hasn't already been freed
#define MPO_FREE(ptr)                                                          \
    if (ptr != 0) {                                                            \
        free(ptr);                                                             \
        ptr = 0;                                                               \
    }

const unsigned int uVidWidth  = 320;
const unsigned int uVidHeight = 240;

typedef const struct vldp_out_info *(*initproc)(const struct vldp_in_info *in_info);
initproc pvldp_init; // pointer to the init proc ...

// pointer to all functions the VLDP exposes to us ...
const struct vldp_out_info *g_vldp_info = NULL;

// info that we provide to the VLDP DLL
struct vldp_in_info g_local_info;

unsigned int g_uFrameCount = 0; // to measure framerate

unsigned int g_uQuitFlag = 0;

SDL_Window *g_window     = NULL;
SDL_Renderer *g_renderer = NULL;
SDL_Texture *g_texture   = NULL;
FC_Font     *g_font      = NULL;
SDL_RendererInfo g_rendererinfo;

void printerror(const char *cpszErrMsg) { puts(cpszErrMsg); }

int prepare_frame_callback(uint8_t *Yplane, uint8_t *Uplane, uint8_t *Vplane, int Ypitch, int Upitch, int Vpitch)
{
    VLDP_BOOL result = VLDP_FALSE;

#ifdef SHOW_FRAMES
    result = (SDL_UpdateYUVTexture(g_texture, NULL,
			    Yplane, Ypitch,
			    Uplane, Upitch,
			    Vplane, Vpitch) == 0) ? VLDP_TRUE : VLDP_FALSE;
#endif // SHOW FRAMES

    return result;
}

void display_frame_callback()
{
    g_uFrameCount++;

#ifdef SHOW_FRAMES
    SDL_RenderClear(g_renderer);
    SDL_RenderCopy(g_renderer, g_texture, NULL, NULL);
    FC_Draw(g_font, g_renderer, 0, 0, "%d", g_uFrameCount);
    SDL_RenderPresent(g_renderer); // display it!
#endif                             // SHOW_FRAMES
}

void report_parse_progress_callback(double percent_complete_01) {}

#include <signal.h>

void report_mpeg_dimensions_callback(int width, int height)
{
#ifdef SHOW_FRAMES
    if (!g_texture) {
        SDL_SetWindowSize(g_window, width, height);
        g_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_YV12,
                                      SDL_TEXTUREACCESS_STATIC, width, height);
    }
#endif // SHOW_FRAMES
}

void blank_overlay_callback() {}

unsigned int get_ticks() { return SDL_GetTicks(); }

void start_test(const char *cpszMpgName)
{
    SDL_Event event;

    g_uFrameCount = 0;
    if (g_vldp_info->open_and_block(cpszMpgName)) {
        unsigned int uStartMs = get_ticks();
        g_local_info.uMsTimer = uStartMs;
        if (g_vldp_info->play(uStartMs)) {
            unsigned int uEndMs, uDiffMs;
            // wait for entire mpeg to play ...
            while ((g_vldp_info->status == STAT_PLAYING) && (!g_uQuitFlag) &&
                   (g_uFrameCount < 720)) {
                g_local_info.uMsTimer = get_ticks();
                usleep(1000);
                while (SDL_PollEvent(&event)) switch (event.type) {
                    case SDL_QUIT:
                        g_uQuitFlag = 1;
                        break;
                    case SDL_WINDOWEVENT_RESIZED:
                        SDL_SetWindowSize(g_window, event.window.data1, event.window.data2);
                        break;
                    default:
                        break;
                    }
            }
            uEndMs  = get_ticks();
            uDiffMs = uEndMs - uStartMs;
            printf("%u frames played in %u ms (%u fps)\n", g_uFrameCount,
                   uDiffMs, (g_uFrameCount * 1000) / (uDiffMs));
        } else
            printerror("play() failed");
    } else
        printerror("Could not open video file");
}

void set_cur_dir(const char *exe_loc)
{
    int index = strlen(exe_loc) - 1; // start on last character
    char path[512];

    // locate the preceeding / or \ character
    while ((index >= 0) && (exe_loc[index] != '/') && (exe_loc[index] != '\\')) {
        index--;
    }

    // if we found a leading / or \ character
    if (index >= 0) {
        strncpy(path, exe_loc, index);
        path[index] = 0;
        if (chdir(path) != 0) printf("Unable to chdir: %s\n", path);
    }
}

int main(int argc, char **argv)
{
    set_cur_dir(argv[0]);
#ifdef SHOW_FRAMES
    SDL_Init(SDL_INIT_VIDEO);
    g_window = SDL_CreateWindow("testvldp", SDL_WINDOWPOS_UNDEFINED,
                                SDL_WINDOWPOS_UNDEFINED, uVidWidth, uVidHeight,
                                SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    g_renderer = SDL_CreateRenderer(g_window, -1, 0);
    if (!g_renderer) {
        printf("Unable to create renderer: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_GetRendererInfo(g_renderer, &g_rendererinfo);
    printf("Using %s rendering\n", g_rendererinfo.name);

    g_font = FC_CreateFont();
    if (FC_LoadFont(g_font, g_renderer, "fonts/default.ttf", 18, FC_MakeColor(255,255,0,255), TTF_STYLE_NORMAL) == 0)
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
#endif

    pvldp_init = (initproc)&vldp_init;

    // if all functions were found, then return success
    if (pvldp_init) {
        g_local_info.prepare_frame          = prepare_frame_callback;
        g_local_info.display_frame          = display_frame_callback;
        g_local_info.report_parse_progress  = report_parse_progress_callback;
        g_local_info.report_mpeg_dimensions = report_mpeg_dimensions_callback;
        g_local_info.render_blank_frame     = blank_overlay_callback;
        g_local_info.blank_during_searches  = 0;
        g_local_info.blank_during_skips     = 0;
        g_local_info.GetTicksFunc           = get_ticks;
        g_vldp_info                         = pvldp_init(&g_local_info);

        // if initialization succeeded
        if (g_vldp_info != NULL) {
            start_test(argv[1]);
            g_vldp_info->shutdown();
        } else
            printerror("vldp_init() failed");
    } else {
        printerror("VLDP LOAD ERROR : vldp_init could not be loaded");
    }

#ifdef SHOW_FRAMES
    SDL_DestroyTexture(g_texture);
    SDL_Quit();
#endif // SHOW_FRAMES

    return 0;
}
