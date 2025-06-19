#pragma once
#include <map>
#include <string>
#include <vector>
#include "types.h"
#include "lsa.h"

class Router {
public:
    std::string name;
    std::string ip;
    bool default_origin = false;

    std::map<std::string, Link> neighbors;
    std::map<std::string, Route> routing_table;

    void add_link(const std::string& neighbor_ip, const std::string& neighbor_name, int capacity, bool active);
    void calculate_routes(const std::map<std::string, Router>& network);
    void apply_routes();
    LSA generate_lsa() const;
    void display_neighbors() const;
    void remove_inactive_links();

private:
    void dijkstra(const std::map<std::string, Router>& network);
};