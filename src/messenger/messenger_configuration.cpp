//
// Created by Giulio Luzzati on 08/10/18.
//

#include "messenger/configuration.h"

void Configuration::incorporate(const Configuration& addendum){
    for(auto && el :addendum.entries)
        entries[el.first] = el.second;
}

std::string Configuration::safe_at(const std::string &key) const {
    auto it = entries.find(key);
    return it == entries.end() ? "" : it->second;
}

