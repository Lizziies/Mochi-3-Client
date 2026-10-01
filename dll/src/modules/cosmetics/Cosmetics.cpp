#include "Cosmetics.hpp"
#include "core/Http.hpp"
#include "core/Log.hpp"
#include "core/Paths.hpp"
#include "modules/common/Image.hpp"

#include <imgui_internal.h>
#include <json.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <functional>
#include <fstream>
#include <mutex>
#include <set>
#include <unordered_map>
#include <thread>

using nlohmann::json;
namespace fs = std::filesystem;

namespace cosmetics {

int Item::cubeCount() const {
    int n = 0;
    for (auto& b : bones) n += int(b.cubes.size());
    return n;
}

namespace {

constexpr float pi = 3.14159265f;

std::vector<std::unique_ptr<Item>> loaded;
std::deque<fs::path> pending;
std::set<std::string> seen;
bool started = false;
std::atomic<bool> synced{false};
std::atomic<bool> syncing{false};
std::mutex lock;
std::string status;

bool validId(const std::string& id) {
    return !id.empty() && id.size() <= 40 && std::all_of(id.begin(), id.end(), [](unsigned char c) { return std::islower(c) || std::isdigit(c) || c == '_'; });
}

uint32_t hexColor(const std::string& s, uint32_t fallback) {
    if (s.size() != 7 || s[0] != '#') return fallback;
    for (size_t i = 1; i < 7; i++)
        if (!std::isxdigit((unsigned char)s[i])) return fallback;
    return (uint32_t)std::strtoul(s.c_str() + 1, nullptr, 16);
}

template <size_t N>
bool numbers(const json& j, const char* key, float (&out)[N]) {
    if (!j.contains(key)) return false;
    auto& a = j[key];
    if (!a.is_array() || a.size() != N) return false;
    for (size_t i = 0; i < N; i++) {
        if (!a[i].is_number()) return false;
        out[i] = a[i].get<float>();
    }
    return true;
}

Anim animOf(const std::string& s) {
    static const char* names[] = {"none", "flap", "sway", "bob", "wag", "twitch", "float", "spin", "sparkle"};
    for (int i = 0; i < 9; i++)
        if (s == names[i]) return Anim(i);
    return Anim::None;
}

char defaultAxis(Anim a) {
    switch (a) {
    case Anim::Sway: return 'z';
    case Anim::Twitch: return 'z';
    default: return 'y';
    }
}

bool bones(const json& j, Item& out, std::string& why) {
    if (!j.contains("bones") || !j["bones"].is_array() || j["bones"].empty()) {
        why = "no bones";
        return false;
    }
    int cubes = 0;
    for (auto& jb : j["bones"]) {
        Bone b;
        b.name = jb.value("name", "bone");
        numbers(jb, "pivot", b.pivot);
        numbers(jb, "rotation", b.rot);
        if (jb.contains("anim") && jb["anim"].is_object()) {
            auto& a = jb["anim"];
            b.anim = animOf(a.value("type", "none"));
            std::string axis = a.value("axis", std::string(1, defaultAxis(b.anim)));
            b.axis = axis == "x" || axis == "z" ? axis[0] : 'y';
            b.amp = std::clamp(a.value("amplitude", 0.f), -180.f, 180.f);
            b.speed = std::clamp(a.value("speed", 1.f), 0.f, 10.f);
            b.phase = a.value("phase", 0.f);
        }
        b.spring = b.anim != Anim::None;
        if (jb.contains("physics")) {
            auto& ph = jb["physics"];
            std::string type = ph.is_string() ? ph.get<std::string>() : ph.is_object() ? ph.value("type", "spring") : "";
            b.cloth = type == "cloth";
            b.spring = b.spring || type == "spring";
            if (ph.is_object()) {
                b.stiffness = std::clamp(ph.value("stiffness", b.cloth ? 38.f : 60.f), 1.f, 400.f);
                b.damping = std::clamp(ph.value("damping", b.cloth ? 2.4f : 7.f), 0.1f, 60.f);
                b.inertia = std::clamp(ph.value("inertia", 1.f), 0.f, 5.f);
                b.wind = std::clamp(ph.value("wind", 1.f), 0.f, 4.f);
                if (ph.contains("drive") && ph["drive"].is_object()) {
                    numbers(ph["drive"], "air", b.driveAir);
                    numbers(ph["drive"], "sprint", b.driveSprint);
                    numbers(ph["drive"], "sneak", b.driveSneak);
                    numbers(ph["drive"], "speed", b.driveSpeed);
                }
            }
        }
        if (!jb.contains("cubes") || !jb["cubes"].is_array()) continue;
        for (auto& jc : jb["cubes"]) {
            Cube c;
            float uv[2]{};
            if (!numbers(jc, "origin", c.origin) || !numbers(jc, "size", c.size)) {
                why = "cube without origin or size";
                return false;
            }
            numbers(jc, "uv", uv);
            c.flat = jc.value("flat", false);
            c.mirror = jc.value("mirror", false);
            c.uv[0] = int(uv[0]);
            c.uv[1] = int(uv[1]);
            for (float s : c.size)
                if (s <= 0.f || s > 64.f) {
                    why = "cube size out of range";
                    return false;
                }
            if (jc.contains("tint") && jc["tint"].is_string()) {
                std::string tn = jc["tint"].get<std::string>();
                for (size_t i = 0; i < out.tints.size(); i++)
                    if (out.tints[i].name == tn) c.tint = int(i);
            }
            if (jc.contains("tint2") && jc["tint2"].is_string()) {
                std::string tn = jc["tint2"].get<std::string>();
                for (size_t i = 0; i < out.tints.size(); i++)
                    if (out.tints[i].name == tn) c.tint2 = int(i);
                c.mix = std::clamp(jc.value("mix", 0.f), 0.f, 1.f);
            }
            b.cubes.push_back(c);
            if (++cubes > maxCubes) {
                why = "more than 80 cubes";
                return false;
            }
        }
        out.bones.push_back(std::move(b));
    }
    return cubes > 0;
}

void makeTexture(Item& item, const img::Pixels& px) {
    auto* t = IM_NEW(ImTextureData)();
    t->Create(ImTextureFormat_RGBA32, px.w, px.h);
    std::memcpy(t->GetPixels(), px.rgba.data(), px.rgba.size() * 4);
    t->UsedRect = {0, 0, (unsigned short)px.w, (unsigned short)px.h};
    t->UpdateRect = t->UsedRect;
    t->UseColors = true;
    ImGui::RegisterUserTexture(t);
    item.tex = t;
    item.texW = px.w;
    item.texH = px.h;
}

std::string readFile(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return in ? std::string(std::istreambuf_iterator<char>(in), {}) : std::string();
}

void loadOne(const fs::path& dir) {
    std::error_code ec;
    auto metaPath = dir / "item.json";
    if (fs::file_size(metaPath, ec) > (64u << 10)) {
        logger::warn("cosmetic {}: item.json too large", dir.filename().string());
        return;
    }
    auto item = std::make_unique<Item>();
    std::string why;
    std::string text = readFile(metaPath);
    if (!parse(text, dir, *item, why)) {
        logger::warn("cosmetic {}: {}", dir.filename().string(), why);
        return;
    }
    for (auto& have : loaded)
        if (have->id == item->id) return;
    json meta = json::parse(text, nullptr, false);
    std::string texName = meta.value("texture", "tex.png");
    fs::path texPath = dir / fs::path(texName).filename();
    if (fs::file_size(texPath, ec) > (128u << 10)) {
        logger::warn("cosmetic {}: texture file too large", item->id);
        return;
    }
    img::Pixels px;
    if (!img::load(texPath, 4096, px) || px.w > maxTexture || px.h > maxTexture) {
        logger::warn("cosmetic {}: texture missing or larger than {}x{}", item->id, maxTexture, maxTexture);
        return;
    }
    makeTexture(*item, px);
    loaded.push_back(std::move(item));
}

std::vector<fs::path> roots() {
    std::vector<fs::path> out;
    if (const char* env = std::getenv("MOCHI_COSMETICS")) {
        std::string all = env;
        size_t from = 0;
        while (from <= all.size()) {
            size_t to = all.find(';', from);
            std::string part = all.substr(from, to == std::string::npos ? std::string::npos : to - from);
            if (!part.empty()) out.emplace_back(logger::widen(part));
            if (to == std::string::npos) break;
            from = to + 1;
        }
    }
    out.push_back(paths::root() / L"cosmetics");
    out.push_back(paths::dllDir() / L"cosmetics");
    return out;
}

void setStatus(std::string text) {
    std::scoped_lock g(lock);
    status = std::move(text);
}

std::vector<std::string> indexIds(const json& j) {
    std::vector<std::string> out;
    const json* list = &j;
    if (j.is_object())
        for (const char* key : {"items", "cosmetics"})
            if (j.contains(key)) list = &j[key];
    if (!list->is_array()) return out;
    for (auto& e : *list) {
        if (e.is_string()) out.push_back(e.get<std::string>());
        else if (e.is_object() && e.contains("id") && e["id"].is_string()) out.push_back(e["id"].get<std::string>());
    }
    return out;
}

void sync() {
    if (syncing.exchange(true)) return;
    std::thread([] {
        auto index = http::get(L"raw.githubusercontent.com", http::repoRawPath(L"cosmetics/index.json"), 5000);
        if (!index) {
            setStatus("");
            syncing = false;
            return;
        }
        int fetched = 0;
        json j = json::parse(*index, nullptr, false);
        fs::path base = paths::root() / L"cosmetics";
        for (auto& id : indexIds(j)) {
            if (!validId(id)) continue;
            fs::path dir = base / logger::widen(id);
            std::error_code ec;
            if (fs::exists(dir / "item.json", ec) && fs::exists(dir / "tex.png", ec)) continue;
            std::wstring wid = logger::widen(id);
            auto meta = http::get(L"raw.githubusercontent.com", http::repoRawPath(L"cosmetics/" + wid + L"/item.json"), 5000);
            auto tex = http::get(L"raw.githubusercontent.com", http::repoRawPath(L"cosmetics/" + wid + L"/tex.png"), 5000);
            if (!meta || !tex || meta->size() > (64u << 10) || tex->size() > (128u << 10)) continue;
            fs::create_directories(dir, ec);
            std::ofstream(dir / "item.json", std::ios::binary) << *meta;
            std::ofstream(dir / "tex.png", std::ios::binary) << *tex;
            fetched++;
        }
        if (fetched) {
            logger::info("cosmetics: downloaded {} new", fetched);
            synced = true;
        }
        setStatus("");
        syncing = false;
    }).detach();
}

struct V {
    float x, y, z;
};

V operator+(V a, V b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
V operator-(V a, V b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
V operator*(V a, float k) { return {a.x * k, a.y * k, a.z * k}; }
float dot(V a, V b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
V cross(V a, V b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }

V rotX(V p, float a) {
    float c = std::cos(a), s = std::sin(a);
    return {p.x, p.y * c - p.z * s, p.y * s + p.z * c};
}

V rotY(V p, float a) {
    float c = std::cos(a), s = std::sin(a);
    return {p.x * c + p.z * s, p.y, -p.x * s + p.z * c};
}

V rotZ(V p, float a) {
    float c = std::cos(a), s = std::sin(a);
    return {p.x * c - p.y * s, p.x * s + p.y * c, p.z};
}

V rotate(V p, const float deg[3]) {
    constexpr float k = pi / 180.f;
    return rotZ(rotY(rotX(p, deg[0] * k), deg[1] * k), deg[2] * k);
}

struct Chain {
    std::vector<float> th, om, ph, op;
};

struct BoneSim {
    float ang[3]{};
    float vel[3]{};
    float phase = 0.f;
    bool init = false;
    unsigned frame = 0;
    Chain chain;
};

}

struct Rig::Impl {
    std::unordered_map<const Bone*, BoneSim> bones;
    Motion m, prev;
    bool havePrev = false;
    float dt = 0.016f;
    float accF = 0.f, accS = 0.f, accU = 0.f;
    float air = 0.f, sprint = 0.f, sneak = 0.f;
    double clock = 0.0;
    unsigned frame = 1;
};

Rig::Rig() : d(std::make_unique<Impl>()) {}
Rig::~Rig() = default;

void Rig::step(float dt, const Motion& m) {
    dt = std::clamp(dt, 0.001f, 0.05f);
    auto& r = *d;
    auto follow = [&](float& value, float target, float rate) { value += (target - value) * std::min(1.f, dt * rate); };
    if (r.havePrev) {
        follow(r.accF, (m.fwd - r.prev.fwd) / dt, 12.f);
        follow(r.accS, (m.side - r.prev.side) / dt, 12.f);
        follow(r.accU, (m.up - r.prev.up) / dt, 12.f);
    }
    follow(r.air, m.air ? 1.f : 0.f, 9.f);
    follow(r.sprint, m.sprint ? 1.f : 0.f, 6.f);
    follow(r.sneak, m.sneak ? 1.f : 0.f, 8.f);
    r.prev = m;
    r.m = m;
    r.havePrev = true;
    r.dt = dt;
    r.clock += dt;
    r.frame++;
}

namespace {

struct Pose {
    float deg[3]{};
    V move{0, 0, 0};
    std::vector<float> pitch;
    std::vector<float> roll;
};

constexpr float rad = pi / 180.f;

void animTarget(const Bone& b, const BoneSim& s, const Rig::Impl& r, float out[3], V& move) {
    out[0] = b.rot[0];
    out[1] = b.rot[1];
    out[2] = b.rot[2];
    float tau = s.phase;
    int axis = b.axis == 'x' ? 0 : b.axis == 'z' ? 2 : 1;
    float speedN = std::clamp(r.m.fwd / 5.6f, 0.f, 1.2f);
    switch (b.anim) {
    case Anim::Flap: out[axis] += b.amp * (1.f + 1.1f * r.air + 0.15f * speedN) * std::sin(tau + b.phase); break;
    case Anim::Sway:
    case Anim::Wag: out[axis] += b.amp * (1.f + 0.6f * speedN) * std::sin(tau * 0.5f + b.phase); break;
    case Anim::Twitch: out[axis] += b.amp * std::pow(std::max(0.f, std::sin(tau * 0.25f + b.phase)), 12.f); break;
    case Anim::Spin: out[axis] += tau * 57.29578f + b.phase; break;
    case Anim::Bob: move.y = b.amp * std::sin(tau + b.phase); break;
    case Anim::Float:
        move.y = b.amp * std::sin(tau * 0.5f + b.phase);
        move.x = b.amp * 0.3f * std::sin(tau * 0.25f + b.phase);
        break;
    default: break;
    }
    for (int i = 0; i < 3; i++)
        out[i] += r.air * b.driveAir[i] + r.sprint * b.driveSprint[i] + r.sneak * b.driveSneak[i] + speedN * b.driveSpeed[i];
}

void springStep(const Bone& b, BoneSim& s, const Rig::Impl& r, float h) {
    float target[3];
    V move{};
    animTarget(b, s, r, target, move);
    float kick[3] = {-8.f * r.accF + 2.f * r.accU, -1.5f * r.m.turn, 8.f * r.accS};
    for (int i = 0; i < 3; i++) {
        float acc = b.stiffness * (target[i] - s.ang[i]) - b.damping * s.vel[i] + b.inertia * kick[i];
        s.vel[i] += acc * h;
        s.ang[i] += s.vel[i] * h;
    }
}

void clothStep(const Bone& b, BoneSim& s, const Rig::Impl& r, const std::vector<const Cube*>& order, float h) {
    size_t n = order.size();
    Chain& c = s.chain;
    float windDeg = std::clamp(r.m.fwd * 5.2f, 0.f, 60.f) * b.wind + std::clamp(-r.m.up * 3.f, 0.f, 35.f) + r.sneak * 7.f;
    float gravity = b.stiffness, drag = b.damping, link = 22.f;
    for (size_t i = 0; i < n; i++) {
        float frac = n > 1 ? float(i) / float(n - 1) : 0.f;
        float target = windDeg * rad * (0.3f + 0.7f * frac);
        float gust = 0.35f * std::sin(float(r.clock) * 2.3f + float(i) * 0.8f) * (0.25f + std::min(1.f, r.m.fwd / 4.f));
        float up = i ? c.th[i - 1] : 0.f, down = i + 1 < n ? c.th[i + 1] : c.th[i];
        float acc = -gravity * std::sin(c.th[i] - target) - drag * c.om[i] + link * (up - c.th[i]) + link * 0.6f * (down - c.th[i]);
        acc += b.inertia * (r.accF * 0.09f + r.accU * 0.03f) * (0.4f + frac) + gust;
        c.om[i] += acc * h;
        float upR = i ? c.ph[i - 1] : 0.f, downR = i + 1 < n ? c.ph[i + 1] : c.ph[i];
        float accR = -gravity * 0.8f * std::sin(c.ph[i]) - drag * c.op[i] + link * (upR - c.ph[i]) + link * 0.6f * (downR - c.ph[i]);
        accR += b.inertia * (r.accS * 0.09f - r.m.turn * 0.004f) * (0.4f + frac) + gust * 0.4f;
        c.op[i] += accR * h;
    }
    float zmax = 0.35f, z = 0.f;
    for (size_t i = 0; i < n; i++) {
        c.th[i] += c.om[i] * h;
        c.ph[i] += c.op[i] * h;
        c.th[i] = std::clamp(c.th[i], -1.2f, 1.9f);
        c.ph[i] = std::clamp(c.ph[i], -0.9f, 0.9f);
        float len = order[i]->size[1];
        float nz = z - len * std::sin(c.th[i]);
        if (nz > zmax) {
            c.th[i] = -std::asin(std::clamp((zmax - z) / len, -1.f, 1.f));
            if (c.om[i] > 0.f) c.om[i] *= -0.2f;
            nz = zmax;
        }
        z = nz;
    }
}

void integrate(const Bone& b, BoneSim& s, Rig::Impl& r, const std::vector<const Cube*>& order) {
    if (s.frame == r.frame) return;
    float dt = s.frame ? std::min(r.dt * float(r.frame - s.frame), 0.05f) : 0.f;
    s.frame = r.frame;
    if (!s.init) {
        s.init = true;
        float target[3];
        V move{};
        animTarget(b, s, r, target, move);
        for (int i = 0; i < 3; i++) s.ang[i] = target[i];
        s.chain.th.assign(order.size(), 0.f);
        s.chain.om.assign(order.size(), 0.f);
        s.chain.ph.assign(order.size(), 0.f);
        s.chain.op.assign(order.size(), 0.f);
        return;
    }
    int steps = std::max(1, int(std::ceil(dt * 120.f)));
    float h = dt / float(steps);
    float rate = b.speed * (b.anim == Anim::Flap ? 1.f + 2.f * r.air + 0.4f * r.sprint : 1.f);
    for (int k = 0; k < steps; k++) {
        s.phase += h * 2.f * pi * rate;
        if (b.cloth) clothStep(b, s, r, order, h);
        else if (b.spring) springStep(b, s, r, h);
    }
}

struct Face {
    ImVec2 p[4];
    ImVec2 uv[4];
    float depth;
    ImU32 color;
    ImTextureData* tex;
    bool nearest;
};

struct Camera {
    float yaw, pitch, unit, dist;
    ImVec2 center;
    V light;
};

V view(const Camera& c, V p) {
    p = rotY(p, c.yaw * pi / 180.f);
    return rotX(p, c.pitch * pi / 180.f);
}

ImVec2 screen(const Camera& c, V v) {
    float s = c.dist / (c.dist - v.z);
    return {c.center.x + v.x * s * c.unit, c.center.y - v.y * s * c.unit};
}

ImU32 shade(uint32_t rgb, float light, float alpha) {
    auto ch = [&](int shift) { return int(std::clamp(float((rgb >> shift) & 255) * light, 0.f, 255.f)); };
    return IM_COL32(ch(16), ch(8), ch(0), int(std::clamp(alpha, 0.f, 1.f) * 255.f));
}

void addCube(std::vector<Face>& faces, const Camera& cam, const Cube& c, const std::function<V(V)>& place, float light0, uint32_t rgb, float alpha, const Item* item) {
    float x0 = c.origin[0], y0 = c.origin[1], z0 = c.origin[2];
    float x1 = x0 + c.size[0], y1 = y0 + c.size[1], z1 = z0 + c.size[2];
    float k = item ? item->texel : 1.f;
    float w = c.size[0] * k, h = c.size[1] * k, d = c.size[2] * k;
    float u = float(c.uv[0]), v = float(c.uv[1]);

    struct Def {
        V corner[4];
        float ux, uy, uw, uh;
    };
    Def defs[6] = {
        {{{x0, y1, z1}, {x1, y1, z1}, {x1, y0, z1}, {x0, y0, z1}}, u + d, v + d, w, h},
        {{{x1, y1, z0}, {x0, y1, z0}, {x0, y0, z0}, {x1, y0, z0}}, u + 2 * d + w, v + d, w, h},
        {{{x0, y1, z0}, {x0, y1, z1}, {x0, y0, z1}, {x0, y0, z0}}, u, v + d, d, h},
        {{{x1, y1, z1}, {x1, y1, z0}, {x1, y0, z0}, {x1, y0, z1}}, u + d + w, v + d, d, h},
        {{{x0, y1, z0}, {x1, y1, z0}, {x1, y1, z1}, {x0, y1, z1}}, u + d, v, w, d},
        {{{x0, y0, z1}, {x1, y0, z1}, {x1, y0, z0}, {x0, y0, z0}}, u + d + w, v, w, d},
    };

    if (c.flat) {
        defs[0].ux = defs[1].ux = u;
        defs[0].uy = defs[1].uy = v;
        defs[0].uw = defs[1].uw = w;
        defs[0].uh = defs[1].uh = h;
        for (int i = 2; i < 6; i++) {
            defs[i].ux = u + 0.5f;
            defs[i].uy = v + h + 0.5f;
            defs[i].uw = defs[i].uh = 0.f;
        }
    }

    if (item && item->texel > 1.f)
        for (auto& d : defs)
            if (d.uw > 1.f && d.uh > 1.f) {
                d.ux += 0.5f;
                d.uy += 0.5f;
                d.uw -= 1.f;
                d.uh -= 1.f;
            }

    for (int di = 0; di < 6; di++) {
        const Def& def = defs[di];
        V wp[4];
        for (int i = 0; i < 4; i++) wp[i] = place(def.corner[i]);
        V n = cross(wp[3] - wp[0], wp[1] - wp[0]);
        float len = std::sqrt(dot(n, n));
        if (len < 1e-5f) continue;
        n = n * (1.f / len);
        V vp[4];
        Face f{};
        for (int i = 0; i < 4; i++) {
            vp[i] = view(cam, wp[i]);
            f.p[i] = screen(cam, vp[i]);
        }
        float area = 0.f;
        for (int i = 0; i < 4; i++) {
            ImVec2 a = f.p[i], b = f.p[(i + 1) % 4];
            area += a.x * b.y - b.x * a.y;
        }
        if (area <= 0.f) continue;
        float lit = light0 + (1.f - light0) * std::max(0.f, dot(n, cam.light));
        f.depth = (vp[0].z + vp[1].z + vp[2].z + vp[3].z) * 0.25f;
        f.color = shade(rgb, lit, alpha);
        f.tex = item ? item->tex : nullptr;
        f.nearest = !item || item->texel <= 1.f;
        if (item && item->tex) {
            float iw = 1.f / float(item->texW), ih = 1.f / float(item->texH);
            f.uv[0] = {def.ux * iw, def.uy * ih};
            f.uv[1] = {(def.ux + def.uw) * iw, def.uy * ih};
            f.uv[2] = {(def.ux + def.uw) * iw, (def.uy + def.uh) * ih};
            f.uv[3] = {def.ux * iw, (def.uy + def.uh) * ih};
            if (c.flat && di < 2 && ((di == 1) != c.mirror)) {
                std::swap(f.uv[0].x, f.uv[1].x);
                std::swap(f.uv[2].x, f.uv[3].x);
            }
        }
        faces.push_back(f);
    }
}

void addBone(std::vector<Face>& faces, const Camera& cam, const Item& item, const Bone& b, const Piece& piece, const View& vw, Rig::Impl& rig) {
    std::vector<const Cube*> order;
    for (auto& c : b.cubes) order.push_back(&c);
    if (b.cloth)
        std::sort(order.begin(), order.end(), [](const Cube* a, const Cube* c) { return a->origin[1] + a->size[1] > c->origin[1] + c->size[1]; });

    BoneSim& sim = rig.bones[&b];
    if (vw.animate) integrate(b, sim, rig, order);
    else if (!sim.init) integrate(b, sim, rig, order);

    float deg[3] = {b.rot[0], b.rot[1], b.rot[2]};
    V move{0, 0, 0};
    if (vw.animate) {
        if (b.spring || b.cloth) {
            if (b.spring) {
                deg[0] = sim.ang[0];
                deg[1] = sim.ang[1];
                deg[2] = sim.ang[2];
            }
            float scratch[3];
            animTarget(b, sim, rig, scratch, move);
        } else {
            float target[3];
            animTarget(b, sim, rig, target, move);
            deg[0] = target[0];
            deg[1] = target[1];
            deg[2] = target[2];
        }
    }
    V pivot{b.pivot[0], b.pivot[1], b.pivot[2]};
    auto base = [&](V p) { return pivot + rotate(p - pivot, deg) + move; };

    for (size_t i = 0; i < order.size(); i++) {
        const Cube& c = *order[i];
        uint32_t rgb = c.tint >= 0 ? tintOf(piece, c.tint) : 0xffffff;
        if (c.tint2 >= 0) {
            uint32_t other = tintOf(piece, c.tint2);
            auto ch = [&](int shift) { return uint32_t(float((rgb >> shift) & 255) * (1.f - c.mix) + float((other >> shift) & 255) * c.mix + 0.5f); };
            rgb = (ch(16) << 16) | (ch(8) << 8) | ch(0);
        }
        float alpha = 1.f;
        if (vw.animate && b.anim == Anim::Sparkle) alpha = 0.55f + 0.45f * std::sin(sim.phase * 2.f + b.phase + float(i) * 1.7f);

        auto place = [&](V p) {
            if (b.cloth && vw.animate && sim.chain.th.size() == order.size()) {
                for (int j = int(i); j >= 0; j--) {
                    const Cube& jc = *order[size_t(j)];
                    V joint{jc.origin[0] + jc.size[0] * 0.5f, jc.origin[1] + jc.size[1], jc.origin[2] + jc.size[2] * 0.5f};
                    float prevTh = j ? sim.chain.th[size_t(j - 1)] : 0.f, prevPh = j ? sim.chain.ph[size_t(j - 1)] : 0.f;
                    float rel[3] = {(sim.chain.th[size_t(j)] - prevTh) / rad, 0.f, (sim.chain.ph[size_t(j)] - prevPh) / rad};
                    p = joint + rotate(p - joint, rel);
                }
            }
            return base(p);
        };
        addCube(faces, cam, c, place, 0.84f, rgb, alpha, &item);
    }
}

}

uint32_t tintOf(const Piece& piece, int index) {
    if (index < 0 || !piece.item || size_t(index) >= piece.item->tints.size()) return 0xffffff;
    if (size_t(index) < piece.tint.size() && piece.tint[size_t(index)]) return piece.tint[size_t(index)];
    return piece.item->tints[size_t(index)].def;
}

bool parse(const std::string& text, const fs::path& dir, Item& out, std::string& why) {
    json j = json::parse(text, nullptr, false);
    if (j.is_discarded() || !j.is_object()) {
        why = "not valid json";
        return false;
    }
    try {
        out.id = j.value("id", "");
        out.name = j.value("name", out.id);
        out.slot = j.value("slot", "");
        if (!validId(out.id)) {
            why = "bad id";
            return false;
        }
        if (std::find(slots.begin(), slots.end(), out.slot) == slots.end()) {
            why = "unknown slot";
            return false;
        }
        if (j.contains("tags") && j["tags"].is_array())
            for (auto& t : j["tags"])
                if (t.is_string() && out.tags.size() < 12) out.tags.push_back(t.get<std::string>());
        if (j.contains("tint") && j["tint"].is_array())
            for (auto& t : j["tint"]) {
                if (out.tints.size() >= size_t(maxTints) || !t.is_object()) break;
                out.tints.push_back({t.value("name", "Color"), hexColor(t.value("default", "#ffffff"), 0xffffff)});
            }
        out.texel = std::clamp(j.value("texel", 1.f), 1.f, 8.f);
        out.dir = dir;
        return bones(j, out, why);
    } catch (const std::exception& e) {
        why = e.what();
        return false;
    }
}

void rescan() {
    for (auto& root : roots()) {
        std::error_code ec;
        if (!fs::is_directory(root, ec)) continue;
        for (auto& entry : fs::directory_iterator(root, ec)) {
            if (!entry.is_directory(ec) || !fs::exists(entry.path() / "item.json", ec)) continue;
            std::string key = fs::weakly_canonical(entry.path(), ec).string();
            if (seen.insert(key).second) pending.push_back(entry.path());
        }
    }
}

void tick(bool download) {
    if (!started) {
        started = true;
        rescan();
        if (download) sync();
    }
    if (synced.exchange(false)) rescan();
    if (pending.empty()) return;
    fs::path next = pending.front();
    pending.pop_front();
    loadOne(next);
}

const std::vector<std::unique_ptr<Item>>& items() { return loaded; }

const Item* find(const std::string& id) {
    for (auto& i : loaded)
        if (i->id == id) return i.get();
    return nullptr;
}

int loading() { return int(pending.size()); }

std::string folder() { return logger::narrow((paths::root() / L"cosmetics").wstring()); }

std::string syncStatus() {
    std::scoped_lock g(lock);
    return status;
}

void draw(ImDrawList* dl, ImVec2 min, ImVec2 max, const std::vector<Piece>& pieces, const View& vw) {
    float w = max.x - min.x, h = max.y - min.y;
    if (w < 8.f || h < 8.f) return;

    Camera cam;
    cam.yaw = vw.yaw;
    cam.pitch = vw.pitch;
    cam.dist = 160.f;
    cam.unit = h * 0.78f / 32.f * vw.zoom;
    cam.center = {min.x + w * 0.5f, min.y + h * 0.5f + 16.f * cam.unit * 0.9f};
    float ll = std::sqrt(0.3f * 0.3f + 0.8f * 0.8f + 0.5f * 0.5f);
    cam.light = {0.3f / ll, 0.8f / ll, 0.5f / ll};

    static Rig fallback;
    Rig::Impl* rig = (vw.rig ? vw.rig : &fallback)->d.get();
    std::vector<Face> faces;
    if (vw.mannequin) {
        struct Part {
            float x, y, z, w, h, d;
            float shade;
        };
        const Part parts[] = {{-4, 24, -4, 8, 8, 8, 1.0f}, {-4, 12, -2, 8, 12, 4, 0.82f}, {-8, 12, -2, 4, 12, 4, 0.9f}, {4, 12, -2, 4, 12, 4, 0.9f}, {-4, 0, -2, 4, 12, 4, 0.7f}, {0, 0, -2, 4, 12, 4, 0.7f}};
        for (auto& p : parts) {
            Cube c;
            c.origin[0] = p.x;
            c.origin[1] = p.y;
            c.origin[2] = p.z;
            c.size[0] = p.w;
            c.size[1] = p.h;
            c.size[2] = p.d;
            uint32_t rgb = (uint32_t(float((vw.skin >> 16) & 255) * p.shade) << 16) | (uint32_t(float((vw.skin >> 8) & 255) * p.shade) << 8) | uint32_t(float(vw.skin & 255) * p.shade);
            addCube(faces, cam, c, [](V q) { return q; }, 0.72f, rgb, 1.f, nullptr);
        }
    }
    for (auto& piece : pieces) {
        if (!piece.item || !piece.item->tex) continue;
        for (auto& b : piece.item->bones) addBone(faces, cam, *piece.item, b, piece, vw, *rig);
    }
    std::stable_sort(faces.begin(), faces.end(), [](const Face& a, const Face& b) { return a.depth < b.depth; });

    dl->PushClipRect(min, max, true);
    auto& pio = ImGui::GetPlatformIO();
    bool canSwitch = pio.DrawCallback_SetSamplerNearest && pio.DrawCallback_SetSamplerLinear;
    bool nearestOn = false;
    ImDrawListFlags flags = dl->Flags;
    dl->Flags &= ~ImDrawListFlags_AntiAliasedFill;
    for (auto& f : faces) {
        if (f.tex) {
            if (canSwitch && f.nearest != nearestOn) {
                dl->AddCallback(f.nearest ? pio.DrawCallback_SetSamplerNearest : pio.DrawCallback_SetSamplerLinear, nullptr);
                nearestOn = f.nearest;
            }
            dl->AddImageQuad(f.tex->GetTexRef(), f.p[0], f.p[1], f.p[2], f.p[3], f.uv[0], f.uv[1], f.uv[2], f.uv[3], f.color);
        } else {
            if (canSwitch && nearestOn) {
                dl->AddCallback(pio.DrawCallback_SetSamplerLinear, nullptr);
                nearestOn = false;
            }
            dl->AddQuadFilled(f.p[0], f.p[1], f.p[2], f.p[3], f.color);
            dl->AddQuad(f.p[0], f.p[1], f.p[2], f.p[3], IM_COL32(0, 0, 0, 46), 1.f);
        }
    }
    if (canSwitch && nearestOn) dl->AddCallback(pio.DrawCallback_SetSamplerLinear, nullptr);
    dl->Flags = flags;
    dl->PopClipRect();
}

}
