#include "protocol.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include "include/json.hpp"

using json = nlohmann::json;

bool Protocol::load_config(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) return false;

    json conf;
    in >> conf;
    self.name = conf["name"];
    self.ip = conf["ip"];
    self.default_origin = conf.value("default_origin", false);

    for (auto& iface : conf["interfaces"]) {
        self.add_link(
            iface["neighbor_ip"],
            iface["neighbor_name"],
            iface["capacity"],
            iface.value("active", true)
        );
    }

    return true;
}

void Protocol::start() {
    running = true;
    listener_thread = std::thread(&Protocol::listen_loop, this);
    update_thread = std::thread(&Protocol::update_loop, this);
}

void Protocol::listen_loop() {
    while (running) {
        LSA incoming;
        if (receive_lsa(incoming)) {
            handle_lsa(incoming);
        }
    }
}

void Protocol::handle_lsa(const LSA& lsa) {
    network_db[lsa.origin_ip] = lsa.to_router();
    self.calculate_routes(network_db);
    self.apply_routes();
}

void Protocol::update_loop() {
    while (running) {
        LSA lsa = self.generate_lsa();
        broadcast_lsa(lsa);
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

void Protocol::stop() {
    running = false;
    if (listener_thread.joinable()) listener_thread.join();
    if (update_thread.joinable()) update_thread.join();
}