set( LIB_SOURCES
    astron.cpp
    badlands.cpp
    bega.cpp
    boardinfo.cpp
    cliff.cpp
    cobraconv.cpp
    esh.cpp
    game.cpp
    gpworld.cpp
    interstellar.cpp
    lair.cpp
    lair2.cpp
    laireuro.cpp
    mach3.cpp
    superd.cpp
    thayers.cpp
)

set( LIB_HEADERS
    astron.h
    badlands.h
    bega.h
    boardinfo.h
    cliff.h
    cobraconv.h
    esh.h
    game.h
    gpworld.h
    interstellar.h
    lair2.h
    laireuro.h
    lair.h
    lair_util.h
    mach3.h
    superd.h
    thayers.h
)

if( DEFUNC )
  list( APPEND LIB_SOURCES
    benchmark.cpp
    cputest.cpp
    ffr.cpp
    firefox.cpp
    lgp.cpp
    multicputest.cpp
    releasetest.cpp
    seektest.cpp
    speedtest.cpp
    starrider.cpp
    test_sb.cpp
    timetrav.cpp
)
  list( APPEND LIB_HEADERS
    benchmark.h
    cputest.h
    ffr.h
    firefox.h
    lgp.h
    multicputest.h
    releasetest.h
    seektest.h
    speedtest.h
    starrider.h
    test_sb.h
    timetrav.h
)
endif( DEFUNC )

if( BUILD_SINGE )
    add_subdirectory( singe )
    list( APPEND LIB_SOURCES singe.cpp )
    list( APPEND LIB_HEADERS singe.h )

# ignore gcc: warning: j = *((int*)(&s)); ASCII char*=>int (handle_cmdline_arg)
set_source_files_properties( singe.cpp PROPERTIES COMPILE_FLAGS -Wno-strict-aliasing)
endif( BUILD_SINGE )

# ignore gcc: warning: promoted ~unsigned is always non-zero [-Wsign-compare] (do_nmi coin check)
set_source_files_properties( laireuro.cpp PROPERTIES COMPILE_FLAGS -Wno-sign-compare)

add_library( game ${LIB_SOURCES} ${LIB_HEADERS} )
target_link_libraries( game sound scoreboard video io plog )
if( BUILD_SINGE )
    target_link_libraries( game singeproxy )
endif( BUILD_SINGE )
