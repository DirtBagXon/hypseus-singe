set( LIB_SOURCES
    singeproxy.cpp lapi.c lcode.c ldo.c lgc.c llex.c
    loadlib.c loslib.c lstate.c ltable.c lundump.c print.c
    lauxlib.c ldblib.c ldump.c linit.c lmathlib.c lobject.c
    lparser.c lstring.c ltablib.c lvm.c random.c lbaselib.c
    ldebug.c lfunc.c liolib.c lmem.c lopcodes.c lrandom.c
    luretro.c lstrlib.c ltm.c lzio.c lfs.c singe_utils.cpp

)

set( LIB_HEADERS
    lapi.h lcode.h ldo.h lgc.h llimits.h lobject.h lparser.h
    lstring.h ltm.h lua.h lundump.h lzio.h singeproxy.h
    lauxlib.h ldebug.h lfunc.h llex.h lmem.h lopcodes.h luretro.h
    lstate.h ltable.h luaconf.h lualib.h lvm.h singe_interface.h lfs.h
)

if( APPLE )
    set( SDL2_LIBRARIES "" )
endif( APPLE )

set_source_files_properties( random.c PROPERTIES COMPILE_FLAGS -Wno-unused-function )

add_library( singeproxy ${LIB_SOURCES} ${LIB_HEADERS} )
target_link_libraries( singeproxy ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} )
