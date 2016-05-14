add_library( plog dummy.cpp )
target_include_directories( plog PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include> )
