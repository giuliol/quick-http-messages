set(TESTPATH ${CMAKE_SOURCE_DIR}/test)
include_directories(${CMAKE_SOURCE_DIR}/lib)
find_package(Threads REQUIRED)

file(GLOB TEST_SRC
        "${TESTPATH}/*.cpp"
        )

add_library(example_service
        utils/tutorial_time_service.cpp
        utils/tutorial_time_service.h
        utils/tutorial_relay_service.h
        utils/tutorial_relay_service.cpp
        utils/test_utils.h
        )

set(LIBRARIES
        core
        Threads::Threads
        httplib
        example_service
        messenger
        )
#
#foreach (testSrc ${TEST_SRC})
#    get_filename_component(testName ${testSrc} NAME_WE)
#    message("adding test \"" ${testName} "\"")
#    add_executable(${testName} ${testSrc})
#    target_link_libraries(${testName} ${LIBRARIES})
#    add_test(NAME ${testName} COMMAND ${testName})
#endforeach ()



add_executable(messenger_test
        messenger_test.cpp
        )
target_link_libraries(messenger_test
        ${LIBRARIES}
        )
add_test(messenger_test messenger_test)



add_executable(route_matching_test
        route_matching_test.cpp
        )
target_link_libraries(route_matching_test
        ${LIBRARIES}
        )
add_test(route_matching_test route_matching_test)



add_executable(udp_test
        udp_test.cpp
        )
target_link_libraries(udp_test
        core
        udp
        Threads::Threads
        )
add_test(udp_test udp_test)

add_executable(http_test
        http_test.cpp
        )
target_link_libraries(http_test
        httplib
        messenger
        Threads::Threads
        )
add_test(http_test http_test)


add_executable(send_request_test
        send_request_test.cpp
        )
target_link_libraries(send_request_test
        ${LIBRARIES}
        )
add_test(send_request_test send_request_test)


add_executable(configuration_test
        configuration_test.cpp
        )
target_link_libraries(configuration_test
        ${LIBRARIES}
        )
add_test(configuration_test configuration_test)
