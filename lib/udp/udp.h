//
// Created by giulio on 05/10/18.
//

#ifndef NEWCORE_UDP_H
#define NEWCORE_UDP_H

// UDP Client Server -- send/receive UDP packets
// Copyright (C) 2013  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#ifndef SNAP_UDP_CLIENT_SERVER_H
#define SNAP_UDP_CLIENT_SERVER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdexcept>
#include <map>

namespace QhmSockets
{

#define BUFFER_LEN  65535
#define RCVTIMEO    1
#define SNDTIMEO    2

const static std::string ECHO_PKT =
        R"(::::::::::::::::::::::::UDP_ECHO::::::::::::::::::::::::)";

const static std::string ECHO_REPLY =
            R"(::::::::::::::::::::::::UDP_ECHO_REPLY::::::::::::::::::::::::)";

    class udp_client_server_runtime_error : public std::runtime_error
    {
    public:
        udp_client_server_runtime_error(const char *w) : std::runtime_error(w) { }
    };


    class udp_client
    {
    public:
        udp_client(const std::string& addr, int port);
        udp_client(const std::string& addr, int port, const std::string& outaddr);
        ~udp_client();

        int                 get_socket() const;
        int                 get_port() const;
        std::string         get_addr() const;
        void                setip(const std::string&ip);

        int                 send(const char *msg, size_t size);

    private:
        int                 f_socket;
        int                 f_port;
        std::string         f_addr;
        std::string         o_addr;

        struct addrinfo *   f_addrinfo;
    };


    class udp_server
    {
    public:
        udp_server(const std::string& addr, int port);
        ~udp_server();

        int                 get_socket() const;
        int                 get_port() const;
        std::string         get_addr() const;

        int                 recv(char *msg, size_t max_size);
        int                 timed_recv(char *msg, size_t max_size, int max_wait_ms);
        int                 timed_recvfrom(char *msg, size_t max_size, int max_wait_ms, sockaddr* addr, socklen_t* len);


    private:
        int                 f_socket;
        int                 f_port;
        std::string         f_addr;
        struct addrinfo *   f_addrinfo;
    };

    class Socket {
    public:
        Socket();
        ~Socket();
        void bind(const std::string& endpoint);
        void unbind(const std::string& endpoint);
        bool is_bound() const;
        void connect(const std::string& endpoint);
        void connect(const std::string& endpoint, const std::string& ip);
        void disconnect(const std::string& endpoint);
        void close();
        void setsockopt(int key, int val);
        std::string recv();
        std::string recv(int timeout);
        void send(const std::string& buf);
        std::string sender_endpoint() const;

    private:
        bool                parse_endpoint(const std::string& endpoint);
        std::string         address;
        int                 port;
        udp_server*         server;
        udp_client*         client;
        bool                server_initialized = false;
        bool                client_initialized = false;
        std::map<int,int>   options;
        char                buffer[65535];
        std::string         sender;
        std::string         advertised_ip;
    };

    class Message {
    public:
        Message() = default;
        Message(const std::string& arg): buffer(arg) {}
        int                 recv(Socket& socket);
        int                 recv(Socket &socket, int timeout);
        void                send(Socket& socket) const;
        void                rebuild(const void* data, size_t len);
        void                rebuild(const std::string& in);
        void *              data() const;
        size_t              size() const;
        std::string         str() const;
        std::string         sender_ip() const;
        bool                empty() const;
    private:
        std::string         buffer;
        std::string         sender;
    };

    typedef Message multipart_t;

} // namespace udp_client_server
#endif
// SNAP_UDP_CLIENT_SERVER_H
// vim: ts=4 sw=4 et

#endif //NEWCORE_UDP_H
