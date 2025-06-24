#pragma once
#include <string>

struct Link
{
    std::string neighbor_ip;
    std::string neighbor_name;
    bool active;
    int capacity;
    bool operator==(const Link &other) const
    {
        return neighbor_ip == other.neighbor_ip &&
               neighbor_name == other.neighbor_name &&
               active == other.active &&
               capacity == other.capacity;
    }
};

struct Route
{
    std::string destination;
    std::string next_hop;
    int metric;
};