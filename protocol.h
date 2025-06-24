#pragma once
#include <string>
#include <map>
#include <thread>
#include <atomic>
#include "router.h"
#include "lsa.h"
#include "network.h"

class Protocol
{
public:
    void init(const std::string &name, const std::vector<std::string> &ips, bool default_origin);
    void set_local_networks(const std::vector<std::string> &nets);
    void start();
    void stop();

private:
    Router self;
    std::map<std::string, Router> network_db; // LSDB
    std::thread listener_thread;
    std::thread update_thread;
    std::map<std::string, std::chrono::steady_clock::time_point> last_lsa_time;
    std::atomic<bool> running{true};
    std::thread hello_thread;

    void listen_loop();
    void update_loop();
    void handle_lsa(const LSA &lsa);
    void send_hello(const std::string &router_ip, const std::string &router_name);
    bool receive_hello(std::string &neighbor_ip, std::string &neighbor_name);
};