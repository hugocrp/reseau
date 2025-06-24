#pragma once
#include <map>
#include <string>
#include <vector>
#include "types.h"
#include "lsa.h"

class Router
{
public:
    std::string name;
    std::string ip;
    std::vector<std::string> ips;
    std::vector<std::string> local_networks;
    bool default_origin = false;

    std::map<std::string, Link> neighbors;
    std::map<std::string, Route> routing_table;

    void add_link(const std::string &neighbor_ip, const std::string &neighbor_name, int capacity, bool active);
    void calculate_routes(const std::map<std::string, Router> &network);
    void apply_routes();
    LSA generate_lsa() const;
    void display_neighbors() const;
    void remove_inactive_links();
    void set_link_active(const std::string &neighbor_ip, bool active);
    std::vector<std::string> get_broadcasts();

private:
    void dijkstra(const std::map<std::string, Router> &network);
};