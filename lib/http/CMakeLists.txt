set(SOURCES
        uri_t.hpp
        parser.h
        http_parser_core.h
        util.cpp
        util.h
        parser.cpp
        http_parser_core.cpp
        uri_t.cpp
        http_types.h)

add_library(httplib ${SOURCES})
