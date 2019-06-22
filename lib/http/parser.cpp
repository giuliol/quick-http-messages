#include <regex>
#include <core/logger.h>
#include <http/uri_t.hpp>

#include "http/parser.h"
#include "util.h"

namespace http {

    static const std::map<int, std::string> status_strings = {
            {100, "CONTINUE"},
            {101, "SWITCHING_PROTOCOLS"},
            {102, "PROCESSING"},
            {200, "OK"},
            {201, "CREATED"},
            {202, "ACCEPTED"},
            {203, "NON_AUTHORITATIVE_INFORMATION"},
            {204, "NO_CONTENT"},
            {205, "RESET_CONTENT"},
            {206, "PARTIAL_CONTENT"},
            {207, "MULTI_STATUS"},
            {208, "ALREADY_REPORTED"},
            {226, "IM_USED"},
            {300, "MULTIPLE_CHOICES"},
            {301, "MOVED_PERMANENTLY"},
            {302, "FOUND"},
            {303, "SEE_OTHER"},
            {304, "NOT_MODIFIED"},
            {305, "USE_PROXY"},
            {307, "TEMPORARY_REDIRECT"},
            {308, "PERMANENT_REDIRECT"},
            {400, "BAD_REQUEST"},
            {401, "UNAUTHORIZED"},
            {402, "PAYMENT_REQUIRED"},
            {403, "FORBIDDEN"},
            {404, "NOT_FOUND"},
            {405, "METHOD_NOT_ALLOWED"},
            {406, "NOT_ACCEPTABLE"},
            {407, "PROXY_AUTHENTICATION_REQUIRED"},
            {408, "REQUEST_TIMEOUT"},
            {409, "CONFLICT"},
            {410, "GONE"},
            {411, "LENGTH_REQUIRED"},
            {412, "PRECONDITION_FAILED"},
            {413, "PAYLOAD_TOO_LARGE"},
            {414, "URI_TOO_LONG"},
            {415, "UNSUPPORTED_MEDIA_TYPE"},
            {416, "RANGE_NOT_SATISFIABLE"},
            {417, "EXPECTATION_FAILED"},
            {421, "MISDIRECTED_REQUEST"},
            {422, "UNPROCESSABLE_ENTITY"},
            {423, "LOCKED"},
            {424, "FAILED_DEPENDENCY"},
            {426, "UPGRADE_REQUIRED"},
            {428, "PRECONDITION_REQUIRED"},
            {429, "TOO_MANY_REQUESTS"},
            {431, "REQUEST_HEADER_FIELDS_TOO_LARGE"},
            {451, "UNAVAILABLE_FOR_LEGAL_REASONS"},
            {500, "INTERNAL_SERVER_ERROR"},
            {501, "NOT_IMPLEMENTED"},
            {502, "BAD_GATEWAY"},
            {503, "SERVICE_UNAVAILABLE"},
            {504, "GATEWAY_TIMEOUT"},
            {505, "HTTP_VERSION_NOT_SUPPORTED"},
            {506, "VARIANT_ALSO_NEGOTIATES"},
            {507, "INSUFFICIENT_STORAGE"},
            {508, "LOOP_DETECTED"},
            {510, "NOT_EXTENDED"},
            {511, "NETWORK_AUTHENTICATION_REQUIRED"}

    };

    parser::parser() {
        request_complete_flag = false;
        response_complete_flag = false;
        gzip_flag = false;
        http_parser_init(&core, HTTP_REQUEST);
        core.data = this;

        http_parser_settings_init(&settings);
        settings.on_url = on_url;
        settings.on_header_field = on_header_field;
        settings.on_header_value = on_header_value;
        settings.on_headers_complete = on_headers_complete;
        settings.on_body = on_body;
        settings.on_message_complete = on_message_complete;
    }

    bool parser::parse(const std::string &body, enum http_parser_type type, Message **result) {
        if (core.type != type) {
            http_parser_init(&core, type);
        }
        if (core.type == HTTP_REQUEST) {
            request.append(body);
        } else if (core.type == HTTP_RESPONSE) {
            response.append(body);
        }
        size_t parse_bytes = http_parser_execute(&core, &settings, body.c_str(), body.size() + 1);

        headers_map *headers;
        std::string *the_appropriate_body;
        if (HTTP_PARSER_ERRNO(&core) == HPE_OK) {
            if (core.type == HTTP_REQUEST) {
                *result = new Request();
                headers = &request_headers;
                the_appropriate_body = &request_body;
                ((Request *) (*result))->method = http_method_from_str(method.data());
                ((Request *) (*result))->path = url;
            } else if (core.type == HTTP_RESPONSE) {
                *result = new Response();
                headers = &response_headers;
                the_appropriate_body = &response_body;
                ((Response *) (*result))->status = get_status();
            }
            for (auto &&h:*headers)
                (*result)->headers[h.first] = h.second;

            if(!the_appropriate_body->empty())
                the_appropriate_body->pop_back();
            (*result)->body.append(*the_appropriate_body);
        } else {
            std::cerr << http_errno_name(HTTP_PARSER_ERRNO(&core));
        }

        return parse_bytes > 0 && HTTP_PARSER_ERRNO(&core) == HPE_OK;
    }

    const std::string &parser::get_response_body() const {
        return response_body;
    }

    void parser::set_addr(const std::string &src_addr, const std::string &dst_addr) {
        this->request_address = src_addr;
        this->response_address = dst_addr;
    }

    int parser::on_url(http_parser_core *the_parser, const char *at, size_t length) {
        parser *self = reinterpret_cast<parser *>(the_parser->data);
        self->url.assign(at, length);
        self->method.assign(http_method_str(static_cast<enum http_method>(the_parser->method)));
        return 0;
    };

    int parser::on_header_field(http_parser_core *the_parser, const char *at, size_t length) {
        parser *self = reinterpret_cast<parser *>(the_parser->data);
        self->temp_header_field.assign(at, length);
        for (size_t i = 0; i < length; ++i) {
            if (at[i] >= 'A' && at[i] <= 'Z') {
                self->temp_header_field[i] = at[i] ^ (char) 0x20;
            }
        }
        return 0;
    }

    int parser::on_header_value(http_parser_core *the_parser, const char *at, size_t length) {
        parser *self = reinterpret_cast<parser *>(the_parser->data);
        if (the_parser->type == HTTP_RESPONSE) {
            self->response_headers[self->temp_header_field] = std::string(at, length);
            if (self->temp_header_field == "content-encoding" && std::strstr(at, "gzip")) {
                self->gzip_flag = true;
            }
        } else {
            self->request_headers[self->temp_header_field] = std::string(at, length);
            if (self->temp_header_field == "host") {
                self->host.assign(at, length);
            }
        }

//     std::cout << self->temp_header_field <<  ":" << std::string(at, length) << std::endl;
        return 0;
    }

    int parser::on_headers_complete(http_parser_core *the_parser) {
        parser *self = reinterpret_cast<parser *>(the_parser->data);
        if (the_parser->type == HTTP_REQUEST) {
            self->request_header = self->request.substr(0, the_parser->nread);
        } else if (the_parser->type == HTTP_RESPONSE) {
            self->response_header = self->response.substr(0, the_parser->nread);
        }
        return 0;
    }

    int parser::on_body(http_parser_core *the_parser, const char *at, size_t length) {
        parser *self = reinterpret_cast<parser *>(the_parser->data);
        // std::cout << __func__ << " " << self->url << std::endl;
        if (the_parser->type == HTTP_REQUEST) {
            self->request_body.append(at, length);
        } else if (the_parser->type == HTTP_RESPONSE) {
            self->response_body.append(at, length);
        }
        return 0;
    }

    int parser::on_message_complete(http_parser_core *the_parser) {
        parser *self = reinterpret_cast<parser *>(the_parser->data);
        if (the_parser->type == HTTP_REQUEST) {
            self->request_complete_flag = true;
        } else if (the_parser->type == HTTP_RESPONSE) {
            self->response_complete_flag = true;
        }
        if (self->gzip_flag) {
            std::string new_body;
            if (gzip_decompress(self->response_body, new_body)) {
                self->response_body = new_body;
            } else {
                std::cerr << ANSI_COLOR_RED << "[decompress error]" << ANSI_COLOR_RESET << std::endl;
            }
        }
        return 0;
    }

    bool parser::filter_url(const std::regex *url_filter, const std::string &url) {
        return !url_filter ? true : std::regex_search(url, *url_filter);
    }

    void parser::save_http_request(const std::regex *url_filter, const std::string &output_path,
                                   const std::string &join_addr) {
        std::string host_with_url = host + url;
        if (!filter_url(url_filter, host_with_url)) {
            return;
        }
        std::cout << ANSI_COLOR_CYAN << request_address << " -> " << response_address << " " << host_with_url
                  << ANSI_COLOR_RESET << std::endl;
        if (!output_path.empty()) {
            std::string save_filename = output_path + "/" + host;
            std::ofstream out(save_filename.c_str(), std::ios::app | std::ios::out);
            if (out.is_open()) {
                out << *this << std::endl;
                out.close();
            } else {
                std::cerr << "ofstream [" << save_filename << "] is not opened." << std::endl;
                out.close();
                exit(1);
            }
        } else {
            std::cout << *this << std::endl;
        }
    }

    std::ostream &operator<<(std::ostream &out, const parser &the_parser) {
        out
                << ANSI_COLOR_GREEN
                << the_parser.request_header
                << ANSI_COLOR_RESET;
        if (!is_atty || is_plain_text(the_parser.request_body)) {
            out << the_parser.request_body;
        } else {
            out << ANSI_COLOR_RED << "[binary request body]" << ANSI_COLOR_RESET;
        }
        out << std::endl
            << ANSI_COLOR_BLUE
            << the_parser.response_header
            << ANSI_COLOR_RESET;
        if (the_parser.response_body.empty()) {
            out << ANSI_COLOR_RED << "[empty response body]" << ANSI_COLOR_RESET;
        } else if (!is_atty || is_plain_text(the_parser.response_body)) {
            out << the_parser.response_body;
        } else {
            out << ANSI_COLOR_RED << "[binary response body]" << ANSI_COLOR_RESET;
        }
        return out;
    }

    std::ofstream &operator<<(std::ofstream &out, const parser &the_parser) {
        out
                << the_parser.request_header
                << the_parser.request_body
                << the_parser.response_header
                << the_parser.response_body;
        return out;
    }

    inline Request parse_request(const std::string &src) {
        http::parser parser;
        http::Message *parsed;
        Request ret;
        if (parser.parse(src, HTTP_REQUEST, &parsed)) {
            ret = *((Request *) parsed);
            http_free(parsed);
            ret.success = true;
        }
        return ret;
    }

    inline Response parse_response(const std::string &src) {
        http::parser parser;
        http::Message *parsed;
        Response ret;
        if (parser.parse(src, HTTP_RESPONSE, &parsed)) {
            ret = *((Response *) parsed);
            http_free(parsed);
            ret.success = true;
        }
        return ret;
    }

    Request parse_request(const void *src, size_t len) {
        std::string wrapper((char *) src, len);
        return parse_request(wrapper);
    }

    Response parse_response(const void *src, size_t len) {
        std::string wrapper((char *) src, len);
        return parse_response(wrapper);
    }


    std::string serialize(const Message *msg) {
        std::string ret, startline;

        switch (msg->type) {
            case http_message_type::REQUEST: {
                auto request = (Request *) msg;
                startline += std::string(http_method_str(request->method)) + " " + request->path + " HTTP/1.1\n";
                break;
            }
            case http_message_type::RESPONSE: {
                auto response = (Response *) msg;
                startline +=
                        "HTTP/1.1 " + std::to_string(response->status) + " " + status_strings.at(response->status) +
                        "\n";
                break;
            }
        }

        ret.append(startline);

        for (auto &&header:msg->headers)
            if (strcasecmp(header.first.c_str(), "content-length") != 0)
                ret.append(header.first + ": " + header.second + "\n");

        ret.append("content-length: " + std::to_string(msg->body.size() + 2) + "\n");

        ret.append("\r\n");

        if (!msg->body.empty()) ret.append(msg->body);

        return ret;
    }


    const headers_map &parser::get_request_headers_map() {
        return request_headers;
    }

    const headers_map &parser::get_response_headers_map() {
        return response_headers;
    }

    const std::string &parser::get_request_body() const {
        return request_body;
    }

    uint32_t parser::get_status() const {
        return core.status_code;
    }

    const std::string &parser::get_method() const {
        return method;
    }

    const std::string &parser::get_url() const {
        return url;
    }

    void http_free(Message *msg) {
        if (!msg) return;

        switch (msg->type) {
            case REQUEST: {
                Request *casted = (Request *) msg;
                delete casted;
                break;
            }
            case RESPONSE: {
                Response *casted = (Response *) msg;
                delete casted;
                break;
            }
        }
    }
}