set( LIB_SOURCES
    ldp.cpp
    framemod.cpp
    ldp-vldp.cpp
    ldp-vldp-audio.cpp
)


set( LIB_HEADERS
    framemod.h
    ldp.h
    ldp-vldp.h
)

IF(NOT APPLE)
    PKG_SEARCH_MODULE(VORBISFILE REQUIRED vorbisfile)
ENDIF(NOT APPLE)

find_package(Vorbis REQUIRED)
find_package(Ogg REQUIRED)

include_directories( ${Vorbis_File_INCLUDE_DIR} ${OGG_INCLUDE_DIRS})

add_library( ldp-out ${LIB_SOURCES} ${LIB_HEADERS} )
target_link_libraries( ldp-out vldp game ${VORBISFILE_LIBRARIES} ${SDL2_LIBRARY} )
