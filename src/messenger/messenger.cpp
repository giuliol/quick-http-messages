//
// Created by Giulio Luzzati on 19/07/18.
//

#include <zconf.h>
#include "json/single_include/nlohmann/json.hpp"
#include "messenger/messenger.h"

using namespace QhmSockets;

DECLARE_MESSAGE_HANDLER(default_handler, req, rep, ctx) {
    return CORE_CONTINUE;
}

DECLARE_MESSAGE_HANDLER(terminate_handler, req, rep, ctx) {
    *rep = reply_back(req);
    __as_response(*rep)->status = HTTP_STATUS_OK;
    dispatch_event(ctx, SERVICE_TERMINATE);
    return CORE_CONTINUE;
}

DECLARE_EVENT_HANDLER(delete_node, params, ctx){
    core_assert(params.type() == nlohmann::json::value_t::array, return CORE_CONTINUE;);
    for(auto && el : params){
        auto node = parse_qhm_endpoint(el.get<std::string>());
        del_node(ctx, node);
    }
    return CORE_CONTINUE;
}

Messenger::Messenger(Configuration p) {
    QhmEndpoint n =  {p[CONFIG_KEY_SELF_IP], std::stoi(p[CONFIG_KEY_PORT]), p[CONFIG_KEY_TAG]};

    node_self = n;
    rtr_socket = new Socket();
    if(!p.empty()) configuration = p;
}

Status Messenger::init() {
    rtr_socket->bind(node_self.endpoint);

    register_msg_handler(SERVICE_TERMINATE, &terminate_handler);
    register_evt_handler(DELETE_NODE, &delete_node);

    if (!context) context = std::make_shared<MessengerContext>(MessengerContext());
    context->should_run = true;
    context->node_self = &node_self;

    auto verbose = configuration.safe_at("verbose");
    if(!verbose.empty()) context->verbose = (verbose == "true");

    return CORE_OK;
}

Status Messenger::after_init() {
    return CORE_OK;
}

Status Messenger::finalize() {
    rtr_socket->unbind(node_self.endpoint);
    delete rtr_socket;

    for(auto&& node: context->known_nodes) delete node.second;

    return CORE_OK;
}

void Messenger::run() {
    core_assert(init() == CORE_OK, return);
    core_assert(context, core_err << "context not initialized"; return;);
    core_assert(after_init() == CORE_OK, return);
    Status rv;
    Message request, reply, event;
    QhmEndpoint dest;

    while (context->should_run) {
        if (!context->event_queue.empty())
            if(process_event() == CORE_TERMINATE) break;

        if (!request.recv(*rtr_socket, timeout)) continue;

        rv = process_message(request, &reply, &dest);

        if(rv < CORE_OK) { rv = error(context.get(), &reply, dest.tag, HTTP_STATUS_INTERNAL_SERVER_ERROR);
            core_assert(rv == CORE_OK, continue;); }

        if (rv == CORE_OK) transaction_commit(context.get(), reply, dest);
    }

    core_assert(finalize() == CORE_OK, return);
}

Status Messenger::process_event() {

    Event evt = context->event_queue.front();
    context->event_queue.pop();

    if(context->verbose)
        core_ok_tag(node_self.tag) << "reacting to "<< evt.headers.at(HEADER_KEY_EVENT_TYPE) << " event";

    auto event_type = (uint32_t) std::stoi(evt.headers.at(HEADER_KEY_EVENT_TYPE));

    switch (event_type) {
        case SERVICE_TERMINATE: context->should_run = false; return CORE_TERMINATE;
        default: {
            get_evt_handler((EventType) event_type) (context.get(), evt.params);
            break;
        }
    }

    timeout = context->event_queue.empty() ? SOCKET_TIMEOUT : 100;

    return CORE_OK;
}

Status Messenger::process_message(const Message &udp_message_in,
                                  Message *udp_message_out, QhmEndpoint *dest) {
    Status  rv;
    http::Message *http_in = nullptr;
    http::Message *http_out = nullptr;

    if(context->verbose)
        core_log_tag(node_self.tag) << "received msg from " << udp_message_in.sender_ip();

    rv = parse_http(udp_message_in.data(), udp_message_in.size(), &http_in); // allocs http_in
    core_assert(rv == CORE_OK, core_err << "error parsing \n" << udp_message_in.str() ;return CORE_GENERIC_ERROR;);
    if(context->verbose)
        core_log_tag(node_self.tag) << "received http:\n" << http::serialize(http_in);

    if(headers_have(http_in->headers, HEADER_KEY_APP_MESSAGETYPE)){
        auto app_msgtype = (uint32_t) std::stoi(http_in->headers.at(HEADER_KEY_APP_MESSAGETYPE));
        if(context->verbose)
            core_ok_tag(node_self.tag) << "received app message type " << app_msgtype_string(app_msgtype);
        rv = (get_message_handler(app_msgtype))(context.get(), http_in, &http_out); // allocs http_out
    } else
        rv = process_http(http_in, &http_out);

#define cleanup http::http_free(http_out); http::http_free(http_in);
    if(rv != CORE_OK) {cleanup; return rv;}

    udp_message_out->rebuild(http::serialize(http_out));
    *dest = parse_qhm_endpoint(http_out->headers[HEADER_KEY_SERVICE_DST]);

    cleanup;
#undef cleanup

    return rv;
}

Status Messenger::process_http(http::Message* in, http::Message** out) {
    using nlohmann::json;

    Status rv = CORE_OK;
    core_assert(in->type == http::REQUEST, return CORE_CONTINUE;);
    std::string requested_path = __as_request(in)->path;

    // retrieve the handler and use it (and parse the path to get the params)
    Route*  route = context->router.match(requested_path);
    if(route) {
        json params = parse_all_params_in_url(requested_path, route->params);
        rv = (*route->handler)(context.get(), params, in, out);
        core_assert(rv == CORE_OK, return CORE_GENERIC_ERROR;);
    } else {
        core_warn << "no handler for " << requested_path;
        generate_response(in, out, HTTP_STATUS_NOT_FOUND);
    }

    // fill the sender
    (*out)->headers[HEADER_KEY_SERVICE_SRC] = serialize_qhm_endpoint(*context->node_self);

    // validate the http message
    core_assert(validate_http_message(*out, msg_schema),
                core_warn_tag(context->node_self->tag)
                        << "handler did not build valid http. Check handler for url " << __as_request(in)->path << "?";
                        return CORE_GENERIC_ERROR;
    );

    return rv;
}

MessageHandler Messenger::get_message_handler(ApplicationMessageType type) {
    core_assert(app_msg_handlers.find(type) != app_msg_handlers.end(),
                core_warn_tag(node_self.tag) << "Can't handle message type: " << type;
                        return &default_handler;
    );
    return app_msg_handlers[type];
}

EventHandler Messenger::get_evt_handler(EventType type) {
    core_assert(evt_handlers.find(type) != evt_handlers.end(),
                core_warn_tag(node_self.tag) << "Can't handle event type: " << type;
                        return nullptr;
    );
    return evt_handlers[type];
}

Status Messenger::register_msg_handler(ApplicationMessageType type, MessageHandler handler) {
    if(app_msg_handlers.find(type) != app_msg_handlers.end()) return CORE_GENERIC_ERROR;
    app_msg_handlers[type] = handler;
    return CORE_OK;
}

ApplicationMessageType Messenger::register_msg_handler(MessageHandler handler) {
    int applicationmessage_number(0);
    while(register_msg_handler((ApplicationMessageType) (++applicationmessage_number), handler) != CORE_OK) ;
    return (EventType) applicationmessage_number;
}

Status Messenger::register_evt_handler(EventType type, EventHandler handler) {
    if(evt_handlers.find(type) != evt_handlers.end()) return CORE_GENERIC_ERROR;
    evt_handlers[type] = handler;
    return CORE_OK;
}

EventType Messenger::register_evt_handler(EventHandler handler) {
    int event_number(0);
    while(register_evt_handler((EventType) (++event_number), handler) != CORE_OK) ;
    return (EventType) event_number;
}

Status Messenger::deregister_msg_handler(ApplicationMessageType type) {
    auto it = app_msg_handlers.find(type);
    core_assert(it != app_msg_handlers.end(), return CORE_GENERIC_ERROR);
    app_msg_handlers.erase(it);
    return CORE_OK;
}

Status Messenger::deregister_evt_handler(EventType type) {
    auto it = evt_handlers.find(type);
    core_assert(it != evt_handlers.end(), return CORE_GENERIC_ERROR);
    evt_handlers.erase(it);
    return CORE_OK;
}

NeighbourNode* add_node(MessengerContext* context, const QhmEndpoint &new_node) {
    auto known_nodes = &context->known_nodes;
    auto node_self = context->node_self;
    core_assert(known_nodes->find(new_node.tag) == known_nodes->end(),
                core_warn_tag(new_node.tag) << " was already known"; return nullptr);
    if(context->verbose)
        core_ok_tag(node_self->tag) << "adding node "<<  new_node.tag << ", connecting to " << new_node.endpoint;

    auto newnode = new NeighbourNode(new_node, context->node_self->ip_address);

    core_assert(newnode->connected(),
                core_err_tag(node_self->tag) << "could not connect to " << newnode->tag; return nullptr;);

    (*known_nodes)[new_node.tag] = newnode;

    if(context->verbose) core_ok_tag(node_self->tag) << "...connected!";

    return newnode;
}

Status del_node(MessengerContext* context, const QhmEndpoint &deleteme) {
    auto known_nodes = &context->known_nodes;
    auto it = known_nodes->find(deleteme.tag);
    core_assert(it != known_nodes->end(), return CORE_GENERIC_ERROR);
    known_nodes->erase(it);
    delete it->second;
}

bool validate_http_message(const http::Message *msg, const HttpHeaderSchema &schema) {
    switch (msg->type) {
        case http::REQUEST:
            if(__as_request(msg)->path.empty()) return false;
            if(__as_request(msg)->method == HTTP_INVALID_METHOD) {core_err << "invalid method!";return false;}
            break;
        case http::RESPONSE:
            int status = __as_response(msg)->status;
            if (status < HTTP_STATUS_CONTINUE ||
                status > HTTP_STATUS_NETWORK_AUTHENTICATION_REQUIRED) return false;;
            break;
    }
    for (auto &&header: schema)
        core_assert(headers_have(msg->headers, header),
                    core_err << "missing http header \"" << header << "\""; return false;);
    return true;
}

void dispatch_event(MessengerContext *ctx, const Event &evt) {
    ctx->event_queue.push(evt);
}

Status transaction_commit(MessengerContext* context, const Message &reply, const QhmEndpoint& dest) {
    NeighbourNode * node = find_node_by_uri(dest.tag , &context->known_nodes, &node);
    if(!node){
        if(context->verbose) core_warn << dest.tag << " was unknown, adding it...";
        node = add_node(context, dest);
    }

    if(context->verbose)
        core_ok_tag(context->node_self->tag) << "sending "<< reply.size() << " bytes to "<< dest.tag.data();

    reply.send(*node->socket);

    if(node->tag.substr(0,5) == "temp_") {
        Event evt(DELETE_NODE);
        evt.params.push_back(serialize_qhm_endpoint(*node));
        dispatch_event(context,evt);
    }

    return CORE_OK;
}

Status error(MessengerContext* context, Message *resp, const NodeTag& dest, uint32_t status) {
    NeighbourNode * node = find_node_by_uri(dest , &context->known_nodes, &node);
    core_assert(node, core_warn_tag(context->node_self->tag) << "node " << dest << " not found";
            return CORE_GENERIC_ERROR;);

    http::Response msg;
    msg.headers[HEADER_KEY_SERVICE_DST] = serialize_qhm_endpoint((QhmEndpoint) *node);
    msg.status = status;
    std::string buffer = http::serialize(&msg);
    resp->rebuild(buffer.data(), buffer.length() + 1);
    return CORE_OK;
}

NeighbourNode* find_node_by_uri(const NodeTag &uri, const UriSocketMap * nodes, NeighbourNode **dest) {
    auto&& it = nodes->find(uri);
    if(it == nodes->end()) return nullptr;
    return it->second;
}

QhmEndpoint qhm_endpoint_from_configuration(const Configuration &c) {
    auto self_ip    = c.safe_at(CONFIG_KEY_SELF_IP);
    auto port_str   = c.safe_at(CONFIG_KEY_PORT);
    auto tag        = c.safe_at(CONFIG_KEY_TAG);

    core_assert(!self_ip.empty(), return QhmEndpoint());
    core_assert(!port_str.empty(), return QhmEndpoint());
    core_assert(!tag.empty(), return QhmEndpoint());

    int port;
    core_try(port = std::stoi(port_str),  core_err << e.what(););

    return QhmEndpoint(self_ip, port, tag);
}

QhmEndpoint parse_url(const std::string &url_string) {
    std::vector<std::string> tokens;
    if (url_string.substr(0,7) == "http://" )
        tokens = split(url_string.substr(7), ':');
    else if(url_string.substr(0,6) == "tcp://" || url_string.substr(0,6) == "udp://")
        tokens = split(url_string.substr(6), ':');
    else
        tokens = split(url_string, ':');
    core_assert(tokens.size() >= 2, core_err << "invalid endpoint " << url_string; return QhmEndpoint(););
    std::string address;
    int port;
    address = tokens[0];
    core_try(port = std::stoi(tokens[1]), core_err << "invalid port " << tokens[1]; return QhmEndpoint(););
    return QhmEndpoint({address, port, address});
}

std::string parse_path(const std::string& url_string){
    std::vector<std::string> tokens;
    int skip = 0;
    if (url_string.substr(0,7) == "http://" ) skip = 7;
    else if(url_string.substr(0,6) == "tcp://" || url_string.substr(0,6) == "udp://") skip = 6;

    tokens = split(url_string.substr(skip), ':');
    core_assert(tokens.size() >= 2, core_err << "invalid endpoint " << url_string; return "";);
    tokens = split(url_string.substr(skip), '/');
    return url_string.substr(skip + tokens[0].length());
}

QhmEndpoint::QhmEndpoint(const IpAddress &ip, const int p, const NodeTag &uri):
        port(p),ip_address(ip), tag(uri) {
    endpoint = ip_address + ":" + std::to_string(port);
}