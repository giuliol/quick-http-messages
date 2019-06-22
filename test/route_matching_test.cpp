//
// Created by Giulio Luzzati on 12/09/18.
//
#include <cassert>
#include <regex>
#include <iostream>
#include <core/common.h>
#include "messenger/messenger.h"


bool regex_test() {
    using std::regex;
    using std::regex_match;
    using std::string;

    regex r("(.*)/ab(cd)?e");

    assert(regex_match("/abcde",r));
    assert(regex_match("qhm://Namf/api/v2/abcde",r));
    assert(!regex_match("/ab2cde",r));

    assert(regex_match("test/url/yo", regex("test/url/yo")));

    return true;
}

static const std::regex params_regex(R"(\{.*?\})");

bool route_parsing_test(){
    using std::regex_match;
    using std::smatch;
    using std::regex;
    using std::string;
    using nlohmann::json;

    {
        std::string url = "example.com/namf-comm/v1/ue-contexts/5g-guti-000001111111/release";
        std::string route = "example.com/namf-comm/v1/ue-contexts/{ueContextId}/release";

        regex guti_regex(
                "^(5g-guti-[0-9]{5,6}[0-9a-fA-F]{14}|imsi-[0-9]{5,15}|nai-.+|imei-[0-9]{15}|imeisv-[0-9]{16}|.+)$");

        auto tokens = split(url, '/');
        assert(regex_match(tokens[4], guti_regex));

        // check that the algorithm to find the params in the route works
        auto params_schema = parse_param_ids_in_route(route);
        auto route_tokens = split(route, '/');

        /* url params test */
        json params = parse_all_params_in_url(url, params_schema);

        assert(params["ueContextId"]["value"] == "5g-guti-000001111111");
        assert(regex_match(params["ueContextId"]["value"].get<string>(), guti_regex));

        regex route_regex(generate_route_regex(route));
        assert(generate_route_regex(route) == "example.com/namf-comm/v1/ue-contexts/.*/release");
        assert(regex_match("example.com/namf-comm/v1/ue-contexts/5g-guti-000001111111/release", route_regex));
        assert(regex_match("example.com/namf-comm/v1/ue-contexts/BOGUS/release", route_regex));
        assert(!regex_match("example.com/namf-comm/v1/ue-contexts/5g-guti-000001111111/releases", route_regex));
        assert(!regex_match("example.com/namf-comm/v1/ue-context/5g-guti-000001111111/release", route_regex));
    }

    {
        /* query params test */
        string url = "example.com/nudm-sdm/v1/imsi-23591000001?dataset-names=name1,name2,name3&other-param=value-23591000001";
        std::string route = "example.com/nudm-sdm/v1/{supi}";

        // check that the algorithm to find the params in the route works
        auto params_schema = parse_param_ids_in_route(route);

        auto route_tokens = split(route, '/');
        for (auto p:params_schema)
            assert(regex_match(route_tokens[p.pos], params_regex));

        /* url params test */
        json params = parse_all_params_in_url(url, params_schema);

        assert(params["supi"]["value"] == "imsi-23591000001");
        assert(params["supi"]["params"]["dataset-names"].size() == 3);
        assert(params["supi"]["params"]["other-param"] == "value-23591000001");

        regex route_regex(generate_route_regex(route));
        assert(regex_match(url, route_regex));
    }

    return true;
}

bool router_test(){
    using std::string;

    std::string supi_route      = "example.com/nudm-sdm/v1/{supi}";
    std::string context_route   = "example.com/namf-comm/v1/ue-contexts/{ueContextId}";
    std::string release_route   = "example.com/namf-comm/v1/ue-contexts/{ueContextId}/release";

    string supi_url     = "example.com/nudm-sdm/v1/imsi-23591000001?dataset-names=name1,name2,name3&other-param=value-23591000001";
    string context_url  = "example.com/namf-comm/v1/ue-contexts/5g-guti-000001111111";
    string release_url  = "example.com/namf-comm/v1/ue-contexts/5g-guti-000001111111/release";

    RouteHandler supi_handler;
    RouteHandler context_handler;
    RouteHandler release_handler;

    Router router;

    router.add_route(supi_route, supi_handler);
    router.add_route(context_route, context_handler);
    router.add_route(release_route, release_handler);

    assert(router.match(supi_url)->handler == supi_handler);
    assert(router.match(context_url)->handler == context_handler);
    assert(router.match(release_url)->handler == release_handler);
    assert(!router.match("example.com/nudm-sdm/v2/imsi-23591000001"));

    return true;
}


int main() {
    assert(regex_test());
    assert(route_parsing_test());
    assert(router_test());
    return 0;
}
