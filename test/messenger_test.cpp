//
// Created by Giulio Luzzati on 19/07/18.
//

#include <cstdio>
#include <thread>
#include <zconf.h>
#include <cmath>
#include "utils/tutorial_time_service.h"
#include "utils/test_utils.h"
#include "utils/tutorial_relay_service.h"

Configuration time_service_configuration = {
        {CONFIG_KEY_SELF_IP,    "127.0.0.10"},
        {CONFIG_KEY_PORT,       std::to_string(QHM_DEFAULT_SERVICE_PORT)},
        {CONFIG_KEY_TAG,        "time_service"}
};

Configuration relay_service_configuration = {
        {CONFIG_KEY_SELF_IP,    "127.0.0.11"},
        {CONFIG_KEY_PORT,       std::to_string(QHM_DEFAULT_SERVICE_PORT)},
        {CONFIG_KEY_TAG,        "relay_service"}
};

Configuration client1_configuration = {
        {CONFIG_KEY_SELF_IP,    "127.0.0.20"},
        {CONFIG_KEY_PORT,       std::to_string(QHM_DEFAULT_SERVICE_PORT)},
        {CONFIG_KEY_TAG,        "client1"}
};

Configuration client2_configuration = {
        {CONFIG_KEY_SELF_IP,    "127.0.0.21"},
        {CONFIG_KEY_PORT,       std::to_string(QHM_DEFAULT_SERVICE_PORT)},
        {CONFIG_KEY_TAG,        "client2"}
};

auto relay_service_node = qhm_endpoint_from_configuration(relay_service_configuration);
auto time_service_node  = qhm_endpoint_from_configuration(time_service_configuration);
auto client1_node       = qhm_endpoint_from_configuration(client1_configuration);

bool apitree_test(){
    std::thread service([&]() {
        Configuration  p { {"verbose", "true"} };
        p.incorporate(time_service_configuration);
        TimeService a(p);
        a.run();
        core_ok << "terminating service";
    });

    usleep(500000);

    http::Request request;
    request.path = "/api/v1/get_time?key=value";
    request.method = HTTP_GET;
    http::Response response = sync_send_request(&request, client1_node, time_service_node, 10000);
    assert(response.status == HTTP_STATUS_OK);

    kill_node(time_service_node);
    service.join();
    return true;
}

bool tutorial_test(){
    bool ret(true);

    std::thread t_service([&]() {
        TimeService a(time_service_configuration);
        a.run();
        core_ok << "terminating time service";
    });

    std::thread r_service([&]() {
        Configuration parameters;
        parameters["signature"] = "[A wonderful signature]";
        parameters.incorporate(relay_service_configuration);
        RelayService a(parameters);
        a.run();
        core_ok << "terminating relay service";
    });

    usleep(500000);

    http::Request advertisement;
    advertisement.path = "/advertise";
    advertisement.method = HTTP_PUT;
    advertisement.body = serialize_qhm_endpoint(time_service_node);
    auto resp = sync_send_request(&advertisement, client1_node, relay_service_node);
    assert(resp.status == HTTP_STATUS_CREATED);

    nlohmann::json body; body["callbackReference"] = time_service_node.endpoint + "/api/v1/get_time";

    http::Request request;
    request.path = "/api/v1/sync_relay/imsi-23592000001?key1=val1&key2=val2";
    request.method = HTTP_GET;
    request.body = body.dump();
    resp = sync_send_request(&request, client1_node, relay_service_node);
    assert(resp.status == HTTP_STATUS_OK);
    core_log << resp.body;

    request.path = "/api/v1/sync_relay/imsi-23592000001/add_sign";
    request.method = HTTP_GET;
    resp = sync_send_request(&request, client1_node, relay_service_node);
    assert(resp.status == HTTP_STATUS_OK);
    core_log << resp.body;

    kill_node(relay_service_node);
    kill_node(time_service_node);

    r_service.join();
    t_service.join();
    return ret;
}



int main() {

    do_test(apitree_test());
    do_test(tutorial_test());

    // you need a large number to get a correct estimate (overhead weighs less), keeping the number low to make tests faster
//    int num = 100000;
    int num = 100;

#define  BENCHMARK
#ifdef BENCHMARK
    do_test(benchmark(num, 1));
    do_test(benchmark(num, 10));
    do_test(benchmark(num*2, 50));
#endif

    return 0;
}





















std::string IPAddressToString(int ip)
{
    char result[16];

    sprintf(result, "%d.%d.%d.%d",
            (ip >> 24) & 0xFF,
            (ip >> 16) & 0xFF,
            (ip >>  8) & 0xFF,
            (ip      ) & 0xFF);

    return std::string(result);
}


bool benchmark(int num, int threads) {
    using namespace std::chrono;

    std::thread service([&]() {
        TimeService a(time_service_configuration);
        a.run();
        core_ok << "terminating service";
    });

    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    __test_utils::success = 0;

    int tot(num);

    auto __api_msg = [](const std::string& url, const QhmEndpoint& self){
        std::vector<std::string> path = split(url, '/');
        std::string service_uri = path.front();
        std::string resource_path = url.substr(service_uri.length());
        auto msg = message_from_type(self, resource_path);
        return msg;
    };

    auto ping = [&__api_msg](int a) -> bool {
        int thread_id = __get_thread_id();

        QhmEndpoint node = {
                "127.0.10." + std::to_string(15),
                8000 + thread_id,
                "client_" + std::to_string(thread_id)
        };
        auto api_msg = __api_msg("time_service/api/v1/ping", node);

        client_fixture client(node);
        client.set_send_endpoint(time_service_node.endpoint);
        client.known_nodes["time_service"] = time_service_node;
        client.set_send_endpoint(time_service_node.endpoint);
        usleep(10000);

        int ok(0);

        high_resolution_clock::time_point _t1 = high_resolution_clock::now();
        while (a--) {
            client.send_and_recv(&api_msg);
            if (client.is200()) ok++;
        }
        __update_avg(_toc(_t1) / ok, ok);
        return true;
    };

    int PINGS_PER_THREAD = (int) (floor(num / threads));
    int remainder = num - (PINGS_PER_THREAD * threads);

    core_log << "" << num << " pings, " << threads << " threads, "<< PINGS_PER_THREAD << " pings per thread";

    if (remainder == 0) {
        _spawn_threads(threads, ping, PINGS_PER_THREAD);
    } else {
        _spawn_threads(threads - 1, ping, PINGS_PER_THREAD);
        _spawn_threads(1, ping, remainder);
    }

    printf("%d threads: %d/%d success\n", threads, __test_utils::success, tot);
    printf("avg. latency:  %f\n", __get_avg_latency());

    auto relay_service_node = qhm_endpoint_from_configuration(relay_service_configuration);
    auto time_service_node = qhm_endpoint_from_configuration(time_service_configuration);
    auto client1_node  = qhm_endpoint_from_configuration(client1_configuration);

    kill_node(time_service_node);

    service.join();
    return true;
}