//
// Created by Giulio Luzzati on 20/07/18.
//

#ifndef NEWCORE_UDPLIB_H
#define NEWCORE_UDPLIB_H

#include <netinet/in.h>
#include <cerrno>
#include <cstring>
#include <cassert>
#include <zconf.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include "common.h"
#include <list>
#include <algorithm>

typedef unsigned char byte;           //  Single unsigned byte = 8 bits

typedef struct {
    //  Own address
    struct sockaddr_in address;
    //  Broadcast address
    struct sockaddr_in broadcast;
} iface_t;

typedef struct {
    int handle;                 //  Socket for send/recv
    int port;                   //  UDP port we work on
    std::list<iface_t> ifaces;
} udp_t;

//  Handle error from I/O operation

static void
s_handle_io_error(const std::string reason) {
#   ifdef __WINDOWS__
    switch (WSAGetLastError ()) {
        case WSAEINTR:        errno = EINTR;      break;
        case WSAEBADF:        errno = EBADF;      break;
        case WSAEWOULDBLOCK:  errno = EAGAIN;     break;
        case WSAEINPROGRESS:  errno = EAGAIN;     break;
        case WSAENETDOWN:     errno = ENETDOWN;   break;
        case WSAECONNRESET:   errno = ECONNRESET; break;
        case WSAECONNABORTED: errno = EPIPE;      break;
        case WSAESHUTDOWN:    errno = ECONNRESET; break;
        case WSAEINVAL:       errno = EPIPE;      break;
        default:              errno = GetLastError ();
    }
#   endif
    if (errno == EAGAIN
        || errno == ENETDOWN
        || errno == EPROTO
        || errno == ENOPROTOOPT
        || errno == EHOSTDOWN
        || errno == ENONET
        || errno == EHOSTUNREACH
        || errno == EOPNOTSUPP
        || errno == ENETUNREACH
        || errno == EWOULDBLOCK
        || errno == EINTR)
        return;             //  Ignore error and try again
    else if (errno == EPIPE
             || errno == ECONNRESET)
        return;             //  Ignore error and try again
    else {
        core_err << "E: (udp) error '"<< strerror(errno) <<"' on " << reason;
        assert(false);
    }
}

//  -----------------------------------------------------------------
//  Constructor

static udp_t *
udp_new(int port) {
    udp_t *self = new udp_t();
    self->port = port;

    //  Create UDP socket
    self->handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (self->handle == -1)
        s_handle_io_error("socket");

    //  Ask operating system to let us do broadcasts from socket
    int on = 1;
    if (setsockopt(self->handle, SOL_SOCKET,
                   SO_BROADCAST, &on, sizeof(on)) == -1)
        s_handle_io_error("setsockopt (SO_BROADCAST)");

    //  Bind UDP socket to local port so we can receive pings
    struct sockaddr_in _sockaddr = {0};
    _sockaddr.sin_family = AF_INET;
    _sockaddr.sin_port = htons(self->port);
    _sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(self->handle, (struct sockaddr *) &_sockaddr, sizeof(_sockaddr)) == -1)
        s_handle_io_error("bind");


    struct ifaddrs *interfaces;
    if (getifaddrs(&interfaces) == 0) {
        struct ifaddrs *interface = interfaces;
        while (interface) {
            if (interface->ifa_addr)
                //  Hopefully the last interface will be WiFi
                if (interface->ifa_addr->sa_family == AF_INET) {
                    self->ifaces.push_back(iface_t());
                    self->ifaces.back().address = *(struct sockaddr_in *) interface->ifa_addr;
                    self->ifaces.back().broadcast = *(struct sockaddr_in *) interface->ifa_broadaddr;
                    self->ifaces.back().broadcast.sin_port = htons(self->port);
                }
            interface = interface->ifa_next;
        }
    }
    freeifaddrs(interfaces);
    return self;
}

//  -----------------------------------------------------------------
//  Destructor

static void
udp_destroy(udp_t **self_p) {
    assert (self_p);
    if (*self_p) {
        udp_t *self = *self_p;
        close(self->handle);
        delete *self_p;
        *self_p = NULL;
    }
}

//  Returns UDP socket handle

static int
udp_handle(udp_t *self) {
    assert (self);
    return self->handle;
}

//  Send message using UDP broadcast

static void
udp_send(udp_t *self, byte *buffer, size_t length) {
//    inet_aton("255.255.255.255", &self->broadcast.sin_addr);
    for (auto &&iface:self->ifaces) {
        if (sendto(self->handle, buffer, length, 0,
                   (struct sockaddr *) &iface.broadcast, sizeof(struct sockaddr_in)) == -1)
            s_handle_io_error("sendto");
    }

}

//  Receive message from UDP broadcast
//  Returns size of received message, or -1

static bool
udp_recv(udp_t *self, byte *buffer, size_t length) {
    struct sockaddr_in sockaddr;
    socklen_t si_len = sizeof(struct sockaddr_in);

    auto &ifaces = self->ifaces;
    ssize_t size = recvfrom(self->handle, buffer, length, 0, (struct sockaddr *) &sockaddr, &si_len);
    if (size == -1)
        s_handle_io_error("recvfrom");
    else
        return std::find_if(ifaces.begin(), ifaces.end(),
                            [&sockaddr](const iface_t &iface) {
                                return sockaddr.sin_addr.s_addr == iface.address.sin_addr.s_addr;
                            }) == ifaces.end();

}


static void udp_broadcast_endpoint(uint32_t port, const std::string &endpoint) {
    udp_t *udp = udp_new(port);
    udp_send(udp, (byte *) endpoint.data(), endpoint.size());
    udp_destroy(&udp);
}

#endif //NEWCORE_UDPLIB_H
