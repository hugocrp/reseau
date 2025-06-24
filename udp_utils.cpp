#include "udp_utils.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <string>
#include <iostream>
#include <ifaddrs.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <map>

std::vector<std::string> get_broadcasts()
{
    std::vector<std::string> broadcasts;
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1)
        return broadcasts;
    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET &&
            !(ifa->ifa_flags & IFF_LOOPBACK) && (ifa->ifa_flags & IFF_UP))
        {
            struct sockaddr_in *broad = (struct sockaddr_in *)ifa->ifa_broadaddr;
            if (broad)
            {
                char buf[INET_ADDRSTRLEN];
                if (inet_ntop(AF_INET, &broad->sin_addr, buf, sizeof(buf)))
                    broadcasts.push_back(buf);
            }
        }
    }
    freeifaddrs(ifaddr);
    return broadcasts;
}

void udp_send(const std::string &message, int port)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    int broadcast = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    auto broadcasts = get_broadcasts();
    std::cout << "[DEBUG] Broadcasts détectés pour l'envoi : ";
    for (const auto &bcast : broadcasts)
        std::cout << bcast << " ";
    std::cout << std::endl;
    for (const auto &bcast : broadcasts)
    {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(bcast.c_str());
        sendto(sock, message.c_str(), message.size(), 0, (sockaddr *)&addr, sizeof(addr));
    }
    close(sock);
}

std::vector<std::string> udp_receive(int port)
{
    static std::map<int, int> socks;
    if (socks.count(port) == 0)
    {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        int reuse = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        bind(sock, (sockaddr *)&addr, sizeof(addr));
        socks[port] = sock;
    }
    int sock = socks[port];
    std::vector<std::string> messages;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    timeval tv{0, 0};
    while (select(sock + 1, &fds, nullptr, nullptr, &tv) > 0)
    {
        char buf[1024];
        ssize_t len = recv(sock, buf, sizeof(buf) - 1, 0);
        if (len > 0)
        {
            buf[len] = 0;
            messages.emplace_back(buf);
        }
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
    }
    return messages;
}