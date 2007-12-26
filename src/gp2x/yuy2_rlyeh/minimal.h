/* GP2X minimal library v0.B
   Written by rlyeh, (c) 2005.

   Please check readme for licensing and other conditions. */

#ifndef __MINIMAL_H__
#define __MINIMAL_H__

#include <math.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>

#define MINILIB_VERSION  "GP2X minimal library v0.B by rlyeh, (c) 2005."

enum  { GP2X_UP=0x1,         GP2X_LEFT=0x4,         GP2X_DOWN=0x10,    GP2X_RIGHT=0x40,
        GP2X_START=(1<<8),   GP2X_SELECT=1<<9,      GP2X_L=(1<<10),    GP2X_R=(1<<11),
        GP2X_A=1<<12,        GP2X_B=1<<13,          GP2X_X=1<<14,      GP2X_Y=1<<15,
        GP2X_VOL_UP=1<<23,   GP2X_VOL_DOWN=(1<<22), GP2X_PUSH=1<<27 };

typedef struct gp2x_font        { int x,y,w,wmask,h,fg,bg,solid; unsigned char *data; } gp2x_font;
typedef struct gp2x_queue       { volatile unsigned long head, tail, items, max_items; unsigned long *place920t, *place940t; } gp2x_queue;
typedef struct gp2x_rect        { int x,y,w,h,solid; void *data; } gp2x_rect;

typedef struct gp2x_video_layer { unsigned char  *screen8;
                                  unsigned short *screen16;
                                  unsigned long  *screen32;

                                  unsigned long i[8];  //physical addresses of each buffer. they might have other usages too
									unsigned int uWhichBuf;	// 0 = buffer #1, 1 = buffer #2, only two buffers (double buffered)
                                  void *p[8];          //virtual address of each buffer
                                } gp2x_video_layer;


extern gp2x_video_layer         gp2x_video_RGB[1], gp2x_video_YUV[4];
extern volatile unsigned long  *gp2x_dualcore_ram;


extern void gp2x_video_setgammacorrection(float);
extern void gp2x_video_setdithering(int);
extern void gp2x_video_setluminance(int, int);
extern void gp2x_video_waitvsync(void);
extern void gp2x_video_waithsync(void);
extern void gp2x_video_cursor_show(int);
extern void gp2x_video_cursor_move(int, int);
extern void gp2x_video_cursor_setalpha(int, int);
extern void gp2x_video_cursor_setup(unsigned char *, int, unsigned char, int, int, int, int, unsigned char, int, int, int, int);
extern void gp2x_video_logo_enable(int);

extern void           gp2x_video_RGB_color8 (int, int, int, int);
extern unsigned short gp2x_video_RGB_color15(int, int, int, int);
extern unsigned short gp2x_video_RGB_color16(int, int, int);
extern void           gp2x_video_RGB_setpalette(void);
extern void           gp2x_video_RGB_setcolorkey(int, int, int);
extern void           gp2x_video_RGB_setscaling(int, int);
extern void           gp2x_video_RGB_flip(int);
extern void           gp2x_video_RGB_setwindows(int, int, int, int, int, int);

extern unsigned long  gp2x_video_YUV_color(int, int, int);
extern void           gp2x_video_YUV_setscaling(int, int, int);
extern void           gp2x_video_YUV_flip(int);
extern void           gp2x_video_YUV_setparts(int, int, int, int, int, int);

extern void         (*gp2x_blitter_rect)(gp2x_rect *);

extern unsigned long  gp2x_joystick_read(void);

extern unsigned long  gp2x_timer_read(void);
extern void           gp2x_timer_delay(unsigned long);

extern void           gp2x_sound_frame (void *, void *, int);
extern void           gp2x_sound_volume(int, int);
extern void           gp2x_sound_pause(int);

extern void           gp2x_dualcore_pause(int);
extern void           gp2x_dualcore_sync(void);
extern void           gp2x_dualcore_exec(unsigned long);
extern void           gp2x_dualcore_launch_program(unsigned long *, unsigned long);
extern void           gp2x_dualcore_launch_program_from_disk(const char *, unsigned long, unsigned long);

extern void           gp2x_printf(gp2x_font *, int, int, const char *, ...);
extern void           gp2x_printf_init(gp2x_font *, int, int, void *, int, int, int);

extern void           gp2x_init(int, int, int, int, int, int, int);
extern void           gp2x_deinit(void);





#define GP2X_QUEUE_MAX_ITEMS           1000
#define GP2X_QUEUE_ARRAY_PTR           ((0x1000-(sizeof(gp2x_queue)<<2)))
#define GP2X_QUEUE_DATA_PTR            (GP2X_QUEUE_ARRAY_PTR-(GP2X_QUEUE_MAX_ITEMS<<2))

#define gp2x_2ndcore_code(v)           (*(volatile unsigned long *)(v))
#define gp2x_1stcore_code(v)           gp2x_dualcore_ram[(v)>>2]
#define gp2x_2ndcore_data(v)           gp2x_2ndcore_code((v)+0x100000)
#define gp2x_1stcore_data(v)           gp2x_1stcore_code((v)+0x100000)

#define gp2x_2ndcore_code_ptr(v)       ((volatile unsigned long *)(v))
#define gp2x_1stcore_code_ptr(v)       (&gp2x_dualcore_ram[(v)>>2])
#define gp2x_2ndcore_data_ptr(v)       gp2x_2ndcore_code_ptr((v)+0x100000)
#define gp2x_1stcore_data_ptr(v)       gp2x_1stcore_code_ptr((v)+0x100000)

#define gp2x_dualcore_data(v)          gp2x_1stcore_data(v)

#define gp2x_dualcore_declare_subprogram(name) extern void gp2x_dualcore_launch_## name ##_subprogram(void);
#define gp2x_dualcore_launch_subprogram(name)  gp2x_dualcore_launch_## name ##_subprogram()

#endif

