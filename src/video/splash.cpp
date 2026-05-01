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
#include "splash.h"
#include "video.h"
#include "../hypseus.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

void splash(bool eon) {

    Uint8 alpha = 0;
    SDL_Event event{};
    SDL_Rect logical{};
    bool running = true;
    float move_t = 0.0f;
    Uint8 last_alpha = 0xFF;
    Uint32 move_elapsed = 0;
    Uint32 current_frame = 0;
    Uint32 start_time = SDL_GetTicks();
    Uint32 frame_switch = start_time;

    int frames_indices[FRAMES]{};
    SDL_RWops* ops = SDL_RWFromConstMem(ghc, sizeof(ghc));
    SDL_Surface *splash_surface = IMG_Load_RW(ops, 1);

    if (!splash_surface) return;

    ops = SDL_RWFromConstMem(eon ? ghlq : ghlo, eon ? sizeof(ghlq) : sizeof(ghlo));
    SDL_Surface *logo_surface = IMG_Load_RW(ops, 1);

    if (!logo_surface){
        SDL_FreeSurface(splash_surface);
        splash_surface = NULL;
        return;
    }

    Uint32 key = SDL_MapRGB(splash_surface->format, 0x0, 0x0, 0x0);
    SDL_SetColorKey(splash_surface, SDL_TRUE, key);

    FC_Font* font = FC_CreateFont();
    FC_Scale scale = FC_MakeScale(1.0f, 1.0f);
    SDL_Color color = {0xE0, 0xE0, 0xE0, 0x64};
    SDL_Renderer *renderer = video::get_renderer();
    FC_LoadFont(font, renderer, "fonts/default.ttf", 0x12, color, TTF_STYLE_NORMAL);

    const bool fs = video::get_fullscreen() || video::get_fullwindow();

    logical.w = (fs) ? (int)video::get_logical_width() :
                       (int)video::get_viewport_width();
    logical.h = (fs) ? (int)video::get_logical_height() :
                       (int)video::get_viewport_height();

    const int logo_w = (eon) ? 0x184 : 0x140;
    const int logo_h = (eon) ? 0x017 : 0x016;

    const int start_x = (logical.w - DSC) >> 1;
    const int start_y = (logical.h - DSC) >> 1;
    const char* v = get_hypseus_version();

    SDL_Rect dscrect = {
        (int)(((logical.w - DSC) >> 1) + (EMBW >> 1) - 0x10),
        (int)((logical.h - EMBH) >> 1),
        DSC,
        DSC
    };

    SDL_Rect animRect = dscrect;

    SDL_Rect embrect = {
        (int)(((logical.w - EMBW) >> 1) - 0x14),
        (int)((logical.h - EMBH) >> 1),
        EMBW,
        EMBH
    };

    SDL_Rect frames[] = {
        SDL_Rect{ 0x011, 0x00, EMBW, 0x54 },
        SDL_Rect{ 0x154, 0x05, 0x4A, 0x50 },
        SDL_Rect{ 0x19F, 0x05, 0x4A, 0x50 },
        SDL_Rect{ 0x1EA, 0x05, 0x4A, 0x50 },
        SDL_Rect{ 0x235, 0x05, 0x4A, 0x50 },
        SDL_Rect{ 0x280, 0x05, 0x4A, 0x50 },
        SDL_Rect{ 0x2CB, 0x05, 0x4A, 0x50 }
    };

    SDL_Rect ld_rect = {
        ((logical.w - logo_w) >> 1),
        (logical.h * 94) / 100,
        logo_w, logo_h
    };

    for (int i = 0; i < FRAMES; i++) frames_indices[i] = (i % 6) + 1;

    SDL_Texture *splash_frames = SDL_CreateTextureFromSurface(renderer, splash_surface);
    SDL_FreeSurface(splash_surface);
    splash_surface = NULL;

    if (!splash_frames) {
        FC_FreeFont(font);
        return;
    }

    SDL_Texture* logo_frame = SDL_CreateTextureFromSurface(renderer, logo_surface);
    SDL_FreeSurface(logo_surface);
    logo_surface = NULL;

    if (!logo_frame) {
        SDL_DestroyTexture(splash_frames);
        splash_frames = NULL;
        FC_FreeFont(font);
        return;
    }

    for (SDL_Texture* tex : { splash_frames, logo_frame })
    {
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
#if SDL_VERSION_ATLEAST(2,0,12)
        SDL_SetTextureScaleMode(tex, SDL_ScaleModeLinear);
#endif
    }

    FC_SetFilterMode(font, FC_FILTER_LINEAR);
    FC_Effect effect = FC_MakeEffect(FC_ALIGN_CENTER, scale, color);

    int cw = FC_GetWidth(font, "%c", 0x76);
    int ch = FC_GetHeight(font, "%c", 0x76);
    const int verposx = (logical.w >> 1) - cw;
    const int verposy = (logical.h >> 1) + (ch * 1.75);

    while (running) {

        Uint32 now = SDL_GetTicks();
        Uint32 elspd = now - start_time;
        int i = elspd / FINTL;

        if (elspd >= SDUR)
            break;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN &&
                event.key.keysym.sym == SDLK_ESCAPE) {
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

        int target_x = start_x + (int)((dscrect.x - start_x) * move_t);
        int target_y = start_y + (int)((dscrect.y - start_y) * move_t);

        float scale_t = 2.0f - move_t;

        int scaled_w = (int)(DSC * scale_t);
        int scaled_h = (int)(DSC * scale_t);

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

            SDL_RenderCopy(renderer, splash_frames,
                      &frames[frames_indices[current_frame]], &animRect);

            if (alpha < 0xFF)
                SDL_SetTextureAlphaMod(splash_frames, alpha);

            last_alpha = alpha;

            SDL_RenderCopy(renderer, splash_frames, &frames[0], &embrect);
        }
        else
        {
            if (last_alpha != alpha)
                SDL_SetTextureAlphaMod(splash_frames, alpha);

            SDL_RenderCopy(renderer, splash_frames, &frames[0], &embrect);

            if (alpha < 0xFF)
                SDL_SetTextureAlphaMod(splash_frames, 0xFF);

            last_alpha = 0xFF;

            SDL_RenderCopy(renderer, splash_frames,
                       &frames[frames_indices[current_frame]], &animRect);
        }

        if (alpha)
        {
            if (alpha < 0xFF)
                SDL_SetTextureAlphaMod(logo_frame, alpha);

            SDL_RenderCopy(renderer, logo_frame, NULL, &ld_rect);
        }

        if (elspd > 0xBB8)
            FC_DrawEffect(font, renderer, verposx, verposy, effect, v);

        SDL_RenderPresent(renderer);
    }

    FC_FreeFont(font);
    SDL_DestroyTexture(logo_frame);
    SDL_DestroyTexture(splash_frames);
    splash_frames = NULL;
    logo_frame = NULL;
}
