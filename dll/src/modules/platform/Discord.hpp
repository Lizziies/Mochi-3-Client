#pragma once

#include <cstdint>
#include <string>

namespace discord {

struct Presence {
    std::string details;
    std::string state;
    std::string largeImage;
    std::string largeText;
    std::string smallImage;
    std::string smallText;
    int64_t start = 0;
    bool active = false;

    bool operator==(const Presence&) const = default;
};

enum class Status { Off, Searching, Connected };

void start(const std::string& appId);
void stop();
void set(const Presence& p);
Status status();

std::string frame(uint32_t opcode, const std::string& json);
std::string activityJson(const Presence& p, unsigned long pid, int nonce);

}
