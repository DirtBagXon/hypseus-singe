set( LIB_SOURCES
    cmdline.cpp conout.cpp error.cpp fileparse.cpp homedir.cpp input.cpp
    mpo_fileio.cpp
    parallel.cpp
    network.cpp numstr.cpp sram.cpp unzip.cpp
    
)

set( LIB_HEADERS
    cmdline.h conout.h error.h fileparse.h homedir.h input.h
    mpo_fileio.h
    mpo_mem.h my_stdio.h
    network.h numstr.h parallel.h sram.h unzip.h
)

find_package(ZLIB REQUIRED)
include_directories( ${ZLIB_INCLUDE_DIRS} )

add_library( io ${LIB_SOURCES} ${LIB_HEADERS} )
target_link_libraries( io ldp-out game plog ${ZLIB_LIBRARIES} )
if( WIN32 )
    target_link_libraries( io ws2_32 )
endif( WIN32 )
