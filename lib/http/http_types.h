//
// Created by Giulio Luzzati on 25/07/18.
//

#ifndef NEWCORE_HTTP_TYPES_H
#define NEWCORE_HTTP_TYPES_H

#include <cstdint>
#include <string>
#include <map>
#include "http_parser_core.h"

typedef std::map<std::string, std::string> headers_map;

#define __as_response(m) ((http::Response*)m)
#define __as_request(m) ((http::Request*)m)

namespace http {


    enum http_message_type : uint8_t {
        REQUEST = HTTP_REQUEST,
        RESPONSE = HTTP_RESPONSE
    };

    struct Message {
        Message(http_message_type t): type(t) {}
        std::string body;
        headers_map headers;
        http_message_type type;
        bool success = false;
        bool more = false;
    };


    struct Request : public Message {
        Request():Message(REQUEST) {}
        http_method method = HTTP_INVALID_METHOD;
        std::string path;
        std::string uri;
    };

    struct Response : public Message {
        Response():Message(RESPONSE) {}
        uint32_t status;
    };

    void http_free(Message* msg);
}


#endif //NEWCORE_HTTP_TYPES_H
