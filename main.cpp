#include "protocol.h"
#include <iostream>
#include <string>
#include <array>
#include <memory>
#include <unistd.h>
#include <limits.h>
#include <csignal>
#include <atomic>
#include <thread>
#include <vector>
#include <regex>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#define HOST_NAME_MAX 1024

std::atomic<bool> running{true};

void signal_handler(int)
{
    running = false;
}

std::vector<std::string> get_all_ips()
{
    std::vector<std::string> ips;
    std::array<char, 256> buffer;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("ip -o -4 addr show | awk '!/ lo / {print $4}' | cut -d/ -f1", "r"), pclose);
    if (!pipe)
        return ips;
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        std::string ip = buffer.data();
        ip.erase(std::remove(ip.begin(), ip.end(), '\n'), ip.end());
        if (!ip.empty())
            ips.push_back(ip);
    }
    return ips;
}

std::vector<std::string> detect_local_networks()
{
    std::vector<std::string> networks;
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1)
        return networks;
    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET &&
            !(ifa->ifa_flags & IFF_LOOPBACK) && (ifa->ifa_flags & IFF_UP))
        {
            struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
            struct sockaddr_in *netmask = (struct sockaddr_in *)ifa->ifa_netmask;
            if (addr && netmask)
            {
                uint32_t ip = ntohl(addr->sin_addr.s_addr);
                uint32_t mask = ntohl(netmask->sin_addr.s_addr);
                uint32_t network = ip & mask;
                int prefix = 0;
                uint32_t m = mask;
                while (m)
                {
                    prefix += m & 1;
                    m >>= 1;
                }
                struct in_addr net_addr;
                net_addr.s_addr = htonl(network);
                char buf[INET_ADDRSTRLEN];
                if (inet_ntop(AF_INET, &net_addr, buf, sizeof(buf)))
                {
                    std::ostringstream oss;
                    oss << buf << "/" << prefix;
                    networks.push_back(oss.str());
                }
            }
        }
    }
    freeifaddrs(ifaddr);
    return networks;
}

std::string get_hostname()
{
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    return std::string(hostname);
}

int main(int argc, char *argv[])
{
    auto ips = get_all_ips();
    for (const auto &ip : ips)
    {
        std::cout << "[DEBUG] Interface IP détectée: " << ip << std::endl;
    }
    bool default_origin = false;
    std::vector<std::string> local_networks = detect_local_networks();
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "default")
            default_origin = true;
    }
    for (const auto &net : local_networks)
        std::cout << "[DEBUG] Réseau local détecté: " << net << std::endl;

    Protocol proto;
    proto.init(get_hostname(), ips, default_origin);
    proto.set_local_networks(local_networks);
    proto.start();

    std::signal(SIGINT, signal_handler);
    std::cout << "Appuyez sur Ctrl+C pour arrêter le protocole..." << std::endl;
    while (running)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    proto.stop();
    return 0;
}