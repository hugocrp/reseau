#pragma once
#include <string>

struct Link {
    std::string neighbor_ip;
    std::string neighbor_name;
    bool active;
    int capacity;
};

struct Route {
    std::string destination;
    std::string next_hop;
    int metric;
};