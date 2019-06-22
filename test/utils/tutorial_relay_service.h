//
// Created by Giulio Luzzati on 28/08/18.
//

#ifndef NEWCORE_EXAMPLE_RELAY_SERVICE_H
#define NEWCORE_EXAMPLE_RELAY_SERVICE_H

#include "messenger/messenger.h"

struct Callback {
    std::string     event_name;
    http_method     method;
    std::string     callback_uri;
    std::string     callback_path;
    std::string     body;
    nlohmann::json  params;
    nlohmann::json  state;
    QhmEndpoint     destination;
    std::string     nf_instance;
};

static const char* HIGH_MARK_EVT = "HIGH_MARK";

struct HighmarkCallback : public Callback {
    HighmarkCallback(): Callback({HIGH_MARK_EVT, HTTP_POST}) {}
};

struct RelayService_Context : public MessengerContext{
    typedef std::map<EventType , std::vector<std::shared_ptr<Callback>>> CallbacksMap;
    std::string                                     signature;
    CallbacksMap                                    callbacks;
    std::map<std::string, EventType>                events;
    Status                                          add_callback(std::shared_ptr<Callback> cb, EventType e) {
        QhmEndpoint endpoint = parse_url(cb->callback_uri);
        cb->destination = endpoint;
        cb->nf_instance = endpoint.tag;
        cb->callback_path = parse_path(cb->callback_uri);
        callbacks[e].emplace_back(cb);
        return CORE_OK;
    }
    Status                                          del_callback(const std::string& nf_instance){
        for(auto && event: callbacks){
            auto && list = event.second;
            auto cb = std::find_if(list.begin(), list.end(),
                                   [&nf_instance](std::shared_ptr<Callback> c){return c->nf_instance == nf_instance;});
            if(cb != list.end()) list.erase(cb);
        }
        return CORE_OK;
    }

};


class RelayService : public Messenger{
public:
    RelayService(Configuration p):Messenger(p) {}
    Status init() override;
    Status finalize() override;

};

static void process_callback(MessengerContext* ctx, Callback* callback){
    http::Request request;
    request.path = callback->callback_path;
    request.body = callback->body;
    request.method = callback->method;
    async_send_request(&request, *ctx->node_self, callback->destination);
}

static const char* CALLBACK_URI = "callback_uri";
static const char* CALLBACK_BODY = "callback_body";
static const char* CALLBACK_METHOD = "callback_method";

#endif //NEWCORE_EXAMPLE_RELAY_SERVICE_H
