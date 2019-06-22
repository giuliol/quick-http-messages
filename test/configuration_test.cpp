//
// Created by Giulio Luzzati on 08/10/18.
//

#include <cassert>
#include "messenger/configuration.h"

int main(){

    Configuration c1 = {
            {"A", "A"},
            {"B", "B"},
            {"C", "C"},
    };

    Configuration c2 = {
            {"C", "CC"},
            {"D", "D"}
    };

    assert(c1["C"] == "C");

    c1.incorporate(c2);

    assert(c1["C"] == "CC");
    assert(c1["D"] == "D");

    return 0;

}