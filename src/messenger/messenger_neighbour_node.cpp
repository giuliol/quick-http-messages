//
// Created by Giulio Luzzati on 10/09/18.
//
#include "messenger/messenger.h"

NeighbourNode::NeighbourNode(const QhmEndpoint &node) : QhmEndpoint(node) {
    socket = new QhmSockets::Socket();
    socket->setsockopt(SNDTIMEO, 100);
    core_try(socket->connect(endpoint);, core_err << "error connecting to socket " << endpoint;);
}

NeighbourNode::NeighbourNode(const QhmEndpoint &node, const std::string &src_ip) : QhmEndpoint(node) {
    socket = new QhmSockets::Socket();
    socket->setsockopt(SNDTIMEO, 100);
    core_try(socket->connect(endpoint, src_ip); , core_err << "error connecting to socket " << endpoint;);
}

NeighbourNode::~NeighbourNode() {
    core_try(socket->disconnect(endpoint), core_err << "error disconnecting to socket " << endpoint;);
    delete socket;
}

void _set_dst(http::Message* m, QhmEndpoint* n){
    m->headers[HEADER_KEY_SERVICE_DST] = serialize_qhm_endpoint(*n);
}

Status NeighbourNode::generate_request(http::Message ** m) {
    (*m) = new http::Request();
    _set_dst(*m, this);
    return CORE_OK;
}

Status NeighbourNode::generate_response(http::Message ** m) {
    (*m) = new http::Response();
    _set_dst(*m, this);
    return CORE_OK;
}
bool NeighbourNode::connected() {
    // implement ping!
    return true;
}