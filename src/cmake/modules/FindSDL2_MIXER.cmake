# Locate SDL2 MIXER library
#=============================================================================
# Copyright 2003-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

SET(SDL2_MIXER_SEARCH_PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
)

FIND_PATH(SDL2_MIXER_INCLUDE_DIR SDL_mixer.h
	HINTS
	$ENV{SDL2_MIXERDIR}
	PATH_SUFFIXES include/SDL2 include
	PATHS ${SDL2_MIXER_SEARCH_PATHS}
)

FIND_LIBRARY(SDL2_MIXER_LIBRARY_TEMP
	NAMES SDL2_mixer
	HINTS
	$ENV{SDL2_MIXERDIR}
	PATH_SUFFIXES lib64 lib
	PATHS ${SDL2_MIXER_SEARCH_PATHS}
)

IF(NOT SDL2_MIXER_BUILDING_LIBRARY)
	IF(NOT ${SDL2_MIXER_INCLUDE_DIR} MATCHES ".framework")
		# Non-OS X framework versions expect you to also dynamically link to
		# SDL2_MIXERmain. This is mainly for Windows and OS X. Other (Unix) platforms
		# seem to provide SDL2_MIXERmain for compatibility even though they don't
		# necessarily need it.
		FIND_LIBRARY(SDL2MIXERMAIN_LIBRARY
			NAMES SDL2_mixer
			HINTS
			$ENV{SDL2_MIXERDIR}
			PATH_SUFFIXES lib64 lib
			PATHS ${SDL2_MIXER_SEARCH_PATHS}
		)
ENDIF(NOT ${SDL2_MIXER_INCLUDE_DIR} MATCHES ".framework")
ENDIF(NOT SDL2_MIXER_BUILDING_LIBRARY)

# SDL2_MIXER may require threads on your system.
# The Apple build may not need an explicit flag because one of the
# frameworks may already provide it.
# But for non-OSX systems, I will use the CMake Threads package.
IF(NOT APPLE)
	FIND_PACKAGE(Threads)
ENDIF(NOT APPLE)

IF(MINGW)
	SET(MINGW32_LIBRARY mingw32 CACHE STRING "mwindows for MinGW")
ENDIF(MINGW)

IF(SDL2_MIXER_LIBRARY_TEMP)
	# For SDL2_MIXERmain
	IF(NOT SDL2_MIXER_BUILDING_LIBRARY)
		IF(SDL2_MIXERMAIN_LIBRARY)
			SET(SDL2_MIXER_LIBRARY_TEMP ${SDL2_MIXERMAIN_LIBRARY} ${SDL2_MIXER_LIBRARY_TEMP})
		ENDIF(SDL2_MIXERMAIN_LIBRARY)
	ENDIF(NOT SDL2_MIXER_BUILDING_LIBRARY)

	# For threads, as mentioned Apple doesn't need this.
	# In fact, there seems to be a problem if I used the Threads package
	# and try using this line, so I'm just skipping it entirely for OS X.
	IF(NOT APPLE)
		SET(SDL2_MIXER_LIBRARY_TEMP ${SDL2_MIXER_LIBRARY_TEMP} ${CMAKE_THREAD_LIBS_INIT})
	ENDIF(NOT APPLE)

	# For MinGW library
	IF(MINGW)
		SET(SDL2_MIXER_LIBRARY_TEMP ${MINGW32_LIBRARY} ${SDL2_MIXER_LIBRARY_TEMP})
	ENDIF(MINGW)

	# Set the final string here so the GUI reflects the final state.
	SET(SDL2_MIXER_LIBRARY ${SDL2_MIXER_LIBRARY_TEMP} CACHE STRING "Where the SDL2_MIXER Library can be found")
	# Set the temp variable to INTERNAL so it is not seen in the CMake GUI
	SET(SDL2_MIXER_LIBRARY_TEMP "${SDL2_MIXER_LIBRARY_TEMP}" CACHE INTERNAL "")
ENDIF(SDL2_MIXER_LIBRARY_TEMP)

INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL2_MIXER REQUIRED_VARS SDL2_MIXER_LIBRARY SDL2_MIXER_INCLUDE_DIR)
