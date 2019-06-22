//
// Created by Giulio Luzzati on 24/07/18.
//

#include <cassert>
#include <core/logger.h>
#include <algorithm>
#include <http/uri_t.hpp>
#include <http/parser.h>
#include <http/util.h>
#include <core/common.h>
#include <messenger/messenger.h>

void url_test() {
    using namespace http;

    http::uri_t uri("http://user:pass@www.google.com:80/api/service.html?foo=bar&foo2=bar2#frag");
    assert(uri.scheme()=="http");
    assert(uri.host() == "www.google.com");
    assert(uri.port() == "80");
    assert(uri.path()=="/api/service.html");
    auto query = &uri.query();
    std::string val;
    assert(query_find_value_by_key(*query, "foo", &val));
    assert(val == "bar");
    assert(!query_find_value_by_key(*query, "WRONG", &val));
    assert(query_find_value_by_key(*query, "foo2", &val));
    assert(val == "bar2");
    assert(uri.fragment() == "frag" );
    return;
}

void http_test(){
    using namespace http;
    const std::string request = "GET /api/search?k1=v1&k2=v2#frag1 HTTP/1.1\n"
                                "User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\n"
                                "Host: www.tutorialspoint.com\n"
                                "Accept-Language: en-us\n"
                                "Content-Length: 60\n"
                                "Accept-Encoding: gzip, deflate\n"
                                "Connection: Keep-Alive \n"
                                "\n"
                                "REQUEST BODY";

    const std::string response = "HTTP/1.1 200 OK\n"
                                 "Date: Mon, 27 Jul 2009 12:28:53 GMT\n"
                                 "Server: Apache/2.2.14 (Win32)\n"
                                 "Last-Modified: Wed, 22 Jul 2009 19:15:56 GMT\n"
                                 "Content-Length: 88\n"
                                 "Content-Type: text/html\n"
                                 "Connection: Closed\n"
                                 "\n"
                                 "RESPONSE BODY";

    http::Request  parsed_request = http::parse_request(request.data(), request.size());
    http::Response parsed_response =  http::parse_response(response.data(), response.size());

    assert(parsed_request.success);
    assert(parsed_response.success);

    for(auto&& h: parsed_request.headers)
    core_log << h.first << h.second;
    core_log << " ";

    for(auto&& h: parsed_response.headers)
        core_log << h.first << h.second;
    core_log << " ";

    // content-size dictates "AT MOST" . any less is OK.
    core_log << "req body [" <<  parsed_request.body.size() << " bytes]: " << parsed_request.body.data();
    core_log << "resp [status: " << parsed_response.status << "] body: "<< parsed_response.body;

    core_log <<  "method: " << http_method_str(parsed_request.method);
    core_log << "url: " << parsed_request.path.data();

    http::uri_t uri(parsed_request.path);

    core_log << "scheme: " << uri.scheme();
    core_log << "host: " << uri.host();
    core_log << "port: " << uri.port();
    core_log << "path: " << uri.path();
    core_log << "fragment: " << uri.fragment();

    assert( parsed_response.status == 200);
    // http headers are case INSENSITIVE! keep this in mind
    assert(parsed_response.headers.at("content-type") == "text/html");
    assert(parsed_request.method == HTTP_GET);

    std::string v1, v2;
    auto query = &uri.query();
    assert(query_find_value_by_key(*query, "k1", &v1));
    assert(query_find_value_by_key(*query, "k2", &v2));

    core_log << "k1: " << v1;
    core_log << "k2: " << v2;

    assert(uri.scheme().empty());
    assert(uri.host().empty());
    assert(uri.port().empty());
    assert(uri.path()=="/api/search");
    assert(v1 == "v1");
    assert(v2 == "v2");
    assert(uri.fragment() == "frag1" );

    std::string serialized_request, serialized_response;
    serialized_request = http::serialize(&parsed_request);
    serialized_response = http::serialize(&parsed_response);

    core_log << "serialized REQUEST:\n " << serialized_request;
    core_log << "serialized RESPONSE:\n " <<  serialized_response;

    http::Request reparsed_request = http::parse_request(serialized_request.data(), serialized_request.size());
    http::Response reparsed_response = http::parse_response(serialized_response.data(), serialized_response.size());

    uri = http::uri_t(reparsed_request.path);
    assert( reparsed_response.status == 200);
    // http headers are case INSENSITIVE! keep this in mind
    assert(reparsed_response.headers.at("content-type") == "text/html");
    assert(reparsed_request.method == HTTP_GET);

    query = &uri.query();
    v1.clear(); v2.clear();
    assert(query_find_value_by_key(*query, "k1", &v1));
    assert(query_find_value_by_key(*query, "k2", &v2));

    core_log << "k1: " << v1;
    core_log << "k2: " << v2;

    assert(uri.scheme().empty());
    assert(uri.host().empty());
    assert(uri.port().empty());
    assert(uri.path()=="/api/search");
    assert(v1 == "v1");
    assert(v2 == "v2");
    assert(uri.fragment() == "frag1" );

    const std::string wrong_request = "/api/search?k1=v1&k2=v2#frag1 HTTP/1.1\n"
                                      "User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\n"
                                      "Host: www.tutorialspoint.com\n"
                                      "Accept-Language: en-us\n"
                                      "Content-Length: 60\n"
                                      "Accept-Encoding: gzip, deflate\n"
                                      "Connection: Keep-Alive \n"
                                      "\n"
                                      "REQUEST BODY";

    auto parsed = http::parse_request(wrong_request);
    assert(!parsed.success);


}

void test_offending(){
    const char * request = "GET /api/search?k1=v1&k2=v2#frag1 HTTP/1.1\n"
                           "User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\n"
                           "Host: www.tutorialspoint.com\n"
                           "Accept-Language: en-us\n"
                           "Content-Length: 60\n"
                           "Accept-Encoding: gzip, deflate\n"
                           "Connection: Keep-Alive \n"
                           "\n"
                           "REQUEST BODY";

    const std::string offending = "GET /api/test HTTP/1.1\nTYPE: 10001\ncontent-length: 100\n\n";

    http::Request  parsed_request = http::parse_request(offending.data(), offending.size());
    assert(parsed_request.method == HTTP_GET);
    auto mess = message_from_type({"", 1, "TESTURI"}, "/api/v1/test");
    auto from_msg = http::parse_request(mess.data(), mess.size());
    assert(from_msg.method == HTTP_GET);

}


int main(){
    url_test();
    http_test();
    test_offending();
    return 0;
}