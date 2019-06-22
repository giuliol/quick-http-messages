//
// Created by Giulio Luzzati on 27/09/18.
//

#include <thread>
#include <zconf.h>
#include "messenger/messenger.h"
#include "utils/tutorial_time_service.h"
#include "utils/test_utils.h"

Configuration time_service_configuration = {
        {CONFIG_KEY_SELF_IP,    "127.0.0.10"},
        {CONFIG_KEY_PORT,       std::to_string(QHM_DEFAULT_SERVICE_PORT)},
        {CONFIG_KEY_TAG,        "time_service"}
};

Configuration client_configuration = {
        {CONFIG_KEY_SELF_IP,    "127.0.0.11"},
        {CONFIG_KEY_PORT,       std::to_string(QHM_DEFAULT_SERVICE_PORT)},
        {CONFIG_KEY_TAG,        "client_node"}
};



int main(){

    std::thread service ([](){
        Configuration  p;
        p["verbose"] = "true";
        p.incorporate(time_service_configuration);
        TimeService timeService(p);
        timeService.run();
    });

    usleep(1000);

    auto time_service_node = qhm_endpoint_from_configuration(time_service_configuration);
    auto client_node = qhm_endpoint_from_configuration(client_configuration);

    http::Request request;
    request.path = "/api/v1/get_time";
    request.method = HTTP_GET;
        async_send_request(&request, client_node, time_service_node);
    http::Response response = sync_send_request(&request, client_node, time_service_node);
    assert(response.status == HTTP_STATUS_OK);

    request.path = "/api/v1/gets_time";
    response = sync_send_request(&request, client_node, time_service_node);
    assert(response.status == HTTP_STATUS_NOT_FOUND);

    kill_node(time_service_node);

    service.join();

    return 0;
}