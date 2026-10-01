#pragma once

#include <cstdint>
#include <string>

namespace sigs {

void init();
bool takeChanged();

uintptr_t address(const std::string& name);
int offset(const std::string& name, int fallback = -1);

struct Stats {
    std::string gameVersion = "unbekannt";
    std::string source = "–";
    int found = 0;
    int total = 0;
};
Stats stats();

}
