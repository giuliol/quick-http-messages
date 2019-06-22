//
// Created by gl0021 on 28/09/18.
//

#ifndef QHM_EVENT_H
#define QHM_EVENT_H

#include "core/common.h"
#include "http/http_types.h"

class Event : public http::Request {
public:
    Event() = default;

    Event(const http::Request& r) {
        method = r.method;
        path = r.path;
        headers = r.headers;
        body = r.body;
        uri = r.uri;
    }

    Event(const EventType& e) {
        method = HTTP_POST;
        path = "/local/event";
        headers[HEADER_KEY_EVENT_TYPE] = std::to_string(e);
        headers[HEADER_KEY_EVENT_NAME] = app_msgtype_string(e);
    }

    Event(const EventType& e, const std::string& name) {
        method = HTTP_POST;
        path = "/local/event";
        headers[HEADER_KEY_EVENT_TYPE] = std::to_string(e);
        headers[HEADER_KEY_EVENT_NAME] = name;
    }

    nlohmann::json  params;
    std::string     callback_reference;
};

struct Procedure : public Event {
    Procedure():Event() {}
    Procedure(const Event& e):Event(e) {}
    uint64_t                                id;
    http_status                             acknowledged;
    http::Request                           request;
};

#endif //QHM_EVENT_H
