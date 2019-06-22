//
// Created by Giulio Luzzati on 26/09/18.
//

#include <vector>
#include <iostream>
#include <sstream>
#include <random>
#include <climits>
#include <algorithm>
#include <functional>
#include "uuid.h"

unsigned char random_char() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    return static_cast<unsigned char>(dis(gen));
}

std::string generate_hex(const unsigned int len) {
    std::stringstream ss;
    for(auto i = 0; i < len; i++) {
        auto rc = random_char();
        std::stringstream hexstream;
        hexstream << std::hex << int(rc);
        auto hex = hexstream.str();
        ss << (hex.length() < 2 ? '0' + hex : hex);
    }
    return ss.str();
}

// 8-4-4-4-12
Uuid generate_uuid(){
    Uuid ret;
    ret.groups[0] = generate_hex(8);
    ret.groups[1] = generate_hex(4);
    ret.groups[2] = generate_hex(4);
    ret.groups[3] = generate_hex(4);
    ret.groups[4] = generate_hex(12);
    return ret;
}

auto _same = [](const Uuid &lhs, const Uuid &rhs){
    return  (lhs.groups[0] == rhs.groups[0]) &&
            (lhs.groups[1] == rhs.groups[1]) &&
            (lhs.groups[2] == rhs.groups[2]) &&
            (lhs.groups[3] == rhs.groups[3]) &&
            (lhs.groups[4] == rhs.groups[4]) ;
};

bool Uuid::operator==(const Uuid &rhs) {
    return _same(*this, rhs);
}
bool Uuid::operator!=(const Uuid &rhs) {
    return !_same(*this, rhs);
}
std::string Uuid::pretty_print() {
    std::stringstream ss;
    const auto separator =  "-";
    ss << groups[0] << separator << groups[1] << separator << groups[2]
       << separator << groups[3] << separator << groups[4];
    return ss.str();
}
