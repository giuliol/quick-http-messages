//
// Created by Giulio Luzzati on 20/07/18.
//

#ifndef NEWCORE_UTILS_H
#define NEWCORE_UTILS_H

#include <mutex>
#include <thread>
#include <random>
#include <sstream>
#include "messenger/messenger.h"
#include "uuid/uuid.h"

#define __rand64__          ((uint64_t) rand()) << 32 | rand()


static std::string _big_string() {
    int num(0);
    std::string ret("RST");
    while (num++ < 500000)
        ret.push_back((char) rand() % 254);
    return ret;
}

struct client_fixture {


    client_fixture() : send_socket(), recv_socket() {}

    client_fixture(const QhmEndpoint &in) : client_fixture() {
        self = in;
        set_recv_endpoint(self.endpoint);
    }

    void set_send_endpoint(SockEndpoint e) {
        if (!send_endpoint.empty()) {
            send_socket.disconnect(send_endpoint);
            send_socket = QhmSockets::Socket();
        }
        send_endpoint = e;
        send_socket.connect(send_endpoint);
    }

    void set_recv_endpoint(SockEndpoint e) {
        recv_endpoint = e;
        recv_socket.setsockopt(RCVTIMEO, 10000);
        recv_socket.bind(e);
    }

    void send_and_recv(QhmSockets::Message *msg) {
        send(msg);
        recv();
    }

    void send(QhmSockets::Message *msg) {
        msg->send(send_socket);
    }

    void recv() {
        QhmSockets::Message in;
        in.recv(recv_socket);
        ready = false;
        reply.rebuild(in.str());
        ready = true;
        /* what(); */}

    bool is200() {
        if (!ready) return false;
        parcel = http::parse_response(reply.data(), reply.size());
        if (validate_http_message(&parcel, msg_schema)) if (parcel.status == 200) return true;
        return false;
    }

    void what() {
        if (!ready) {
            core_warn << "client recv buffer not ready";
            return;
        }
        core_log << "client received reply: \n" << ANSI_COLOR_YELLOW << reply.str() << ANSI_COLOR_RESET;
    }

    void kill_remote() {
        auto msg = message_from_type(self, "/api", SERVICE_TERMINATE);
        send(&msg);
    }

    void kill_remote(const QhmEndpoint &node) {
        auto msg = message_from_type(self, "/api", SERVICE_TERMINATE);
        set_send_endpoint(node.endpoint);
        send(&msg);
    }

    void call_api(const std::string &url, const std::string &body) {
        std::vector<std::string> path = split(url, '/');
        std::string service_uri = path.front();
        std::string resource_path = url.substr(service_uri.length());
        auto msg = message_from_type(self, resource_path);
        set_send_endpoint(known_nodes.at(service_uri).endpoint);
        send_and_recv(&msg);
    }

    void add_known_node(const QhmEndpoint &node) {
        known_nodes[node.tag] = node;
    }


    http::Response parcel;
    QhmSockets::Socket send_socket;
    QhmSockets::Socket recv_socket;
    QhmSockets::Message reply;
    bool ready;
    SockEndpoint send_endpoint = "";
    SockEndpoint recv_endpoint;
    QhmEndpoint self;
    Status rv;
    std::map<std::string, QhmEndpoint> known_nodes;
};


static void kill_worker(const SockEndpoint &endpoint) {
    client_fixture c;
    c.set_send_endpoint(endpoint);
    c.kill_remote();
}

namespace __test_utils {
    static int success;
    static std::vector<double> latency_entries;
    static std::mutex g_i_mutex;  // protects g_i
    static std::mutex tid_mutex;  // protects g_i
    static int thread_id = 0;
}

static void __update_avg(double t, int howmany) {
    std::lock_guard<std::mutex> lock(__test_utils::g_i_mutex);
    __test_utils::success += howmany;
    __test_utils::latency_entries.push_back(t);
}

static double __get_avg_latency(){
    double sum = 0;
    for(auto el : __test_utils::latency_entries)
        sum += el;
    return sum / __test_utils::latency_entries.size();
}


static int __get_thread_id() {
    std::lock_guard<std::mutex> lock(__test_utils::tid_mutex);
    return ++__test_utils::thread_id;
}


template<typename _Callable, typename... _Args>

static void _spawn_threads(int threads, _Callable &&f, _Args &&... args) {
    using std::thread;
    using std::vector;
    vector<thread> thread_pool;
    int c(threads);
    while (c--)
        thread_pool.push_back(thread(f, args...));

//    while (!thread_pool.empty()) {
//        auto current = thread_pool.begin();
//        current->join();
//        thread_pool.erase(current);
//    }
    for(auto&& th:thread_pool)
        th.join();
}

static double _toc(const std::chrono::high_resolution_clock::time_point &tic) {
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(
            std::chrono::high_resolution_clock::now() - tic);
    return time_span.count();
}

#define do_test( arg ) std::cout << "\n\n:::::::::::: " << #arg << " ::::::::::::\n\n" << std::endl; assert(arg);
bool apitree_test();
bool benchmark(int, int);

static void kill_node(QhmEndpoint w){

    NeighbourNode n(w);
    QhmSockets::Socket   recv_socket;
    recv_socket.setsockopt(RCVTIMEO, 300);

    http::Response response;
    response.status = HTTP_STATUS_SERVICE_UNAVAILABLE;

    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 generator(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<int> port_dist(RANDOM_CLIENT_PORT_RANGE_START, RANDOM_CLIENT_PORT_RANGE_END);
    std::uniform_int_distribution<int> ip_dist(2, 254);


    int max_trials = 300;
    bool bound(false);
    std::string local_endpoint, ip;

    while (max_trials--) {
        auto randport = port_dist(generator);
        auto ss = std::stringstream();
        ss << "127." << ip_dist(generator) << "." << ip_dist(generator) << "." << ip_dist(generator);
        ip = ss.str();
        local_endpoint = ip + ":" + std::to_string(randport);
        try {
            recv_socket.bind(local_endpoint);
            max_trials = 0;
            bound = true;
        }
        catch (std::exception e) {};
    }

    core_assert(bound, "[kill node] cannot find a port to bind to";
            return);

    Uuid uuid = generate_uuid();
    QhmEndpoint src_node = {"", QHM_DEFAULT_REQUEST_PORT, uuid.pretty_print()};

    auto udpmsg = message_from_type(src_node, "/api", SERVICE_TERMINATE);
    udpmsg.send(*n.socket);

}

#endif //NEWCORE_UTILS_H
