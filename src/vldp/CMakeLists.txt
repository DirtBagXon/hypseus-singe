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

if( VLDP_DEBUG )
	set_source_files_properties( ${LIB_SOURCES} PROPERTIES COMPILE_FLAGS -DVLDP_DEBUG )
endif( VLDP_DEBUG )

include_directories( ${MPEG2_INCLUDE_DIRS} )

add_library( vldp ${LIB_SOURCES} ${LIB_HEADERS} )
target_link_libraries( vldp plog ${MPEG2_LIBRARIES} )
