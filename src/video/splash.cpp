/*
 * ____ HYPSEUS COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2023 DirtBagXon
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

#include "splash.h"
#include "video.h"
#include "../hypseus.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

void splash() {

    SDL_Event event;
    SDL_Rect logical;
    int current_frame = 0;
    int frames_indices[FRAMES];
    SDL_RWops* ops = SDL_RWFromConstMem(ghc, sizeof(ghc));
    SDL_Surface *splash_surface = IMG_Load_RW(ops, 1);

    if (!splash_surface) return;

    FC_Font *font = FC_CreateFont();
    FC_Scale scale = FC_MakeScale(1.0f, 1.0f);
    SDL_Color color = {0xE0, 0xE0, 0xE0, 0x64};
    SDL_Renderer *renderer = video::get_renderer();
    FC_LoadFont(font, renderer, "fonts/default.ttf", 0x1A, color, TTF_STYLE_NORMAL);

    bool fs = video::get_fullscreen() || video::get_fullwindow();

    logical.w = (fs) ? (int)video::get_logical_width() :
                       (int)video::get_viewport_width();
    logical.h = (fs) ? (int)video::get_logical_height() :
                       (int)video::get_viewport_height();

    scale = (!fs) ? FC_MakeScale(SCALE, SCALE) : scale;

    const char* v = get_hypseus_version();
    int f = (fs) ? 2 : 1;

    SDL_Rect dscrect = {
        .x = (int)(((logical.w - DSC) >> 1) + (EMBW >> 1) + 0xE),
        .y = (int)(((logical.h - EMBH) >> 1) + (f >> 2)),
        .w = DSC,
        .h = DSC
    };

    SDL_Rect embrect = {
        .x = (int)(((logical.w - EMBW) >> 1) - (f << 2)),
        .y = (int)((logical.h - EMBH) >> 1),
        .w = EMBW,
        .h = EMBH
    };

    SDL_Rect frames[] = {
        {.x = 0x011, .y = 0x00, .w = EMBW, .h = 0x54},
        {.x = 0x154, .y = 0x05, .w = 0x4A, .h = 0x50},
        {.x = 0x19F, .y = 0x05, .w = 0x4A, .h = 0x50},
        {.x = 0x1EA, .y = 0x05, .w = 0x4A, .h = 0x50},
        {.x = 0x235, .y = 0x05, .w = 0x4A, .h = 0x50},
        {.x = 0x280, .y = 0x05, .w = 0x4A, .h = 0x50},
        {.x = 0x2CB, .y = 0x05, .w = 0x4A, .h = 0x50}
    };

    for (int i = 0; i < FRAMES; i++) frames_indices[i] = (i % 6) + 1;

    SDL_Texture *splash_frames = SDL_CreateTextureFromSurface(renderer, splash_surface);
    SDL_FreeSurface(splash_surface);

    if (!splash_frames) return;

    FC_SetFilterMode(font, FC_FILTER_LINEAR);
    FC_Effect effect = FC_MakeEffect(FC_ALIGN_CENTER, scale, color);

    for (int i = 0; i < FRAMES; i++) {

        SDL_RenderClear(renderer);

        while (SDL_PollEvent(&event))
            if (event.type == SDL_KEYDOWN)
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    goto exit;
                }

        SDL_RenderCopy(renderer, splash_frames, &frames[0], &embrect);
        SDL_RenderCopy(renderer, splash_frames,
                       &frames[frames_indices[current_frame]], &dscrect);
        if (i > 0xA)
            FC_DrawEffect(font, renderer, (logical.w >> 1),
                            (logical.h * 90) / 100, effect, v);

        SDL_RenderPresent(renderer);
        current_frame = (current_frame < (FRAMES - 1)) ? current_frame + 1 : 0;
        SDL_Delay(110);
    }

exit:
    FC_FreeFont(font);
    SDL_DestroyTexture(splash_frames);
}
