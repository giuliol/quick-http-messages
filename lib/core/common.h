//
// Created by Giulio Luzzati on 19/07/18.
//

#ifndef NEWCORE_COMMON_H
#define NEWCORE_COMMON_H

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include "json/single_include/nlohmann/json.hpp"
#include "core/logger.h"

typedef int Status;
typedef uint64_t node_id_t;

#define CORE_GENERIC_ERROR            -1
#define CORE_OK                       0
#define CORE_CONTINUE                 1
#define CORE_TERMINATE                10

// types reserved to signal control and events for a worker
#define EVENT_MAP(fun) \
    fun(10001,    SERVICE_TERMINATE) \
    fun(10002,    DELETE_NODE) \

enum EventType {
#define CHOOSE_NUM(num, str) str = num,
    EVENT_MAP(CHOOSE_NUM)
#undef CHOOSE_NUM
};
const std::map<uint64_t, std::string> event_strings = {
#define PAIR(num, str) { num, #str },
        EVENT_MAP(PAIR)
#undef PAIR
};

static inline std::string app_msgtype_string(uint32_t type) {
    if(event_strings.find(type) == event_strings.end()) return "UNKNOWN EVENT";
    return event_strings.at(type);
}

// macros defining KEYs in the html KEY-VALUE headers
#define HTTP_HEADER_KEY_MAP(fun) \
    fun(1,    HEADER_KEY_STATUS,            "application-status") \
    fun(2,    HEADER_KEY_APP_MESSAGETYPE,   "application-messagetype") \
    fun(3,    HEADER_KEY_PAYLOAD,           "application-payload") \
    fun(4,    HEADER_KEY_NODE_ENDPOINT,     "application-endpoint") \
    fun(5,    HEADER_KEY_NODE_IDENTITY,     "application-identity") \
    fun(6,    HEADER_KEY_NODE_URI,          "application-uri") \
    fun(7,    HEADER_KEY_EVENT_TYPE,        "application-event_type") \
    fun(8,    HEADER_KEY_EVENT_NAME,        "application-event_name") \
    fun(9,    HEADER_KEY_SERVICE_SRC,       "application-src") \
    fun(10,   HEADER_KEY_SERVICE_DST,       "application-dst") \
    fun(11,   HEADER_KEY_PROCEDURE_ID,      "application-procid") \

//enum HTTP_HEADER_KEY : uint64_t {
//#define CHOOSE_NUM(num, name, str) ENUM_##name = num,
//    HTTP_HEADER_KEY_MAP(CHOOSE_NUM)
//#undef CHOOSE_NUM
//};

#define DECLARE_AS_STRING(num, name, str) const static std::string name(str);
HTTP_HEADER_KEY_MAP(DECLARE_AS_STRING)
#undef DECLARE_AS_STRING

#define RANDOM_CLIENT_PORT_RANGE_START  5050
#define RANDOM_CLIENT_PORT_RANGE_END    9090
#define QHM_DEFAULT_SERVICE_PORT        40401
#define QHM_DEFAULT_REQUEST_PORT        40400


static inline std::vector<std::string> split(const std::string& s, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter))
        tokens.push_back(token);

    return tokens;
}

static nlohmann::json json_try_parsing(const std::string& in, Status& rv){
    nlohmann::json ret;
    rv = CORE_OK;
    core_try(ret = nlohmann::json::parse(in), rv = CORE_GENERIC_ERROR; core_err << in << "invalid json object";);
    return ret;
}

static std::string form_get_request(const std::string &url, const nlohmann::json &params) {
    std::string ret = url + "?";
    for (nlohmann::json::const_iterator it = params.begin(); it != params.end(); ++it) {
        ret += it.key();
        ret.push_back('=');
        ret += it.value();
        ret.push_back('&');
    }
    return ret.substr(0, ret.size()-1);
}

static inline bool json_has_field(const nlohmann::json& json, const std::string& field){
    return json.count(field) > 0;
}

#endif //NEWCORE_COMMON_H
