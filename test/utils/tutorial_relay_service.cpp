//
// Created by Giulio Luzzati on 28/08/18.
//

#include <base64/base64.h>
#include <json/single_include/nlohmann/json.hpp>
#include "uuid/uuid.h"
#include "tutorial_relay_service.h"
#include "example_common_types.h"

DECLARE_EVENT_HANDLER(high_mark, params, ctx){
    auto subcontext = (RelayService_Context*)ctx;
    auto evt_type = subcontext->events[HIGH_MARK_EVT];
    for(auto&& callback : subcontext->callbacks[evt_type])
        process_callback(ctx, callback.get());

    return CORE_CONTINUE;
}

DECLARE_ROUTE_HANDLER(synchronous_relay_handler, in, out, params, ctx) {
    Status rv;
    auto subcontext = (RelayService_Context*)ctx;

    http::Request request;
    core_assert(ctx->known_nodes.find("time_service") != ctx->known_nodes.end(), return CORE_CONTINUE);
    auto &&time_service = ctx->known_nodes.at("time_service");

    // if there are params for this route, append them in the get request
    std::string path = json_has_field(params["imsi"], "params") ?
                       form_get_request("/api/v1/get_time", params["imsi"]["params"]) : "/api/v1/get_time";
    request.path = path;
    request.body = "[the body from the relay service]";
    request.method = HTTP_GET;

    auto body = json_try_parsing(in->body, rv);
    core_assert(rv == CORE_OK, return CORE_CONTINUE);
    core_assert(json_has_field(body, "callbackReference"), return CORE_CONTINUE);

    auto addendum = std::make_shared<HighmarkCallback>(HighmarkCallback());

    addendum->body = subcontext->uuid;
    addendum->callback_uri = body["callbackReference"].get<std::string>();

    subcontext->add_callback(addendum, subcontext->events[HIGH_MARK_EVT]);

    auto response = sync_send_request(&request, *ctx->node_self, *time_service);

    (*out) = reply_back(in);
    auto resp_out = __as_response(*out);
    resp_out->status = response.status;
    resp_out->body = response.body;

    core_assert(headers_have(response.headers, "count"), return CORE_OK;);
    int count;
    core_try(count = std::stoi(response.headers["count"]), return CORE_OK;);
    if(count >= 1) {
        Event evt(subcontext->events.at(HIGH_MARK_EVT));
        evt.params["count"] = count;
        dispatch_event(ctx, evt);
    }

    return CORE_OK;
}

DECLARE_ROUTE_HANDLER(synchronous_relay_add_sign_handler, in, out, params, ctx) {
    Status rv;
    rv = synchronous_relay_handler(ctx, params, in, out);
    auto relay_context = static_cast<RelayService_Context*>(ctx);

    core_assert(rv == CORE_OK, return CORE_GENERIC_ERROR;);
    if((*out)->type == http::http_message_type::RESPONSE) {
        (*out)->body += "\n" + relay_context->signature;
        __as_response(*out)->status = HTTP_STATUS_OK;
    }
    return rv;
}

DECLARE_ROUTE_HANDLER(advertise, in, out, params, ctx) {
    (*out) = new http::Response();
    auto req = __as_request(in);
    auto resp = __as_response(*out);
    core_assert(req->type == http::REQUEST, resp->status = HTTP_STATUS_BAD_REQUEST; return CORE_OK;);
    core_assert(req->method == HTTP_PUT, resp->status = HTTP_STATUS_METHOD_NOT_ALLOWED; return CORE_OK;);
    auto newnode = parse_qhm_endpoint(req->body);
    core_assert(!newnode.endpoint.empty(), resp->status = HTTP_STATUS_BAD_REQUEST; return CORE_OK;);
    add_node(ctx, newnode);
    resp->headers[HEADER_KEY_SERVICE_DST] = req->headers[HEADER_KEY_SERVICE_SRC];
    resp->status = HTTP_STATUS_CREATED;
    return CORE_OK;
}
/*--------------------------------------------------------------------------------------------------------------------*/


/* THIS will be autogenerated ----------------------------------------------------------------------------------------*/
Status RelayService::init() {
    context = std::make_shared<RelayService_Context>(RelayService_Context());
    context->router.add_route("/api/v1/sync_relay/{imsi}", &synchronous_relay_handler);
    context->router.add_route("/api/v1/sync_relay/{imsi}/add_sign", &synchronous_relay_add_sign_handler);
    context->router.add_route("/advertise", &advertise);

    auto relay_context = static_cast<RelayService_Context*>(context.get());
    relay_context->uuid = generate_uuid().pretty_print();

    auto signature = configuration.safe_at("signature");
    if(!signature.empty())
        relay_context->signature = signature;
    else
        relay_context->signature = "[SIGNATURE FROM RELAY SERVICE]";

    relay_context->events[HIGH_MARK_EVT] = register_evt_handler(high_mark);

    return Messenger::init();
}

Status RelayService::finalize(){
    return Messenger::finalize();
}
/*--------------------------------------------------------------------------------------------------------------------*/

