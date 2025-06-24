#include "router.h"
#include <iostream>
#include <queue>
#include <limits>
extern std::map<std::string, Router> network_db;

void Router::add_link(const std::string &neighbor_ip, const std::string &neighbor_name, int capacity, bool active)
{
    if (active)
    {
        neighbors[neighbor_ip] = {neighbor_ip, neighbor_name, active, capacity};
    }
    if (default_origin)
    {
    }
}

void Router::apply_routes()
{
    std::cout << "[DEBUG] Table de routage courante pour " << ip << " :\n";
    for (const auto &[destination, route] : routing_table)
    {
        std::cout << "[DEBUG] Ajout/maj route: " << destination << " via " << route.next_hop << std::endl;
        std::string command = "ip route replace " + destination + " via " + route.next_hop;
        system(command.c_str());
    }

    if (!default_origin)
    {
        bool default_route_added = false;
        for (const auto &[dest, route] : routing_table)
        {
            auto it = network_db.find(dest);
            if (it != network_db.end() && it->second.default_origin)
            {
                if (route.next_hop == ip)
                {
                    std::cout << "[DEBUG] Refus d'ajouter une route par défaut vers soi-même (" << ip << ")" << std::endl;
                    continue;
                }
                std::cout << "[DEBUG] Ajout/maj route par défaut via " << route.next_hop << std::endl;
                std::string command = "ip route replace default via " + route.next_hop;
                system(command.c_str());
                default_route_added = true;
                break;
            }
        }
        if (!default_route_added)
        {
            std::string fallback = "<IP_DU_ROUTEUR_DEFAULT>";
            std::cout << "[DEBUG] Forçage route par défaut via " << fallback << std::endl;
            std::string command = "ip route replace default via " + fallback;
            system(command.c_str());
        }
    }
}

LSA Router::generate_lsa() const
{
    LSA lsa;
    lsa.origin_ip = ip;
    lsa.origin_name = name;
    lsa.is_default = default_origin;
    for (const auto &[neighbor_ip, link] : neighbors)
        lsa.links.push_back(link);

    // Ajoute les réseaux locaux comme "neighbor" spécial
    for (const auto &net : local_networks)
    {
        Link l;
        l.neighbor_ip = net; // ex: "192.168.1.0/24"
        l.neighbor_name = "NETWORK";
        l.active = true;
        l.capacity = 100;
        lsa.links.push_back(l);
    }
    return lsa;
}

void Router::display_neighbors() const
{
    for (const auto &[ip, link] : neighbors)
    {
        std::cout << "Neighbor: " << link.neighbor_name << " (" << ip << "), Capacity: " << link.capacity << "\n";
    }
}

void Router::remove_inactive_links()
{
    for (auto it = neighbors.begin(); it != neighbors.end();)
    {
        if (!it->second.active)
        {
            it = neighbors.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void Router::calculate_routes(const std::map<std::string, Router> &network)
{
    dijkstra(network);
}

void Router::dijkstra(const std::map<std::string, Router> &network)
{
    std::map<std::string, int> max_min_capacity;
    std::map<std::string, std::string> previous;

    for (const auto &[router_ip, _] : network)
        max_min_capacity[router_ip] = 0;
    max_min_capacity[ip] = std::numeric_limits<int>::max();

    auto compare = [&max_min_capacity](const std::string &a, const std::string &b)
    {
        return max_min_capacity[a] < max_min_capacity[b];
    };
    std::priority_queue<std::string, std::vector<std::string>, decltype(compare)> pq(compare);
    pq.push(ip);

    while (!pq.empty())
    {
        std::string current = pq.top();
        pq.pop();

        auto it = network.find(current);
        if (it == network.end())
            continue;
        for (const auto &[neighbor_ip, link] : it->second.neighbors)
        {
            if (!link.active)
                continue;
            int path_capacity = std::min(max_min_capacity[current], link.capacity);
            if (path_capacity > max_min_capacity[neighbor_ip])
            {
                max_min_capacity[neighbor_ip] = path_capacity;
                previous[neighbor_ip] = current;
                pq.push(neighbor_ip);
            }
        }
    }

    routing_table.clear();

    // Routes vers les autres routeurs (jamais vers soi-même)
    for (const auto &[destination, capacity] : max_min_capacity)
    {
        if (capacity > 0 && destination != ip)
        {
            std::string hop = destination;
            while (previous.count(hop) && previous[hop] != ip)
                hop = previous[hop];
            if (previous.count(hop) && previous[hop] == ip && hop != ip && hop != destination && !hop.empty())
            {
                routing_table[destination] = {destination, hop, capacity};
            }
        }
    }

    // Routes vers les réseaux locaux annoncés par les autres routeurs
    for (const auto &[router_ip, router] : network)
    {
        for (const auto &link : router.neighbors)
        {
            if (link.second.neighbor_name == "NETWORK")
            {
                bool already_on_network = false;
                for (const auto &my_net : local_networks)
                {
                    if (my_net == link.second.neighbor_ip)
                    {
                        already_on_network = true;
                        break;
                    }
                }
                if (already_on_network)
                    continue;

                if (router_ip == ip)
                    continue;
                if (max_min_capacity[router_ip] > 0)
                {
                    std::string hop = router_ip;
                    while (previous.count(hop) && previous[hop] != ip)
                        hop = previous[hop];
                    if (hop != ip && !hop.empty())
                    {
                        std::cout << "[DEBUG] Ajout route réseau (FORCÉ): " << link.second.neighbor_ip << " via " << hop << std::endl;
                        routing_table[link.second.neighbor_ip] = {link.second.neighbor_ip, hop, max_min_capacity[router_ip]};
                    }
                }
            }
        }
    }

    // Ajoute une route pour chaque routeur connu, même si ce n'est pas un voisin direct (évite doublons)
    for (const auto &[router_ip, router] : network)
    {
        if (router_ip == ip)
            continue;
        if (max_min_capacity[router_ip] > 0 && routing_table.count(router_ip) == 0)
        {
            std::string hop = router_ip;
            while (previous.count(hop) && previous[hop] != ip)
                hop = previous[hop];
            if (previous.count(hop) && previous[hop] == ip && hop != ip)
            {
                routing_table[router_ip] = {router_ip, hop, max_min_capacity[router_ip]};
            }
        }
    }
}

void Router::set_link_active(const std::string &neighbor_ip, bool active)
{
    if (neighbors.count(neighbor_ip))
    {
        neighbors[neighbor_ip].active = active;
    }
}