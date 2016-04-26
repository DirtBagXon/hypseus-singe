set( LIB_SOURCES
    cpu.cpp
    mamewrap.cpp
    cpu-debug.cpp
    m80.cpp
    mc6809.cpp
    6809infc.cpp
    nes6502.cpp
    nes_6502.cpp
    cop.cpp
    copintf.cpp
)

set( LIB_HEADERS
    6809infc.h
    cpu.h
    cpu-debug.h
    cop.h
    copintf.h
    generic_z80.h
    m80.h
    m80_internal.h
    m80daa.h
    m80tables.h
    mamewrap.h
    mc6809.h
    nes_6502.h
    nes6502.h
    x86/i86.h
    x86/i86intf.h
    types.h
)

add_subdirectory( x86 )

add_library( cpu ${LIB_SOURCES} ${LIB_HEADERS} )
target_link_libraries( cpu cpu_x86 ldp-in game )
