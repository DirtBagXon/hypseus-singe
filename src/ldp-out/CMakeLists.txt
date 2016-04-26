set( LIB_SOURCES
    sony.cpp
    pioneer.cpp 
    ld-v6000.cpp
    hitachi.cpp
    philips.cpp
    ldp.cpp
    framemod.cpp
    ldp-combo.cpp
    ldp-vldp.cpp
    ldp-vldp-audio.cpp
)


set( LIB_HEADERS
    framemod.h
    hitachi.h
    ldp-combo.h
    ldp.h
    ldp-vldp.h
    ld-v6000.h
    philips.h
    pioneer.h
    sony.h
)

PKG_SEARCH_MODULE(VORBISFILE REQUIRED vorbisfile)
include_directories( ${VORBISFILE_INCLUDE_DIRS} )

add_library( ldp-out ${LIB_SOURCES} ${LIB_HEADERS} )
target_link_libraries( ldp-out vldp game ${VORBISFILE_LIBRARIES} )
