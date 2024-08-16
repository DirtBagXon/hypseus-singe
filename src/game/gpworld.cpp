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

// gpworld.cpp
// by Mark Broadhead
//
// Sega GP World Hardware
//
// GP World is a rare game that came in a huge cabinet with two monitors, side
// by side.
// The image from the laserdisc was stretched to an 8x3 aspect and graphics were
// overlayed
// on top. The hardware is similar to Astron Belt but somewhat more powerful.
//
// TODO:
//
// game wont be playable until vldp can do multiple speeds
// samples are needed for game generated sounds
// game seems to lockup with writes to dac0 and dae0?

#include "config.h"

#include <string.h>
#include "gpworld.h"
#include <plog/Log.h>
#include "../io/conout.h"
#include "../ldp-in/ldv1000.h"
#include "../ldp-out/ldp.h"
#include "../video/palette.h"
#include "../sound/sound.h"
#include "../sound/samples.h"
#include "../cpu/cpu.h"
#include "../cpu/generic_z80.h"

gpworld::gpworld()
{
    struct cpu::def cpu;

    m_shortgamename = "gpworld";
    memset(&cpu, 0, sizeof(struct cpu::def));
    memset(banks, 0xff, 7); // fill banks with 0xFF's
    banks[5] = 0x00;
    banks[6] = 0x00;
    memset(sprite, 0x00, 0x30000);   // making sure sprite[] is zero'd out
    memset(m_cpumem, 0x00, 0x10000); // making sure m_cpumem[] is zero'd out
    palette_modified = true;
    m_disc_fps       = 29.97;
    //	m_game_type = GAME_GPWORLD;

    m_video_overlay_width  = GPWORLD_OVERLAY_W;
    m_video_overlay_height = GPWORLD_OVERLAY_H;
    m_palette_color_count  = GPWORLD_COLOR_COUNT;

    cpu.type              = cpu::type::Z80;
    cpu.hz                = 5000000; // guess based on Astron's clock speed
    cpu.irq_period[0]     = 16.6666; // interrupt from vblank (60hz)
    cpu.nmi_period        = 16.6666; // nmi from LD-V1000 command strobe
    cpu.initial_pc        = 0;
    cpu.must_copy_context = false;
    cpu.mem = m_cpumem;
    cpu::add(&cpu); // add a z80

    m_transparent_color = 0;
    ldp_output_latch    = 0xff;
    nmie                = false;
    m_align             = false;
    m_shifter           = true;

    m_num_sounds = 10;
    // With the lack of any hardware reference....
    m_sound_name[S_GP_ENG1]    = "gp_engine1.wav";
    m_sound_name[S_GP_ENG2]    = "gp_engine2.wav";
    m_sound_name[S_GP_COUNT]   = "gp_count.wav";
    m_sound_name[S_GP_START]   = "gp_signal.wav";
    m_sound_name[S_GP_TIRE]    = "gp_tires.wav";
    m_sound_name[S_GP_REV]     = "gp_roar.wav";
    m_sound_name[S_GP_CRASH]   = "gp_crash.wav";
    m_sound_name[S_GP_COIN]    = "dl_credit.wav";
    m_sound_name[S_GP_DINK]    = "dl2_bad.wav";
    m_sound_name[S_GP_GEAR]    = "gr_alarm4.wav";

    const static struct rom_def gpworld_roms[] =
        {// main z80 rom
         {"gpw_6162a.bin", NULL, &m_cpumem[0x0000], 0x4000, 0},
         {"gpw_6163.bin", NULL, &m_cpumem[0x4000], 0x4000, 0},
         {"gpw_6164.bin", NULL, &m_cpumem[0x8000], 0x4000, 0},

         // character graphics
         {"gpw_6148.bin", NULL, &character[0x0000], 0x1000, 0},

         // sprites
         {"gpw_6149.bin", NULL, &sprite[0x0000], 0x4000, 0},
         {"gpw_6150.bin", NULL, &sprite[0x8000], 0x4000, 0},
         {"gpw_6151.bin", NULL, &sprite[0x4000], 0x4000, 0},
         {"gpw_6152.bin", NULL, &sprite[0xc000], 0x4000, 0},
         {"gpw_6153.bin", NULL, &sprite[0x10000], 0x4000, 0},
         {"gpw_6154.bin", NULL, &sprite[0x18000], 0x4000, 0},
         {"gpw_6155.bin", NULL, &sprite[0x14000], 0x4000, 0},
         {"gpw_6156.bin", NULL, &sprite[0x1c000], 0x4000, 0},
         {"gpw_6157.bin", NULL, &sprite[0x20000], 0x4000, 0},
         {"gpw_6158.bin", NULL, &sprite[0x28000], 0x4000, 0},

         // misc proms (unused)
         {"gpw_6146.bin", NULL, &miscprom[0x000], 0x20, 0},
         {"gpw_6147.bin", NULL, &miscprom[0x020], 0x100, 0},
         {"gpw_5501.bin", NULL, &miscprom[0x120], 0x100, 0},
         {NULL}};

    m_rom_list = gpworld_roms;
}

// does anything special needed to send an IRQ
void gpworld::do_irq(unsigned int which_irq)
{
    if (which_irq == 0) {
        // Redraws the screen (if needed) on interrupt
        recalc_palette();
        blit();
        Z80_ASSERT_IRQ;
    }
}

// does anything special needed to send an NMI
void gpworld::do_nmi()
{
    // only do an nmi if nmie is enabled
    if (nmie) {
        ldv1000::write(ldp_output_latch);
        ldp_input_latch = ldv1000::read();
        Z80_ASSERT_NMI;
    }
}

// reads a byte from the cpu's memory
Uint8 gpworld::cpu_mem_read(Uint16 addr)
{
    Uint8 result = m_cpumem[addr];

    // main rom
    if (addr <= 0xbfff) {
    }

    // sprites
    else if (addr >= 0xc000 && addr <= 0xc7ff) {
    }

    // unknown
    else if (addr >= 0xc800 && addr <= 0xc9ff) {
    }

    // color
    else if (addr >= 0xca00 && addr <= 0xcfff) {
    }

    // tile ram
    else if (addr >= 0xd000 && addr <= 0xd7ff) {
    }

    // ld-v1000 laserdisc player
    else if (addr == 0xd800) {
        result = read_ldp(addr);
        LOGD << fmt("LDP read %x", result);
    }

    // unknown
    else if (addr == 0xd801) {
    }

    // steering
    else if (addr == 0xda00) {
        result = banks[1];
    }
    // unknown
    else if (addr == 0xda01) {
    } else if (addr == 0xda02) {
    } else if (addr == 0xda03) {
    }

    // brake and accelerator
    else if (addr == 0xda20) {
        if (m_cpumem[0xda02] & 0x01) {
            result = banks[5];
        } else {
            result = banks[6];
        }
    }
    // unknown
    else if (addr == 0xda40) {
    } else if (addr == 0xda80) {
    } else if (addr == 0xdaa0) {
    }

    // work ram (all?)
    else if (addr >= 0xe000) {
    }

    else {
        LOGW << fmt("Unmapped read from %x (PC is %x)", addr, Z80_GET_PC);
    }

    return result;
}

// writes a byte to the cpu's memory
void gpworld::cpu_mem_write(Uint16 addr, Uint8 value)
{
    m_cpumem[addr] = value;

    // main rom
    if (addr <= 0xbfff) {
        LOGW << fmt("Attempted write to main ROM! at %x with value %x", addr, value);
    }

    // sprite
    else if (addr >= 0xc000 && addr <= 0xc7ff) {
        m_video_overlay_needs_update = true;
    }

    // unknown
    else if (addr >= 0xc800 && addr <= 0xc9ff) {
    }

    // color (c800-c9ff is tile color and ca00-cbff is sprite color)
    else if (addr >= 0xc800 && addr <= 0xcfff) {
        palette_modified = true;
    }

    // tile ram
    else if (addr >= 0xd000 && addr <= 0xd7ff) {
        m_video_overlay_needs_update = true;
    }

    // disc
    else if (addr == 0xd800) {
        LOGD << fmt("LDP write %x", value);
        write_ldp(value, addr);
    }

    // unknown
    else if (addr == 0xda00) {
    }

    // sound (uses analog hardware) - unsupported - sound with samples
    else if (addr == 0xda01) {

        if (value != 0xff) {

            static Uint8 lastbeep[0xff] = {0};

            // audio streams (primitive control)
            if (++lastbeep[value] > 6) {
                switch (value) {
                 case 0xdc:
                 case 0xde:
                    sound::play(S_GP_TIRE);
                    break;
                 case 0xec:
                 case 0xee:
                    samples::flush_queue();
                    sound::play(S_GP_START);
                    break;
                 case 0xf4:
                    samples::flush_queue();
                    sound::play(S_GP_COUNT);
                    break;
                 case 0xf5:
                    sound::play(S_GP_REV);
                    break;
                   break;
                 case 0xf9:
                 case 0xfB:
                    sound::play(S_GP_COIN);
                    break;
                 case 0xfe:
                    if (++ign > 0x0a) {
                        sound::play(banks[2] ? S_GP_ENG2 : S_GP_ENG1);
                        ign--;
                    }
                   break;
                 default:
                    LOGD << fmt("%x write %x", addr, value);
                    break;
                 }
                 lastbeep[value] = 0;
            }
	}
    }
    else if (addr == 0xdac0 || addr == 0xdae0) {

        switch (value) {
        case 0x90:
           ign = 0x00;
           samples::flush_queue();
           sound::play(S_GP_CRASH);
           break;
        default:
           sound::play(S_GP_DINK);
           LOGD << fmt("%x write %x", addr, value);
           break;
        }
    }
    // bit 0 selects whether brake or accelerater are read through 0xda20
    else if (addr == 0xda02) {
    }

    // unknown
    else if (addr == 0xda03) {
    }

    // unknown
    else if (addr == 0xda20) {
    } else if (addr == 0xda40) {
    } else if (addr == 0xda80) {
    } else if (addr == 0xdaa0) {
    }

    // work ram (all of it?)
    else if (addr >= 0xe000) {
    }

    else {
        m_cpumem[addr] = value;
        LOGW << fmt("Unmapped write to %x with value %x (PC is %x)", addr, value, Z80_GET_PC);
    }
}

Uint8 gpworld::read_ldp(Uint16 addr)
{
    Uint8 result = ldp_input_latch;
    LOGD << fmt("Read from player %x at pc: %x", result, Z80_GET_PC);

    return result;
}

void gpworld::write_ldp(Uint8 value, Uint16 addr)
{
    LOGD << fmt("Write to player %x at pc %x", value, Z80_GET_PC);
    ldp_output_latch = value;
}

// reads a byte from the cpu's port
Uint8 gpworld::port_read(Uint16 port)
{
    port &= 0xff;

    switch (port) {
    // shifter (anything else?)
    case 0x80:
        return banks[2];
        break;
    // Input switches... start, test, coins, etc
    case 0x81:
        return banks[0];
        break;
    // Dipswitch 1
    case 0x82:
        return banks[3];
        break;
    // Dipswitch 2
    case 0x83:
        return banks[4];
        break;
    default:
        LOGW << fmt("ERROR: CPU port %x read requested, but this function is "
                   "unimplemented!", port);
    }

    return (0);
}

// writes a byte to the cpu's port
void gpworld::port_write(Uint16 port, Uint8 value)
{
    port &= 0xff;

    switch (port) {

    // bit 2 - start lamp
    // bit 6 - nmie
    case 0x01:
        if (value & 0x40)
            nmie = true;
        else {
            nmie = false;
            if (value == 0x00) {
                ldv1000::write(GPWORLD_RST);
                lss = GPWORLD_LSS;
                rss = GPWORLD_RSS;
                ign = value;
                align();
            }
        }
        break;

    default:
        LOGW << fmt("ERROR: CPU port %x write requested (value %x) but this "
                   "function is unimplemented!", port, value);
        break;
    }
}

void gpworld::palette_calculate()
{
    // transparent color default to 0, so no change to transparency is needed

    //	color conversion for all 2^12 possible colors
    for (int i = 0; i < 4096; i++) {
        Uint8 bit0, bit1, bit2, bit3;

        // red component
        bit0                = static_cast<Uint8>((i >> 3) & 0x01);
        bit1                = static_cast<Uint8>((i >> 2) & 0x01);
        bit2                = static_cast<Uint8>((i >> 1) & 0x01);
        bit3                = static_cast<Uint8>((i >> 0) & 0x01);
        palette_lookup[i].r = static_cast<Uint8>((0x8f * bit0) + (0x43 * bit1) +
                                                 (0x1f * bit2) + (0x0e * bit3));
        // green component
        bit0                = static_cast<Uint8>((i >> 7) & 0x01);
        bit1                = static_cast<Uint8>((i >> 6) & 0x01);
        bit2                = static_cast<Uint8>((i >> 5) & 0x01);
        bit3                = static_cast<Uint8>((i >> 4) & 0x01);
        palette_lookup[i].g = static_cast<Uint8>((0x8f * bit0) + (0x43 * bit1) +
                                                 (0x1f * bit2) + (0x0e * bit3));
        // blue component
        bit0                = static_cast<Uint8>((i >> 11) & 0x01);
        bit1                = static_cast<Uint8>((i >> 10) & 0x01);
        bit2                = static_cast<Uint8>((i >> 9) & 0x01);
        bit3                = static_cast<Uint8>((i >> 8) & 0x01);
        palette_lookup[i].b = static_cast<Uint8>((0x8f * bit0) + (0x43 * bit1) +
                                                 (0x1f * bit2) + (0x0e * bit3));
    }

    recalc_palette();
}

void gpworld::recalc_palette()
{
    if (palette_modified) {
        m_video_overlay_needs_update = true;
        Uint8 used_tile_colors[4096] = {0};
        // int used_colors = 0;
        int i;

        // load current palette with values from the lookup according to sprite
        // color ram
        for (i = 0; i < 256; i++) {
            int j;
            j         = 0xca00 + (i * 0x02);
            int color = m_cpumem[j] | ((m_cpumem[j + 1] & 0x0f) << 8);

            palette::set_color(i, palette_lookup[color]);

            // the final palette is blank in gpworld so we'll put our tile
            // palette here
            // we need to check if this palette is ever written to
            if (i >= 240 && color) {
                printline("Color written to palette 15! This will screw up the "
                          "tile palette!");
            }
        }

        // start of the blank palette 15
        int k = 240;

        // load current palette with values from the lookup according to color
        // ram
        for (i = 0; i < 256; i++) {
            int j;
            j         = 0xc800 + (i * 0x02);
            int color = m_cpumem[j] | ((m_cpumem[j + 1] & 0x0f) << 8);

            if (color) {
                // we already have this color mapped
                if (used_tile_colors[color]) {
                    // point to the already mapped color
                    tile_color_pointer[i] = used_tile_colors[color];
                }
                // a new color
                else {
                    palette::set_color(k, palette_lookup[color]);

                    used_tile_colors[color] = k;
                    tile_color_pointer[i]   = k;
                    k++;
                    if (k > 255) {
                        printline("Too many tile colors! FIX ME!");
                    }
                }
            }
            // if the color is black
            else {
                tile_color_pointer[i] = 0;
            }
        }

        palette::finalize();
    }
    palette_modified = false;
}

void gpworld::draw_char(char l, int x_offset, int y_offset)
{
    int i = -1;
    Uint8 c = 0x01;

    switch (l) {
        case 0x44: i = 0; c = 0xf3; break;
        case 0x48: i = 1; break;
        case 0x49: i = 2; break;
        case 0x4c: i = 3; break;
        case 0x4f: i = 4; break;
        case 0x55: i = 5; c = 0xeb; break;
        default: return;
    }

    const Uint8* char_bitmap = gear[i];

    for (int y = 0; y < 8; y++) {
        Uint8 pixel_row = char_bitmap[y];
        for (int x = 0; x < 8; x++) {
            Uint8 pixel = (pixel_row & (0x80 >> x)) >> (7 - x);

            if (pixel) {
                int pixel_index = (y_offset * 8 + y) * GPWORLD_OVERLAY_W + (x_offset * 8 + x);
                *((Uint8 *)m_video_overlay[m_active_video_overlay]->pixels + pixel_index) = c;
            }
        }
    }
}

void gpworld::draw_shift(const char* w)
{
    const int x_offset = 0x01, y_offset = 0x1c;

    for (int i = 0; w[i] != '\0'; i++) {
        draw_char(w[i], x_offset + i, y_offset);
    }
}

// updates gpworld's video
void gpworld::repaint()
{
    // This should be much faster
    SDL_FillRect(m_video_overlay[m_active_video_overlay], NULL, m_transparent_color);

    // The sprites are bottom priority so we draw them first
    // START modified Mame code
    // check if sprites need to be drawn
    int spr_number, sprite_bottom_y, sprite_top_y;
    Uint8 *sprite_base;

    for (spr_number = 0; spr_number < 64; spr_number++) {
        sprite_base     = m_cpumem + GPWORLD_SPRITE_ADDRESS + 0x8 * spr_number;
        sprite_top_y    = sprite_base[SPR_Y_TOP];
        sprite_bottom_y = sprite_base[SPR_Y_BOTTOM];
        if (sprite_bottom_y && (sprite_bottom_y - sprite_top_y > 0)) {
            draw_sprite(spr_number);
        }
    }
    // END modified Mame code

    Uint8 pixel[8];
    Uint8 p = 5;

    // loop through video memory and draw characters
    for (int charx = 19; charx < 64; charx++) {
        for (int chary = 0; chary < 32; chary++) {
            for (int y = 0; y < 8; y++) {
                int current_character = chary * 64 + charx + GPWORLD_VID_ADDRESS;

                pixel[0] = static_cast<Uint8>(
                    ((character[m_cpumem[current_character] * 8 + y] & 0x80) >> 7) |
                    ((character[m_cpumem[current_character] * 8 + y + 0x800] & 0x80) >> 6));
                pixel[1] = static_cast<Uint8>(
                    ((character[m_cpumem[current_character] * 8 + y] & 0x40) >> 6) |
                    ((character[m_cpumem[current_character] * 8 + y + 0x800] & 0x40) >> 5));
                pixel[2] = static_cast<Uint8>(
                    ((character[m_cpumem[current_character] * 8 + y] & 0x20) >> 5) |
                    ((character[m_cpumem[current_character] * 8 + y + 0x800] & 0x20) >> 4));
                pixel[3] = static_cast<Uint8>(
                    ((character[m_cpumem[current_character] * 8 + y] & 0x10) >> 4) |
                    ((character[m_cpumem[current_character] * 8 + y + 0x800] & 0x10) >> 3));
                pixel[4] = static_cast<Uint8>(
                    ((character[m_cpumem[current_character] * 8 + y] & 0x08) >> 3) |
                    ((character[m_cpumem[current_character] * 8 + y + 0x800] & 0x08) >> 2));
                pixel[5] = static_cast<Uint8>(
                    ((character[m_cpumem[current_character] * 8 + y] & 0x04) >> 2) |
                    ((character[m_cpumem[current_character] * 8 + y + 0x800] & 0x04) >> 1));
                pixel[6] = static_cast<Uint8>(
                    ((character[m_cpumem[current_character] * 8 + y] & 0x02) >> 1) |
                    ((character[m_cpumem[current_character] * 8 + y + 0x800] & 0x02) >> 0));
                pixel[7] = static_cast<Uint8>(
                    ((character[m_cpumem[current_character] * 8 + y] & 0x01) >> 0) |
                    ((character[m_cpumem[current_character] * 8 + y + 0x800] & 0x01) << 1));

                for (int x = 0; x < 8; x++) {
                    if (pixel[x]) {
                        *((Uint8 *)m_video_overlay[m_active_video_overlay]->pixels +
                          ((chary * 8 + y + p) * GPWORLD_OVERLAY_W) + ((charx - 19) * 7 + x + p)) =
                            tile_color_pointer[(pixel[x]) | ((m_cpumem[current_character]) & 0xfc)];
                    }
                }
            }
        }
    }

    // draw state of the shifter
    if (m_shifter) draw_shift(banks[2] ? "DLO" : "UHI");
}

// this gets called when the user presses a key or moves the joystick
void gpworld::input_enable(Uint8 move, Sint8 mouseID)
{
    switch (move) {
    case SWITCH_UP:
        break;
    case SWITCH_LEFT:
        banks[1] &= ~lss;
        if (lss < 0x80) lss = lss << 1;
        break;
    case SWITCH_RIGHT:
        banks[1] &= ~rss;
        if (rss < 0x08) rss = rss << 1;
        break;
    case SWITCH_DOWN:
        break;
    case SWITCH_START1: // '1' on keyboard
        banks[0] &= ~0x10;
        break;
    case SWITCH_BUTTON1: // space on keyboard
        banks[2] = ~banks[2];
        m_video_overlay_needs_update = true;
        break;
    case SWITCH_BUTTON2: // left shift
        banks[5] = 0xff;
        break;
    case SWITCH_BUTTON3: // left alt
        banks[6] = 0xff;
        break;
    case SWITCH_COIN1:
        banks[0] &= ~0x01;
        break;
    case SWITCH_COIN2:
        banks[0] &= ~0x02;
        break;
    case SWITCH_SERVICE:
        banks[0] &= ~0x08;
        break;
    case SWITCH_TEST:
        banks[0] &= ~0x04;
        break;
    }
}

// this gets called when the user releases a key or moves the joystick back to
// center position
void gpworld::input_disable(Uint8 move, Sint8 mouseID)
{
    switch (move) {
    case SWITCH_UP:
        break;
    case SWITCH_LEFT:
    case SWITCH_RIGHT:
        banks[1] = 0xff;
        lss = GPWORLD_LSS;
        rss = GPWORLD_RSS;
        if (m_align) align();
        break;
    case SWITCH_DOWN:
        break;
    case SWITCH_START1: // '1' on keyboard
        banks[0] |= 0x10;
        break;
    case SWITCH_BUTTON1: // space on keyboard
        sound::play(S_GP_GEAR);
        break;
    case SWITCH_BUTTON2: // left shift
        banks[5] = 0x00;
        break;
    case SWITCH_BUTTON3: // left alt
        banks[6] = 0x00;
        break;
    case SWITCH_COIN1:
        banks[0] |= 0x01;
        break;
    case SWITCH_COIN2:
        banks[0] |= 0x02;
        break;
    case SWITCH_SERVICE:
        banks[0] |= 0x08;
        break;
    case SWITCH_TEST:
        banks[0] |= 0x04;
        break;
    }
}

// used to set dip switch values
bool gpworld::set_bank(Uint8 which_bank, Uint8 value)
{
    bool result = true;

    switch (which_bank) {
    case 0:                               // bank A
        banks[3] = (Uint8)(value ^ 0xff); // dip switches are active low
        break;
    case 1:                               // bank B
        banks[4] = (Uint8)(value ^ 0xff); // switches are active low
        break;
    default:
        printline("ERROR: Bank specified is out of range!");
        result = false;
        break;
    }

    return result;
}

// START modified Mame code
void gpworld::draw_sprite(int spr_number)
{
    int p = 12;
    int sx, sy, row, height, src, sprite_color, sprite_bank;
    Uint8 *sprite_base;
    int skip; /* bytes to skip before drawing each row (can be negative) */

    sprite_base = m_cpumem + GPWORLD_SPRITE_ADDRESS + 0x8 * spr_number;

    src = sprite_base[SPR_GFXOFS_LO] + (sprite_base[SPR_GFXOFS_HI] << 8);

    skip = sprite_base[SPR_SKIP_LO] + (sprite_base[SPR_SKIP_HI] << 8);

    height = sprite_base[SPR_Y_BOTTOM] - sprite_base[SPR_Y_TOP];

    sy = sprite_base[SPR_Y_TOP] + 1;

    sx = sprite_base[SPR_X_LO] + ((sprite_base[SPR_X_HI] & 0x01) << 8) - 19 * 8;

    sprite_color = (sprite_base[SPR_X_HI] >> 4) & 0x0f;

    sprite_bank = (sprite_base[SPR_X_HI] >> 1) & 0x07;

    if (spr_number == 0x1) {

        sx -= 0x2;
        p  -= 0x4;
        if (sx < 0x4) sx -= 0x4;
        if (sy < 0x4) p  -= 0x7;
    }

    LOGD << fmt( "Draw Sprite #%x with src %x, skip %x, height %x, y %x, x %x",
		    spr_number, src, skip, height, sy, sx);

    for (row = p; row < height + p; row++) {
        int x, y;
        int src2;

        src = src2 = src + skip;

        x = sx - 12;
        y = sy + row;

        while (1) {
            int data_lo, data_high;

            data_lo   = sprite[(src2 & 0x7fff) | (sprite_bank << 16)];
            data_high = sprite[(src2 & 0x7fff) | 0x8000 | (sprite_bank << 16)];

            Uint8 pixel1 = data_high >> 0x04;
            Uint8 pixel2 = data_high & 0x0f;
            Uint8 pixel3 = data_lo >> 0x04;
            Uint8 pixel4 = data_lo & 0x0f;

            // draw these guys backwards
            if (src & 0x8000) {
                Uint8 temp_pixel;
                temp_pixel = pixel1;
                pixel1     = pixel4;
                pixel4     = temp_pixel;

                temp_pixel = pixel2;
                pixel2     = pixel3;
                pixel3     = temp_pixel;

                src2--;
            } else {
                src2++;
            }

            // TODO: also check to make sure they don't get drawn off the right
            // of the screen
            if (!(x & 0xf0000000)) {
                // draw the pixel - don't draw if its black as to not cover up
                // any tiles
                if (pixel1 && (pixel1 != 0xf)) {
                    *((Uint8 *)m_video_overlay[m_active_video_overlay]->pixels +
                      (y * GPWORLD_OVERLAY_W) + x + 0) = pixel1 + (0x10 * sprite_color);
                }
                if (pixel2 && (pixel2 != 0xf)) {
                    *((Uint8 *)m_video_overlay[m_active_video_overlay]->pixels +
                      (y * GPWORLD_OVERLAY_W) + x + 1) = pixel2 + (0x10 * sprite_color);
                }
                if (pixel3 && (pixel3 != 0xf)) {
                    *((Uint8 *)m_video_overlay[m_active_video_overlay]->pixels +
                      (y * GPWORLD_OVERLAY_W) + x + 2) = pixel3 + (0x10 * sprite_color);
                }
                if (pixel4 && (pixel4 != 0xf)) {
                    *((Uint8 *)m_video_overlay[m_active_video_overlay]->pixels +
                      (y * GPWORLD_OVERLAY_W) + x + 3) = pixel4 + (0x10 * sprite_color);
                }
            }
            x += 4;

            // stop drawing when the sprite data is 0xf
            if (((data_lo & 0x0f) == 0x0f) && (!(src & 0x8000))) {
                break;
            }

            else if ((src & 0x8000) && ((data_high & 0xf0) == 0xf0)) {
                break;
            }
        }
    }
}
// END modified Mame code

void gpworld::set_preset(int preset)
{
    if (preset == 1) m_align = true;
    if (preset == 2) m_shifter = false;
}

void gpworld::align()
{
    m_cpumem[0xe02e] = 0x00;
}
