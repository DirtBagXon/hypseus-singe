/*
 * ____ DAPHNE COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2001 Mark Broadhead
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

// gpworld.h

#ifndef GPWORLD_H
#define GPWORLD_H

#include "game.h"

#define GPWORLD_OVERLAY_W 360 // width of overlay
#define GPWORLD_OVERLAY_H 256 // height of overlay

#define GPWORLD_VID_ADDRESS 0xd000
#define GPWORLD_SPRITE_ADDRESS 0xc000

#define GPWORLD_LSS 0x10
#define GPWORLD_RSS 0x01
#define GPWORLD_RST 0xA3

// we really need 512, but 256 is the max we can use with an 8 bit palette
#define GPWORLD_COLOR_COUNT 256

// START modified Mame code
#define SPR_Y_TOP 0
#define SPR_Y_BOTTOM 1
#define SPR_X_LO 2
#define SPR_X_HI 3
#define SPR_SKIP_LO 4
#define SPR_SKIP_HI 5
#define SPR_GFXOFS_LO 6
#define SPR_GFXOFS_HI 7
// END modified Mame code

class gpworld : public game
{
  public:

    typedef enum {
        S_GP_ENG1, S_GP_ENG2, S_GP_CRASH, S_GP_COUNT, S_GP_START,
        S_GP_COIN, S_GP_DINK, S_GP_TIRE, S_GP_GEAR, S_GP_REV
    } GPWorldSound;

    gpworld();
    void do_irq(unsigned int);                    // does an IRQ tick
    void do_nmi();                                // does an NMI tick
    Uint8 cpu_mem_read(Uint16 addr);              // memory read routine
    void cpu_mem_write(Uint16 addr, Uint8 value); // memory write routine
    Uint8 port_read(Uint16 port);                 // read from port
    void port_write(Uint16 port, Uint8 value);    // write to a port
    virtual void input_enable(Uint8, Sint8);
    virtual void input_disable(Uint8, Sint8);
    bool set_bank(Uint8, Uint8);
    void palette_calculate();
    void repaint(); // function to repaint video
    static constexpr int max_sprites = 0x30000;
    virtual void write_ldp(Uint8, Uint16);
    virtual Uint8 read_ldp(Uint16);
    void set_preset(int);

  protected:
    void align();
    void recalc_palette();
    void draw_sprite(int);
    void draw_char(char, int, int);
    void draw_shift(const char*);
    Uint8 rombank[0x8000];
    Uint8 character[0x1000];
    Uint8 sprite[max_sprites];
    Uint8 miscprom[0x220];
    SDL_Color palette_lookup[4096]; // all possible color entries
    int tile_color_pointer[256];
    Uint8 m_transparent_color; // which color is to be transparent
    bool palette_modified;     // has our palette been modified?
    Uint8 ldp_output_latch;    // holds data to be sent to the LDV1000
    Uint8 ldp_input_latch;     // holds data that was retrieved from the LDV1000
    Uint8 lss;                 // steering sensitivity modifier (left)
    Uint8 rss;                 // steering sensitivity modifier (right)
    Uint8 ign;
    bool nmie;
    bool m_align;
    bool m_shifter;
    Uint8 banks[7];

    const Uint8 gear[6][8] = {
      {0x00,0x18,0x18,0x18,0x7e,0x7e,0x3c,0x18},
      {0x24,0x24,0x24,0x7E,0x24,0x24,0x24,0x24},
      {0x00,0x00,0x38,0x10,0x10,0x10,0x10,0x38},
      {0x70,0x20,0x20,0x20,0x20,0x20,0x24,0x7c},
      {0x00,0x00,0x38,0x44,0x44,0x44,0x44,0x38},
      {0x18,0x3c,0x7e,0x7e,0x18,0x18,0x18,0x00}
    };
};

#endif
