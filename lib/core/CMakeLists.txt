set(SOURCES
        common.h
        udplib.h
        periodic_task.cpp periodic_task.h
        logger_time.cpp
        logger.h
        )
add_library(core ${SOURCES})
target_link_libraries(core
        udp
        )