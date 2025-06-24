#pragma once
#include <string>
#include <vector>

void udp_send(const std::string &message, int port);
std::vector<std::string> udp_receive(int port);