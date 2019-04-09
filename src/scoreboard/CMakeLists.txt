set( LIB_SOURCES
    hw_scoreboard.cpp
    usb_scoreboard.cpp
    img_scoreboard.cpp
    null_scoreboard.cpp
    overlay_scoreboard.cpp
    scoreboard_collection.cpp
    scoreboard_factory.cpp
    scoreboard_interface.cpp
)

set( LIB_HEADERS
    hw_scoreboard.h
    usb_scoreboard.h
    img_scoreboard.h
    null_scoreboard.h
    overlay_scoreboard.h
    scoreboard_collection.h
    scoreboard_factory.h
    scoreboard_interface.h
)

add_library( scoreboard ${LIB_SOURCES} ${LIB_HEADERS} )
target_link_libraries( scoreboard io )
