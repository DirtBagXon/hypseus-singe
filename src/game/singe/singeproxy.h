/*
 * singeproxy.h
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

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "../../vldp2/vldp/vldp.h"  // to get the vldp structs

#include "SDL_image.h"
#include "SDL_ttf.h"

// since lua is written in C, we need to specify that all functions are C-styled
extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

////////////////////////////////////////////////////////////////////////////////

unsigned char sep_byte_clip(int value);
void          sep_call_lua(const char *func, const char *sig, ...);
void          sep_capture_vldp();
void          sep_die(const char *fmt, ...);
void          sep_do_blit(SDL_Surface *srfDest);
void          sep_do_mouse_move(Uint16 x, Uint16 y, Sint16 xrel, Sint16 yrel);
void          sep_error(const char *fmt, ...);
int           sep_lua_error(lua_State *L);
int           sep_prepare_frame_callback(struct yuv_buf *src);
void          sep_print(const char *fmt, ...);
void          sep_release_vldp();
void          sep_set_static_pointers(double *m_disc_fps, unsigned int *m_uDiscFPKS);
void          sep_set_surface(int width, int height);
void          sep_shutdown(void);
void          sep_sound_ended(Uint8 *buffer, unsigned int slot);
bool          sep_srf32_to_srf8(SDL_Surface *src, SDL_Surface *dst);
void          sep_startup(const char *script);
void          sep_unload_fonts(void);
void          sep_unload_sounds(void);
void          sep_unload_sprites(void);

////////////////////////////////////////////////////////////////////////////////

// Singe API Calls

static int sep_audio_control(lua_State *L);
static int sep_change_speed(lua_State *L);
static int sep_color_set_backcolor(lua_State *L);
static int sep_color_set_forecolor(lua_State *L);
static int sep_daphne_get_height(lua_State *L);
static int sep_daphne_get_width(lua_State *L);
static int sep_debug_say(lua_State *L);
static int sep_font_load(lua_State *L);
static int sep_font_quality(lua_State *L);
static int sep_font_select(lua_State *L);
static int sep_font_sprite(lua_State *L);
static int sep_get_current_frame(lua_State *L);
static int sep_get_overlay_height(lua_State *L);
static int sep_get_overlay_width(lua_State *L);
static int sep_mpeg_get_height(lua_State *L);
static int sep_mpeg_get_pixel(lua_State *L);
static int sep_mpeg_get_width(lua_State *L);
static int sep_overlay_clear(lua_State *L);
static int sep_pause(lua_State *L);
static int sep_play(lua_State *L);
static int sep_say(lua_State *L);
static int sep_say_font(lua_State *L);
static int sep_screenshot(lua_State *L);
static int sep_search(lua_State *L);
static int sep_search_blanking(lua_State *L);
static int sep_set_disc_fps(lua_State *L);
static int sep_skip_backward(lua_State *L);
static int sep_skip_blanking(lua_State *L);
static int sep_sound_load(lua_State *L);
static int sep_sound_play(lua_State *L);
static int sep_sprite_draw(lua_State *L);
static int sep_sprite_height(lua_State *L);
static int sep_sprite_load(lua_State *L);
static int sep_sprite_width(lua_State *L);
static int sep_skip_forward(lua_State *L);
static int sep_skip_to_frame(lua_State *L);
static int sep_step_backward(lua_State *L);
static int sep_step_forward(lua_State *L);
static int sep_stop(lua_State *L);
