set( LIB_SOURCES
    video.cpp
    tms9128nl.cpp
    SDL_FontCache.c
    led.cpp
    palette.cpp
    splash.cpp
    rgb2yuv.cpp
    rgb2yuv-gas.s
)

set( LIB_HEADERS
    led.h
    palette.h
    rgb2yuv.h
    SDL_FontCache.h
    tms9128nl.h
    video.h
    splash.h
    icon.h
    yuv2rgb_lookup.h
)

add_library( video ${LIB_SOURCES} ${LIB_HEADERS} )
target_link_libraries( video plog )
