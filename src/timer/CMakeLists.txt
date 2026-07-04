set( LIB_SOURCES
    timer.cpp
)

set( LIB_HEADERS
    timer.h
)

add_library( timer ${LIB_SOURCES} ${LIB_HEADERS} )
target_link_libraries(timer PRIVATE sdl3_deps)
