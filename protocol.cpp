#include "protocol.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <sstream>
#include <set>
#include "udp_utils.h"
const int HELLO_PORT = 60001;
std::map<std::string, Router> network_db;

void Protocol::send_hello(const std::string &router_ip, const std::string &router_name)
{
    std::string msg = "HELLO;" + router_ip + ";" + router_name;
    std::cout << "[DEBUG] Envoi UDP Hello: " << msg << std::endl;
    udp_send(msg, HELLO_PORT);
}

bool Protocol::receive_hello(std::string &neighbor_ip, std::string &neighbor_name)
{
    static std::set<std::string> seen;
    auto messages = udp_receive(HELLO_PORT);
    for (const auto &msg : messages)
    {
        std::cout << "[DEBUG] UDP Hello reçu: " << msg << std::endl;
        if (msg.rfind("HELLO;", 0) == 0)
        {
            std::istringstream iss(msg);
            std::string tag;
            std::getline(iss, tag, ';');
            std::getline(iss, neighbor_ip, ';');
            std::getline(iss, neighbor_name, ';');
            std::string key = neighbor_ip + neighbor_name;
            if (neighbor_ip != self.ip && !seen.count(key))
            {
                seen.insert(key);
                return true;
            }
        }
    }
    return false;
}

void Protocol::init(const std::string &name, const std::vector<std::string> &ips, bool default_origin)
{
    self.name = name;
    self.ip = ips.empty() ? "" : ips[0];
    self.ips = ips;
    self.default_origin = default_origin;
}

void Protocol::start()
{
    running = true;
    listener_thread = std::thread(&Protocol::listen_loop, this);
    update_thread = std::thread(&Protocol::update_loop, this);
    hello_thread = std::thread([this]()
                               {
    std::map<std::string, std::chrono::steady_clock::time_point> last_hello_seen;
    while (running) {
        for (const auto& ip : self.ips) {
            send_hello(ip, self.name);
        }
        std::string neighbor_ip, neighbor_name;
        while (receive_hello(neighbor_ip, neighbor_name)) {
            last_hello_seen[neighbor_ip] = std::chrono::steady_clock::now();
            if (self.neighbors.find(neighbor_ip) == self.neighbors.end()) {
                self.add_link(neighbor_ip, neighbor_name, 100, true);
                std::cout << "Nouveau voisin détecté: " << neighbor_name << " (" << neighbor_ip << ")\n";
                std::cout << "[DEBUG] Ajout du voisin " << neighbor_ip << " à la table des voisins de " << self.ip << std::endl;
            } else {
                self.set_link_active(neighbor_ip, true);
            }
        }
        // Désactive les voisins injoignables
        auto now = std::chrono::steady_clock::now();
        for (auto& [ip, link] : self.neighbors) {
            if (last_hello_seen.count(ip) &&
                std::chrono::duration_cast<std::chrono::seconds>(now - last_hello_seen[ip]).count() > 20) {
                self.set_link_active(ip, false);
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(5));
    } });
}

void Protocol::listen_loop()
{
    while (running)
    {
        LSA incoming;
        if (receive_lsa(incoming))
        {
            handle_lsa(incoming);
        }
    }
}

void Protocol::handle_lsa(const LSA &lsa)
{
    std::cout << "[DEBUG] handle_lsa: LSA reçu de " << lsa.origin_ip << " (" << lsa.origin_name << "), seq=" << lsa.seq << std::endl;
    network_db[lsa.origin_ip] = lsa.to_router();
    last_lsa_time[lsa.origin_ip] = std::chrono::steady_clock::now();

    std::cout << "[DEBUG] LSDB actuelle :\n";
    for (const auto &[k, r] : network_db)
    {
        std::cout << "  " << k << " (" << r.name << ") voisins : ";
        for (const auto &[n, l] : r.neighbors)
        {
            std::cout << n << " ";
        }
        std::cout << std::endl;
    }

    self.calculate_routes(network_db);
    self.apply_routes();
}

void Protocol::update_loop()
{
    LSA last_sent_lsa;
    int seq = 0;
    while (running)
    {
        auto now = std::chrono::steady_clock::now();
        for (auto it = network_db.begin(); it != network_db.end();)
        {
            if (last_lsa_time.count(it->first) &&
                std::chrono::duration_cast<std::chrono::seconds>(now - last_lsa_time[it->first]).count() > 30)
            {
                it = network_db.erase(it);
            }
            else
            {
                ++it;
            }
        }

        LSA lsa = self.generate_lsa();
        if (lsa.links != last_sent_lsa.links)
        {
            seq++;
            lsa.seq = seq;
            broadcast_lsa(lsa);
            last_sent_lsa = lsa;
        }

        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

void Protocol::stop()
{
    running = false;
    if (listener_thread.joinable())
        listener_thread.join();
    if (update_thread.joinable())
        update_thread.join();
    if (hello_thread.joinable())
        hello_thread.join();
}

void Protocol::set_local_networks(const std::vector<std::string> &nets)
{
    self.local_networks = nets;
}