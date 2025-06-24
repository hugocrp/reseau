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

#include "udp_utils.h"
const int LSA_PORT = 60002;

void broadcast_lsa(const LSA &lsa)
{
    std::ostringstream oss;
    oss << "LSA;" << lsa.origin_ip << ";" << lsa.origin_name << ";" << lsa.seq << ";" << (lsa.is_default ? "1" : "0") << ";";
    for (size_t i = 0; i < lsa.links.size(); ++i)
    {
        const auto &l = lsa.links[i];
        oss << l.neighbor_ip << "," << l.neighbor_name << "," << (l.active ? "1" : "0") << "," << l.capacity;
        if (i + 1 < lsa.links.size())
            oss << "|";
    }
    udp_send(oss.str(), LSA_PORT);
}

bool receive_lsa(LSA &out)
{
    static std::map<std::string, int> last_seq_seen;
    auto messages = udp_receive(LSA_PORT);
    for (const auto &msg : messages)
    {
        std::cout << "[DEBUG] UDP LSA reçu: " << msg << std::endl;
        if (msg.rfind("LSA;", 0) == 0)
        {
            std::istringstream iss(msg);
            std::string tag;
            std::getline(iss, tag, ';');
            if (!std::getline(iss, out.origin_ip, ';'))
                continue;
            if (!std::getline(iss, out.origin_name, ';'))
                continue;
            std::string seq_str;
            if (!std::getline(iss, seq_str, ';'))
                continue;
            try
            {
                out.seq = std::stoi(seq_str);
            }
            catch (...)
            {
                out.seq = 0;
            }
            std::string is_default_str;
            if (!std::getline(iss, is_default_str, ';'))
                continue;
            out.is_default = (is_default_str == "1");

            std::string links_str;
            if (!std::getline(iss, links_str, ';'))
                links_str = "";
            std::istringstream lss(links_str);
            std::string link;
            out.links.clear();
            while (std::getline(lss, link, '|'))
            {
                if (link.empty())
                    continue;
                std::istringstream lnk(link);
                Link l;
                std::string active_str, capacity_str;
                if (!std::getline(lnk, l.neighbor_ip, ','))
                    continue;
                if (!std::getline(lnk, l.neighbor_name, ','))
                    continue;
                if (!std::getline(lnk, active_str, ','))
                    continue;
                if (!std::getline(lnk, capacity_str, ','))
                    continue;
                l.active = (active_str == "1");
                try
                {
                    l.capacity = std::stoi(capacity_str);
                }
                catch (...)
                {
                    l.capacity = 1;
                }
                out.links.push_back(l);
            }
            if (last_seq_seen[out.origin_ip] < out.seq)
            {
                last_seq_seen[out.origin_ip] = out.seq;
                return true;
            }
        }
    }
    return false;
}

Router LSA::to_router() const
{
    Router r;
    r.name = origin_name;
    r.ip = origin_ip;
    r.default_origin = is_default;
    for (const auto &link : links)
    {
        r.neighbors[link.neighbor_ip] = link;
        if (link.neighbor_name == "NETWORK")
        {
            r.local_networks.push_back(link.neighbor_ip);
        }
    }
    return r;
}