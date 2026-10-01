#pragma once

#include <windows.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <string>

namespace mcopt {

inline bool cursorFree() {
    CURSORINFO ci{sizeof(ci)};
    return GetCursorInfo(&ci) && (ci.flags & CURSOR_SHOWING);
}

inline std::filesystem::path appData() {
    wchar_t buf[MAX_PATH]{};
    DWORD n = GetEnvironmentVariableW(L"APPDATA", buf, MAX_PATH);
    return n ? std::filesystem::path(buf) : std::filesystem::path();
}

inline std::filesystem::path mojang() { return appData() / L"Minecraft Bedrock" / L"Users"; }

inline std::filesystem::path newest(const wchar_t* tail) {
    std::error_code ec;
    std::filesystem::path best;
    std::filesystem::file_time_type bestTime{};
    for (auto& user : std::filesystem::directory_iterator(mojang(), ec)) {
        auto p = user.path() / L"games" / L"com.mojang" / tail;
        auto t = std::filesystem::last_write_time(p, ec);
        if (ec) {
            ec.clear();
            continue;
        }
        if (best.empty() || t > bestTime) {
            best = p;
            bestTime = t;
        }
    }
    return best;
}

inline std::filesystem::path optionsFile() { return newest(L"minecraftpe\\options.txt"); }

inline std::map<std::string, std::string> readOptions() {
    std::map<std::string, std::string> out;
    std::ifstream in(optionsFile());
    std::string line;
    while (std::getline(in, line)) {
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string v = line.substr(colon + 1);
        while (!v.empty() && (v.back() == '\r' || v.back() == ' ')) v.pop_back();
        out[line.substr(0, colon)] = v;
    }
    return out;
}

}
