set( LIB_SOURCES
    manymouse.c linux_evdev.c macosx_hidmanager.c
    macosx_hidutilities.c windows_wminput.c
    x11_xinput2.c
)

set( LIB_HEADERS
    manymouse.h
)

add_library( manymouse ${LIB_SOURCES} ${LIB_HEADERS} )
target_link_libraries( manymouse ${CMAKE_DL_LIBS} )

