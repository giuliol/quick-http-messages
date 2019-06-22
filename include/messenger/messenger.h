//
// Created by Giulio Luzzati on 19/07/18.
//

#ifndef NEWCORE_MESSENGER_H
#define NEWCORE_MESSENGER_H

#include <map>
#include <list>
#include <queue>
#include "udp/udp.h"
#include "http/parser.h"
#include "core/common.h"
#include "event.h"
#include "configuration.h"

/* enums and constants */

static const char*      MESSENGER_NODE_ENDPOINT = "endpoint";
static const char*      MESSENGER_NODE_TAG = "tag";
static const int        SOCKET_TIMEOUT = 3000;

/* forward declarations */
struct      QhmEndpoint;
struct      NeighbourNode;
struct      MessengerContext;
struct      RouteParameter;
class       Messenger;

/* typedefs */
typedef     uint32_t ApplicationMessageType;
typedef     std::string SockEndpoint;
typedef     std::string NodeTag;
typedef     std::string IpAddress;
typedef     std::string UuidString;
typedef     std::list<std::string> HttpHeaderSchema;
typedef     std::map<NodeTag, NeighbourNode*> UriSocketMap;
typedef     Status(* MessageHandler) (MessengerContext *,const http::Message *, http::Message **);
typedef     Status(* RouteHandler)   (MessengerContext*, const nlohmann::json&, const http::Message *, http::Message **);
typedef     Status(* EventHandler)   (MessengerContext*, const nlohmann::json& p);
typedef     std::map<ApplicationMessageType, MessageHandler> ApplicationMsgHandlersMap;
typedef     std::map<EventType , EventHandler> EventHandlersMap;
typedef     std::vector<RouteParameter> RouteParams;

#define DECLARE_MESSAGE_HANDLER(_name, _in, _out, _ctx)  \
    Status _name (MessengerContext* _ctx, const http::Message* _in, http::Message ** _out)

#define DECLARE_EVENT_HANDLER(_name, _params, _ctx) \
    Status _name (MessengerContext* _ctx, const nlohmann::json& _params)

#define DECLARE_ROUTE_HANDLER(_name, _in, _out, _params, _ctx) \
    Status _name (MessengerContext* _ctx, const nlohmann::json& _params, const http::Message* _in, http::Message** _out)


struct QhmEndpoint {
    QhmEndpoint() = default;
    QhmEndpoint(const IpAddress& ip, const int p, const NodeTag& uri);
    int                                     port;
    IpAddress                               ip_address;
    NodeTag                                 tag;
    SockEndpoint                            endpoint;
};

struct NeighbourNode : public QhmEndpoint {
    NeighbourNode(const QhmEndpoint& n);
    NeighbourNode(const QhmEndpoint& n, const std::string& src_ip);
    ~NeighbourNode();
    Status                                  generate_request(http::Message**);
    Status                                  generate_response(http::Message**);
    bool                                    connected();

    QhmSockets::Socket*                     socket = nullptr;
};

struct RouteParameter {
    unsigned int                            pos;
    std::string                             name;
};

struct Route {
    std::regex                              regex;
    RouteHandler                            handler;
    size_t                                  depth;
    RouteParams                             params;
};

class Router {
public:
    Route*                                  match(const std::string& url);
    void                                    add_route(const std::string& route, RouteHandler handler);
    std::vector<Route>                      routes;
};

struct MessengerContext {
    bool                                    should_run;
    UriSocketMap                            known_nodes;
    QhmEndpoint *                           node_self;
    std::queue<Event>                       event_queue;
    Router                                  router;
    bool                                    verbose = false;
    UuidString                              uuid;
};

class Messenger {
public:
    Messenger(const Configuration p);
    void                                    run();

protected:
    virtual Status                          init();
    virtual Status                          after_init();
    virtual Status                          finalize();
    Status                                  register_msg_handler(ApplicationMessageType, MessageHandler);
    ApplicationMessageType                  register_msg_handler(MessageHandler);
    Status                                  register_evt_handler(EventType , EventHandler);
    EventType                               register_evt_handler(EventHandler);
    Status                                  deregister_msg_handler(ApplicationMessageType);
    Status                                  deregister_evt_handler(EventType);

    QhmEndpoint                             node_self;
    std::shared_ptr<MessengerContext>       context;
    Configuration                           configuration;
    int                                     timeout = SOCKET_TIMEOUT;

private:
    MessageHandler                          get_message_handler(ApplicationMessageType);
    EventHandler                            get_evt_handler(EventType);
    Status                                  process_message(const QhmSockets::Message &udp_message_in,
                                                            QhmSockets::Message *udp_message_out, QhmEndpoint *dest);
    Status                                  process_event();
    Status                                  process_http(http::Message* in, http::Message** out);

    QhmSockets::Socket *                    rtr_socket = nullptr;
    ApplicationMsgHandlersMap               app_msg_handlers;
    EventHandlersMap                        evt_handlers;
};

DECLARE_MESSAGE_HANDLER(default_handler, req, rep, ctx);
DECLARE_MESSAGE_HANDLER(terminate_handler, req, rep, ctx);

QhmSockets::Message         message_from_type(const QhmEndpoint& src, const std::string& url,uint32_t type = 0,
                                              const std::string& body = "", http_method m = HTTP_GET);
void                        dispatch_event(MessengerContext *ctx, const Event &evt);
bool                        validate_http_message(const http::Message *msg, const HttpHeaderSchema &schema);
Status                      transaction_commit(MessengerContext* context, const QhmSockets::Message &reply,
                                               const QhmEndpoint& dest);
Status                      error(MessengerContext* context, QhmSockets::Message *resp,
                                  const NodeTag& dest, uint32_t status);
NeighbourNode*              find_node_by_uri(const NodeTag &uri, const UriSocketMap * nodes,
                                             NeighbourNode **dest);
Status                      parse_http(const void *src, size_t len, http::Message **);
QhmEndpoint                 parse_url(const std::string&);
std::string                 parse_path(const std::string& url_string);
QhmEndpoint                 parse_qhm_endpoint(const std::string &);
std::string                 serialize_qhm_endpoint(const QhmEndpoint &);
QhmEndpoint                 qhm_endpoint_from_configuration(const Configuration& c);
NeighbourNode*              add_node(MessengerContext*,const QhmEndpoint &);
Status                      del_node(MessengerContext*,const QhmEndpoint &);
nlohmann::json              parse_params_in_token (const std::string &token);
nlohmann::json              parse_all_params_in_url(const std::string &url, const RouteParams &params_schema);
std::string                 generate_route_regex (const std::string &route);
RouteParams                 parse_param_ids_in_route (const std::string& url);
void                        generate_response(const http::Message *in, http::Message **out, http_status status);
http::Response              sync_send_request(http::Request *request, const QhmEndpoint &host,
                                              const QhmEndpoint &dest_node, int timeout = 300);
void                        async_send_request(http::Request *request, const QhmEndpoint &host,
                                               const QhmEndpoint &dest_node, int timeout = 300);

/* http headers schemas */

static const HttpHeaderSchema msg_schema = {
        HEADER_KEY_SERVICE_SRC,
        HEADER_KEY_SERVICE_DST,
//        HEADER_KEY_APP_MESSAGETYPE
};
static const HttpHeaderSchema event_schema = {
        HEADER_KEY_EVENT_TYPE
};


/* convenience functions */

static inline bool              headers_have(const headers_map& hdrs, const std::string& field){
    return (hdrs.find(field) != hdrs.end());
}

inline static size_t            url_depth(const std::string& url) { return split(url, '/').size(); }

inline static http::Response*   reply_back(const http::Message* m) {
    auto r = new http::Response();
    r->headers[HEADER_KEY_SERVICE_DST] = m->headers.at(HEADER_KEY_SERVICE_SRC);
    return r;
}

#endif //NEWCORE_MESSENGER_H
