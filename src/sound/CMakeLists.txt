set( LIB_SOURCES
    sound.cpp
    ssi263.cpp
    tqsynth.cpp
    sn_intf.cpp
    tms9919-sdl.cpp
    tms9919.cpp
    pc_beeper.cpp
    gisound.cpp
    dac.cpp
    tonegen.cpp
    samples.cpp
    mix.cpp
    mix_mmx-gas.s
)

set( LIB_HEADERS
    dac.h
    gisound.h
    mix.h
    pc_beeper.h
    samples.h
    sn_intf.h
    sound.h
    ssi263.h
    tonegen.h
    tqsynth.h
)

add_library( sound ${LIB_SOURCES} ${LIB_HEADERS} )
target_link_libraries( sound plog )
