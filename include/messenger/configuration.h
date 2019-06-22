//
// Created by Giulio Luzzati on 08/10/18.
//

#ifndef NEWCORE_CONFIGURATION_H
#define NEWCORE_CONFIGURATION_H

#include <map>
#include <string>
#include <initializer_list>


struct Configuration {
    typedef std::map<std::string, std::string> StringStringMap;
    typedef std::initializer_list<std::pair<std::string const, std::string>> _initializer_list;

    Configuration() = default;
    Configuration(const _initializer_list& l): entries(l) {}
    Configuration(const StringStringMap& in): entries(in) {}
    std::string& operator [](const std::string& k) {return entries[k];}

    std::string             safe_at(const std::string& key) const;
    void                    incorporate(const Configuration& addendum);
    bool                    empty() { return entries.empty(); }

    StringStringMap         entries;
};

static const char*    CONFIG_KEY_SELF_IP     = "self_ip";
static const char*    CONFIG_KEY_PORT        = "port";
static const char*    CONFIG_KEY_TAG         = "tag";

#endif //NEWCORE_CONFIGURATION_H
