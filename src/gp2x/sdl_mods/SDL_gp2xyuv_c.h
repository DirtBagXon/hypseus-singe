/*
    SDL - Simple DirectMedia Layer - SDL_gp2xyuv extension
    Copyright (C) 2007 Matt Ownby

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* This is the GP2X implementation of YUV video overlays */

#include "SDL_video.h"
#include "SDL_gp2xvideo.h"

extern SDL_Overlay *GP2X_CreateYUVOverlay(_THIS, int width, int height, Uint32 format, SDL_Surface *display);

extern int GP2X_LockYUVOverlay(_THIS, SDL_Overlay *overlay);

extern void GP2X_UnlockYUVOverlay(_THIS, SDL_Overlay *overlay);

extern int GP2X_DisplayYUVOverlay(_THIS, SDL_Overlay *overlay, SDL_Rect *dstrect);

extern void GP2X_FreeYUVOverlay(_THIS, SDL_Overlay *overlay);

