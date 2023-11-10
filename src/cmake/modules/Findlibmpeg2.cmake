# - Try to find the MPEG2 library
# Once done, this will define
#
# MPEG2_FOUND - system has MPEG2
# MPEG2_INCLUDE_DIRS - the MPEG2 include directories
# MPEG2_LIBRARIES - Link these to use MPEG2
# MPEG2_DEFINITIONS - Compiler switches required for using MPEG2

SET(MPEG2_SEARCH_PATHS
        ~/Library/Frameworks
        /Library/Frameworks
        /usr/local
        /usr
        /sw # Fink
        /opt/local # DarwinPorts
        /opt/csw # Blastwave
        /opt
)

if (MPEG2_INCLUDE_DIRS AND MPEG2_LIBRARIES)
    # Already in cache, be silent
    set(MPEG2_FIND_QUIETLY TRUE)
endif ()

find_path(MPEG2_INCLUDE_DIRS
    NAMES mpeg2.h
    HINTS
    PATH_SUFFIXES include/mpeg2dec include
    PATHS ${MPEG2_SEARCH_PATHS}
)

find_library(MPEG2_LIBRARIES
    NAMES mpeg2
    HINTS
    PATHS ${MPEG2_SEARCH_PATHS}
)

include(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set MPEG2_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(MPEG2 DEFAULT_MSG
    MPEG2_LIBRARIES MPEG2_INCLUDE_DIRS
)

mark_as_advanced(MPEG2_INCLUDE_DIRS MPEG2_LIBRARIES)

# Provide an imported target for clients that use the CMake imported target
if (MPEG2_FOUND)
    add_library(MPEG2::MPEG2 INTERFACE IMPORTED)
    set_target_properties(MPEG2::MPEG2 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MPEG2_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${MPEG2_LIBRARIES}"
    )
endif ()
