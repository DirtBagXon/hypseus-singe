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

if (TARGET_I386)
    list( APPEND LIB_SOURCES mix_mmx-gas.s )
endif (TARGET_I386)

add_library( sound ${LIB_SOURCES} ${LIB_HEADERS} )
