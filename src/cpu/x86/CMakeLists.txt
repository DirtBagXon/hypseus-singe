set( LIB_SOURCES
    i86.cpp
    i86dasm.cpp
)

set( LIB_HEADERS
    i86.h
    i86intf.h
    instr86.h
    modrm.h
    table86.h
    instr86.h
)

add_library( cpu_x86 ${LIB_SOURCES} ${LIB_HEADERS} )
target_link_libraries( cpu_x86 PRIVATE sdl3_deps )
