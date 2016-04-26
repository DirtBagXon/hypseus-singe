set( LIB_SOURCES
    vldp.c
    vldp_internal.c
    mpegscan.c
)

set( LIB_HEADERS
    mpegscan.h
    vldp_common.h
    vldp.h
    vldp_internal.h
)

add_library( vldp ${LIB_SOURCES} ${LIB_HEADERS} )
target_link_libraries( vldp ${MPEG2_LIBRARIES} )
