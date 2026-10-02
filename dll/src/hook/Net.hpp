#pragma once

#include <string>
#include <vector>

namespace net {

struct Peer {
    std::string ip;
    std::string host;
    int packets = 0;
};

void install();
void waitIdle(int timeoutMs);
bool session();
std::vector<Peer> drain();

}
