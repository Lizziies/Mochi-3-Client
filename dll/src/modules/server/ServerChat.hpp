#pragma once

#include "modules/common/Text.hpp"
#include "sdk/Inject.hpp"
#include "render/Ui.hpp"

#include <cctype>
#include <deque>
#include <string>
#include <vector>

namespace srv {

inline std::string plain(const std::string& raw) { return text::lower(text::strip(raw)); }

inline std::vector<std::string> words(const std::string& list) { return text::split(text::lower(list), ','); }

inline bool hasAny(const std::string& line, const std::vector<std::string>& list) {
    for (auto& w : list)
        if (!w.empty() && line.find(w) != std::string::npos) return true;
    return false;
}

inline std::string fill(std::string pattern, const std::string& key, const std::string& value) {
    for (size_t at = pattern.find(key); at != std::string::npos; at = pattern.find(key, at + value.size()))
        pattern.replace(at, key.size(), value);
    return pattern;
}

inline std::vector<std::string> endWords(const std::string& server) {
    if (server == "The Hive") return {"game over", "victory", "you win"};
    if (server == "Zeqa") return {"has won", "winner", "won the duel"};
    if (server == "CubeCraft") return {"won the game", "game over", "winners"};
    if (server == "Lifeboat") return {"winner", "has won", "game over"};
    if (server == "Galaxite") return {"game over", "winner", "you won", "victory"};
    if (server == "Mineville") return {"game over", "winner", "has won"};
    if (server == "NetherGames") return {"winner", "game over", "won the game"};
    return {"game over", "you won", "victory"};
}

inline bool nameChar(char c) { return std::isalnum((unsigned char)c) || c == '_' || c == '-'; }

inline std::string nameBefore(const std::string& raw, const std::vector<std::string>& phrases) {
    std::string clean = text::strip(raw), low = text::lower(clean);
    for (auto& p : phrases) {
        size_t at = p.empty() ? std::string::npos : low.find(p);
        if (at == std::string::npos) continue;
        size_t end = at;
        while (end > 0 && clean[end - 1] == ' ') end--;
        size_t begin = end;
        while (begin > 0 && (nameChar(clean[begin - 1]) || (clean[begin - 1] == ' ' && begin > 1 && nameChar(clean[begin - 2])))) begin--;
        while (begin < end && clean[begin] == ' ') begin++;
        if (end > begin && end - begin <= 24) return clean.substr(begin, end - begin);
    }
    return "";
}

inline std::string wordAfter(const std::string& raw, const std::vector<std::string>& phrases, size_t minLen = 3, size_t maxLen = 12) {
    std::string clean = text::strip(raw), low = text::lower(clean);
    for (auto& p : phrases) {
        size_t at = p.empty() ? std::string::npos : low.find(p);
        if (at == std::string::npos) continue;
        size_t from = at + p.size();
        while (from < clean.size() && !nameChar(clean[from])) from++;
        size_t to = from;
        while (to < clean.size() && nameChar(clean[to])) to++;
        if (to - from >= minLen && to - from <= maxLen) return clean.substr(from, to - from);
    }
    return "";
}

inline bool playerChat(const std::string& stripped, std::string& tag) {
    size_t colon = stripped.find(": ");
    if (colon == std::string::npos || colon > 40) return false;
    std::string head = stripped.substr(0, colon);
    tag.clear();
    if (!head.empty() && head[0] == '[') {
        size_t close = head.find(']');
        if (close == std::string::npos) return false;
        tag = head.substr(1, close - 1);
        head = head.substr(close + 1);
    }
    size_t a = head.find_first_not_of(' ');
    if (a == std::string::npos) return false;
    head = head.substr(a);
    if (head.size() > 16) return false;
    for (char c : head)
        if (!nameChar(c) && c != ' ') return false;
    return true;
}

class Outbox {
public:
    void later(double delay, std::string text) {
        for (auto& p : pending_)
            if (p.text == text) return;
        pending_.push_back({ui::time() + delay, std::move(text)});
    }

    void clear() { pending_.clear(); }
    bool waiting() const { return !pending_.empty(); }

    void flush(int chatKey) {
        double now = ui::time();
        for (size_t i = 0; i < pending_.size();) {
            if (pending_[i].due > now) {
                i++;
                continue;
            }
            inject::say(pending_[i].text, chatKey);
            pending_.erase(pending_.begin() + long(i));
        }
    }

private:
    struct Pending {
        double due;
        std::string text;
    };
    std::vector<Pending> pending_;
};

}
