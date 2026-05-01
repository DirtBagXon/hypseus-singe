macro( build_libmpeg2 )

include( CheckCCompilerFlag )

find_program( LIBTOOLIZE_EXECUTABLE
    NAMES glibtoolize libtoolize
    REQUIRED )

if( CMAKE_CROSSCOMPILING )
    string( REGEX MATCH "([-A-Za-z0-9\\._]+)-(gcc|cc)$" RESULT ${CMAKE_C_COMPILER} )
    string( REGEX REPLACE "-(gcc|cc)$" "" RESULT ${RESULT} )
    set( CONFIGURE_ARGS "--host=${RESULT}" )
endif()

check_c_compiler_flag(
    "-Wdeprecated-non-prototype"
    HAVE_DEPRECATED_NON_PROTOTYPE
)

set( LIBMPEG2_CFLAGS "-std=gnu89" )

if( HAVE_DEPRECATED_NON_PROTOTYPE )
    string( APPEND LIBMPEG2_CFLAGS " -Wno-deprecated-non-prototype" )
endif()

externalproject_add( libmpeg2
	PREFIX ${CMAKE_CURRENT_BINARY_DIR}/3rdparty
	URL ../../../src/3rdparty/libmpeg2/libmpeg2-master.tgz
	URL_HASH SHA256=5aad06f396553c5b6afb5393ff26187bb1120928d6ed4f88d2482dd41d04cf75

	CONFIGURE_COMMAND
		${LIBTOOLIZE_EXECUTABLE} --copy --force &&
		autoreconf -f -i &&
		<SOURCE_DIR>/configure
		${CONFIGURE_ARGS}
		--quiet
		--prefix=${CMAKE_CURRENT_BINARY_DIR}/3rdparty
		--disable-shared
		--enable-static
		--disable-sdl

	BUILD_IN_SOURCE 1
	BUILD_COMMAND make V=0 CFLAGS=${LIBMPEG2_CFLAGS}
	INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/3rdparty
	INSTALL_COMMAND make LIBTOOLFLAGS=--silent install
	${DOWNLOAD_ARGS}
)

set( MPEG2_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/3rdparty/include/mpeg2dec )
set( MPEG2_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/3rdparty/lib/libmpeg2.a )
set( MPEG2_FOUND ON )

endmacro( build_libmpeg2 )
