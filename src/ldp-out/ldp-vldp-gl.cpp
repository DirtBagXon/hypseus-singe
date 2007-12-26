
#ifdef USE_OPENGL

#ifdef MAC_OSX
#include <glew.h>
#else
#include <GL/glew.h>
#endif

#include <string.h>	// for memcpy
#include "../vldp2/vldp/vldp.h"	// to get the vldp structs
#include "ldp-vldp.h"
#include "ldp-vldp-gl.h"
#include "ldp.h"
#include "../io/mpo_mem.h"
#include "../io/conout.h"
#include "../game/game.h"
#include "../daphne.h"
#include "../video/video.h"
#include "../video/blend.h"
#include "../video/palette.h"

#ifdef DEBUG
#include <assert.h>
#endif

enum
{
	TEX_Y,	// holds Y buffer
	TEX_U,	// U buffer
	TEX_V,	// V buffer
	NUM_YUV_TEXTURES,
};

enum
{
	TEX_VID_OVERLAY,	// video overlay
	TEX_VID_PALETTE,	// color palette look-up texture
	TEX_SCANLINES,	// scanlines texture (if enabled by user)
	NUM_TEXTURES	// this must be last!
};

// for speed.. I am in a hurry and don't want to mess w/ functions :(
extern Uint32 g_uRGBAPalette[];
extern unsigned int g_filter_type;

// for speed
extern unsigned g_uCPUMsBehind;

// for speed
extern bool g_take_screenshot;	// if true, a screenshot will be taken at the next available opportunity

// buffers to hold video frame texture
// There are two sets of each buffer to hold copies of the YUV buffers
GLuint g_textureYUVIDs[2][NUM_YUV_TEXTURES];

// buffers to hold non-YUV textures
GLuint g_textureIDs[NUM_TEXTURES];

// g_textureYUVIDs[g_uCurTextureArray] is the currently active texture array :)
unsigned int g_uCurTextureArray = 0;

// dimensions of YUV frame (in pixels)
unsigned int g_uTexWidth = 0, g_uTexHeight = 0;

// dimensions of output window (in pixels)
unsigned int g_uVidWidth = 0, g_uVidHeight = 0;

// the height of the video overlay (pre-calculate for speed)
// The video overlay's height often goes off the screen
unsigned int g_uOverlayHeight = 0;

// the same as g_game->get_video_visible_lines()
unsigned int g_uVisibleLines = 240;

bool g_bShaderInitialized = false;
bool g_bOpenGLInitialized = false;

GLuint g_FSHandle = 0, g_PHandle = 0;
GLuint g_hndF8Bit = 0, g_prgF8Bit = 0;

// how many frames have been prepared
unsigned int g_uFramePrepReq = 0;

// last frame index to be requested to be displayed, and last frame index that was displayed
// (index corresponds to g_uFramePrepReq)
unsigned int g_uFrameDispReq = 0;
unsigned int g_uFrameDispAck = 0;

// blank requested means the next frame we display should be blank, with overlay drawn on top
bool g_bBlankRequested = false;

// what gamevid was last time the think() function was called
SDL_Surface *g_gamevid_old = NULL;

// so we know when a new vblank has occurred
unsigned int g_uVblankCountOld = 0;

#define Y_IDX 0
#define U_IDX 1
#define V_IDX 2

// This number logically only needs to be 2 but performs much better as 4 :)
#define YUV_BUF_COUNT 4

unsigned char *g_ptr[YUV_BUF_COUNT][3];

// Macros to lock and unlock the mutex to make sure two threads aren't accessing shared vars at the same time
#define VLDP_GL_LOCK	SDL_mutexP(g_vldp_gl_mutex)
#define VLDP_GL_UNLOCK	SDL_mutexV(g_vldp_gl_mutex)

// mutex to protect shared data
SDL_mutex *g_vldp_gl_mutex = NULL;

// Thanks to Peter Bengtsson for the reference code to help me figure out
//  how to convert YUV to RGB using a fragment shader.
const GLchar *g_FProgram=
  "uniform sampler2D Ytex;\n"
  "uniform sampler2D Utex,Vtex;\n"
  "void main(void) {\n"
  "  float r,g,b,y,u,v;\n"
  "  y=texture2D(Ytex,gl_TexCoord[0].st).r;\n"
  "  u=texture2D(Utex,gl_TexCoord[0].st).r;\n"
  "  v=texture2D(Vtex,gl_TexCoord[0].st).r;\n"

  "  y=1.1643*(y-0.0625);\n"
  "  u=u-0.5;\n"
  "  v=v-0.5;\n"

  "  r=y+1.5958*v;\n"
  "  g=y-0.39173*u-0.81290*v;\n"
  "  b=y+2.017*u;\n"

  "  gl_FragColor=vec4(r,g,b,1.0);\n"
  "}\n";

// 8-bit texture fragment program (by Matt Ownby, my first fragment shader! woohoo!)
const GLchar *g_F8Bit =
  "uniform sampler1D smpColorPalette;\n"
  "uniform sampler2D smpPixels;\n"
  "void main(void) {\n"
  "  float idx;\n"
  // idx gets a value between 0.0 and 1.0 which corresponds to 0-255 on our color palette
  "  idx=texture2D(smpPixels, gl_TexCoord[0].st).r;\n"
  // 255/256 (0.99609375) converts 0.0-1.0 to the left border of the color that we want in the palette texture
  // Adding 1/512 (0.001953125) centers us inside the color that we want so that there can be no ambiguity
  "  gl_FragColor=texture1D(smpColorPalette, (idx * 0.99609375) + 0.001953125);\n"
  "}\n";

void ldp_vldp_gl_blend_y(Uint8 *g_ptrY)
{
	// NOTE : this function is unoptimized, because it is low-priority.
	// I just coded it up quickly to make sure it is supported to save myself any questions
	//  from people who are wondering why it's not working.

	g_blend_line1 = g_ptrY;
	g_blend_line2 = g_ptrY + g_uTexWidth;
	g_blend_dest = MPO_MALLOC(g_uTexWidth);
	g_blend_iterations = g_uTexWidth;

	if (g_blend_dest)
	{
		// two rows at a time ...
		for (unsigned int uRow = 0; uRow < g_uTexHeight; uRow += 2)
		{
			g_blend_func();

			// overwrite two source lines with blended line
			memcpy(g_blend_line1, g_blend_dest, g_uTexWidth);
			memcpy(g_blend_line2, g_blend_dest, g_uTexWidth);

			g_blend_line1 += (g_uTexWidth << 1);
			g_blend_line2 += (g_uTexWidth << 1);
		}
		MPO_FREE(g_blend_dest);
	}
}

void draw_prepared_frame(SDL_Surface *gamevid)
{
	int w_half = g_uVidWidth >> 1, h_half = g_uVidHeight >> 1;

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// if this isn't supposed to be a blank frame
	if (!g_bBlankRequested)
	{
		glUseProgram(g_PHandle);	// start using the program

		// ASSUMPTION: YUV textures have already been prepared

		// bind the YUV textures for the current frame
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D,g_textureYUVIDs[g_uCurTextureArray][TEX_U]);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D,g_textureYUVIDs[g_uCurTextureArray][TEX_V]);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D,g_textureYUVIDs[g_uCurTextureArray][TEX_Y]);

		// draw the textured rectangle
		glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex3i(-w_half, -h_half, 0);
		glTexCoord2f(1, 1); glVertex3i(w_half, -h_half, 0);
		glTexCoord2f(1, 0); glVertex3i(w_half, h_half, 0);
		glTexCoord2f(0, 0); glVertex3i(-w_half, h_half, 0);
		glEnd();

		glUseProgram(0);	// stop using fragment shader
	} // end if it's not supposed to be a blank frame
	// else the frame is supposed to be blank, so we don't draw the VLDP frame
	else
	{
		// we've done our duty and blanked...
		g_bBlankRequested = false;
	}

	// STEP 2: Display 8-bit video overlay (if it exists)

	// if there is game video that needs to be displayed
	if (gamevid)
	{
		g_uOverlayHeight = gamevid->h;

		// start using 8-bit texture fragment program
		glUseProgram(g_prgF8Bit);

		// bind color palette to texture unit 1
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_1D, g_textureIDs[TEX_VID_PALETTE]);
		// The palette texture has already been copied over in on_palette_change_gl()

		// update 8-bit pixels to texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, g_textureIDs[TEX_VID_OVERLAY]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, gamevid->w, gamevid->h,
			0, GL_LUMINANCE, GL_UNSIGNED_BYTE, gamevid->pixels);

		glPushMatrix();

		// take vertical offset into account

		// The bottom of the rectangle must be the bottom of the overlay,
		//  relative to the size of the mpeg video.
		// (many overlays are too big vertically to fit on the screen)

		// We have to rescale the vertical coordinates in case g_uVisibleLines
		//  is not the same as the screen's height.
		GLfloat fScaleFactor = (GLfloat) g_uVidHeight / g_uVisibleLines;
		glScalef(1.0, fScaleFactor, 1.0);

		// We have to do the translation after we do the scale, because
		//  the video row offset must be scaled too.
		glTranslatef(0, (GLfloat) -g_game->get_video_row_offset(), 0);

		// iY is initialized as the bottom row
		GLint iY = ((g_uVisibleLines / 2) - g_uOverlayHeight);

		// draw the textured rectangle
		glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex3i(-w_half, iY, 1);	// bottom left
		glTexCoord2f(1, 1); glVertex3i(w_half, iY, 1);	// bottom right

		iY += g_uOverlayHeight;

		glTexCoord2f(1, 0); glVertex3i(w_half, iY, 1);	// top right
		glTexCoord2f(0, 0); glVertex3i(-w_half, iY, 1);	// top left
		glEnd();
		glPopMatrix();

		// stop using fragment program
		glUseProgram(0);

#if 0
		glDisable(GL_TEXTURE_2D);

		glPushMatrix();
		fScaleFactor = (GLfloat) g_uVidHeight / g_uVisibleLines;
		glScalef(1.0, fScaleFactor, 1.0);
		glTranslatef(0, (GLfloat) -g_game->get_video_row_offset(), 0);
		iY = ((g_uVisibleLines / 2) - g_uOverlayHeight);

		glColor4f(1.0, 1.0, 1.0, 0.35f);	// draw border so we can see where overlay's border is
		glBegin(GL_QUADS);
		glVertex3i(-w_half, iY, 3);	// bottom left
		glVertex3i(w_half, iY, 3);	// bottom right

		iY += g_uOverlayHeight;

		glVertex3i(w_half, iY, 3);	// top right
		glVertex3i(-w_half, iY, 3);	// top left
		glEnd();
		glPopMatrix();

		glColor4f(1.0, 0.0, 0.0, 0.25);
		glBegin(GL_QUADS);
		glVertex3i(-w_half, -h_half, 4);	// bottom left
		glVertex3i(w_half, -h_half, 4);	// bottom right
		glVertex3i(w_half, h_half, 4);	// top right
		glVertex3i(-w_half, h_half, 4);	// top left
		glEnd();
		glEnable(GL_TEXTURE_2D);
#endif

	} // end if game video exists

	// if scanlines are enabled
	if (g_filter_type & FILTER_SCANLINES)
	{
		glBindTexture(GL_TEXTURE_2D, g_textureIDs[TEX_SCANLINES]);
		glBegin(GL_QUADS);
		glTexCoord2i(0, g_uVidHeight >> 1);	glVertex3i(-w_half, -h_half, 2);
		glTexCoord2i(1, g_uVidHeight >> 1); glVertex3i(w_half, -h_half, 2);
		glTexCoord2i(1, 0); glVertex3i(w_half, h_half, 2);
		glTexCoord2i(0, 0); glVertex3i(-w_half, h_half, 2);
		glEnd();
	}
}

void ldp_vldp_gl_think(unsigned int uVblankCount)
{
	// this is used by multiple branches so we define it here
	SDL_Surface *gamevid = g_game->get_finished_video_overlay();
	bool bOkToDraw = false;

	VLDP_GL_LOCK;

	// if we have at least 1 new frame requested to be displayed
	if (g_uFrameDispReq > g_uFrameDispAck)
	{
#ifdef DEBUG
		assert(g_ldp->is_blitting_allowed() == false);
#endif

		// STEP 1: copy frame into video card texture memory

		// Use the other texture array so that we can buffer our next YUV image and
		//  still allow access to the current one in case the video overlay changes before
		//  VLDP is ready to display the next YUV image
		unsigned int uNextTextureArray = g_uCurTextureArray ^ 1;

		// acknowledge that we've drawn (or will draw very soon) this new frame
		++g_uFrameDispAck;

		// this allows us to display frames that are slightly behind but buffered
		unsigned int uIdx = g_uFrameDispAck % YUV_BUF_COUNT;

		// lock while we access g_ptr* and g_bFrameQueued
		// copy YUV frame into openGL texture buffer
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D,g_textureYUVIDs[uNextTextureArray][TEX_U]);
		glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE, g_uTexWidth >> 1, g_uTexHeight >> 1,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,
			g_ptr[uIdx][U_IDX]);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D,g_textureYUVIDs[uNextTextureArray][TEX_V]);
		glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE,g_uTexWidth >> 1,g_uTexHeight >> 1,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,
			g_ptr[uIdx][V_IDX]);

		// if blending is requested, then blend the Y plane now before sending it to the texture
		if (g_filter_type & FILTER_BLEND)
		{
			ldp_vldp_gl_blend_y(g_ptr[uIdx][Y_IDX]);
		}

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D,g_textureYUVIDs[uNextTextureArray][TEX_Y]);
		glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE,g_uTexWidth,g_uTexHeight,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,
			g_ptr[uIdx][Y_IDX]);

		g_uCurTextureArray ^= 1;	// flip texture array so we're using our new VLDP frame when we draw

		bOkToDraw = true;	// it's ok to draw the frame now ...
	}

	// if our ROM-generated video overlay has changed
	// (THIS SHOULD NOT BE AN 'ELSE IF' BECAUSE WE WANT TO UPDATE BOTH AT THE SAME FRAME IF NEEDED DUE TO VSYNC DELAY)
	if (gamevid != g_gamevid_old)
	{
		g_gamevid_old = gamevid;
		bOkToDraw = true;
	}
	// else do nothing ...

	VLDP_GL_UNLOCK;

	// If it's time to draw a new frame and if we have a new frame to draw
	// (We don't want to draw every time bOkToDraw is true because if the laserdisc video
	//  changes at 30 FPS and the overlay changes at 60 FPS, we will have updates at 90 FPS
	//  which is too fast for most monitor refresh rates, although it does work fine with
	//  vsync disabled)
	if ((uVblankCount != g_uVblankCountOld) && (bOkToDraw))
	{
		// if we're not too far behind, then display the frame
		// I chose 33 because it is approximately 2 frames on a 60 Hz monitor, and 1 NTSC frame
		if (g_uCPUMsBehind < 33)
		{
			// draw it! (this should go really fast since the textures are already set up)
			draw_prepared_frame(gamevid);
			SDL_GL_SwapBuffers();

			// if we've been instructed to take a screenshot, do so now that the image has been displayed
			if (g_take_screenshot)
			{
				g_take_screenshot = false;
				take_screenshot_GL();
			}
		}
		// else we're too far behind and SDL_GL_SwapBuffers can take a long time if vsync is enabled,
		//  so drop this frame to try to catch up.

		// TODO : maybe we should show every 4th frame even if we are dropping just to avoid
		//  having 'dead' air

		g_uVblankCountOld = uVblankCount;
	}
}

extern unsigned int g_draw_width, g_draw_height;

bool check_shader(GLuint uFHandle, GLuint &uPHandle)
{
	char s[32768];
	bool bResult = false;

	GLint i = 0;
	GLsizei iCharsWritten = 0;

	// see if compile succeeded
	glGetShaderiv(uFHandle,GL_COMPILE_STATUS,&i);
	if (i == GL_TRUE)
	{
		// create the program
		uPHandle = glCreateProgram();

		/* Create a complete program object. */
		glAttachShader(uPHandle,uFHandle);
		glLinkProgram(uPHandle);

		// see if link succeeded
		glGetProgramiv(uPHandle, GL_LINK_STATUS, &i);

		// if link succeeded
		if (i == GL_TRUE)
		{
			bResult = true;
		}
		else
		{
			glGetProgramiv(uPHandle, GL_INFO_LOG_LENGTH, &i);
			glGetProgramInfoLog(uPHandle,i,&iCharsWritten,s);
			outstr("OpenGL Shader Link Failed: ");
			printline(s);
		}
	}
	// else compile failed
	else
	{
		glGetShaderiv(uFHandle, GL_INFO_LOG_LENGTH, &i);
		glGetShaderInfoLog(uFHandle,i,&iCharsWritten,s);
		outstr("OpenGL Shader Compile Failed: ");
		printline(s);
	}

	return bResult;
}

bool init_shader()
{
	bool bResult = false;

	glewInit();
	if (glewIsSupported("GL_VERSION_2_0"))
	{
		printline("OpenGL v2.0 is supported.");

		/* Set up program objects. */
		g_FSHandle=glCreateShader(GL_FRAGMENT_SHADER);
		g_hndF8Bit=glCreateShader(GL_FRAGMENT_SHADER);

		/* Compile the shader. */
		glShaderSource(g_FSHandle,1,&g_FProgram,NULL);
		glCompileShader(g_FSHandle);

		glShaderSource(g_hndF8Bit, 1, &g_F8Bit, NULL);
		glCompileShader(g_hndF8Bit);

		// check to see if compilation succeeded
		if (check_shader(g_FSHandle, g_PHandle))
		{
			if (check_shader(g_hndF8Bit, g_prgF8Bit))
			{
				// setup our shader variables
				int i = 0;

				// setup variables for YUV->RGB program
				glUseProgram(g_PHandle);
				i=glGetUniformLocation(g_PHandle,"Utex");
				glUniform1i(i,1); 
				i=glGetUniformLocation(g_PHandle,"Vtex");
				glUniform1i(i,2);     
				i=glGetUniformLocation(g_PHandle,"Ytex");
				glUniform1i(i,0);

				// setup variables for 8-bit texture program
				glUseProgram(g_prgF8Bit);
				i=glGetUniformLocation(g_prgF8Bit,"smpColorPalette");
				glUniform1i(i,1); 
				// pixels on texture unit 0
				i=glGetUniformLocation(g_prgF8Bit,"smpPixels");
				glUniform1i(i,0); 
				glUseProgram(0);

				bResult = true;
			}
		}
	}
	else
	{
		printline("OpenGL v2.0 is required for VLDP OpenGL mode.");
	}

	return bResult;
}

bool init_vldp_opengl()
{
	bool bResult = false;
	int i = 0;

	// if we've never initialized
	if (!g_bOpenGLInitialized)
	{
		g_uVidWidth = g_draw_width;
		g_uVidHeight = g_draw_height;

		// find out how many visible lines the video overlay supports
		g_uVisibleLines = g_game->get_video_visible_lines();

		glGenTextures(NUM_TEXTURES, g_textureIDs);	// generate texture buffers
		glGenTextures(NUM_YUV_TEXTURES, g_textureYUVIDs[0]);	// " " "
		glGenTextures(NUM_YUV_TEXTURES, g_textureYUVIDs[1]);	// " " "

		// setup texture parameters
		for (i = 0; i < NUM_TEXTURES; i++)
		{
			GLuint uType = GL_LINEAR;
			GLuint uParam = GL_CLAMP_TO_EDGE;	// no reason to ever use GL_CLAMP based on what I've now learned :)
			GLenum uTarget = GL_TEXTURE_2D;

			switch (i)
			{
			// the color palette texture is 1-Dimensional
			// We _must_ use GL_NEAREST for TEX_VID_PALETTE because it is a lookup table.
			case TEX_VID_PALETTE:
				uTarget = GL_TEXTURE_1D;
				uType = GL_NEAREST;
				break;
			// the scanlines texture must repeat due to its design
			case TEX_SCANLINES:
				uParam = GL_REPEAT;
				break;
			// IMPORTANT: For TEX_VID_OVERLAY, we _must_ use GL_NEAREST for our shader to work properly.
			case TEX_VID_OVERLAY:
				uType = GL_NEAREST;
				break;
			}

			glBindTexture(uTarget, g_textureIDs[i]);
			glTexParameteri(uTarget, GL_TEXTURE_WRAP_S, uParam);
			glTexParameteri(uTarget, GL_TEXTURE_WRAP_T, uParam);
			glTexParameteri(uTarget, GL_TEXTURE_MAG_FILTER, uType);
			glTexParameteri(uTarget, GL_TEXTURE_MIN_FILTER, uType);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		}

		// setup YUV texture parameters
		for (i = 0; i < NUM_YUV_TEXTURES; i++)
		{
			GLenum uTarget = GL_TEXTURE_2D;
			GLuint uParam = GL_CLAMP_TO_EDGE;	// this solves the 'green border' problem (GL_CLAMP was causing it)
			GLuint uType = GL_LINEAR;

			for (int j = 0; j < 2; ++j)
			{
				glBindTexture(uTarget, g_textureYUVIDs[j][i]);
				glTexParameteri(uTarget, GL_TEXTURE_WRAP_S, uParam);
				glTexParameteri(uTarget, GL_TEXTURE_WRAP_T, uParam);
				glTexParameteri(uTarget, GL_TEXTURE_MAG_FILTER, uType);
				glTexParameteri(uTarget, GL_TEXTURE_MIN_FILTER, uType);
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			}
		}

		// NOW CREATE SCANLINES TEXTURE (it is so small it's ok to create it even if we don't use it)
		glBindTexture(GL_TEXTURE_2D, g_textureIDs[TEX_SCANLINES]);

		Uint32 uScanLineTex[4] =
		{
			0xFF000000, 0xFF000000,	// first line (black)
				0, 0	// second line (totally transparent)
		};

		// apply the texture
		glTexImage2D(GL_TEXTURE_2D,
			0,	// level is 0
			GL_RGBA,
			2,	// 2 wide
			2,	// 2 tall
			0,	// border is 0
			GL_RGBA, GL_UNSIGNED_BYTE, uScanLineTex);

		// if shaders don't init properly, then shutdown
		if (init_shader())
		{
			// we need a mutex to handle thread syncing
			g_vldp_gl_mutex = SDL_CreateMutex();
			
			// if mutex creates successfully
			if (g_vldp_gl_mutex)
			{
				// IMPORTANT: this must be set to true before on_palette_change_gl is called!!!
				g_bOpenGLInitialized = true;

				// This is important for us to do here, because the palette may have been finalized
				//  before we initialized VLDP GL, and it may never change hereafter.  Therefore,
				//  we need to make sure that it is set at least once.
				on_palette_change_gl();

				bResult = true;
			}
			else
			{
				printline("g_vldp_gl_mutex could not be created");
			}
		}
	}
	else bResult = true;

	return bResult;
}

void free_gl_resources()
{
	// if we successfully created a mutex previously, then destroy it now
	if (g_vldp_gl_mutex)
	{
		SDL_DestroyMutex(g_vldp_gl_mutex);
		g_vldp_gl_mutex = NULL;
	}

	for (unsigned int u = 0; u < YUV_BUF_COUNT; ++u)
	{
		MPO_FREE(g_ptr[u][Y_IDX]);
		MPO_FREE(g_ptr[u][U_IDX]);
		MPO_FREE(g_ptr[u][V_IDX]);
	}
}

// gets called when the color palette changes
void on_palette_change_gl()
{
	// this function may get called before we've initialized our GL stuff, in which
	//  case, we will have to ignore it.
	if (g_bOpenGLInitialized)
	{
		// These shouldn't be necessary to do here, but I've left them here, commented out,
		//  in case problems are discovered on other hardware and they are needed.
//		glActiveTexture(GL_TEXTURE1);
//		glBindTexture(GL_TEXTURE_1D, g_textureIDs[TEX_VID_PALETTE]);

		// apply modified palette to palette texture
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE,
			g_uRGBAPalette);
	}
	// else we're not initialized, so there's nothing we can do ...
}

//////////////////////////////
// threaded stuff here

// opengl version of the callback
int prepare_frame_GL_callback(struct yuv_buf *src)
{
	int result = VLDP_TRUE;

	VLDP_GL_LOCK;

	/*
	// used to test super don overflow problem, can probably be removed once we're
	//  sure the problem is really gone :)
	if (*((unsigned int *) src->Y) == 0xbbbec1c2)
	{
		int i = 0;
	}
	*/

	// NOTE : we have multiple YUV buffers because, due to threading, it's possible
	//  for these callbacks to get called many times before the gl_think function
	//  is called.
	// One scenario: prepare callback, display callback, followed by another prepare
	//  callback.  Now we have 2 frames prepared, 1 frame requested to be displayed,
	//  and our gl_think hasn't even run yet!  We need a way to keep track of all
	//  of this stuff without throwing anything away (hopefully).

	// if we're out of buffer room, we'll have to kill an old frame and
	//  not display it
	while ((g_uFrameDispReq - g_uFrameDispAck) >= YUV_BUF_COUNT)
	{
		// this effectively will ensure that the frame is dropped
		// (this should never happen but it's here as a safety)
		++g_uFrameDispAck;
	}

	// If a prepared frame hasn't been displayed, then we should overwrite it instead of keeping it.
	g_uFramePrepReq = g_uFrameDispReq + 1;

	// this handles auto-wraparound for us nicely
	unsigned int uIdx = g_uFramePrepReq % YUV_BUF_COUNT;

	// store decoded frame for displaying later
	memcpy(g_ptr[uIdx][Y_IDX], src->Y, src->Y_size);
	memcpy(g_ptr[uIdx][U_IDX], src->U, src->UV_size);
	memcpy(g_ptr[uIdx][V_IDX], src->V, src->UV_size);

	VLDP_GL_UNLOCK;

	return result;
}

void display_frame_GL_callback(struct yuv_buf *buf)
{
	VLDP_GL_LOCK;

	// set the request display frame to the last prepared frame
	g_uFrameDispReq = g_uFramePrepReq;

	VLDP_GL_UNLOCK;
}

void report_mpeg_dimensions_GL_callback(int width, int height)
{
	VLDP_GL_LOCK;

	// blitting is not allowed once we create the YUV overlay ...
	g_ldp->set_blitting_allowed(false);

	if (((unsigned) width != g_uTexWidth) || ((unsigned) height != g_uTexHeight))
	{
		for (unsigned int u = 0; u < YUV_BUF_COUNT; ++u)
		{
			MPO_FREE(g_ptr[u][Y_IDX]);
			MPO_FREE(g_ptr[u][U_IDX]);
			MPO_FREE(g_ptr[u][V_IDX]);
			g_ptr[u][Y_IDX] = MPO_MALLOC(width * height);
			g_ptr[u][U_IDX] = MPO_MALLOC((width * height) >> 2);
			g_ptr[u][V_IDX] = MPO_MALLOC((width * height) >> 2);
		}
	}

	g_uTexWidth = width;
	g_uTexHeight = height;

	VLDP_GL_UNLOCK;
}

void render_blank_frame_GL_callback()
{
	VLDP_GL_LOCK;
	g_bBlankRequested = true;

	// force a frame update to ensure blanking happens
	// (we don't need to populate the YUV buffer because the frame will be blank)
	++g_uFramePrepReq;
	++g_uFrameDispReq;

	VLDP_GL_UNLOCK;
}

#endif // USE_OPENGL
