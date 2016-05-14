set( LIB_SOURCES
    video.cpp
    tms9128nl.cpp
    SDL_FontCache.c
    led.cpp
    palette.cpp
    rgb2yuv.cpp
    blend.cpp
    rgb2yuv-gas.s
    blend_mmx-gas.s
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

set_source_files_properties(SDL_FontCache.c PROPERTIES COMPILE_FLAGS -Wno-maybe-uninitialized)

add_library( video ${LIB_SOURCES} ${LIB_HEADERS} )
target_link_libraries( video plog )
