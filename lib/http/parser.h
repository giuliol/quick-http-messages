#ifndef custom_parser_h
#define custom_parser_h

#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include "http_parser_core.h"
#include "http_types.h"

namespace http {
    class parser {

        friend std::ofstream &operator<<(std::ofstream &out, const parser &f);
        friend std::ostream &operator<<(std::ostream &out, const parser &f);

    private:
        http_parser_core        core;
        http_parser_settings    settings;
        std::string             method;

    public:
        const std::string &get_method() const;
        const std::string &get_url() const;

    private:
        std::string url;

        std::string request_address;
        std::string response_address;

        std::string request;
        std::string request_header;
        std::string request_body;

        bool        request_complete_flag;

        std::string response;
        std::string response_header;
        std::string response_body;

        bool        response_complete_flag;

        std::string temp_header_field;
        bool        gzip_flag;
        std::string host;
        headers_map request_headers;
        headers_map response_headers;

    public:
        parser();
        bool                parse(const std::string &body, enum http_parser_type type, Message** result);

        const std::string&  get_response_body() const;
        const std::string&  get_request_body() const;

        uint32_t            get_status() const;

        inline bool         is_response_complete() const {
            return response_complete_flag;
        }

        inline bool         is_request_complete() const {
            return request_complete_flag;
        }

        inline bool         is_request_address(const std::string &address) const {
            return request_address == address;
        }

        void                set_addr(const std::string &src_addr, const std::string &dst_addr);
        bool                filter_url(const std::regex *url_filter, const std::string &url);
        void                save_http_request(
                const std::regex *url_filter, const std::string &output_path, const std::string &join_addr);
        const headers_map&  get_request_headers_map();
        const headers_map&  get_response_headers_map();
        static int          on_url(http_parser_core *the_parser, const char *at, size_t length);
        static int          on_header_field(http_parser_core *the_parser, const char *at, size_t length);
        static int          on_header_value(http_parser_core *the_parser, const char *at, size_t length);
        static int          on_headers_complete(http_parser_core *the_parser);
        static int          on_body(http_parser_core *the_parser, const char *at, size_t length);
        static int          on_message_complete(http_parser_core *the_parser);
    };

    std::ostream &operator<<(std::ostream &out, const parser &the_parser);
    std::ofstream &operator<<(std::ofstream &out, const parser &the_parser);

    Request           parse_request(const std::string& src);
    Response          parse_response(const std::string& src);

    Request           parse_request(const void * src, size_t len);
    Response          parse_response(const void * src, size_t len);

    std::string         serialize(const Message * msg);
}
#endif