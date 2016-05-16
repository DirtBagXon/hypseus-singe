set( LIB_SOURCES
    astron.cpp
    badlands.cpp
    bega.cpp
    benchmark.cpp
    boardinfo.cpp
    cliff.cpp
    cobraconv.cpp
    cputest.cpp
    esh.cpp
    ffr.cpp
    firefox.cpp
    game.cpp
    gpworld.cpp
    interstellar.cpp
    lair.cpp
    lair2.cpp
    laireuro.cpp
    lgp.cpp
    mach3.cpp
    multicputest.cpp
    releasetest.cpp
    seektest.cpp
    speedtest.cpp
    starrider.cpp
    superd.cpp
    test_sb.cpp
    timetrav.cpp
    thayers.cpp
)

set( LIB_HEADERS
    astron.h
    badlands.h
    bega.h
    benchmark.h
    boardinfo.h
    cliff.h
    cobraconv.h
    cputest.h
    esh.h
    ffr.h
    firefox.h
    game.h
    gpworld.h
    interstellar.h
    lair2.h
    laireuro.h
    lair.h
    lgp.h
    mach3.h
    multicputest.h
    releasetest.h
    seektest.h
    speedtest.h
    starrider.h
    superd.h
    test_sb.h
    thayers.h
    timetrav.h
)

if( BUILD_SINGE )
    add_subdirectory( singe )
    list( APPEND LIB_SOURCES singe.cpp )
    list( APPEND LIB_HEADERS singe.h )
endif( BUILD_SINGE )

# ignore gcc: warning: promoted ~unsigned is always non-zero [-Wsign-compare] (do_nmi coin check)
set_source_files_properties( laireuro.cpp PROPERTIES COMPILE_FLAGS -Wno-sign-compare)

add_library( game ${LIB_SOURCES} ${LIB_HEADERS} )
target_link_libraries( game sound scoreboard video io plog )
if( BUILD_SINGE )
    target_link_libraries( game singeproxy )
endif( BUILD_SINGE )
