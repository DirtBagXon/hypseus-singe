set( LIB_SOURCES
    singeproxy.cpp lapi.c lcode.c ldo.c lgc.c llex.c
    loadlib.c loslib.c lstate.c ltable.c lundump.c print.c
    lauxlib.c ldblib.c ldump.c linit.c lmathlib.c lobject.c
    lparser.c lstring.c ltablib.c lvm.c random.c lbaselib.c
    ldebug.c lfunc.c liolib.c lmem.c lopcodes.c lrandom.c
    luretro.c luars232.c rs232.c lstrlib.c ltm.c lzio.c lbit.c
    md5lib.c md5.c lfs.c singe_utils.cpp sdl2_gfx/SDL2_rotozoom.c
    cjson/strbuf.c cjson/fpconv.c cjson/lua_cjson.c
)

set( LIB_HEADERS
    lapi.h lcode.h ldo.h lgc.h llimits.h lobject.h lparser.h
    lstring.h ltm.h lua.h lundump.h lzio.h singeproxy.h
    lauxlib.h ldebug.h lfunc.h llex.h lmem.h lopcodes.h luretro.h
    lstate.h ltable.h luaconf.h lualib.h lvm.h singe_interface.h
    lfs.h md5.h rs232.h sdl2_gfx/SDL2_rotozoom.h cjson/strbuf.h
    cjson/fpconv.h
)

if( WIN32 )
    list( APPEND LIB_SOURCES rs232_windows.c )
    list( APPEND LIB_HEADERS rs232_windows.h )
    add_definitions( -DRS232_EXPORT )
else( WIN32 )
    if( APPLE )
        set( SDL2_LIBRARIES "" )
    endif( APPLE )
    list( APPEND LIB_SOURCES rs232_posix.c )
    list( APPEND LIB_HEADERS rs232_posix.h )
endif( WIN32 )

if( ABSTRACT_SINGE )
    set_source_files_properties( luretro.c PROPERTIES COMPILE_FLAGS -DABSTRACT_SINGE )
endif( ABSTRACT_SINGE )

set_source_files_properties( random.c rs232_windows.c PROPERTIES COMPILE_FLAGS -Wno-unused-function )

if( CMAKE_SYSTEM_PROCESSOR MATCHES "arm*|aarch64" )
    set_source_files_properties( singeproxy.cpp PROPERTIES COMPILE_FLAGS -Wno-psabi )
endif()

add_library( singeproxy ${LIB_SOURCES} ${LIB_HEADERS} )
target_link_libraries( singeproxy io ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} )
