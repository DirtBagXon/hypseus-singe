set( LIB_SOURCES
    singeproxy.cpp
)

set( LIB_HEADERS
    singeproxy.h
)

find_package( Lua REQUIRED )
include_directories( ${LUA_INCLUDE_DIR} )

add_library( singeproxy ${LIB_SOURCES} ${LIB_HEADERS} )
target_link_libraries( singeproxy ${LUA_LIBRARIES} )
