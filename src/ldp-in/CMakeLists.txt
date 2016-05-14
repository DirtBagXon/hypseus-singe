set( LIB_SOURCES
    ldv1000.cpp
    pr8210.cpp
    vip9500sg.cpp
    ldp1000.cpp
    vp931.cpp
    pr7820.cpp
    vp380.cpp
    vp932.cpp
)

set( LIB_HEADERS
    ldp1000.h
    ldv1000.h
    pr7820.h
    pr8210.h
    vip9500sg.h
    vp380.h
    vp931.h
    vp932.h
)

add_library( ldp-in ${LIB_SOURCES} ${LIB_HEADERS} )
target_link_libraries( ldp-in plog )
