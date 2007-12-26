/* Title: GP2X minimal library v0.B
   Written by rlyeh, (c) 2005.

   Please check readme for licensing and other conditions. */

#include "minimal.h"

// MPO : to make sure this is constant where it's used
unsigned int MLC_VLA_TP_PXW = 320;

gp2x_video_layer gp2x_video_YUV[4];

static          unsigned long   gp2x_dev[8]={0,0,0,0,0,0,0,0}, gp2x_ticks_per_second;
static volatile unsigned short *gp2x_memregs;
static volatile unsigned long   gp2x_ticks;
static volatile unsigned long  *gp2x_memregl, gp2x_exit;

/*
   ,--------------------.
   |                    |X
   |    GP2X - VIDEO    |X
   |                    |X
   `--------------------'X
	XXXXXXXXXXXXXXXXXXXXXX 
*/


/* Function: gp2x_video_waitvsync
   This function halts the program until a vertical sync is done.

   See also:
   <gp2x_video_waithsync> */

void gp2x_video_waitvsync(void)
{
//  while(gp2x_memregs[0x1182>>1]&(1<<4));

	// MPO
	// if we're in the middle of vblank, then wait until next one
	while (gp2x_memregs[0x1182>>1]&(1<<4));

	// wait until next vblank starts
	while (!(gp2x_memregs[0x1182>>1]&(1<<4)));
}

/* Function: gp2x_video_waithsync
   This function halts the program until an horizontal sync is done.

   See also:
   <gp2x_video_waitvsync> */

void gp2x_video_waithsync(void)
{
	while (gp2x_memregs[0x1182>>1]&(1<<5));
}

void gp2x_video_RGB_setwindows(int window0, int window1, int window2, int window3, int x, int y)
{
	int window,mode,mode2,x1,y1,x2,y2;

	// disable all RGB Windows
	gp2x_memregs[0x2880>>1] &= ~ ((1<<6)|(1<<5)|(1<<4)|(1<<3)|(1<<2));
}

/* Function: gp2x_video_YUV_setscaling
   This function adjusts a given resolution to fit the whole display (320,240).

   Notes:
   - Draw at (0,0) coordinates of each framebuffer always, whatever resolution you are working with.
   - Call this function once. Do not call this function in every frame.
  
   Parameters:
   region (0..3) - YUV region (0..3)
   W (1..) - virtual width in pixels to scale to 320.
   H (1..) - virtual height in pixels to scale to 240.

   Default:
   - (W,H) are set to (320,240) for each region. */

void gp2x_video_YUV_setscaling(int region, int W, int H)
{
	// MLC_YUVA_TP_HSC (horizontal scale factor of Region A, top)
	gp2x_memregs[0x2886>>1]       = (unsigned short)((float)1024.0  * (W/320.0));

	// MLC_YUVA_TP_VSC[L/H] (vertical scale factor of Region A, top)
	gp2x_memregs[0x288A>>1] = (unsigned short)(MLC_VLA_TP_PXW *(H/240.0));
	gp2x_memregs[0x288C>>1]       = 0;

}

/* Function: gp2x_video_YUV_flip
   This function flips video buffers. It updates pointers located at <gp2x_video_YUV> struct too.
   The current pointers will point to the hidden display until a new gp2x_video_YUV_flip() call is done.

   Note:
   - This function does not wait for the vertical/horizontal retraces. You should use <gp2x_video_waitvsync> or <gp2x_video_waithsync> if needed.
   - YUV has 2 layers (A/B), 4 regions and no windows.
   - It is a mistake confusing 'window', 'region', 'part' and 'layer'. They are not the same.

   Parameters:
   region (0..3) - YUV layer to flip 

   See also:
   <gp2x_video_RGB_flip> */

void gp2x_video_YUV_flip(int unused)
{
	unsigned long address = gp2x_video_YUV[0].i[gp2x_video_YUV[0].uWhichBuf];

	// move to next buffer, if we overflow, go back to 0
	if (++gp2x_video_YUV[0].uWhichBuf==2) gp2x_video_YUV[0].uWhichBuf=0;

	gp2x_video_YUV[0].screen32=(unsigned long  *)gp2x_video_YUV[0].p[gp2x_video_YUV[0].uWhichBuf]; 

	// region A, odd fields
	gp2x_memregs[0x28A0>>1] = (address & 0xFFFF);
	gp2x_memregs[0x28A2>>1] = (address >> 16);

	// region A, even fields
	gp2x_memregs[0x28A4>>1] = (address & 0xFFFF);
	gp2x_memregs[0x28A6>>1] = (address >> 16);

//	printf("YUV_flip: address is %x\n", address);
}

void gp2x_video_YUV_setparts(int part0, int part1, int part2, int part3, int x, int y)
{
	// disable YUV Planes A and B
	gp2x_memregs[0x2880>>1]&=~((1<<1)|(1<<0));

	// enable YUV plane A
	gp2x_memregs[0x2880>>1]|= 1;

	printf("MLC_OVLAY_CNTR 2880 value is %x\n", gp2x_memregs[0x2880>>1]);

	// disable bottom regions for A & B, and turn off all mirroring
	gp2x_memregs[0x2882>>1] &= 0xFC00;

	printf("MLC_YUV_EFFECT 2882 value is %x (we expect it to be 0)\n", gp2x_memregs[0x2882>>1]);

	// MLC_YUVA_STX (X start coordinate of region A)
	gp2x_memregs[0x2896>>1]=0;

	// MLC_YUVA_ENDX (X stop coordinate of region A)
	gp2x_memregs[0x2898>>1]=319;

	// MLC_YUV_TP_STY (Y start coordinate of region A top)
	gp2x_memregs[0x289A>>1]=0;

	// MLC_YUV_TP_ENDY (Y stop coordinate of region A top, and the start of the bottom)
	gp2x_memregs[0x289C>>1]=239;

	// MLC_YUV_BT_ENDY (Y stop coordinate of region A bottom)
	gp2x_memregs[0x289E>>1]=239;

	// X start of region B
	gp2x_memregs[0x28C0>>1]=319;
	// X stop of region B
	gp2x_memregs[0x28C2>>1]=319;

	// Y start of region B top
	gp2x_memregs[0x28C4>>1]=0;
	// Y stop of region B top, start of bottom
	gp2x_memregs[0x28C6>>1]=239;
	// Y stop of region B bottom
	gp2x_memregs[0x28C8>>1]=239;

	//flush mirroring changes (if any) into current framebuffers
	gp2x_video_YUV_flip(0);
	gp2x_video_YUV_flip(0);
}

/*
   ,--------------------.
   |                    |X
   |   GP2X - JOYSTICK  |X
   |                    |X
   `--------------------'X
	XXXXXXXXXXXXXXXXXXXXXX 
*/



/* Function: gp2x_joystick_read
   This function returns the active <GP2X joystick values>.

   Note:
   Call this function once per frame keep your joystick values updated.

   Usage:
   In order to detect simultaneous values you will have to mask the value.

   See also:
   <GP2X joystick values>

   Example:
   > unsigned long pad=gp2x_joystick_read();
   >
   > if(pad&GP2X_R) if(pad&GP2X_L) ... //both L and R are pressed. */

unsigned long gp2x_joystick_read(void)
{
	unsigned long value=(gp2x_memregs[0x1198>>1] & 0x00FF);

	if (value==0xFD) value=0xFA;
	if (value==0xF7) value=0xEB;
	if (value==0xDF) value=0xAF;
	if (value==0x7F) value=0xBE;

	return ~((gp2x_memregs[0x1184>>1] & 0xFF00) | value | (gp2x_memregs[0x1186>>1] << 16));
}

/*
   ,--------------------.
   |                    |X
   |    GP2X - TIMER    |X
   |                    |X
   `--------------------'X
	XXXXXXXXXXXXXXXXXXXXXX 
*/

/* Function: gp2x_timer_read
   This function returns elapsed ticks in <gp2x_ticks> units since last <gp2x_init> call.

   Note:
   - There is no currently way to reset <gp2x_ticks> value to 0.

   See also:
   <gp2x_timer_delay>, <gp2x_ticks> */

unsigned long gp2x_timer_read(void)
{
	return gp2x_memregl[0x0A00>>2]/gp2x_ticks_per_second;
}

static void gp2x_save_registers(int mode)  //0=save, 1=restore, 2=restore & exit 
{                 
	static unsigned short *reg=NULL; unsigned long ints, i,c;
	unsigned short savereg[]={
		0x2880>>1,0x28AE>>1, //YUV A
		0x28B0>>1,0x28D8>>1, //YUV B
		0x28DA>>1,0x28E8>>1, //RGB
		0x290C>>1,0x290C>>1, //RGB - line width
		0x291E>>1,0x2932>>1, //Cursor
		0,0};

	if (reg==NULL) reg=(unsigned short *)malloc(0x10000);

	ints=gp2x_memregl[0x0808>>2];
	gp2x_memregl[0x0808>>2] = 0xFF8FFFE7;  //mask interrupts

	if (mode)
	{
		for (c=0;savereg[c];c+=2) for (i=savereg[c];i<=savereg[c+1];i++) gp2x_memregl[i]=reg[i];
	}
	else
	{
		for (c=0;savereg[c];c+=2) for (i=savereg[c];i<=savereg[c+1];i++) reg[i]=gp2x_memregl[i];
	}

	if (mode==2)
	{
		free(reg); reg=NULL;
	}

	gp2x_memregl[0x0808>>2]=ints;
}

/*
   ,--------------------.
   |                    |X
   |   GP2X - LIBRARY   |X
   |                    |X
   `--------------------'X
	XXXXXXXXXXXXXXXXXXXXXX 
*/



/* Function: gp2x_init
   This function sets up the whole library.

   Notes:
   - *You have to call this function before any other function*.
   - If you are merging both SDL and Minimal Library in your project,
   you will have to call gp2x_init after any SDL_Init() / SDL_SetVideoMode() / SDL_CreateRGBSurface() call.

   Parameters:                     
   ticks_per_second (1..7372800) - sets internal <gp2x_ticks>. Eg, set it to 1000 to work in milliseconds.
   bpp (8,15,16) - sets the bits per pixel video mode
   rate (11025,22050,44100) - sets the sound frequency rate
   bits (16) - sets the sound bits. GP2X supports 16 bits only.
   stereo (0,1) - sets the stereo mode. 1 for stereo, 0 for mono.
   Hz (unused) - sets sound poll frequency, in hertzs. Unused in this release.
   solid_font (0,1) - sets default 6x10 font to have solid background or not.

   See also:
   <gp2x_deinit> */


void gp2x_init(int ticks_per_second, int bpp, int rate, int bits, int stereo, int Hz, int solid_font)
{
	struct fb_fix_screeninfo fixed_info;
	static int first=1;

	//init misc variables
	if (first) gp2x_exit=0, gp2x_ticks=0, gp2x_ticks_per_second=7372800/ticks_per_second;

	//open devices
	if (!gp2x_dev[2])  gp2x_dev[2] = open("/dev/mem",   O_RDWR);

	//map memory for our cursor + 4 YUV regions (double buffered each one)
	gp2x_video_YUV[0].p[0]=(void *)mmap(0, 0x4B000, PROT_READ|PROT_WRITE, MAP_SHARED, gp2x_dev[2], (gp2x_video_YUV[0].i[0]=0x04000000-0x4B000*8) );
	gp2x_video_YUV[0].p[1]=(void *)mmap(0, 0x4B000, PROT_READ|PROT_WRITE, MAP_SHARED, gp2x_dev[2], (gp2x_video_YUV[0].i[1]=0x04000000-0x4B000*7) );
	gp2x_memregl=(unsigned long  *)mmap(0, 0x10000,                    PROT_READ|PROT_WRITE, MAP_SHARED, gp2x_dev[2], 0xc0000000);

	if (gp2x_memregl == -1)
	{
		perror("mmap failed: ");
	}
	gp2x_memregs=(unsigned short *)gp2x_memregl;

	if (first)
	{
		printf(MINILIB_VERSION "\n");
		gp2x_save_registers(0);
		gp2x_memregs[0x0F16>>1] = 0x830a; usleep(100000);
		gp2x_memregs[0x0F58>>1] = 0x100c; usleep(100000);
	}

	//MPO : debug
	printf("MLC_VLA_TP_PXW (before changing) is %x %x %x %x\n",
		   gp2x_memregs[0x2892>>1], gp2x_memregs[0x2894>>1], gp2x_memregs[0x28BC>>1],
		   gp2x_memregs[0x28BE>>1]);

	// set YUV pixel width for A/B top/bottom regions 
	gp2x_memregs[0x2892>>1] = gp2x_memregs[0x2894>>1] = gp2x_memregs[0x28BC>>1] = gp2x_memregs[0x28BE>>1] = MLC_VLA_TP_PXW;	// MPO /*YUV*/

	// MLC_YUV_CNTL: set YUV source to external memory for regions A and B, turn off "stepping"
	gp2x_memregs[0x2884>>1]=0;     

	//set RGB pixel width (8,15/16)
	gp2x_memregs[0x290C>>1]=320*((bpp+1)/8);  

	//set RGB bpp (8,15/16) ; enable RGB window 1
	gp2x_memregs[0x28DA>>1]=(((bpp+1)/8)<<9)|0x0AB;																   /*RGB: 8/15/16/24bpp and 5 layers active*/

	//clear icounters+pcounters at gp2x_video_layer structs
	memset(&gp2x_video_YUV[0].i[6], 0, sizeof(unsigned long)*2);

	//set some video settings
	if (first)
	{
		gp2x_video_RGB_setwindows(0x11,-1,-1,-1,319,239);
		gp2x_video_YUV_setscaling(0,320,240);
		gp2x_video_YUV_setparts(-1,-1,-1,-1,319,239);
	}

	gp2x_video_YUV_flip(0);

	//set some default options, launches sound engine, and ready to rock... :-)
	if (first)
	{
		atexit(gp2x_deinit);
		printf("atexit is being set\n");
		first=0;
	}
}


/* Function: gp2x_deinit
   This function unsets the whole library and returns to GP2X menu.

   Note:    
   - You can exit() your program, or call this function directly.

   See also:
   <gp2x_init> */

extern int fcloseall(void);

void gp2x_deinit(void)
{
	gp2x_init(1000, 16, 44100,16,1,60, 1);

	sleep(2); // sleep for unknown reasons, maybe to let threads close?

	gp2x_save_registers(2); 

	gp2x_video_RGB_setwindows(0x11,-1,-1,-1,319,239);
	gp2x_video_YUV_setparts(-1,-1,-1,-1,319,239);

	munmap((void *)gp2x_memregl,      0x10000);
	munmap(gp2x_video_YUV[0].p[0],    0x4B000);
	munmap(gp2x_video_YUV[0].p[1],    0x4B000);

	{ int i; for (i=0;i<8;i++) if (gp2x_dev[i])	close(gp2x_dev[i]);}

	fcloseall();

	//chdir("/usr/gp2x");
	//execl("gp2xmenu","gp2xmenu",NULL);
}
