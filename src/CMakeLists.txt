cmake_minimum_required(VERSION 3.5)

set(PKG_VERSION "v2.11.6-RPi")

project(hypseus)

option(BENCHMARK        "Benchmark"             OFF)
option(DEBUG            "Debug"                 OFF)
option(VLDP_DEBUG       "VLDP Debug"            OFF)
option(CPU_DEBUG        "CPU Debug"             OFF)
option(BUILD_SINGE      "Singe"                 ON)
option(BUILDBOT         "Buildbot"              OFF)

# Abstract out -retropath ES extension: Use .hypseus NOT .daphne
option(ABSTRACT_SINGE   "Redefine Singe LUA Rewrites"     OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

if (UNIX)
  if (CMAKE_SIZEOF_VOID_P EQUAL 4)
      message(STATUS "Detected 32bit: Compiling with -D_FILE_OFFSET_BITS=64")
      add_definitions(-D_FILE_OFFSET_BITS=64)
  endif()
endif()

set(DOWNLOAD_ARGS)
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    list(APPEND DOWNLOAD_ARGS DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
endif()

add_compile_options(-Wall)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

include(TargetArch)
target_architecture(CMAKE_TARGET_ARCHITECTURES)

# Config.h defines
string(TOUPPER ${CMAKE_SYSTEM_NAME} TARGET_SYSTEM)
string(CONCAT TARGET_SYSTEM TARGET_ ${TARGET_SYSTEM})
string(TOUPPER ${CMAKE_TARGET_ARCHITECTURES} TARGET_ARCH)
string(CONCAT TARGET_ARCH TARGET_ ${TARGET_ARCH})

include(BuildSupport)
include(GNUInstallDirs)
include(GetGitRevisionDescription)
include(InstallRequiredSystemLibraries)
include(FindPkgConfig)

use_cxx11( )

PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
PKG_SEARCH_MODULE(SDL2_TTF REQUIRED SDL2_ttf)
PKG_SEARCH_MODULE(SDL2_IMAGE REQUIRED SDL2_image)
PKG_SEARCH_MODULE(SDL2_MIXER REQUIRED SDL2_mixer)
PKG_SEARCH_MODULE(MPEG2 REQUIRED libmpeg2)

message(STATUS "Target: ${CMAKE_SYSTEM_NAME} ${CMAKE_TARGET_ARCHITECTURES}")

git_describe(VER_TAG --tags --abbrev=0)
if("${VER_TAG}" MATCHES "-NOTFOUND")
    set(VER_TAG  ${PKG_VERSION})
    set(VER_GIT_COUNT "0")
else()
    git_revlist(VER_GIT_COUNT --count ${VER_TAG}..HEAD)
endif()
string(REGEX REPLACE "^v" "" RESULT ${VER_TAG})
string(REGEX REPLACE "[-\\.]" ";" RESULT ${RESULT})
list(GET RESULT 0 VER_MAJOR)
list(GET RESULT 1 VER_MINOR)
list(GET RESULT 2 VER_PATCH)
if(NOT "${VER_GIT_COUNT}" EQUAL "0")
    set(VER_TAG "${VER_TAG}-${VER_GIT_COUNT}")
endif()
git_describe(VER_GIT_DIRTY --always --dirty)
if("${VER_GIT_DIRTY}" MATCHES "-dirty")
    set(VER_TAG "${VER_TAG}-dirty")
endif()
message(STATUS "Version: ${VER_TAG}")

include_directories(
    ${PROJECT_BINARY_DIR} # config.h
    ${SDL2_INCLUDE_DIRS}
    ${MPEG2_INCLUDE_DIRS}
)

if( "${CMAKE_TARGET_ARCHITECTURES}" MATCHES "i386" )
    enable_language( ASM )
    set(can_use_assembler TRUE)
endif()

configure_file(
    "${PROJECT_SOURCE_DIR}/config.h.in"
    "${PROJECT_BINARY_DIR}/config.h"
)

add_subdirectory(3rdparty)

add_subdirectory(cpu)
add_subdirectory(game)
add_subdirectory(manymouse)
add_subdirectory(io)
add_subdirectory(ldp-in)
add_subdirectory(ldp-out)
add_subdirectory(scoreboard)
add_subdirectory(sound)
add_subdirectory(timer)
add_subdirectory(video)
add_subdirectory(vldp)

add_dependencies( ldp-out vldp )
add_dependencies( game vldp )
add_dependencies( sound vldp )

find_package(SDL2 REQUIRED)
find_package(SDL2_TTF REQUIRED)
find_package(SDL2_IMAGE REQUIRED)
find_package(SDL2_MIXER REQUIRED)
find_package(Vorbis REQUIRED)

if(APPLE)
    set( SDL2_LIBRARIES ${SDL2_LIBRARY} )
endif(APPLE)

add_executable( hypseus hypseus.cpp globals.h )
target_link_libraries( hypseus plog io timer sound video cpu game ${Vorbis_Enc_LIBRARY} Vorbis::vorbis ${OGG_LIBRARIES} ${VORBISFILE_LIBRARIES} ${Vorbis_File_LIBRARY} ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARY} ${SDL2_MIXER_LIBRARY} ${SDL2_IMAGE_LIBRARY} ${LIBZIP_LIBRARY} )

#[[
set_source_files_properties( testvldp.cpp PROPERTIES COMPILE_FLAGS -DSHOW_FRAMES)
add_executable( testvldp testvldp.cpp )
target_link_libraries( testvldp video vldp ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARY} ${SDL2_MIXER_LIBRARY} ${SDL2IMAGE_LIBRARY})
]]

if(BUILDBOT)
    add_custom_command( TARGET hypseus
        POST_BUILD
	COMMAND ${CMAKE_STRIP} $<TARGET_FILE:hypseus>
        COMMAND ${CMAKE_COMMAND} -E remove -f hypseus.zip
        COMMAND 7za a -mpass=4 -mfb=255 -y -tzip hypseus.zip $<TARGET_FILE:hypseus>
        COMMAND ${CMAKE_COMMAND} -E md5sum hypseus.zip > hypseus.md5
        COMMENT "Create zip and md5 for buildbot")
endif()

set(CPACK_PACKAGE_NAME "Hypseus Singe")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Hypseus Singe - Arcade Laserdisc Emulator")
set(CPACK_PACKAGE_VENDOR "DirtBagXon")
set(CPACK_PACKAGE_CONTACT "hypseus@users.noreply.github.com")
set(CPACK_PACKAGE_VERSION "${VER_TAG}")
set(CPACK_PACKAGE_VERSION_MAJOR "${VER_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${VER_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${VER_PATCH}")

include(CPack)
