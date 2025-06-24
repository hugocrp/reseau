#pragma once
#include <string>
#include <vector>
class Router; // déclaration anticipée
#include "types.h"

struct LSA
{
    std::string origin_ip;
    std::string origin_name;
    std::vector<Link> links;
    int seq = 0;
    bool is_default = false;
    Router to_router() const;
};

bool receive_lsa(LSA &out);
void broadcast_lsa(const LSA &lsa);