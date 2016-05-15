set( LIB_SOURCES
    vldp.cpp
    vldp_internal.cpp
    mpegscan.cpp
)

set( LIB_HEADERS
    mpegscan.h
    vldp_common.h
    vldp.h
    vldp_internal.h
)

include_directories( ${MPEG2_INCLUDE_DIRS} )

add_library( vldp ${LIB_SOURCES} ${LIB_HEADERS} )
target_link_libraries( vldp ${MPEG2_LIBRARIES} )
