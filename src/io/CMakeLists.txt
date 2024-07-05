set( LIB_SOURCES
    cmdline.cpp conout.cpp error.cpp fileparse.cpp homedir.cpp input.cpp
    toolbox.cpp mpo_fileio.cpp parallel.cpp keycodes.cpp serialib.cpp
    network.cpp numstr.cpp sram.cpp zippp.cpp unzip.cpp
    
)

set( LIB_HEADERS
    cmdline.h conout.h error.h fileparse.h homedir.h input.h
    mpo_fileio.h mpo_mem.h my_stdio.h keycodes.h serialib.h
    network.h numstr.h parallel.h sram.h zippp.h unzip.h
    limits.h
)

find_package(ZLIB REQUIRED)

find_path(LIBZIP_INCLUDE_DIR NAMES zip.h)
mark_as_advanced(LIBZIP_INCLUDE_DIR)

find_library(LIBZIP_LIBRARY NAMES zip)
mark_as_advanced(LIBZIP_LIBRARY)

get_filename_component(_libzip_libdir ${LIBZIP_LIBRARY} DIRECTORY)
find_file(_libzip_pkgcfg libzip.pc
    HINTS ${_libzip_libdir} ${LIBZIP_INCLUDE_DIR}/..
    PATH_SUFFIXES pkgconfig lib/pkgconfig libdata/pkgconfig
    NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    LIBZIP
    REQUIRED_VARS
        LIBZIP_LIBRARY
        LIBZIP_INCLUDE_DIR
        _libzip_pkgcfg
)

if (LIBZIP_FOUND)
    if (NOT TARGET libzip::zip)
        add_library(libzip::zip UNKNOWN IMPORTED)
        set_target_properties(libzip::zip
            PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES ${LIBZIP_INCLUDE_DIR}
                INTERFACE_LINK_LIBRARIES ZLIB::ZLIB
                IMPORTED_LOCATION ${LIBZIP_LIBRARY}
        )
    endif()
endif()

include_directories( ${ZLIB_INCLUDE_DIRS} )

set_source_files_properties( zippp.cpp PROPERTIES COMPILE_FLAGS -Wno-deprecated-declarations )

add_library( io ${LIB_SOURCES} ${LIB_HEADERS} )
target_link_libraries( io ldp-out game plog manymouse ${ZLIB_LIBRARIES} ${LIBZIP_LIBRARY})
if( WIN32 )
    target_link_libraries( io ws2_32 )
endif( WIN32 )
