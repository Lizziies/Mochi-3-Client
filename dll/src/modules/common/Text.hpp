#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <format>
#include <string>

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
    static const char* r[] = {"", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X"};
    return n >= 0 && n <= 10 ? r[n] : std::to_string(n);
}

inline std::string effect(const std::string& id) {
    struct Pair {
        const char* id;
        const char* name;
    };
    static const Pair names[] = {
        {"speed", "Schnelligkeit"},       {"slowness", "Langsamkeit"},       {"haste", "Eile"},
        {"mining_fatigue", "Abbaulähmung"}, {"strength", "Stärke"},          {"instant_health", "Sofortheilung"},
        {"instant_damage", "Sofortschaden"}, {"jump_boost", "Sprungkraft"},  {"nausea", "Übelkeit"},
        {"regeneration", "Regeneration"}, {"resistance", "Resistenz"},       {"fire_resistance", "Feuerresistenz"},
        {"water_breathing", "Wasseratmung"}, {"invisibility", "Unsichtbarkeit"}, {"blindness", "Blindheit"},
        {"night_vision", "Nachtsicht"},   {"hunger", "Hunger"},              {"weakness", "Schwäche"},
        {"poison", "Vergiftung"},         {"wither", "Wither"},              {"health_boost", "Lebensschub"},
        {"absorption", "Absorption"},     {"saturation", "Sättigung"},       {"levitation", "Schwebekraft"},
        {"slow_falling", "Sanftes Fallen"}, {"darkness", "Dunkelheit"},
    };
    for (auto& p : names)
        if (id == p.id) return p.name;
    return pretty(id);
}

inline std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return s;
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

}
