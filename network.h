#pragma once
#include "lsa.h"

bool receive_lsa(LSA& out);
void broadcast_lsa(const LSA& lsa);