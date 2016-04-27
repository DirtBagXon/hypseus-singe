cmake_minimum_required(VERSION 2.9)

project(daphne)

option(BENCHMARK    "Benchmark"  OFF)
option(DEBUG        "Debug"      OFF)
option(VLDP_DEBUG   "VLDP Debug" OFF)
option(CPU_DEBUG    "CPU Debug"  OFF)
option(BUILD_SINGE  "Singe"      OFF)

if( NOT CMAKE_BUILD_TYPE )
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

include(TargetArch)
target_architecture(CMAKE_TARGET_ARCHITECTURES)

# Config.h defines
string(TOUPPER ${CMAKE_SYSTEM_NAME} TARGET_SYSTEM)
string(CONCAT TARGET_SYSTEM TARGET_ ${TARGET_SYSTEM})
string(TOUPPER ${CMAKE_TARGET_ARCHITECTURES} TARGET_ARCH)
string(CONCAT TARGET_ARCH TARGET_ ${TARGET_ARCH})
set(daphne_VERSION_MAJOR 1)
set(daphne_VERSION_MINOR 0)
set(daphne_VERSION_POINT 13)

configure_file(
    "${PROJECT_SOURCE_DIR}/config.h.in"
    "${PROJECT_BINARY_DIR}/config.h"
)


include(InstallRequiredSystemLibraries)
include(FindPkgConfig)

find_package(SDL2 REQUIRED)
PKG_SEARCH_MODULE(SDL2_TTF REQUIRED SDL2_ttf)
PKG_SEARCH_MODULE(MPEG2 REQUIRED libmpeg2)

message(STATUS "Target: ${CMAKE_SYSTEM_NAME} ${CMAKE_TARGET_ARCHITECTURES}")

include_directories(
    ${PROJECT_BINARY_DIR} # config.h
    ${SDL2_INCLUDE_DIR}
    ${MPEG2_INCLUDE_DIRS}
)

enable_language( ASM )
if( "${CMAKE_TARGET_ARCHITECTURES}" MATCHES "i386" )
    set(can_use_assembler TRUE)
endif()

add_subdirectory(cpu)
add_subdirectory(game)
add_subdirectory(io)
add_subdirectory(ldp-in)
add_subdirectory(ldp-out)
add_subdirectory(scoreboard)
add_subdirectory(sound)
add_subdirectory(timer)
add_subdirectory(video)
add_subdirectory(vldp)

set(EXECUTABLE_NAME "daphne")
add_executable( ${EXECUTABLE_NAME} daphne.cpp globals.h )
target_link_libraries( ${EXECUTABLE_NAME} io timer sound video cpu game ${SDL2_LIBRARY} ${SDL2_TTF_LIBRARIES} )
