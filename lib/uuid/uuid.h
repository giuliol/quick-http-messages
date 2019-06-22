//
// Created by Giulio Luzzati on 26/09/18.
//

#ifndef NEWCORE_UUID_H
#define NEWCORE_UUID_H

#include <string>

struct Uuid {
    std::string         pretty_print();
    std::string         groups[5];
    bool operator==(const Uuid& rhs);
    bool operator!=(const Uuid& rhs);
};

Uuid                    generate_uuid();

#endif //NEWCORE_UUID_H
