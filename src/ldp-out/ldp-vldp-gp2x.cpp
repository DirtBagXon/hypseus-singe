#ifdef GP2X

#include "../vldp2/940/interface_920.h"
#include "../vldp2/vldp/vldp.h"
#include "../io/conout.h"
#include "../timer/timer.h"

extern SDL_Overlay *g_hw_overlay;
extern SDL_Rect *g_screen_clip_rect;

struct sdl_gp2x_yuvhwdata
{
	// the first few fields in this struct
	Uint8 *YUVBuf[2];
	unsigned int GP2XAddr[2];
	unsigned int uBackBuf;
};

int prepare_gp2x_frame_callback(struct yuy2_buf *src)
{
	VLDP_BOOL result = VLDP_TRUE;

	if (SDL_LockYUVOverlay(g_hw_overlay) == 0)
	{
		struct sdl_gp2x_yuvhwdata *pData = (struct sdl_gp2x_yuvhwdata *) g_hw_overlay->hwdata;

		// NOTE : the method of redirecting SDL's buffer is fast but doesn't quite work because vldp is designed
		//  to globber the buffer as soon as this function returns, and we need the buffer intact as long as the image
		//  is displayed.  So we really do need to make a copy (or redesign vldp which is difficult when it has to go through
		//  the process of dropping extra frames when doing a new search or skip and we need to keep the image displayed.)
//		pData->GP2XAddr[pData->uBackBuf] = src->uPhysAddress;	// redirect sdl's buffer to the buffer that already has the 
		
		// use hardware blitter to copy image
		mp2_920_start_fast_yuy2_blit(pData->GP2XAddr[pData->uBackBuf]);

		SDL_UnlockYUVOverlay(g_hw_overlay);
	}
	else
	{
		printline("gp2x yuy2 callback: Can't lock surface!");
	}

	return result;
}

void display_gp2x_frame_callback()
{
	// wait for blitter to finish the copy (hopefully we will never wait at this point)
	while (mp2_920_is_blitter_busy())
	{
		MAKE_DELAY(0);
	}
	SDL_DisplayYUVOverlay(g_hw_overlay, g_screen_clip_rect);
}

#endif // GP2X
