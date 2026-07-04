/*
 * ____ HYPSEUS COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2026 DirtBagXon
 *
 * This file is part of HYPSEUS SINGE, a laserdisc arcade game emulator
 *
 * HYPSEUS SINGE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HYPSEUS SINGE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <algorithm>
#include <vector>
#include "splash.h"
#include "video.h"
#include "../hypseus.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

void splash(bool eon) {

    Uint8 alpha = 0;
    SDL_Event event{};
    SDL_Rect workspace;
    SDL_FRect logical{};
    bool running = true;
    float move_t = 0.0f;
    Uint8 last_alpha = 0xFF;
    Uint32 move_elapsed = 0;
    Uint32 current_frame = 0;
    Uint32 start_time = SDL_GetTicks();
    Uint32 frame_switch = start_time;
    TTF_Font *font = TTF_OpenFont("fonts/default.ttf", 16);

    if (!font) return;

    int frames_indices[FRAMES]{};
    SDL_IOStream* ops = SDL_IOFromConstMem(ghc, sizeof(ghc));
    SDL_Surface *splash_surface = IMG_Load_IO(ops, 1);

    if (!splash_surface) {
        TTF_CloseFont(font);
        font = nullptr;
        return;
    }

    ops = SDL_IOFromConstMem(eon ? ghlq : ghlo, eon ? sizeof(ghlq) : sizeof(ghlo));
    SDL_Surface *logo_surface = IMG_Load_IO(ops, 1);

    if (!logo_surface){
        SDL_DestroySurface(splash_surface);
        TTF_CloseFont(font);
        splash_surface = NULL;
        font = nullptr;
        return;
    }

    const SDL_PixelFormatDetails* fmt = SDL_GetPixelFormatDetails(splash_surface->format);
    Uint32 key = SDL_MapRGB(fmt, nullptr, 0x0, 0x0, 0x0);
    SDL_SetSurfaceColorKey(splash_surface, true, key);

    SDL_Window *window = video::get_window();
    SDL_Renderer *renderer = video::get_renderer();
    SDL_DisplayID winId = SDL_GetDisplayForWindow(video::get_window());

    SDL_GetDisplayBounds(winId, &workspace);
    int which = video::get_display_no();

    SDL_RectToFRect(&workspace, &logical);

    if (!video::get_fullscreen())
        logical = SDL_FRect{ 0x0, 0x0, 640, 480 };

    if (which != 0) {
        std::vector<SDL_Rect> dimensions = video::get_displays();
        SDL_SetWindowPosition(window, dimensions[which].x +
                             ((dimensions[which].w - (int)logical.w) >> 1),
                                dimensions[which].y + ((dimensions[which].h
                                    - (int)logical.h) >> 1));
        if (video::get_fullscreen()) SDL_RectToFRect(&dimensions[which], &logical);
    }

    const float logo_w = (eon) ? 0x184 : 0x140;
    const float logo_h = (eon) ? 0x017 : 0x016;

    const int start_x = (logical.w - DSC) / 2;
    const int start_y = (logical.h - DSC) / 2;
    const char* v = get_hypseus_version();

    SDL_FRect dscrect = {
        (float)(((logical.w - DSC) / 2) + (EMBW / 2) - 0x10),
        (float)((logical.h - EMBH) / 2),
        DSC,
        DSC
    };

    SDL_FRect animRect = dscrect;

    SDL_FRect embrect = {
        (float)(((logical.w - EMBW) / 2) - 0x14),
        (float)((logical.h - EMBH) / 2),
        EMBW,
        EMBH
    };

    SDL_FRect frames[] = {
        SDL_FRect{ 0x011, 0x00, EMBW, 0x54 },
        SDL_FRect{ 0x154, 0x05, 0x4A, 0x50 },
        SDL_FRect{ 0x19F, 0x05, 0x4A, 0x50 },
        SDL_FRect{ 0x1EA, 0x05, 0x4A, 0x50 },
        SDL_FRect{ 0x235, 0x05, 0x4A, 0x50 },
        SDL_FRect{ 0x280, 0x05, 0x4A, 0x50 },
        SDL_FRect{ 0x2CB, 0x05, 0x4A, 0x50 }
    };

    SDL_FRect ld_rect = {
        ((logical.w - logo_w) / 2),
        (logical.h * 94) / 100,
        logo_w, logo_h
    };

    for (int i = 0; i < FRAMES; i++) frames_indices[i] = (i % 6) + 1;

    SDL_Texture *splash_frames = SDL_CreateTextureFromSurface(renderer, splash_surface);
    SDL_DestroySurface(splash_surface);
    splash_surface = NULL;

    if (!splash_frames) {
        TTF_CloseFont(font);
        font = nullptr;
        return;
    }

    SDL_Texture* logo_frame = SDL_CreateTextureFromSurface(renderer, logo_surface);
    SDL_DestroySurface(logo_surface);
    logo_surface = NULL;

    if (!logo_frame) {
        SDL_DestroyTexture(splash_frames);
        TTF_CloseFont(font);
        splash_frames = NULL;
        font = nullptr;
        return;
    }

    for (SDL_Texture* tex : { splash_frames, logo_frame })
    {
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);
    }

    int cw, ch;
    TTF_GetStringSize(font, v, 0, &cw, &ch);

    TTF_Text *vers = TTF_CreateText(video::get_font_engine(), font, v, strlen(v));
    TTF_SetTextColor(vers, 0xE0, 0xE0, 0xE0, 0x64);

    const int verposx = (logical.w / 2) - (cw / 2);
    const int verposy = (logical.h / 2) + (ch * 1.75);

    while (running) {

        Uint32 now = SDL_GetTicks();
        Uint32 elspd = now - start_time;
        int i = elspd / FINTL;

        if (elspd >= SDUR)
            break;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_KEY_DOWN &&
                event.key.key == SDLK_ESCAPE) {
                set_quitflag();
                running = false;
            }
        }

        if (now - frame_switch >= FINTL) {
            current_frame = (current_frame < (FRAMES - 1))
                            ? current_frame + 1 : 0;
            frame_switch = now;
        }

        if (elspd > MDLY)
            move_elapsed = elspd - MDLY;

        if (move_t < 1.0f)
            move_t = std::min((float)move_elapsed / (float)MDUR, 1.0f);

        int target_x = start_x + (float)((dscrect.x - start_x) * move_t);
        int target_y = start_y + (float)((dscrect.y - start_y) * move_t);

        float scale_t = 2.0f - move_t;

        int scaled_w = (float)(DSC * scale_t);
        int scaled_h = (float)(DSC * scale_t);

        animRect.w = scaled_w;
        animRect.h = scaled_h;
        animRect.x = target_x - (scaled_w - DSC) / 2;
        animRect.y = target_y - (scaled_h - DSC) / 2;

        if (elspd > (MDLY + MDUR))
        {
            float fade_t = (float)(elspd - MDLY - MDUR) / (float)FADES;
            if (fade_t > 1.0f) fade_t = 1.0f;

            fade_t = smoothout(fade_t);
            alpha = (Uint8)(255.0f * fade_t);
        }

        SDL_RenderClear(renderer);

        int blk = (i < 8) ? 4 : std::max(1, 3 - (i - 8) / 6);

        if (alpha > 0x80 && (i / blk) % 2 == 0)
        {
            if (last_alpha != 0xFF)
                SDL_SetTextureAlphaMod(splash_frames, 0xFF);

            SDL_RenderTexture(renderer, splash_frames,
                      &frames[frames_indices[current_frame]], &animRect);

            if (alpha < 0xFF)
                SDL_SetTextureAlphaMod(splash_frames, alpha);

            last_alpha = alpha;

            SDL_RenderTexture(renderer, splash_frames, &frames[0], &embrect);
        }
        else
        {
            if (last_alpha != alpha)
                SDL_SetTextureAlphaMod(splash_frames, alpha);

            SDL_RenderTexture(renderer, splash_frames, &frames[0], &embrect);

            if (alpha < 0xFF)
                SDL_SetTextureAlphaMod(splash_frames, 0xFF);

            last_alpha = 0xFF;

            SDL_RenderTexture(renderer, splash_frames,
                       &frames[frames_indices[current_frame]], &animRect);
        }

        if (alpha)
        {
            if (alpha < 0xFF)
                SDL_SetTextureAlphaMod(logo_frame, alpha);

            SDL_RenderTexture(renderer, logo_frame, NULL, &ld_rect);
        }

        if (elspd > 0xBB8)
            TTF_DrawRendererText(vers, verposx, verposy);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(logo_frame);
    SDL_DestroyTexture(splash_frames);
    TTF_CloseFont(font);
    splash_frames = NULL;
    logo_frame = NULL;
    font = nullptr;
}
