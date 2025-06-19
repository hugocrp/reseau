#pragma once
#include <string>
#include <map>
#include <thread>
#include <atomic>
#include "router.h"
#include "lsa.h"
#include "network.h"


class Protocol {
public:
    bool load_config(const std::string& filepath);
    void start();

private:
    Router self;
    std::map<std::string, Router> network_db; // LSDB
    std::thread listener_thread;
    std::thread update_thread;
    std::atomic<bool> running{true};

    void listen_loop();
    void update_loop();
    void handle_lsa(const LSA& lsa);
    void stop();

};