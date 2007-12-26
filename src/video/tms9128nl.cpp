/*
 * tms9128nl.cpp
 *
 * Copyright (C) 2001 Matt Ownby
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

// undefine this to get rid of debug messages, or 1 to view them
//#define TMS_DEBUG 1

#include <SDL.h>
#include "SDL_DrawText.h"
#include "tms9128nl.h"
#include "palette.h"
#include "video.h"
#include "../game/game.h"
#include "../io/conout.h"
#include "../ldp-out/ldp.h"	// to check to see if blitting is allowed
#include <stdio.h>
#include <string.h>

static unsigned char g_vidbuf[TMS9128NL_OVERLAY_W * TMS9128NL_OVERLAY_H];	// video buffer needed because we clobber the SDL_Surface buffer when we do real-time scaling
static unsigned char vidmem[32767] = { 0 };	// video memory
static unsigned char lowbyte = 0;
static unsigned char highbyte = 0;
static unsigned int rvidindex;
static unsigned int wvidindex;
static int toggleflag = 0; //keep track of low / high byte
static int g_vidmode = 0;  //keep track of video mode
static int viddisp = 1; //enable video display 0=off 1=on
static int vidreg[8] = { 0 }; //registers 0-7
static int rowdiv = 40; //text mode

unsigned char g_tms_pnt_addr = 0;	// pattern name table address
unsigned char g_tms_ct_addr = 0;	// color table address
unsigned char g_tms_pgt_addr = 0;	// pattern generation table address
unsigned char g_tms_sat_addr = 0;	// sprite attribute table address
unsigned char g_tms_sgt_addr = 0;	// sprite generation table address
unsigned char g_tms_foreground_color = 0xF;	// white (3 bit)
unsigned char g_tms_background_color = 0;	// black (3 bit)

bool g_tms_interrupt_enabled = false;	// whether NMI is on or off

int g_transparency_enabled = 0;
int g_transparency_latch = 0;

// BARBADEL: Added
int introHack = 0;
int prevg_vidmode = 0;
void tms9128nl_clear_overlay();
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

// resets all variables to their initial state
void tms9128nl_reset()
{
	memset(vidmem, 0, sizeof(vidmem));
	lowbyte = 0;
	highbyte = 0;
	rvidindex = 0;
	wvidindex = 0;
	toggleflag = 0;
	g_vidmode = 0;
	viddisp = 1;
	memset(vidreg, 0, sizeof(vidreg));
	rowdiv = 40;
	g_tms_pnt_addr = 0;
	g_tms_ct_addr = 0;
	g_tms_pgt_addr = 0;
	g_tms_sat_addr = 0;
	g_tms_sgt_addr = 0;
	g_tms_foreground_color = 0xF;
	g_tms_background_color = 0;
	g_tms_interrupt_enabled = false;
	g_transparency_enabled = 0;
	g_transparency_latch = 0;
	introHack = 0;
	prevg_vidmode = 0;
}

bool tms9128nl_int_enabled()
{
	return(g_tms_interrupt_enabled);
}

void tms9128nl_writechar(unsigned char value)
//write a character on the screen, using 40*24 mode emulation
// this is the same as writing to register #0 on a TMS chip
{
    unsigned int row = 0,col = 0;
    int base = 0;

    if (viddisp==0) return; //video display is off
    //if (g_vidmode!=1) return; //only text mode 1 for now

	// MODE 1 (bitmapped text)
	if (g_vidmode == 1)
	{
		// if the coordinates requested are within the viewable area
		if ((wvidindex >= 0) && (wvidindex <= 0x3c0))
		{
			base=0;
			rowdiv=40; //40*24

			row = (wvidindex-base-1) / rowdiv;
			col = (wvidindex-base-1) % rowdiv;

		    if ((col==31) && (rowdiv==32)) return; //problems with col31? or bug in fancyclearscreen routine?

		    tms9128nl_drawchar(value, col, row);
		}
		// else, they're outside the viewable area, so draw nothing
	} // end mode 1

	// MODE 2 (Cliff Hanger graphical logo)
	else if (g_vidmode == 2)
	{
		if ((wvidindex>=0x3c00) && (wvidindex<=0x3f00)) //pattern name table - graphics mode
		{
			base=0x3c01;
			rowdiv=32; //32*24

			row = (wvidindex-base-1) / rowdiv;
			col = (wvidindex-base-1) % rowdiv;

		    if ((col==31) && (rowdiv==32)) return; //problems with col31? or bug in fancyclearscreen routine?

		    tms9128nl_drawchar(value, col, row);
		}
		// BARBADEL: Added
		else
		{
			g_tms_foreground_color = (unsigned char) ((value & 0xF0) >> 4);
			g_tms_background_color = (unsigned char) (value & 0x0F);
			tms9128nl_palette_update();	// MATT
		}
	}
} //end writechar

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
unsigned char tms9128nl_getvidmem(void)
{
    return vidmem[rvidindex++];
}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
void tms9128nl_write_port1(unsigned char value)
//port 54 is the index into video memory. First a low byte is sent, then a highbyte
// this is the same as writing to port #1 for a TMS chip (the same rules apply)
{
	static int tempindex = 0;

#ifdef TMS_DEBUG
	char s[81] = { 0 };	// just a temp string
#endif
	
    if (toggleflag==0)
    {
        wvidindex=0;
        rvidindex=0;
        lowbyte=value;
        //    printf("lowbyte: %d\n",lowbyte);
	}
	
	// if toggleflag is not zero
	else
	{
		highbyte=value;

		// if bit 7 is set, it means a TMS register is being written to
		if (highbyte & 0x80)
		{
			// AND'ing by 0x7F strips off the high bit, it has nothing to do with TMS_TRANSPARENT_COLOR
			int which_reg = highbyte & 0x7F;	// the register that is being written to

			vidreg[which_reg] = lowbyte; // set the register in memory in case we need to refer to it later

			// handle register access
			switch (which_reg)
			{
			// register 0 (select screen mode 2 or Black & White)
			case 0:
				// if only bit 1 is set, it means to go to graphics mode 2
				if (lowbyte & 2)
				{
					g_vidmode = 2;
					if(prevg_vidmode != g_vidmode)	// Barbadel: if we're switching video modes, clear the overlay
					{
#ifdef TMS_DEBUG
						printline("TMS: mode 2");
#endif
						tms9128nl_clear_overlay();
						prevg_vidmode = g_vidmode;
					}
				}
				// if the B&W bit is set, flag an error
				if (lowbyte & 1)
				{
#ifdef TMS_DEBUG
					printline("TMS: B&W bit set, unsupported");
#endif
				}
				else
				{
					#ifdef TMS_DEBUG
						printline("TMS : B&W bit cleared");
					#endif
				}
				break;

			// register 1 (4/16K, blank screen, GINT, mode1, mode3, sprite mode, sprite scale)
			case 1:
				// if bit 0 is set, MAG
				if (lowbyte & 1)
				{
#ifdef TMS_DEBUG
					printline("TMS: double sprite size not supported");
#endif
				}

				// if bit 1 is set, 16x16 sprites is what we use, else 8x8
				if (lowbyte & 2)
				{
#ifdef TMS_DEBUG
					printline("TMS: 16x16 sprites requested");
#endif
				}
				else
				{
#ifdef TMS_DEBUG
					//printline("TMS: 8x8 sprites requested");
					// don't print anything since this is what we expect
#endif
				}

				// bit 3 set == mode 3
				if (lowbyte & 0x08)
				{
					g_vidmode=3;
					if(prevg_vidmode != g_vidmode)	// Barbadel: if we're switching video modes, clear the overlay
					{
#ifdef TMS_DEBUG
						printline("TMS: mode 3");
#endif
						tms9128nl_clear_overlay();
						prevg_vidmode = g_vidmode;
					}
				}
				// bit 4 set == mode 1
				else if (lowbyte & 0x10) //bit 4 set - text mode 1
				{
					g_vidmode=1;
					if(prevg_vidmode != g_vidmode)	// Barbadel: if we're switching video modes, clear the overlay
					{
#ifdef TMS_DEBUG
						printline("TMS: mode 1");
#endif
						tms9128nl_clear_overlay();
						prevg_vidmode = g_vidmode;
					}
				}
				// if all vid bits have been cleared, then set g_vidmode to 0
				// we need to check this somewhere, here is as good a place as any I suppose
				else if (!(vidreg[0]&0x02)) //no vid bits set
				{
					g_vidmode=0;
					if(prevg_vidmode != g_vidmode)	// Barbadel: if we're switching video modes, clear the overlay
					{
						#ifdef TMS_DEBUG
							printline("TMS: g_vidmode 0 special");
						#endif
						tms9128nl_clear_overlay();
						prevg_vidmode = g_vidmode;
					}
				}
				// bit 5 set == generate interrupts
				if (lowbyte & 0x20)
				{
					#ifdef TMS_DEBUG
						// if they weren't previously enabled
						if (!g_tms_interrupt_enabled)
						{
							printline("TMS: Generate interrupts enabled");
						}
					#endif
					// don't print anything since this is what we expect
					g_tms_interrupt_enabled = 1;
				}
				else
				{
					#ifdef TMS_DEBUG
						// only notify us if they weren't already disabled
						if (g_tms_interrupt_enabled)
						{
							printline("TMS: Generate interrupts disabled");
						}
					#endif
					g_tms_interrupt_enabled = 0;
				}
					
				// don't blank screen
				if (lowbyte & 0x40)
				{
					viddisp=1; // enable video display
				}
				// blank screen, leaving only backdrop
				else
				{
	                viddisp=0; // disable video display (blank screen)

					tms9128nl_clear_overlay();
					#ifdef TMS_DEBUG
						printline("TMS: VIDEO DISPLAY TO BE BLANKED!");
					#endif
				}

				// bit 7 set == select 16K ram, else 4K ram
				if (lowbyte & 0x80)
				{
					#ifdef TMS_DEBUG
						// printline("TMS: 16K ram selected");
					#endif
					// don't print anything since this is what we expect
				}
				else
				{
					#ifdef TMS_DEBUG
						printline("TMS: 4k ram selected, unsupported");
					#endif
				}
				break;

			// register #2 sets address for pattern name table
			case 2:
				#ifdef TMS_DEBUG
				{
					unsigned char temp = (unsigned char) (lowbyte & 0xF); // only lowest 4 bits
					if (temp != g_tms_pnt_addr)
					{
						sprintf(s, "TMS: Pattern Name Table Address changed to %x", g_tms_pnt_addr);
						printline(s);
					}
				}
				#endif
				g_tms_pnt_addr = (unsigned char) (lowbyte & 0xF); // only lowest 4 bits
				break;
			// register #3 sets address for color table
			case 3:
				#ifdef TMS_DEBUG
					if (lowbyte != g_tms_ct_addr)
					{
						sprintf(s, "TMS: Color Table Address changed to %x", g_tms_ct_addr);
						printline(s);
					}
				#endif

				g_tms_ct_addr = lowbyte;
				break;

			// register #4 sets address for pattern generation table
			case 4:
				#ifdef TMS_DEBUG
				{
					unsigned char temp = (unsigned char) (lowbyte & 7);
					if (temp != g_tms_pgt_addr)
					{
						sprintf(s, "TMS: Pattern Generation Table changed to %x", g_tms_pgt_addr);
						printline(s);
					}
				}
				#endif
				
				g_tms_pgt_addr = (unsigned char) (lowbyte & 7); // only lowest 3 bits
				break;

			// register #5 sets address for sprite attribute table
			case 5:
				#ifdef TMS_DEBUG
					{
						unsigned char temp = (unsigned char) (lowbyte & 0x7F);
						if (temp != g_tms_sat_addr)
						{
							sprintf(s, "TMS: Sprite Attribute Table address changed to %x", g_tms_sat_addr);
							printline(s);
						}
					}
				#endif
				g_tms_sat_addr = (unsigned char) (lowbyte & 0x7F); // discard high bit
				break;
				
			// register #6 sets address for sprite generation table
			case 6:
				#ifdef TMS_DEBUG
					{
						unsigned char temp = (unsigned char) (lowbyte & 0x7);
						if (temp != g_tms_sgt_addr)
						{
							sprintf(s, "TMS: Sprite Generator Table address changed to %x", g_tms_sgt_addr);
							printline(s);
						}
					}
				#endif
				g_tms_sgt_addr = (unsigned char) (lowbyte & 0x7);	// only lowest 3 bits
				break;

			// register #7 sets foreground and background colors
			case 7:
				#ifdef TMS_DEBUG
				{
					unsigned char t1 = (unsigned char) ((lowbyte & 0xF0) >> 4);
					unsigned char t2 = (unsigned char) (lowbyte & 0x0F);
					if (t1 != g_tms_foreground_color)
					{
						sprintf(s, "TMS : Foreground color changed to %x", g_tms_foreground_color);
						printline(s);
					}
					if (t2 != g_tms_background_color)
					{
						sprintf(s, "TMS : Background color changed to %x", g_tms_background_color);
						printline(s);
					}
				}
				#endif
				
				g_tms_foreground_color = (unsigned char) ((lowbyte & 0xF0) >> 4);
				g_tms_background_color = (unsigned char) (lowbyte & 0x0F);
				tms9128nl_palette_update();
				break;

			default:
				#ifdef TMS_DEBUG
					sprintf(s,"TMS: Register %d was written to (unsupported)", which_reg);
					printline(s);
				#endif
				break;
			} //end switch
		} //end if bit 7 is set (and we're writing to a register)

		// bit 7 is clear so we're not writing to a register
		// instead, we are modifying the video memory address
		else
		{
			#ifdef TMS_DEBUG
				// NOTE : we don't currently keep track of whether we're in memory read/write mode
				// this seems to work fine for now but it may be an issue later
				
				// if bit 6 is set it means we're in memory write mode
				if (highbyte & 0x40)
				{
	//				printline("Memory write mode requested");
				}
				// otherwise we're in memory read mode
				else
				{
					printline("Memory read mode requested");
				}
			#endif
			
			highbyte = (unsigned char) (highbyte & 0x3f); //strip top 2 bits
			tempindex = (highbyte << 8) | lowbyte;
			wvidindex = tempindex;
			rvidindex = tempindex;
		} //end if high bit not set
	} //end if toggleflag is not zero
	toggleflag=toggleflag^1; //flip bit 1
}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

// writes a byte to port 0 of the TMS chip
void tms9128nl_write_port0(unsigned char Value)
//return a 1 if the screen needs updating
{
	vidmem[wvidindex] = Value;
	wvidindex++;	// the index always advances when we write
	tms9128nl_writechar(Value); // update the screen
}

void tms9128nl_convert_color(unsigned char color_src, SDL_Color *color)
{

	char s[81] = { 0 };

	switch(color_src)
	{
	case 0: // transparent
		color->r = 0;
		color->g = 0;
		color->b = 0;
		break;
	case 1: // black
		color->r = 0;
		color->g = 0;
		color->b = 0;
		break;
	case 2: // medium green;
		color->r = 36;
		color->g = 219;
		color->b = 36;
		break;
	case 3: // light green
		color->r = 109;
		color->g = 255;
		color->b = 109;
		break;
	case 4: // dark blue
		color->r = 36;
		color->g = 36;
		color->b = 255;
		break;
	case 5: // light blue
		color->r = 73;
		color->g = 109;
		color->b = 255;
		break;
	case 6:	// dark red
		if (!g_transparency_latch)
		{
			color->r = 182;
			color->g = 36;
			color->b = 36;
		}
		// if we're in transparency mode, the move prompts
		else
		{
			// these colors aren't correct for the TMS chip but they make Cliffy look a bit more
			// authentic (the move prompts are more purplish than red)
			color->r = 125;
			color->g = 0;
			color->b = 128;	//BARBADEL:	128 makes it more purple than 50
		}
		break;
	case 7: // light cyan
		color->r = 73;
		color->g = 219;
		color->b = 255;
		break;
	case 8:	// medium red
		color->r = 255;
		color->g = 36;
		color->b = 36;
		break;
	case 9: // light red
		color->r = 255;
		color->g = 109;
		color->b = 109;
		break;
	case 10:	// dark yellow
		color->r = 219;
		color->g = 219;
		color->b = 36;
		break;
	case 11: // light yellow
		color->r = 219;
		color->g = 219;
		color->b = 146;
		break;
	case 12: // dark green
		color->r = 36;
		color->g = 146;
		color->b = 36;
		break;
	case 13:	// magenta
		color->r = 219;
		color->g = 73;
		color->b = 182;
		break;
	case 14: // grey
		color->r = 182;
		color->g = 182;
		color->b = 182;
		break;
	case 15: // white
		color->r = 255;
		color->g = 255;
		color->b = 255;
		break;
	default:
		sprintf(s, "UNSUPPORTED COLOR passed into convert color : %d", color_src);
		printline(s);
		break;
	}
}

// draws a character to the Cliff video display
// 40 columns by 24 rows
// uses Cliffy's video memory to retrieve 8x8 character bitmap
void tms9128nl_drawchar(unsigned char ch, int col, int row)
{
	const int CHAR_WIDTH = 8;
	const int CHAR_HEIGHT = 8;

	int bmp_index = (ch * 8) + (g_tms_pgt_addr << 11); // index in vid mem where bmp data is located
	int i = 0, j = 0;	// temp indices
	int x = col * CHAR_WIDTH;
	int y = row * CHAR_HEIGHT;
	unsigned char line = 0;
	unsigned char background_color = TMS_BG_COLOR;

	// if character is 0 and we're in transparency mode, make bitmap transparent	
	if (g_transparency_latch)
	{
		static bool latched = false;
		
		if ((ch == 0) || (ch == 0xFF))
		{
			// we need two 0's in a row to make a transparent block
			if (!latched)
			{
				latched = true;
				return;
			}
			else
			{
				background_color = TMS_TRANSPARENT_COLOR;
			}
		}		
		else
		{
			latched = false;
		}
	}

	// colors are special in g_vidmode 2, we don't handle them properly yet
	if (g_vidmode == 2)
	{
		// MODIFIED SECTION BY BARBADEL

		if (ch != 255)
		{
			x += 4;	// MATT : even scaled, it is still 1 character off-center.  Perhaps it has a 1 character border?

			// major hack : If the row is less than 12 we know it's the Cliff Hanger graphic, hence we hard-code the
			// correct values in here.  Someone should fix this sometime :)
			if (row <= 11)
			{
				g_tms_foreground_color = 0x5;	// Light Blue 
				g_tms_background_color = 0x1; // Black
				tms9128nl_palette_update();
				bmp_index = (ch * 8) + 8*256 * (row > 7);
			}
			else
			{
// MATT : these color hacks don't seem to be necessary anymore
//				g_tms_foreground_color = 0x1; // Black
//				g_tms_background_color = 0x5;	// Light Blue
				bmp_index = (ch * 8) + 0x3800; 
			}
		}
		else
		{
			x += 4;
			g_tms_foreground_color = 0x0;           // Black
			g_tms_background_color = 0x5;           // Light Blue
			tms9128nl_palette_update();
			
			if(col != 0)
			{
//				g_transparent_color              = 0xff;
				bmp_index = (ch * 8) + 8*256 * (row > 7);
			}
			else
			{
//				g_transparent_color              = 0x7f;
//				background_color = 0;
				bmp_index = (ch * 8) + 8*256 * (row > 7);
			}
		}
	}

	// draw each line of character into new surface
	for (i = 0; i < CHAR_HEIGHT; i++)
	{
		line = vidmem[bmp_index + i];	// get a line

		// handle each pixel across
		for (j = CHAR_WIDTH - 1; j >= 0; j--)
		{
			// if rightmost bit is 1, it means draw the pixel
			if (line & 1)
			{
				*((Uint8 *) g_vidbuf + ((y+i+TMS_VERTICAL_OFFSET) * TMS9128NL_OVERLAY_W) + (x+j)) = TMS_FG_COLOR;
			}
			// else draw the background
			else
			{
				*((Uint8 *) g_vidbuf + ((y+i+TMS_VERTICAL_OFFSET) * TMS9128NL_OVERLAY_W) + (x+j)) = background_color;
			}
			line = (unsigned char) (line >> 1);
		}
	} // end for loop

	// In transparency mode, if we draw a solid character, we need to make the character after it non-transparent
	// This seems to be how Cliff Hanger behaves.  I haven't found it documented anywhere though.
	if ((g_transparency_latch) && (ch != 0) && (ch != 0xFF))
	{
			int row, col;
			Uint8 *ptr = ((Uint8 *) g_vidbuf) + ((y + TMS_VERTICAL_OFFSET) * TMS9128NL_OVERLAY_W) + x + CHAR_WIDTH;
			for (row = 0; row < CHAR_HEIGHT; row++)
			{
				for (col = 0; col < CHAR_WIDTH; col++)
				{
					// make it non-transparent if it is
					if (*ptr == TMS_TRANSPARENT_COLOR)
					{
						*ptr = TMS_BG_COLOR;
					}
					ptr++;
				}
				ptr += (TMS9128NL_OVERLAY_W - CHAR_WIDTH);	// move to the next line
			}
	}

	g_game->set_video_overlay_needs_update(true);
}

void tms9128nl_outcommand(char *s,int col,int row)
{
// gp2x doesn't have enough resolution to display this schlop anyway...
#ifndef GP2X
    SDL_Rect dest;

    dest.x = (short) ((col*6) + 200);
    dest.y = (short) ((row*13) + 100);
    dest.w = (unsigned short) (6 * strlen(s)); // width of rectangle area to draw (width of font * length of string)
    dest.h = 13;	// height of area (height of font)

    // VLDP freaks out if it's not the only thing drawing to the screen
    if (!g_ldp->is_vldp())
    {
		vid_blank();
	    SDLDrawText(s, get_screen_blitter(), FONT_SMALL, dest.x, dest.y);
		// TODO : get this working again under the new video scheme
    }
#endif
}

// called everytime the color palette changes
// We don't make this part of the 'palette_update()' function because that function also
// does some stuff with the transparent color, which is only a one-time initialization requirement
void tms9128nl_palette_update()
{
	SDL_Color fore, back;	// the foreground and background colors
	
	tms9128nl_convert_color(g_tms_foreground_color, &fore);
	tms9128nl_convert_color(g_tms_background_color, &back);

	palette_set_color(0, back);
	palette_set_color(255, fore);

	// if we should do extra calculations for stretching
	if (g_vidmode == 2)
	{
		SDL_Color fore75back25, fore5back5, fore25back75;	// mixtures of the foreground and background colors (for stretching)
		MIX_COLORS_75_25(fore75back25, fore, back);	// 3/4, 1/4
		MIX_COLORS_50(fore5back5, fore, back);	// average
		MIX_COLORS_75_25(fore25back75, back, fore);	// 1/4, 3/4
		palette_set_color(1, fore25back75);
		palette_set_color(2, fore5back5);
		palette_set_color(3, fore75back25);
	}

	palette_finalize();
	g_game->set_video_overlay_needs_update(true);
}

// calculates the initial color palette (basically our main routine is in palette_update)
void tms9128nl_palette_calculate()
{
	// transparent color set to a light grey color so we can debug more effectively in 'noldp' mode
	SDL_Color color;
	color.r = color.g = color.b = TMS_TRANSPARENT_COLOR;
	palette_set_transparency(0, false);	// change default to non-transparent
	palette_set_transparency(TMS_TRANSPARENT_COLOR, true);	// make transparent color transparent :)
	palette_set_color(TMS_TRANSPARENT_COLOR, color);
	
	tms9128nl_palette_update();
	tms9128nl_reset();
}

void tms9128nl_video_repaint()
{
	// if the transparency state has changed
	if (g_transparency_enabled != g_transparency_latch)
	{
	int i = 0; 	 

		Uint8 *ptr = (Uint8 *) g_vidbuf + (TMS9128NL_OVERLAY_W * TMS_VERTICAL_OFFSET);

		// I don't believe we want to do the stretched overlay here

		// if transparency was off and is now on and we're in vidmode 1 (HACK)
		if ((g_transparency_enabled) && (g_vidmode == 1))
		{

			for (i = 0; i < TMS9128NL_OVERLAY_W * (TMS9128NL_OVERLAY_H - (TMS_VERTICAL_OFFSET << 1)); i++)
			{
				// if color is a background color, make it 0x7F (transparency color)
				if (*ptr == 0) *ptr = TMS_TRANSPARENT_COLOR;
				ptr++;
			}
		}
		else
		{
			for (i = 0; i < TMS9128NL_OVERLAY_W * (TMS9128NL_OVERLAY_H - (TMS_VERTICAL_OFFSET << 1)); i++)
			{
				// if color is transparent (0x7F), make it a background color
				if (*ptr == TMS_TRANSPARENT_COLOR) *ptr = 0;
				ptr++;
			}
		}

		g_transparency_latch = g_transparency_enabled;
	}

	g_transparency_enabled = 0;	// apparently this has to be set to true every pulse of the NMI in order
	// to maintain the transparency.  The Cliff ROM does this.

	// if we're in video mode 2, we have to display our stretched overlay instead of our regular one
	if (g_vidmode == 2)
	{
		tms9128nl_video_repaint_stretched();
	}
	
	// if we're not in mode 2, display our non-stretched overlay
	else
	{
		memcpy(g_game->get_active_video_overlay()->pixels, g_vidbuf, TMS9128NL_OVERLAY_W * TMS9128NL_OVERLAY_H);
	}
	

}

// creates the stretched overlay, using the contents of the normal overlay as its source
// the stretched overlay is simply a 256x192 window scaled to 320x192 using a hard-coded algorithm (hopefully it's fast)
void tms9128nl_video_repaint_stretched()
{
	int x256 = 0;
	int y = 0;

	Uint8 *ptr256 = (Uint8 *) g_vidbuf;	// source ...
	Uint8 *ptr320 = (Uint8 *) g_game->get_active_video_overlay()->pixels;	// destination ...

	// these values correspond to colors in the color palette	
	unsigned char blend[4][2] = 
	{
		{ 0, 0 },
		{ 3, 1 },
		{ 2, 2 },
		{ 1, 3 },
	};

	// do every row	
	for (y = 0; y < TMS9128NL_OVERLAY_H; y++)
	{
		// do each pixel, but divide it up into smallest integer sections so we can use a hard-coded algorithm
		// there is a 4:5 correspondance between the 256 and 320 surfaces
		for (x256 = 0; x256 < 256; x256 += 4)
		{
			// PIXEL +0
			*(ptr320) = *(ptr256);
			
			// PIXEL +1 to PIXEL +3 (blending possibly required)
			for (int i = 1; i < 4; i++)
			{
				// if prev pixel is not the same as cur pixel, blending is required
				if (*(ptr256+i-1) != *(ptr256+i))
				{
					// if prev pixel is background color, cur pixel must be foreground
					if (*(ptr256+i-1) == 0)
					{
						*(ptr320+i) = blend[i][0];
					}
					// else prev pixel is foreground, and therefore cur pixel must be background
					else
					{
						*(ptr320+i) = blend[i][1];
					}
				}
				else *(ptr320+i) = *(ptr256+i);	// else no blending required
			}

			// PIXEL +4			
			*(ptr320+4) = *(ptr256+3);
			
			ptr320 += 5;
			ptr256 += 4;
		}
		ptr256 += (320-256);	// move to the next line, ptr320 is already at the next line
	}
}

void tms9128nl_clear_overlay()
{
	Uint8 clear_color = TMS_BG_COLOR;

	int i = 0;

	Uint8 *ptr = g_vidbuf;
	
//	printf("Overlay is being cleared, transparency is %d, latch is %d\n", g_transparency_enabled, g_transparency_latch);
	
	// if transparency mode is on ...
	if (g_transparency_latch)
	{
		clear_color = TMS_TRANSPARENT_COLOR;
	}

	// top area always gets border color and is never transparent
	for (i = 0; i < TMS9128NL_OVERLAY_W * TMS_VERTICAL_OFFSET; i++)
	{

		*ptr = 0;
		ptr++;
	}
	
	// erase viewable area with either the background color or the transparent color
	for (i = 0; i < TMS9128NL_OVERLAY_W * (TMS9128NL_OVERLAY_H - (TMS_VERTICAL_OFFSET << 1)); i++)
	{
		*ptr = clear_color;
		ptr++;
	}

	// bottom area always gets border color and is never transparent
	for (i = 0; i < TMS9128NL_OVERLAY_W * TMS_VERTICAL_OFFSET; i++)
	{
		*ptr = 0;
	}

	g_game->set_video_overlay_needs_update(true);
}

// kind of a hack for Cliff Hanger, not sure if this is part of the TMS9128NL chip or not
// sets the "transparency value" for one NMI tick (it gets cleared at each NMI tick)
void tms9128nl_set_transparency()
{
	g_transparency_enabled = 1;
}

