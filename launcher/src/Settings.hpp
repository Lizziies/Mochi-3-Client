#pragma once

#include <string>

struct Settings {
    bool beta = false;
    bool autoInject = true;
    bool closeAfterInject = false;
    std::string customDll;

    static Settings load();
    void save() const;
};
