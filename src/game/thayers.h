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

#ifndef THAYERS_H
#define THAYERS_H

#include <SDL.h>
#include "game.h"
#include "../scoreboard/scoreboard_collection.h"

#define THAYERS_CPU_HZ 4000000 // speed of cpu

#define KMATRIX 1000.0f

class thayers : public game
{
  public:

    typedef enum {
        TQ_ONE, TQ_TWO, TQ_THREE, TQ_FOUR, TQ_COIN1, TQ_COIN2,
        TQ_YES, TQ_ITEMS, TQ_DROP, TQ_GIVE, TQ_REPLAY,
        TQ_COMBINE, TQ_SAVE, TQ_UPDATE, TQ_HINT, TQ_NO,
        TQ_A, TQ_B, TQ_C, TQ_D, TQ_E, TQ_F, TQ_G, TQ_H,
        TQ_I, TQ_J, TQ_K, TQ_L, TQ_M, TQ_N, TQ_O, TQ_P,
        TQ_Q, TQ_R, TQ_S, TQ_T, TQ_U, TQ_V, TQ_W, TQ_X,
        TQ_Y, TQ_Z, TQ_EMPTY
    } KeyMapType;

    thayers();
    bool init();
    void shutdown();
    void set_version(int);
    void do_irq(unsigned int which);
    void do_nmi(); // dummy function to generate timer IRQ
    void thayers_irq();
    Uint8 cpu_mem_read(Uint16 addr);              // memory read routine
    void cpu_mem_write(Uint16 addr, Uint8 value); // memory write routine
    Uint8 port_read(Uint16 port);                 // read from port
    void port_write(Uint16 port, Uint8 value);    // write to a port
    void process_keydown(SDL_Keycode);
    void process_keyup(SDL_Keycode);
    void input_enable(Uint8, Sint8);              // for keyboard bezel
    void input_disable(Uint8, Sint8);
    bool set_bank(unsigned char, unsigned char);
    void palette_calculate();
    void repaint();
    void set_preset(int);
    void init_overlay_scoreboard();
    void write_scoreboard(Uint8, Uint8, int); // function to decode scoreboard
                                              // data

    // COP421 interface functions
    void thayers_write_d_port(unsigned char); // Write to D port
    void thayers_write_l_port(unsigned char); // Write L port
    unsigned char thayers_read_l_port(void);  // Read L port
    void thayers_write_g_port(unsigned char);
    unsigned char thayers_read_g_port(void);  // Read to G I/O port
    void thayers_write_so_bit(unsigned char); // Write to SO
    unsigned char thayers_read_si_bit(void);  // Read to SI
    void OnMouseMotion(Uint16, Uint16, Sint16, Sint16, Sint8);

    // To turn off speech synthesis (only called from cmdline.cpp)
    void no_speech();

    // Called by ssi263.cpp whenever it has something to say <g>.
    void show_speech_subtitle();

  protected:
    //	void string_draw(char*, int, int);
    Uint8 coprom[0x400];
    bool key_press;
    Uint8 cop_read_latch;
    Uint8 cop_write_latch;
    Uint8 cop_g_read_latch;
    Uint8 cop_g_write_latch;
    Uint8 m_irq_status;
    Uint8 banks[4]; // thayers's banks
                    // bank 1 is Dip Bank A
    // bank 2 is bits 0-3 is Dip Bank B, 4 and 5 Coin 1 and 2, 6 and 7 laserdisc
    // ready

  private:
    // Overlay text control stuff.
    bool m_use_overlay_scoreboard;
    bool m_show_speech_subtitle;
    bool m_show_timerboard;
    bool m_show_startup;

    // Text-to-speech related vars/methods.
    bool m_use_speech;
    void speech_buffer_cleanup(char *src, char *dst, int len);

    // Keyboard
    Uint16 m_axis_x;
    Uint16 m_axis_y;
    SDL_Cursor* g_cursor;
    Uint16 get_keymap(Uint16, Uint16);
    SDL_Keycode get_keycode(int value);
    SDL_Rect m_keyrect = { 0, 0, 0, 0 };

    // pointer to our scoreboard interface
    IScoreboard *m_pScoreboard;

    // whether overlay scoreboard is visible or not
    bool m_bScoreboardVisibility;

    const int m_tq_keys[TQ_EMPTY][4] =
    {
        { 16,  52,  95, 462}, { 16, 548,  95, 955}, {900,  52, 982, 462},
        {900, 548, 982, 955}, {125, 825, 205, 970}, {795, 825, 875, 970},
        {120,  52, 185, 290}, {195,  52, 265, 290}, {275,  52, 340, 290},
        {350,  52, 415, 290}, {425,  52, 495, 290}, {505,  52, 570, 290},
        {580,  52, 645, 290}, {655,  52, 725, 290}, {735,  52, 800, 290},
        {810,  52, 880, 290}, {155, 585, 225, 765}, {540, 790, 610, 975},
        {390, 790, 455, 975}, {310, 585, 380, 765}, {275, 380, 340, 560},
        {390, 585, 455, 765}, {465, 585, 530, 765}, {540, 585, 610, 765},
        {655, 380, 725, 560}, {620, 585, 690, 765}, {700, 585, 765, 765},
        {775, 585, 845, 765}, {700, 790, 765, 975}, {620, 790, 690, 975},
        {735, 380, 800, 560}, {810, 380, 880, 560}, {120, 380, 185, 560},
        {350, 380, 415, 560}, {235, 585, 300, 765}, {425, 380, 495, 560},
        {580, 380, 645, 560}, {465, 790, 530, 975}, {195, 380, 265, 560},
        {310, 790, 380, 975}, {505, 380, 570, 560}, {235, 790, 300, 975}
    };
};

#endif // THAYERS_H
