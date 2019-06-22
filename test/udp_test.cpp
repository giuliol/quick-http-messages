//
// Created by Giulio Luzzati on 20/07/18.
//

#include <cassert>
#include <core/udplib.h>
#include <thread>

using std::thread;

#define PING_PORT_NUMBER 9999
#define PING_MSG_SIZE    5
#define PING_INTERVAL    1000  //  Once per second

bool test1() {
    udp_t *udp = udp_new(PING_PORT_NUMBER);

    byte tx_buf[PING_MSG_SIZE];
    byte rx_buf[PING_MSG_SIZE];

    std::string msg = "PING";
    memcpy(tx_buf, msg.data(), 5);

    thread rx([&](){
        bool rv = udp_recv(udp, rx_buf, PING_MSG_SIZE);
        core_log << "received "<< rx_buf <<": valid " << strncmp((char*)rx_buf, msg.data(), 5) << ", else " << rv;
    });

    thread tx([&](){
        usleep(500000);
        udp_send(udp, tx_buf, PING_MSG_SIZE);
    });

    rx.join();
    tx.join();
    udp_destroy(&udp);
    return true;
}

#include "udp/udp.h"

bool test3(){
    using namespace QhmSockets;

    std::thread server([](){
        Socket insocket;
        insocket.bind("127.0.0.2:50501");
        insocket.setsockopt(RCVTIMEO, 2000);
        Message in;
        in.recv(insocket);
        core_ok << in.str() << " from " << in.sender_ip();
        assert(in.str() == "WOWZA");
        in.recv(insocket);
        insocket.unbind("");
    });

    usleep(1000000);

    Socket outsocket;
//    outsocket.connect("127.0.0.2:50501");
    outsocket.connect("127.0.0.2:50501", "127.0.0.3");
    Message out; out.rebuild("WOWZA");

    out.send(outsocket);

    outsocket.disconnect("");

    server.join();

    return true;
};

int main() {
//    assert(test1());
//    assert(test2());
    assert(test3());
    return 0;
}