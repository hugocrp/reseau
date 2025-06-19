#include "lsa.h"
#include "router.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>

// Pour la démo : chaque routeur lit/écrit dans un fichier LSA partagé (à remplacer par du vrai UDP plus tard)
static std::mutex lsa_mutex;

bool receive_lsa(LSA& out) {
    std::lock_guard<std::mutex> lock(lsa_mutex);
    std::ifstream fin("lsa_in.txt");
    if (!fin.is_open()) return false;

    std::string line;
    if (!std::getline(fin, line)) return false;
    fin.close();

    // Format simple : ip;name;neighbor_ip1,neighbor_name1,active1,capacity1|neighbor_ip2,neighbor_name2,active2,capacity2
    std::istringstream iss(line);
    std::getline(iss, out.origin_ip, ';');
    std::getline(iss, out.origin_name, ';');
    std::string links_str;
    std::getline(iss, links_str, ';');
    std::istringstream lss(links_str);
    std::string link;
    out.links.clear();
    while (std::getline(lss, link, '|')) {
        std::istringstream lnk(link);
        Link l;
        std::string active_str, capacity_str;
        std::getline(lnk, l.neighbor_ip, ',');
        std::getline(lnk, l.neighbor_name, ',');
        std::getline(lnk, active_str, ',');
        std::getline(lnk, capacity_str, ',');
        l.active = (active_str == "1");
        l.capacity = std::stoi(capacity_str);
        out.links.push_back(l);
    }
    return true;
}

void broadcast_lsa(const LSA& lsa) {
    std::lock_guard<std::mutex> lock(lsa_mutex);
    std::ofstream fout("lsa_in.txt", std::ios::trunc);
    fout << lsa.origin_ip << ";" << lsa.origin_name << ";";
    for (size_t i = 0; i < lsa.links.size(); ++i) {
        const auto& l = lsa.links[i];
        fout << l.neighbor_ip << "," << l.neighbor_name << "," << (l.active ? "1" : "0") << "," << l.capacity;
        if (i + 1 < lsa.links.size()) fout << "|";
    }
    fout << std::endl;
    fout.close();
}

Router LSA::to_router() const {
    Router r;
    r.name = origin_name;
    r.ip = origin_ip;
    for (const auto& link : links) {
        r.neighbors[link.neighbor_ip] = link;
    }
    return r;
}