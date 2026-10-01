#pragma once

#include "I18n.hpp"

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <format>
#include <string>
#include <utility>
#include <vector>

namespace text {

inline std::string num(float v, int decimals) {
    switch (std::clamp(decimals, 0, 4)) {
    case 0: return std::format("{:.0f}", v);
    case 1: return std::format("{:.1f}", v);
    case 2: return std::format("{:.2f}", v);
    case 3: return std::format("{:.3f}", v);
    default: return std::format("{:.4f}", v);
    }
}

inline std::string clock(float seconds) {
    int s = std::max(0, int(std::ceil(seconds)));
    if (s >= 3600) return std::format("{}:{:02}:{:02}", s / 3600, (s / 60) % 60, s % 60);
    return std::format("{}:{:02}", s / 60, s % 60);
}

inline std::string pretty(std::string id) {
    bool up = true;
    for (auto& c : id) {
        if (c == '_') {
            c = ' ';
            up = true;
        } else if (up) {
            c = char(std::toupper((unsigned char)c));
            up = false;
        }
    }
    return id;
}

inline std::string roman(int n) {
    if (n <= 0 || n >= 4000) return std::to_string(n);
    static const std::pair<int, const char*> steps[] = {{1000, "M"}, {900, "CM"}, {500, "D"}, {400, "CD"}, {100, "C"}, {90, "XC"}, {50, "L"},
                                                         {40, "XL"},  {10, "X"},   {9, "IX"},  {5, "V"},    {4, "IV"},  {1, "I"}};
    std::string out;
    for (auto& [value, glyphs] : steps)
        for (; n >= value; n -= value) out += glyphs;
    return out;
}

inline std::string effect(const std::string& id) {
    struct Pair {
        const char* id;
        const char* name;
    };
    static const Pair names[] = {
        {"speed", "Speed"},       {"slowness", "Slowness"},       {"haste", "Haste"},
        {"mining_fatigue", "Mining Fatigue"}, {"strength", "Strength"},          {"instant_health", "Instant Health"},
        {"instant_damage", "Instant Damage"}, {"jump_boost", "Jump Boost"},  {"nausea", "Nausea"},
        {"regeneration", "Regeneration"}, {"resistance", "Resistance"},       {"fire_resistance", "Fire Resistance"},
        {"water_breathing", "Water Breathing"}, {"invisibility", "Invisibility"}, {"blindness", "Blindness"},
        {"night_vision", "Night Vision"},   {"hunger", "Hunger"},              {"weakness", "Weakness"},
        {"poison", "Poison"},         {"wither", "Wither"},              {"health_boost", "Health Boost"},
        {"absorption", "Absorption"},     {"saturation", "Saturation"},       {"levitation", "Levitation"},
        {"slow_falling", "Slow Falling"}, {"darkness", "Darkness"},
    };
    for (auto& p : names)
        if (id == p.id) return i18n::tr(p.name);
    return pretty(id);
}

inline std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

inline std::vector<std::string> split(const std::string& in, char sep) {
    std::vector<std::string> out;
    size_t from = 0;
    while (from <= in.size()) {
        size_t to = in.find(sep, from);
        if (to == std::string::npos) to = in.size();
        size_t a = in.find_first_not_of(' ', from), b = in.find_last_not_of(' ', to ? to - 1 : 0);
        if (a != std::string::npos && a < to && b >= a) out.push_back(in.substr(a, b - a + 1));
        from = to + 1;
    }
    return out;
}

inline std::string strip(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); i++) {
        if ((unsigned char)s[i] == 0xC2 && i + 2 < s.size() && (unsigned char)s[i + 1] == 0xA7) {
            i += 2;
            continue;
        }
        out += s[i];
    }
    return out;
}

struct Segment {
    std::string text;
    ImU32 color;
};

inline std::vector<Segment> colored(const std::string& in, ImU32 base) {
    static const ImU32 palette[16] = {IM_COL32(0, 0, 0, 255),       IM_COL32(0, 0, 170, 255),     IM_COL32(0, 170, 0, 255),    IM_COL32(0, 170, 170, 255),
                                      IM_COL32(170, 0, 0, 255),     IM_COL32(170, 0, 170, 255),   IM_COL32(255, 170, 0, 255),  IM_COL32(170, 170, 170, 255),
                                      IM_COL32(85, 85, 85, 255),    IM_COL32(85, 85, 255, 255),   IM_COL32(85, 255, 85, 255),  IM_COL32(85, 255, 255, 255),
                                      IM_COL32(255, 85, 85, 255),   IM_COL32(255, 85, 255, 255),  IM_COL32(255, 255, 85, 255), IM_COL32(255, 255, 255, 255)};
    std::vector<Segment> out;
    ImU32 color = base;
    std::string cur;
    auto flush = [&] {
        if (!cur.empty()) out.push_back({cur, color});
        cur.clear();
    };
    for (size_t i = 0; i < in.size(); i++) {
        if ((unsigned char)in[i] == 0xC2 && i + 2 < in.size() && (unsigned char)in[i + 1] == 0xA7) {
            char code = (char)std::tolower((unsigned char)in[i + 2]);
            flush();
            int idx = code >= '0' && code <= '9' ? code - '0' : code >= 'a' && code <= 'f' ? code - 'a' + 10 : -1;
            if (idx >= 0) color = palette[idx];
            else if (code == 'r') color = base;
            i += 2;
            continue;
        }
        cur += in[i];
    }
    flush();
    return out;
}

}
