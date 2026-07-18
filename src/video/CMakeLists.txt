set( LIB_SOURCES
    video.cpp
    tms9128nl.cpp
    led.cpp
    palette.cpp
    splash.cpp
)

set( LIB_HEADERS
    led.h
    palette.h
    tms9128nl.h
    video.h
    splash.h
    icon.h
)

add_library( video ${LIB_SOURCES} ${LIB_HEADERS} )
target_link_libraries( video PRIVATE plog sdl3_deps )
