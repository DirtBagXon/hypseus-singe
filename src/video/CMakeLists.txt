set( LIB_SOURCES
    video.cpp
    tms9128nl.cpp
    SDL_FontCache.c
    led.cpp
    palette.cpp
    rgb2yuv.cpp
    blend.cpp
)

set( LIB_HEADERS
    blend.h
    led.h
    palette.h
    rgb2yuv.h
    SDL_FontCache.h
    tms9128nl.h
    video.h
    yuv2rgb_lookup.h
)

if (TARGET_I386)
    list( APPEND LIB_SOURCES rgb2yuv-gas.s blend_mmx-gas.s )
endif (TARGET_I386)

add_library( video ${LIB_SOURCES} ${LIB_HEADERS} )
