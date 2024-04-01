/*
 * ____ HYPSEUS COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2024 DirtBagXon
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
 *
 *  Keypad:
 *  ┌─────┬─────┬─────┬─────┐
 *  │     │  P  │  B  │  S- │
 *  ├─────┼─────┼─────┼─────┤
 *  │  @- │  U  │  @+ │     │
 *  ├─────┼─────┼─────┤  S+ │
 *  │ L   │ Rst │  R  │     │
 *  ├─────┼─────┼─────┼─────┤
 *  │     │  D  │     │     │
 *  ├─────┼─────┼─────┤ Log │
 *  │    Swt    │ Inc │     │
 *  └───────────┴─────┴─────┘
 *
 *  B = Bezel Display Toggle
 *  P = Bezel Priority Toggle
 *  @ = Rotation + or -
 *  S = Scale + or -
 *  Swt = Layer Switching (0, 1, 2)
 *  Inc = Step Increment (1, 10)
 *  Log = Write values to log
 *
 */

#include "../video/video.h"
#include <algorithm>
#include <math.h>

void input_toolbox(SDL_Keycode key, SDL_Keycode recent, bool thayers)
{
    char txt[14];
    SDL_Rect rect;
    static int8_t mplex = 10;
    static int8_t layer = 0;
    int h, v, s;
    float r = video::get_fRotateDegrees();

    switch(layer) {
    case 1:
        rect = video::get_sb_rect();
        s = video::get_score_bezel_scale();
        h = rect.x;
        v = rect.y;
        break;
    case 2:
        rect = video::get_aux_rect();
        s = video::get_aux_bezel_scale();
        h = rect.x;
        v = rect.y;
        break;
    default:
        s = video::get_scalefactor();
        h = video::get_scale_h_shift();
        v = video::get_scale_v_shift();
        break;
    }

    switch(key) {
    case SDLK_RETURN:
        if (recent != key && !thayers) video::vid_toggle_fullscreen();
        break;
    case SDLK_BACKSPACE:
        if (recent != key) video::vid_toggle_scanlines();
        break;
    case SDLK_KP_MINUS:
        switch(layer) {
        case 1:
            video::reset_scalefactor(std::max(1, (int)fmod(s - 1.0, 30.0)), layer);
            break;
        case 2:
            if (thayers) video::scalekeyboard(std::max(1, (int)fmod(s - 1.0, 30.0)));
            else video::reset_scalefactor(std::max(1, (int)fmod(s - 1.0, 30.0)), layer);
            break;
        default:
            video::reset_scalefactor(std::max(25, (int)fmod(s - 1.0, 110.0)), layer);
            break;
        }
        break;
    case SDLK_KP_PLUS:
        switch(layer) {
        case 1:
            video::reset_scalefactor(std::min(25, (int)fmod(s + 1.0, 30.0)), layer);
            break;
        case 2:
            if (thayers) video::scalekeyboard(std::min(25, (int)fmod(s + 1.0, 30.0)));
            else video::reset_scalefactor(std::min(25, (int)fmod(s + 1.0, 30.0)), layer);
            break;
        default:
            video::reset_scalefactor(std::min(100, (int)fmod(s + 1.0, 110.0)), layer);
            break;
        }
        break;
    case SDLK_KP_DIVIDE:
        if (recent != key) video::set_bezel_reverse(true);
        break;
    case SDLK_KP_MULTIPLY:
        if (recent != key) video::vid_toggle_bezel();
        break;
    case SDLK_KP_PERIOD:
        mplex = (mplex == 10) ? 1 : 10;
        snprintf(txt, sizeof(txt), "stepping: %d", mplex == 10);
        video::draw_subtitle(txt, true, true);
        break;
    case SDLK_KP_ENTER:
        if (recent != key) video::notify_positions();
        break;
    case SDLK_KP_0:
        layer = (layer + 1) % 3;
        snprintf(txt, sizeof(txt), "layer: %d", layer);
        video::draw_subtitle(txt, true, true);
        break;
    case SDLK_KP_2:
        switch(layer) {
        case 0:
            video::reset_shiftvalue(std::min(200, (int)fmod(v + 1.0, 210.0)), true, layer);
            break;
        default:
            video::reset_shiftvalue(std::min((int)video::get_logical_height(), v + mplex), true, layer);
            break;
        }
        break;
    case SDLK_KP_4:
        switch(layer) {
        case 0:
            video::reset_shiftvalue(std::max(0, (int)fmod(h - 1.0, 200.0)), false, layer);
            break;
        default:
            video::reset_shiftvalue(std::max(-600, h - mplex), false, layer);
            break;
        }
        break;
    case SDLK_KP_5:
        if (recent != key && layer == 0) {
            video::reset_shiftvalue(0x64, true, 0);
            video::reset_shiftvalue(0x64, false, 0);
        }
        break;
    case SDLK_KP_6:
        switch(layer) {
        case 0:
            video::reset_shiftvalue(std::min(200, (int)fmod(h + 1.0, 210.0)), false, layer);
            break;
        default:
            video::reset_shiftvalue(std::min((int)video::get_logical_width(), h + mplex), false, layer);
            break;
        }
        break;
    case SDLK_KP_7:
        video::set_fRotateDegrees(fmod(r - 1.0, 360.0));
        break;
    case SDLK_KP_8:
        switch(layer) {
        case 0:
            video::reset_shiftvalue(std::max(0, (int)fmod(v - 1.0, 200.0)), true, layer);
            break;
        default:
            video::reset_shiftvalue(std::max(-600, v - mplex), true, layer);
            break;
        }
        break;
    case SDLK_KP_9:
        video::set_fRotateDegrees(fmod(r + 1.0, 360.0));
        break;
    default:
        break;
    }
}
