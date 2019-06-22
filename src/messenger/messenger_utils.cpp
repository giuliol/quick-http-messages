//
// Created by Giulio Luzzati on 10/09/18.
//

#include <random>
#include "uuid/uuid.h"
#include "messenger/messenger.h"

QhmSockets::Message message_from_type(
        const QhmEndpoint& src, const std::string& url, uint32_t type, const std::string& body, http_method method) {
    QhmSockets::Message ret;
    http::Request req;
    req.method = method;
    req.path = url;
    if(type) req.headers[HEADER_KEY_APP_MESSAGETYPE] = std::to_string(type);
    req.headers[HEADER_KEY_SERVICE_DST] = serialize_qhm_endpoint({"", 1, "null"});
    req.headers[HEADER_KEY_SERVICE_SRC] = serialize_qhm_endpoint(src);
    req.body = body;
    std::string buf = http::serialize(&req);
    ret.rebuild(buf);
    return ret;
}

QhmEndpoint parse_qhm_endpoint(const std::string &in){
    QhmEndpoint ret;
    bool valid_endpoint = true;
    nlohmann::json obj;
    core_try(obj = nlohmann::json::parse(in), valid_endpoint = false;);
    core_assert(valid_endpoint, core_warn << in << " invalid"; return ret);
    ret.tag = obj[MESSENGER_NODE_TAG];
    ret.endpoint = obj[MESSENGER_NODE_ENDPOINT];
    valid_endpoint = valid_endpoint && !ret.tag.empty() && ! ret.endpoint.empty();
    core_assert(valid_endpoint, );
    return ret;
}

std::string serialize_qhm_endpoint(const QhmEndpoint &node){
    nlohmann::json obj;
    obj[MESSENGER_NODE_ENDPOINT] = node.endpoint;
    obj[MESSENGER_NODE_TAG] = node.tag;
    return obj.dump();
}

Status parse_http(const void *src, size_t len, http::Message ** http_in) {
    *http_in = new http::Request(http::parse_request(src, len));
    if (validate_http_message(*http_in, msg_schema)) return CORE_OK;
    http::http_free(*http_in);
    *http_in = new http::Response(http::parse_response(src, len));
    if (validate_http_message(*http_in, msg_schema)) return CORE_OK;
    http::http_free(*http_in);
    return CORE_GENERIC_ERROR;
}

void generate_response(const http::Message *request, http::Message **out, http_status status){
    *out = new http::Response();
    auto response = __as_response(*out);
    response->headers[HEADER_KEY_SERVICE_DST] = request->headers.at(HEADER_KEY_SERVICE_SRC);
    response->status = status;
}

int _random_endpoint(QhmSockets::Socket& recv_socket, const std::string& ip, std::string& local_endpoint) {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist(RANDOM_CLIENT_PORT_RANGE_START, RANDOM_CLIENT_PORT_RANGE_END);

    int max_trials = 300;
    while (max_trials--){
        auto randport = dist(mt);
        local_endpoint = ip + ":" + std::to_string(randport);
        recv_socket.bind(local_endpoint);
        if(recv_socket.is_bound()) return randport;
    }

    return 0;
};

http::Response sync_send_request(http::Request *request, const QhmEndpoint &host, const QhmEndpoint &dest_node,
                                 int timeout){
    NeighbourNode n(dest_node, host.ip_address);
    n.socket->setsockopt(SNDTIMEO, timeout);

    QhmSockets::Socket   recv_socket;
    recv_socket.setsockopt(RCVTIMEO, timeout);

    http::Response response;
    response.status = HTTP_STATUS_SERVICE_UNAVAILABLE;

    std::string local_endpoint;
    int port = _random_endpoint(recv_socket, host.ip_address, local_endpoint);
    core_assert(port, core_err << "[send request] cannot find a port to bind to"; return response;);

    QhmEndpoint src_node = { host.ip_address, port, "temp_" + host.tag };

    request->headers[HEADER_KEY_SERVICE_SRC] = serialize_qhm_endpoint(src_node);
    request->headers[HEADER_KEY_SERVICE_DST] = serialize_qhm_endpoint(dest_node);

    core_assert(validate_http_message(request, msg_schema), return response;);

    QhmSockets::Message udpmsg = http::serialize(request);
    QhmSockets::Message in;

    udpmsg.send(*n.socket);
    in.recv(recv_socket);

    if(in.empty())
    {core_err << "[send request] timed out"; response.status = HTTP_STATUS_REQUEST_TIMEOUT;
        return response;}

    response = http::parse_response(in.data(), in.size());

    return response;
}

void async_send_request(http::Request *request, const QhmEndpoint &host, const QhmEndpoint &dest_node,
                                 int timeout){
    NeighbourNode n(dest_node, host.ip_address);
    n.socket->setsockopt(SNDTIMEO, timeout);

    QhmSockets::Socket   recv_socket;
    recv_socket.setsockopt(RCVTIMEO, timeout);

    http::Response response;
    response.status = HTTP_STATUS_SERVICE_UNAVAILABLE;

    std::string local_endpoint;
    int port = _random_endpoint(recv_socket, host.ip_address, local_endpoint);
    core_assert(port, core_err << "[send request] cannot find a port to bind to"; return;);

    QhmEndpoint src_node = { host.ip_address, port, "temp_" + host.tag };

    request->headers[HEADER_KEY_SERVICE_SRC] = serialize_qhm_endpoint(src_node);
    request->headers[HEADER_KEY_SERVICE_DST] = serialize_qhm_endpoint(dest_node);

    assert(validate_http_message(request, msg_schema));
    core_assert(validate_http_message(request, msg_schema), return;);

    QhmSockets::Message udpmsg = http::serialize(request);

    udpmsg.send(*n.socket);
}