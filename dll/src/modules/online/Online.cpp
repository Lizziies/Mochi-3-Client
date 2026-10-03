#include "core/Guard.hpp"
#include "Online.hpp"
#include "I18n.hpp"
#include "core/Build.hpp"
#include "core/Log.hpp"
#include "core/Paths.hpp"
#include "modules/common/Text.hpp"

#include <json.hpp>

#include <windows.h>
#include <bcrypt.h>
#include <winhttp.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <format>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <utility>

using nlohmann::json;

namespace online {

namespace {

using Clock = std::chrono::steady_clock;

struct Reply {
    int status = 0;
    std::string body;
};

struct Snapshot {
    Config cfg;
    std::vector<std::string> names;
    std::string self;
};

std::mutex lock;
std::condition_variable wake;
std::thread worker;
std::atomic<bool> stopping{false};

Snapshot snap;
Style ownStyle;
std::vector<Worn> ownWorn;
unsigned profileGen = 1;
std::map<std::string, User> known;
State current = State::Off;
std::string detail;
bool forgetNow = false;
bool demoDirty = false;

std::string secretKey;

std::string lower(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

uint32_t fnv(const std::string& s) {
    uint32_t h = 2166136261u;
    for (unsigned char c : s) h = (h ^ c) * 16777619u;
    return h;
}

void setState(State s, std::string text = {}) {
    std::scoped_lock g(lock);
    current = s;
    detail = std::move(text);
}

std::string loadSecret() {
    auto file = paths::root() / L"online.key";
    std::ifstream in(file);
    std::string key;
    if (in && std::getline(in, key) && key.size() == 48) return key;
    unsigned char raw[24];
    if (BCryptGenRandom(nullptr, raw, sizeof(raw), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) return {};
    static const char* digits = "0123456789abcdef";
    key.clear();
    for (unsigned char b : raw) {
        key += digits[b >> 4];
        key += digits[b & 15];
    }
    std::ofstream out(file, std::ios::trunc);
    out << key << "\n";
    return key;
}

Reply post(const std::string& base, const std::string& path, const json& body, const std::string& token) {
    Reply r;
    std::wstring url = logger::widen(base);
    URL_COMPONENTSW parts{};
    parts.dwStructSize = sizeof(parts);
    parts.dwHostNameLength = parts.dwUrlPathLength = parts.dwSchemeLength = (DWORD)-1;
    if (!WinHttpCrackUrl(url.c_str(), (DWORD)url.size(), 0, &parts) || !parts.lpszHostName || !parts.dwHostNameLength) return r;
    if (!parts.lpszUrlPath) parts.dwUrlPathLength = 0;

    std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
    std::wstring prefix(parts.lpszUrlPath ? parts.lpszUrlPath : L"", parts.dwUrlPathLength);
    while (!prefix.empty() && prefix.back() == L'/') prefix.pop_back();
    std::wstring target = prefix + logger::widen(path);
    bool secure = parts.nScheme == INTERNET_SCHEME_HTTPS;

    HINTERNET session = WinHttpOpen(L"Monchi", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) return r;
    WinHttpSetTimeouts(session, 4000, 4000, 4000, 4000);
    HINTERNET conn = WinHttpConnect(session, host.c_str(), parts.nPort, 0);
    HINTERNET req = conn ? WinHttpOpenRequest(conn, L"POST", target.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, secure ? WINHTTP_FLAG_SECURE : 0) : nullptr;

    std::string payload = body.dump();
    std::wstring headers = L"Content-Type: application/json\r\n";
    if (!token.empty()) headers += L"Authorization: Bearer " + logger::widen(token) + L"\r\n";

    if (req && WinHttpSendRequest(req, headers.c_str(), (DWORD)-1, payload.data(), (DWORD)payload.size(), (DWORD)payload.size(), 0) && WinHttpReceiveResponse(req, nullptr)) {
        DWORD status = 0, size = sizeof(status);
        WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);
        r.status = (int)status;
        DWORD avail = 0;
        while (WinHttpQueryDataAvailable(req, &avail) && avail) {
            std::string chunk(avail, '\0');
            DWORD got = 0;
            if (!WinHttpReadData(req, chunk.data(), avail, &got)) break;
            r.body.append(chunk.data(), got);
            if (r.body.size() > (256u << 10)) break;
        }
    }
    if (req) WinHttpCloseHandle(req);
    if (conn) WinHttpCloseHandle(conn);
    WinHttpCloseHandle(session);
    return r;
}

const char* const modeNames[] = {"solid", "gradient", "rainbow", "pulse"};

json styleJson(const Style& s) {
    return {{"mode", modeNames[(int)s.mode]}, {"a", hex(s.a)}, {"b", hex(s.b)}, {"speed", s.speed}, {"heartColor", hex(s.heartColor)}, {"heart", s.heart}};
}

json wornJson(const std::vector<Worn>& list) {
    json out = json::array();
    for (auto& w : list) {
        json tint = json::array();
        for (auto c : w.tint) tint.push_back(hex(c));
        out.push_back({{"id", w.id}, {"tint", tint}});
    }
    return out;
}

bool validId(const std::string& id) {
    return !id.empty() && id.size() <= 40 && std::all_of(id.begin(), id.end(), [](unsigned char c) { return std::islower(c) || std::isdigit(c) || c == '_'; });
}

Style styleFrom(const json& j) {
    Style s;
    if (!j.is_object()) return s;
    std::string mode = j.value("mode", "solid");
    for (int i = 0; i < 4; i++)
        if (mode == modeNames[i]) s.mode = Mode(i);
    s.a = parseHex(j.value("a", ""), s.a);
    s.b = parseHex(j.value("b", ""), s.b);
    s.speed = std::clamp(j.value("speed", 1.f), 0.1f, 5.f);
    s.heartColor = parseHex(j.value("heartColor", ""), s.heartColor);
    s.heart = j.value("heart", true);
    return s;
}

std::vector<Worn> wornFrom(const json& j) {
    std::vector<Worn> out;
    if (!j.is_array()) return out;
    for (auto& w : j) {
        if (out.size() >= 8 || !w.is_object() || !w.contains("id") || !w["id"].is_string()) continue;
        Worn item;
        item.id = w["id"].get<std::string>();
        if (!validId(item.id)) continue;
        if (w.contains("tint") && w["tint"].is_array())
            for (auto& c : w["tint"])
                if (c.is_string() && item.tint.size() < 4) item.tint.push_back(parseHex(c.get<std::string>(), 0xffffff));
        out.push_back(std::move(item));
    }
    return out;
}

User userFrom(const json& j) {
    User u;
    u.name = j.value("name", "");
    u.style = styleFrom(j.value("style", json::object()));
    u.worn = wornFrom(j.value("worn", json::array()));
    if (auto r = j.value("role", ""); r == "owner" || r == "staff") u.role = r;
    return u;
}

const uint32_t demoColors[] = {0xff7eb6, 0x7ec8ff, 0xffd27e, 0x8be8b0, 0xc77dff, 0xff6b6b};

bool demoMember(const std::string& name) {
    static const char* fixed[] = {"luna", "kiki", "teammate", "finn", "monchiplayer"};
    std::string n = lower(name);
    return std::any_of(std::begin(fixed), std::end(fixed), [&](const char* f) { return n == f; }) || fnv(n) % 6 == 0;
}

User demoUser(const std::string& name) {
    uint32_t h = fnv(lower(name));
    User u;
    u.name = name;
    u.style.mode = Mode(h % 4);
    u.style.a = demoColors[(h >> 3) % 6];
    u.style.b = demoColors[(h >> 7) % 6];
    u.style.speed = 0.6f + float((h >> 11) % 8) * 0.2f;
    u.style.heart = (h >> 9) % 7 != 0;
    if (h % 3 == 0) u.worn.push_back({"sakura_wings", {u.style.a, 0xffffff}});
    return u;
}

void rebuildDemo(const Snapshot& s) {
    std::map<std::string, User> fresh;
    for (auto& n : s.names)
        if (demoMember(n)) fresh[lower(n)] = demoUser(n);
    if (s.cfg.visible && !s.self.empty()) fresh[lower(s.self)] = User{s.self, ownStyle, ownWorn};
    std::scoped_lock g(lock);
    known = std::move(fresh);
    current = State::Demo;
    detail = i18n::tr("Showing made-up Monchi users, nothing is sent.");
}

struct Net {
    std::string token;
    std::string server;
    unsigned sentGen = 0;
    bool sentVisible = false;
    Clock::time_point nextTry{};
    Clock::time_point presenceAt{};
    Clock::time_point lookupAt{};
    std::map<std::string, Clock::time_point> asked;
    std::string url;
};

void fail(Net& net, int status, int backoff) {
    if (status == 401) net.token.clear();
    if (status == 403) setState(State::Claimed, i18n::tr("This gamertag is registered by another Monchi install."));
    else setState(State::Offline, status ? i18n::fmt("Service answered {}.", status) : i18n::tr("Service not reachable."));
    net.nextTry = Clock::now() + std::chrono::seconds(backoff);
}

void hello(Net& net, const Snapshot& s, const Style& st, const std::vector<Worn>& wn, unsigned gen) {
    json body = {{"name", s.self}, {"client", build::version}, {"secret", secretKey}, {"visible", s.cfg.visible}, {"style", styleJson(st)}, {"worn", wornJson(wn)}};
    Reply r = post(net.url, "/v1/hello", body, {});
    if (r.status != 200) return fail(net, r.status, r.status == 403 ? 60 : 15);
    auto j = json::parse(r.body, nullptr, false);
    if (j.is_discarded() || !j.contains("token") || !j["token"].is_string()) return fail(net, 0, 30);
    net.token = j["token"].get<std::string>();
    net.sentGen = gen;
    net.sentVisible = s.cfg.visible;
    net.presenceAt = {};
    net.lookupAt = {};
    net.asked.clear();
    setState(State::Online);
    logger::info("online: signed in as {}", s.self);
}

void sendProfile(Net& net, const Snapshot& s, const Style& st, const std::vector<Worn>& wn, unsigned gen) {
    json body = {{"visible", s.cfg.visible}, {"style", styleJson(st)}, {"worn", wornJson(wn)}};
    Reply r = post(net.url, "/v1/profile", body, net.token);
    if (r.status != 200) return fail(net, r.status, 10);
    net.sentGen = gen;
    net.sentVisible = s.cfg.visible;
}

void sendPresence(Net& net, const Snapshot& s) {
    Reply r = post(net.url, "/v1/presence", {{"server", s.cfg.server}}, net.token);
    if (r.status != 200) return fail(net, r.status, 10);
    net.server = s.cfg.server;
    net.presenceAt = Clock::now();
}

std::vector<std::string> due(const Net& net, const Snapshot& s) {
    std::vector<std::string> out;
    auto now = Clock::now();
    std::scoped_lock g(lock);
    for (auto& n : s.names) {
        if (out.size() >= 100) break;
        std::string key = lower(n);
        auto it = net.asked.find(key);
        auto ttl = std::chrono::seconds(known.count(key) ? 120 : 300);
        if (it == net.asked.end() || now - it->second > ttl) out.push_back(n);
    }
    return out;
}

void sendLookup(Net& net, const Snapshot& s, const std::vector<std::string>& asked) {
    json names = json::array();
    for (auto& n : asked) names.push_back(n);
    Reply r = post(net.url, "/v1/lookup", {{"names", names}}, net.token);
    net.lookupAt = Clock::now();
    if (r.status != 200) return fail(net, r.status, 10);
    auto j = json::parse(r.body, nullptr, false);
    if (j.is_discarded() || !j.contains("users") || !j["users"].is_array()) return;
    std::map<std::string, User> found;
    for (auto& e : j["users"]) {
        User u = userFrom(e);
        if (!u.name.empty()) found[lower(u.name)] = std::move(u);
    }
    std::set<std::string> present;
    for (auto& n : s.names) present.insert(lower(n));
    for (auto& n : asked) net.asked[lower(n)] = net.lookupAt;
    for (auto it = net.asked.begin(); it != net.asked.end();) it = present.count(it->first) ? std::next(it) : net.asked.erase(it);

    std::scoped_lock g(lock);
    for (auto& n : asked) known.erase(lower(n));
    for (auto& [k, u] : found) known[k] = std::move(u);
    for (auto it = known.begin(); it != known.end();) it = present.count(it->first) ? std::next(it) : known.erase(it);
    current = State::Online;
    detail.clear();
}

void signOut(Net& net) {
    if (!net.token.empty()) post(net.url, "/v1/bye", json::object(), net.token);
    net.token.clear();
    net.server.clear();
    std::scoped_lock g(lock);
    known.clear();
}

void loop() {
    Net net;
    for (;;) {
        Snapshot s;
        Style st;
        std::vector<Worn> wn;
        unsigned gen = 0;
        bool erase = false;
        {
            std::unique_lock g(lock);
            wake.wait_for(g, std::chrono::milliseconds(300), [] { return stopping.load(); });
            if (stopping) break;
            s = snap;
            st = ownStyle;
            wn = ownWorn;
            gen = profileGen;
            erase = std::exchange(forgetNow, false);
        }
        if (s.cfg.demo) continue;
        if (s.cfg.url != net.url) {
            signOut(net);
            net.url = s.cfg.url;
        }
        if (erase && !net.url.empty() && !net.token.empty()) {
            post(net.url, "/v1/forget", json::object(), net.token);
            net.token.clear();
            std::scoped_lock g(lock);
            known.clear();
        }
        if (!s.cfg.on) {
            if (!net.token.empty()) signOut(net);
            setState(State::Off);
            continue;
        }
        if (net.url.empty()) {
            setState(State::NoUrl, i18n::tr("No service address set."));
            continue;
        }
        if (s.self.empty() || Clock::now() < net.nextTry) continue;

        if (net.token.empty()) {
            if (secretKey.empty()) secretKey = loadSecret();
            if (secretKey.empty()) continue;
            setState(State::Starting);
            hello(net, s, st, wn, gen);
            continue;
        }
        if (net.sentGen != gen || net.sentVisible != s.cfg.visible) {
            sendProfile(net, s, st, wn, gen);
            continue;
        }
        auto now = Clock::now();
        if (net.server != s.cfg.server || now - net.presenceAt > std::chrono::seconds(120)) {
            sendPresence(net, s);
            continue;
        }
        if (now - net.lookupAt > std::chrono::seconds(15)) {
            auto ask = due(net, s);
            if (!ask.empty()) sendLookup(net, s, ask);
        }
    }
    signOut(net);
}

void ensureWorker() {
    if (!worker.joinable()) worker = std::thread([] { guard::call("online", [] { loop(); }); });
}

}

std::string hex(uint32_t c) {
    char buf[8];
    snprintf(buf, sizeof(buf), "#%06x", c & 0xffffff);
    return buf;
}

uint32_t parseHex(const std::string& s, uint32_t fallback) {
    if (s.size() != 7 || s[0] != '#') return fallback;
    for (size_t i = 1; i < 7; i++)
        if (!std::isxdigit((unsigned char)s[i])) return fallback;
    return (uint32_t)std::strtoul(s.c_str() + 1, nullptr, 16);
}

const char* modeId(Mode m) { return modeNames[(int)m]; }

ImU32 rgb(uint32_t c, float alpha) { return IM_COL32((c >> 16) & 255, (c >> 8) & 255, c & 255, int(std::clamp(alpha, 0.f, 1.f) * 255.f)); }

ImU32 color(const Style& s, double t, int index, int total) {
    auto mix = [](uint32_t a, uint32_t b, float f) {
        auto ch = [&](int shift) { return int(float((a >> shift) & 255) * (1.f - f) + float((b >> shift) & 255) * f); };
        return IM_COL32(ch(16), ch(8), ch(0), 255);
    };
    float f = total > 1 ? float(index) / float(total - 1) : 0.f;
    switch (s.mode) {
    case Mode::Solid: return rgb(s.a);
    case Mode::Gradient: return mix(s.a, s.b, f);
    case Mode::Rainbow: {
        float h = std::fmod(float(t) * 0.15f * s.speed + float(index) * 0.07f, 1.f);
        float r, g, b;
        ImGui::ColorConvertHSVtoRGB(h, 0.6f, 1.f, r, g, b);
        return IM_COL32(int(r * 255), int(g * 255), int(b * 255), 255);
    }
    case Mode::Pulse: return mix(s.a, s.b, 0.5f + 0.5f * std::sin(float(t) * s.speed * 2.4f));
    }
    return rgb(s.a);
}

void heartIcon(ImDrawList* dl, ImVec2 c, float size, ImU32 col) {
    float r = size * 0.27f;
    dl->AddCircleFilled({c.x - size * 0.25f, c.y - size * 0.14f}, r, col, 14);
    dl->AddCircleFilled({c.x + size * 0.25f, c.y - size * 0.14f}, r, col, 14);
    dl->AddTriangleFilled({c.x - size * 0.51f, c.y - size * 0.05f}, {c.x + size * 0.51f, c.y - size * 0.05f}, {c.x, c.y + size * 0.5f}, col);
}

void tick(const Config& cfg, const std::vector<std::string>& names, const std::string& self) {
    Snapshot next{cfg, names, self};
    bool demoChanged = false;
    {
        std::scoped_lock g(lock);
        demoChanged = cfg.demo && (!snap.cfg.demo || snap.names != names || snap.self != self || snap.cfg.visible != cfg.visible || demoDirty);
        if (snap.cfg.demo && !cfg.demo) known.clear();
        demoDirty = false;
        snap = next;
    }
    if (cfg.demo) {
        if (demoChanged) rebuildDemo(next);
        return;
    }
    if (!cfg.on && !worker.joinable()) {
        setState(State::Off);
        return;
    }
    ensureWorker();
}

void setStyle(const Style& s) {
    std::scoped_lock g(lock);
    if (ownStyle == s) return;
    ownStyle = s;
    profileGen++;
    demoDirty = true;
}

void setWorn(const std::vector<Worn>& w) {
    std::scoped_lock g(lock);
    if (ownWorn == w) return;
    ownWorn = w;
    profileGen++;
    demoDirty = true;
}

Style style() {
    std::scoped_lock g(lock);
    return ownStyle;
}

std::vector<Worn> worn() {
    std::scoped_lock g(lock);
    return ownWorn;
}

bool find(const std::string& name, User& out) {
    std::scoped_lock g(lock);
    auto it = known.find(lower(name));
    if (it == known.end()) return false;
    out = it->second;
    return true;
}

std::vector<User> users() {
    std::scoped_lock g(lock);
    std::vector<User> out;
    for (auto& [k, u] : known) out.push_back(u);
    return out;
}

int count() {
    std::scoped_lock g(lock);
    return (int)known.size();
}

State state() {
    std::scoped_lock g(lock);
    return current;
}

std::string stateText() {
    std::scoped_lock g(lock);
    switch (current) {
    case State::Off: return i18n::tr("Off");
    case State::NoUrl: return detail;
    case State::Starting: return i18n::tr("Connecting ...");
    case State::Online: return i18n::tr("Connected");
    case State::Demo: return detail;
    case State::Offline:
    case State::Claimed: return detail;
    }
    return {};
}

void forget() {
    std::scoped_lock g(lock);
    forgetNow = true;
}

void shutdown() {
    stopping = true;
    wake.notify_all();
    if (worker.joinable()) worker.join();
}

std::string tagLine(const std::string& line, bool names, bool hearts) {
    if (!names && !hearts) return line;
    std::string plain = text::strip(line);
    size_t end = plain.find_first_of(">:");
    size_t guillemet = plain.find("\xC2\xBB");
    if (guillemet != std::string::npos && (end == std::string::npos || guillemet < end)) end = guillemet;
    if (end == std::string::npos || end > 48) return line;
    std::string head = plain.substr(0, end);

    std::vector<User> all = users();
    const User* hit = nullptr;
    size_t at = std::string::npos;
    for (auto& u : all) {
        for (size_t p = head.find(u.name); p != std::string::npos; p = head.find(u.name, p + 1)) {
            size_t e = p + u.name.size();
            bool left = p == 0 || !std::isalnum((unsigned char)head[p - 1]);
            bool right = e >= head.size() || !std::isalnum((unsigned char)head[e]);
            if (left && right && (at == std::string::npos || p < at)) {
                hit = &u;
                at = p;
            }
            break;
        }
    }
    if (!hit) return line;

    size_t from = std::string::npos;
    for (size_t p = line.find(hit->name); p != std::string::npos; p = line.find(hit->name, p + 1)) {
        size_t e = p + hit->name.size();
        bool left = p == 0 || !std::isalnum((unsigned char)line[p - 1]);
        bool right = e >= line.size() || !std::isalnum((unsigned char)line[e]);
        if (left && right) {
            from = p;
            break;
        }
    }
    if (from == std::string::npos) return line;
    size_t to = from + hit->name.size();

    std::string out = line.substr(0, from);
    if (names) {
        double t = ImGui::GetTime();
        int total = 0;
        for (unsigned char c : hit->name)
            if ((c & 0xC0) != 0x80) total++;
        int index = 0;
        for (size_t i = 0; i < hit->name.size();) {
            size_t n = 1;
            unsigned char c = (unsigned char)hit->name[i];
            if (c >= 0xF0) n = 4;
            else if (c >= 0xE0) n = 3;
            else if (c >= 0xC0) n = 2;
            ImU32 col = color(hit->style, t, index++, total);
            out += std::format("§#{:02X}{:02X}{:02X};{}", int(col & 255), int((col >> 8) & 255), int((col >> 16) & 255), hit->name.substr(i, n));
            i += n;
        }
        out += "§r";
    } else {
        out += hit->name;
    }
    if (hearts && hit->style.heart) out += std::format(" §#{:06X};\x01§r", hit->style.heartColor & 0xffffff);
    if (!hit->role.empty()) out += " §#3BA7EC;" + badge(hit->role) + "§r";
    return out + line.substr(to);
}

std::string badge(const std::string& role) {
    if (role == "owner") return i18n::tr("[Owner]");
    if (role == "staff") return i18n::tr("[Staff]");
    return {};
}

}
