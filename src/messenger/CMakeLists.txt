set(SOURCES
        messenger.cpp
        messenger_utils.cpp
        messenger_neighbour_node.cpp
        messenger_router.cpp
        messenger_configuration.cpp
        ../../include/messenger/messenger.h
        ../../include/messenger/event.h
        ../../include/messenger/configuration.h
        )

add_library(messenger ${SOURCES})
target_link_libraries(messenger
        httplib
        core
        uuid
        udp
        )