#pragma once

#include <string>
#include <vector>

struct Settings {
    bool beta = false;
    bool autoInject = true;
    bool closeAfterInject = false;
    std::string customDll;
    std::string pinned;
    int accent = -1;
    std::vector<std::string> folders;

    static Settings load();
    void save() const;
};
