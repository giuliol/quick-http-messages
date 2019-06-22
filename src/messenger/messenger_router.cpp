//
// Created by Giulio Luzzati on 13/09/18.
//

#include "messenger/messenger.h"

#define de_curlize(arg) (arg).substr(1, (arg).size() - 2)
static const std::regex params_regex(R"(\{.*?\})");

RouteParams parse_param_ids_in_route (const std::string& url) {
    auto tokens = split(url, '/');
    RouteParams param_ids;
    unsigned int count(0);
    for (auto &&token : tokens) {
        bool is_param = regex_match(token, params_regex);
        if (is_param)
            param_ids.emplace_back(RouteParameter{count, de_curlize(token)});
        if ((count == tokens.size() -1) && !is_param)
            param_ids.emplace_back(RouteParameter{count, token});
        count++;
    }
    return param_ids;
};

nlohmann::json parse_params_in_token (const std::string &token) {
    nlohmann::json ret;
    auto params = split(token, '?');
    if (params.size() > 1){
        auto keys = split(params[1], '&');
        for(auto&& val: keys){
            auto el = split(val, '=');
            auto values = split(el[1], ',');
            if (values.size()>1)
                ret[el[0]] = nlohmann::json(values);
            else
                ret[el[0]] = el[1];
        }
    }
    return ret;
};

nlohmann::json parse_all_params_in_url(const std::string &url, const RouteParams &params_schema){
    nlohmann::json ret;
    auto tokens = split(url, '/');
    for (auto p: params_schema) {
        auto params = parse_params_in_token(tokens[p.pos]);
        ret[p.name]["value"] = split(tokens[p.pos], '?')[0];
        if(!params.is_null()) {
            ret[p.name]["params"] = params;
        }
    }
    return ret;
};

std::string generate_route_regex (const std::string &route) {
    auto tokens = split(route, '/');
    for (auto &&it : tokens)
        if (regex_match(it, params_regex))
            it = ".*";
    std::string ret;
    for (auto && token : tokens)
        ret += token + "/";
    return ret.substr(0, ret.size() - 1);
};


void _remove_query(std::string& in){
    auto limiter = in.find('?');
    in = in.substr(0, limiter);
}

Route* Router::match(const std::string &url) {
    std::string queryless_url = url.substr(0, url.find('?'));
    size_t depth = split(queryless_url, '/').size();
    for (auto && route: routes) {
        if (route.depth == depth)
            if (std::regex_match(queryless_url, route.regex))
                return &route;
    }
    return nullptr;
}

void Router::add_route(const std::string &route, RouteHandler handler) {
    auto params_schema = parse_param_ids_in_route(route);
    routes.emplace_back(Route{ std::regex(generate_route_regex(route)), handler, url_depth(route), params_schema });
}
