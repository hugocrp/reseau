#include "router.h"
#include <iostream>
#include <queue>
#include <limits>

void Router::add_link(const std::string& neighbor_ip, const std::string& neighbor_name, int capacity, bool active) {
    if (active) {
        neighbors[neighbor_ip] = {neighbor_ip, neighbor_name, active, capacity};
    }
}

void Router::apply_routes() {
    for (const auto& [destination, route] : routing_table) {
        // Commande système ou API pour mettre à jour la table de routage
        std::string command = "ip route add " + destination + " via " + route.next_hop;
        system(command.c_str());
    }
}

LSA Router::generate_lsa() const {
    LSA lsa;
    lsa.origin_ip = ip;
    lsa.origin_name = name;
    for (const auto& [neighbor_ip, link] : neighbors) {
        lsa.links.push_back(link);
    }
    return lsa;
}

void Router::display_neighbors() const {
    for (const auto& [ip, link] : neighbors) {
        std::cout << "Neighbor: " << link.neighbor_name << " (" << ip << "), Capacity: " << link.capacity << "\n";
    }
}

void Router::remove_inactive_links() {
    for (auto it = neighbors.begin(); it != neighbors.end();) {
        if (!it->second.active) {
            it = neighbors.erase(it);
        } else {
            ++it;
        }
    }
}

void Router::calculate_routes(const std::map<std::string, Router>& network) {
    dijkstra(network);
}

void Router::dijkstra(const std::map<std::string, Router>& network) {
    // Implémentation de l'algorithme de Dijkstra pour calculer les routes
    std::map<std::string, int> distances;
    std::map<std::string, std::string> previous;

    for (const auto& [router_ip, _] : network) {
        distances[router_ip] = std::numeric_limits<int>::max();
    }
    distances[ip] = 0;

    auto compare = [&distances](const std::string& a, const std::string& b) {
        return distances[a] > distances[b];
    };
    std::priority_queue<std::string, std::vector<std::string>, decltype(compare)> pq(compare);
    pq.push(ip);

    while (!pq.empty()) {
        std::string current = pq.top();
        pq.pop();

        for (const auto& [neighbor_ip, link] : network.at(current).neighbors) {
            if (!link.active) continue;

            int new_distance = distances[current] + 1; // Nombre de sauts
            if (new_distance < distances[neighbor_ip]) {
                distances[neighbor_ip] = new_distance;
                previous[neighbor_ip] = current;
                pq.push(neighbor_ip);
            }
        }
    }

    routing_table.clear();
    for (const auto& [destination, distance] : distances) {
        if (distance < std::numeric_limits<int>::max() && destination != ip) {
            routing_table[destination] = {destination, previous[destination], distance};
        }
    }
}