#include "Cosmetics.hpp"

#include <imgui_internal.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <unordered_map>

namespace cosmetics {

namespace {

constexpr float pi = 3.14159265f;
constexpr float rad = pi / 180.f;

V3 operator+(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
V3 operator-(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
V3 operator*(V3 a, float k) { return {a.x * k, a.y * k, a.z * k}; }
float dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
V3 cross(V3 a, V3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }

V3 norm(V3 a) {
    float l = std::sqrt(dot(a, a));
    return l > 1e-6f ? a * (1.f / l) : V3{0, 1, 0};
}

V3 rotateAxis(V3 p, int axis, float deg) {
    float a = deg * rad, c = std::cos(a), s = std::sin(a);
    if (axis == 0) return {p.x, p.y * c - p.z * s, p.y * s + p.z * c};
    if (axis == 1) return {p.x * c + p.z * s, p.y, -p.x * s + p.z * c};
    return {p.x * c - p.y * s, p.x * s + p.y * c, p.z};
}

V3 rotateXyz(V3 p, const float deg[3]) { return rotateAxis(rotateAxis(rotateAxis(p, 0, deg[0]), 1, deg[1]), 2, deg[2]); }

V3 withAxis(V3 v, int axis, float add) {
    if (axis == 0) v.x += add;
    else if (axis == 1) v.y += add;
    else v.z += add;
    return v;
}

ImTextureData* whiteTex() {
    static ImTextureData* white = [] {
        auto* td = IM_NEW(ImTextureData)();
        td->Create(ImTextureFormat_RGBA32, 1, 1);
        std::memset(td->GetPixels(), 255, 4);
        ImGui::RegisterUserTexture(td);
        return td;
    }();
    return white;
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
    Moving m, prev;
    bool havePrev = false;
    float dt = 0.016f;
    float accF = 0.f, accS = 0.f, accU = 0.f;
    float air = 0.f, sprint = 0.f, sneak = 0.f;
    double clock = 0.0;
    unsigned frame = 1;
    unsigned gen = 0;
};

Rig::Rig() : d(std::make_unique<Impl>()) {}
Rig::~Rig() = default;

void Rig::clear() { d = std::make_unique<Impl>(); }

void Rig::step(float dt, const Moving& m) {
    dt = std::clamp(dt, 0.001f, 0.05f);
    auto& r = *d;
    if (r.gen != generation()) {
        clear();
        d->gen = generation();
        return;
    }
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

struct Face {
    V3 corner[4];
    V3 normal;
    int region;
};

void addFaces(std::vector<Face>& out, const Cube& c) {
    float x0 = c.origin.x, y0 = c.origin.y, z0 = c.origin.z;
    float x1 = x0 + c.size.x, y1 = y0 + c.size.y, z1 = z0 + c.size.z;
    out.push_back({{{x0, y1, z1}, {x1, y1, z1}, {x1, y0, z1}, {x0, y0, z1}}, {0, 0, 1}, 0});
    out.push_back({{{x1, y1, z0}, {x0, y1, z0}, {x0, y0, z0}, {x1, y0, z0}}, {0, 0, -1}, 1});
    out.push_back({{{x1, y1, z1}, {x1, y1, z0}, {x1, y0, z0}, {x1, y0, z1}}, {1, 0, 0}, 2});
    out.push_back({{{x0, y1, z0}, {x0, y1, z1}, {x0, y0, z1}, {x0, y0, z0}}, {-1, 0, 0}, 3});
    out.push_back({{{x0, y1, z0}, {x1, y1, z0}, {x1, y1, z1}, {x0, y1, z1}}, {0, 1, 0}, 4});
    out.push_back({{{x0, y0, z1}, {x1, y0, z1}, {x1, y0, z0}, {x0, y0, z0}}, {0, -1, 0}, 5});
}

// minecraft box layout: top and bottom on the first row, then right, front, left, back.
// flat cubes use one rectangle for front and back, the edges sample the texel below it
void regionUv(const Cube& c, int region, float& u0, float& v0, float& u1, float& v1) {
    float w = c.uvSize.x, h = c.uvSize.y, d = c.uvSize.z;
    float u = float(c.u), v = float(c.v);
    if (c.flat) {
        if (region > 1) {
            u0 = u1 = u + 0.5f;
            v0 = v1 = v + h + 0.5f;
            return;
        }
        u0 = u; v0 = v; u1 = u + w; v1 = v + h;
        if ((region == 1) != c.mirror) std::swap(u0, u1);
        return;
    }
    switch (region) {
    case 4: u0 = u + d; v0 = v; u1 = u0 + w; v1 = v + d; break;
    case 5: u0 = u + d + w; v0 = v; u1 = u0 + w; v1 = v + d; break;
    case 3: u0 = u; v0 = v + d; u1 = u0 + d; v1 = v0 + h; break;
    case 0: u0 = u + d; v0 = v + d; u1 = u0 + w; v1 = v0 + h; break;
    case 2: u0 = u + d + w; v0 = v + d; u1 = u0 + d; v1 = v0 + h; break;
    default: u0 = u + 2 * d + w; v0 = v + d; u1 = u0 + w; v1 = v0 + h; break;
    }
}

float axisOf(V3 v, int axis) { return axis == 0 ? v.x : axis == 1 ? v.y : v.z; }

struct Ring {
    std::vector<V3> pts;
    V3 center;
    V3 tangent;
    float mix;
};

std::vector<Ring> sweep(const Tube& t, double time) {
    size_t n = t.path.size();
    std::vector<V3> c(n);
    for (size_t i = 0; i < n; i++) {
        c[i] = t.path[i].pos;
        if (t.wave.amplitude != 0.f) {
            float prog = float(i) / float(n - 1);
            c[i] = withAxis(c[i], t.wave.axis, t.wave.amplitude * prog * prog * std::sin(float(time) * t.wave.speed * 2.f * pi - prog * t.wave.freq * pi));
        }
    }
    std::vector<Ring> rings(n);
    V3 u{};
    float inv = 2.f / t.power;
    for (size_t i = 0; i < n; i++) {
        V3 tan = norm(c[std::min(i + 1, n - 1)] - c[i ? i - 1 : 0]);
        if (i == 0) {
            V3 ref = std::fabs(tan.x) > 0.9f ? V3{0, 0, 1} : V3{1, 0, 0};
            u = norm(ref - tan * dot(ref, tan));
        } else {
            u = norm(u - tan * dot(u, tan));
        }
        V3 w = cross(tan, u);
        Ring r;
        r.center = c[i];
        r.tangent = tan;
        r.mix = t.path[i].mix;
        for (int k = 0; k < t.sides; k++) {
            float a = 2.f * pi * float(k) / float(t.sides);
            float cs = std::cos(a), sn = std::sin(a);
            float x = std::copysign(std::pow(std::fabs(cs), inv), cs);
            float z = std::copysign(std::pow(std::fabs(sn), inv), sn);
            r.pts.push_back(c[i] + u * (t.path[i].rx * x) + w * (t.path[i].rz * z));
        }
        rings[i] = std::move(r);
    }
    return rings;
}

void animTarget(const Bone& b, const BoneSim& s, const Rig::Impl& r, float out[3], V3& move) {
    out[0] = b.rotation.x;
    out[1] = b.rotation.y;
    out[2] = b.rotation.z;
    const Anim& a = b.anim;
    float tau = s.phase;
    float speedN = std::clamp(r.m.fwd / 5.6f, 0.f, 1.2f);
    switch (a.kind) {
    case Motion::Flap: out[a.axis] += a.amplitude * (1.f + 1.1f * r.air + 0.15f * speedN) * std::sin(tau + a.phase); break;
    case Motion::Sway:
    case Motion::Wag: out[a.axis] += a.amplitude * (1.f + 0.6f * speedN) * std::sin(tau + a.phase); break;
    case Motion::Twitch: out[a.axis] += a.amplitude * std::pow(std::max(0.f, std::sin(tau * 0.5f + a.phase)), 8.f) * std::max(0.f, std::sin(tau * 3.f)); break;
    case Motion::Spin: out[a.axis] += tau * 57.29578f + a.phase; break;
    case Motion::Bob: move.y = a.amplitude * std::sin(tau + a.phase); break;
    case Motion::Float:
        move.y = a.amplitude * std::sin(tau + a.phase);
        move.x = a.amplitude * 0.3f * std::sin(tau * 0.5f + a.phase);
        break;
    default: break;
    }
    const Physics& p = b.physics;
    for (int i = 0; i < 3; i++)
        out[i] += r.air * axisOf(p.air, i) + r.sprint * axisOf(p.sprint, i) + r.sneak * axisOf(p.sneak, i) + speedN * axisOf(p.speed, i);
}

void springStep(const Bone& b, BoneSim& s, const Rig::Impl& r, float h) {
    float target[3];
    V3 move{};
    animTarget(b, s, r, target, move);
    float kick[3] = {-8.f * r.accF + 2.f * r.accU, -1.5f * r.m.turn, 8.f * r.accS};
    for (int i = 0; i < 3; i++) {
        float acc = b.physics.stiffness * (target[i] - s.ang[i]) - b.physics.damping * s.vel[i] + b.physics.inertia * kick[i];
        s.vel[i] += acc * h;
        s.ang[i] += s.vel[i] * h;
    }
}

// pendulum chain over the strips from top to bottom, gravity and wind, the body acts as a wall behind the cape
void clothStep(const Bone& b, BoneSim& s, const Rig::Impl& r, const std::vector<const Cube*>& order, float h) {
    size_t n = order.size();
    Chain& c = s.chain;
    const Physics& p = b.physics;
    float windDeg = std::clamp(r.m.fwd * 5.2f, 0.f, 60.f) * p.wind + std::clamp(-r.m.up * 3.f, 0.f, 35.f) + r.sneak * 7.f;
    float gravity = p.stiffness, drag = p.damping, link = 22.f;
    for (size_t i = 0; i < n; i++) {
        float frac = n > 1 ? float(i) / float(n - 1) : 0.f;
        float target = windDeg * rad * (0.3f + 0.7f * frac);
        float gust = 0.35f * std::sin(float(r.clock) * 2.3f + float(i) * 0.8f) * (0.25f + std::min(1.f, r.m.fwd / 4.f));
        float up = i ? c.th[i - 1] : 0.f, down = i + 1 < n ? c.th[i + 1] : c.th[i];
        float acc = -gravity * std::sin(c.th[i] - target) - drag * c.om[i] + link * (up - c.th[i]) + link * 0.6f * (down - c.th[i]);
        acc += p.inertia * (r.accF * 0.09f + r.accU * 0.03f) * (0.4f + frac) + gust;
        c.om[i] += acc * h;
        float upR = i ? c.ph[i - 1] : 0.f, downR = i + 1 < n ? c.ph[i + 1] : c.ph[i];
        float accR = -gravity * 0.8f * std::sin(c.ph[i]) - drag * c.op[i] + link * (upR - c.ph[i]) + link * 0.6f * (downR - c.ph[i]);
        accR += p.inertia * (r.accS * 0.09f - r.m.turn * 0.004f) * (0.4f + frac) + gust * 0.4f;
        c.op[i] += accR * h;
    }
    float zmax = 0.35f, z = 0.f;
    for (size_t i = 0; i < n; i++) {
        c.th[i] += c.om[i] * h;
        c.ph[i] += c.op[i] * h;
        c.th[i] = std::clamp(c.th[i], -1.2f, 1.9f);
        c.ph[i] = std::clamp(c.ph[i], -0.9f, 0.9f);
        float len = order[i]->size.y;
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
        V3 move{};
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
    float rate = b.anim.speed * (b.anim.kind == Motion::Flap ? 1.f + 2.f * r.air + 0.4f * r.sprint : 1.f);
    for (int k = 0; k < steps; k++) {
        s.phase += h * 2.f * pi * rate;
        if (b.physics.cloth) clothStep(b, s, r, order, h);
        else if (b.physics.spring) springStep(b, s, r, h);
    }
}

struct Draw {
    ImVec2 p[4];
    ImVec2 uv[4];
    float depth;
    ImU32 col;
    ImTextureRef tex;
    bool nearest;
};

}

void drawPreview(ImDrawList* dl, ImVec2 center, float unit, float yaw, float pitch, const std::vector<Worn>& worn, ImVec4 body, const Look& look) {
    static Rig fallback;
    Rig::Impl& rig = *(look.rig ? look.rig : &fallback)->d;
    ImTextureData* white = whiteTex();
    std::vector<Draw> draws;

    auto project = [&](V3 p) {
        V3 r = rotateAxis(rotateAxis(p, 1, yaw), 0, pitch);
        return V3{center.x + r.x * unit, center.y - (r.y - look.focus) * unit, r.z};
    };
    auto push = [&](const V3 (&pts)[4], V3 n, bool twoSided, ImVec4 tint, float alpha, ImTextureData* tex, bool nearest, const ImVec2 (&uv)[4]) {
        V3 vn = rotateAxis(rotateAxis(n, 1, yaw), 0, pitch);
        if (vn.z < -0.02f && !twoSided) return;
        Draw d{};
        float depth = 0.f;
        for (int i = 0; i < 4; i++) {
            V3 sp = project(pts[i]);
            d.p[i] = {sp.x, sp.y};
            depth += sp.z * 0.25f;
        }
        float lit = twoSided ? std::fabs(vn.z) : vn.z;
        float shade = 0.62f + 0.38f * std::clamp(vn.y * 0.5f + lit * 0.8f + 0.2f, 0.f, 1.f);
        d.col = ImGui::GetColorU32({tint.x * shade, tint.y * shade, tint.z * shade, alpha});
        d.depth = depth;
        for (int i = 0; i < 4; i++) d.uv[i] = uv[i];
        d.tex = (tex ? tex : white)->GetTexRef();
        d.nearest = nearest;
        draws.push_back(d);
    };

    struct Place {
        const Bone* bone = nullptr;
        float deg[3]{};
        V3 move{};
        const BoneSim* sim = nullptr;
        const std::vector<const Cube*>* order = nullptr;
        int cube = -1;

        V3 point(V3 p) const {
            if (!bone) return p;
            if (sim && cube >= 0 && bone->physics.cloth && sim->chain.th.size() == order->size()) {
                for (int j = cube; j >= 0; j--) {
                    const Cube& jc = *(*order)[size_t(j)];
                    V3 joint{jc.origin.x + jc.size.x * 0.5f, jc.origin.y + jc.size.y, jc.origin.z + jc.size.z * 0.5f};
                    float prevTh = j ? sim->chain.th[size_t(j - 1)] : 0.f, prevPh = j ? sim->chain.ph[size_t(j - 1)] : 0.f;
                    float rel[3] = {(sim->chain.th[size_t(j)] - prevTh) / rad, 0.f, (sim->chain.ph[size_t(j)] - prevPh) / rad};
                    p = joint + rotateXyz(p - joint, rel);
                }
            }
            V3 pivot = bone->pivot;
            return pivot + rotateXyz(p - pivot, deg) + move;
        }

        V3 normal(V3 n) const { return bone ? rotateXyz(n, deg) : n; }
    };

    auto emitCube = [&](const Cube& cube, const Place& place, const Item* item, ImVec4 tint, float alpha) {
        std::vector<Face> faces;
        addFaces(faces, cube);
        bool textured = item && item->texture;
        for (auto& f : faces) {
            ImVec2 uv[4] = {{0.5f, 0.5f}, {0.5f, 0.5f}, {0.5f, 0.5f}, {0.5f, 0.5f}};
            if (textured) {
                float u0, v0, u1, v1;
                regionUv(cube, f.region, u0, v0, u1, v1);
                if (item->texel > 1.f && std::fabs(u1 - u0) > 1.f && std::fabs(v1 - v0) > 1.f) {
                    float dir = u1 > u0 ? 0.5f : -0.5f;
                    u0 += dir;
                    u1 -= dir;
                    v0 += 0.5f;
                    v1 -= 0.5f;
                }
                float tw = float(item->texW), th = float(item->texH);
                uv[0] = {u0 / tw, v0 / th}; uv[1] = {u1 / tw, v0 / th}; uv[2] = {u1 / tw, v1 / th}; uv[3] = {u0 / tw, v1 / th};
            }
            // the painter's sort works per quad, so big plates are cut into small tiles or they sort wrongly against the body
            auto steps = [](V3 a, V3 b) { return std::clamp(int(std::ceil(std::sqrt(dot(b - a, b - a)) / 2.f)), 1, 10); };
            int nu = steps(f.corner[0], f.corner[1]), nv = steps(f.corner[0], f.corner[3]);
            V3 eu = f.corner[1] - f.corner[0], ev = f.corner[3] - f.corner[0];
            ImVec2 uu = {uv[1].x - uv[0].x, uv[1].y - uv[0].y}, uvv = {uv[3].x - uv[0].x, uv[3].y - uv[0].y};
            auto at = [&](float a, float b, V3& p, ImVec2& t) {
                p = place.point(f.corner[0] + eu * a + ev * b);
                t = {uv[0].x + uu.x * a + uvv.x * b, uv[0].y + uu.y * a + uvv.y * b};
            };
            for (int j = 0; j < nv; j++)
                for (int i = 0; i < nu; i++) {
                    float a0 = float(i) / float(nu), a1 = float(i + 1) / float(nu), b0 = float(j) / float(nv), b1 = float(j + 1) / float(nv);
                    V3 pts[4];
                    ImVec2 tex[4];
                    at(a0, b0, pts[0], tex[0]);
                    at(a1, b0, pts[1], tex[1]);
                    at(a1, b1, pts[2], tex[2]);
                    at(a0, b1, pts[3], tex[3]);
                    push(pts, place.normal(f.normal), item != nullptr, tint, alpha, textured ? item->texture : nullptr, !item || item->texel <= 1.f, tex);
                }
        }
    };

    auto emitTube = [&](const Tube& tube, const Place& place, ImVec4 a, ImVec4 b) {
        auto rings = sweep(tube, rig.clock);
        const ImVec2 flat[4] = {{0.5f, 0.5f}, {0.5f, 0.5f}, {0.5f, 0.5f}, {0.5f, 0.5f}};
        auto mixed = [&](float m) {
            m = std::clamp(m, 0.f, 1.f);
            return ImVec4{a.x + (b.x - a.x) * m, a.y + (b.y - a.y) * m, a.z + (b.z - a.z) * m, 1.f};
        };
        auto facet = [&](V3 p0, V3 p1, V3 p2, V3 p3, V3 away, float m) {
            V3 n = norm(cross(p2 - p0, p3 - p1));
            V3 centre = (p0 + p1 + p2 + p3) * 0.25f;
            if (dot(n, centre - away) < 0.f) n = n * -1.f;
            const V3 pts[4] = {place.point(p0), place.point(p1), place.point(p2), place.point(p3)};
            push(pts, place.normal(n), tube.twoSided, mixed(m), 1.f, nullptr, false, flat);
        };
        int n = tube.sides;
        for (size_t i = 0; i + 1 < rings.size(); i++) {
            V3 away = (rings[i].center + rings[i + 1].center) * 0.5f;
            float m = (rings[i].mix + rings[i + 1].mix) * 0.5f;
            for (int k = 0; k < n; k++) {
                int k2 = (k + 1) % n;
                facet(rings[i].pts[k], rings[i].pts[k2], rings[i + 1].pts[k2], rings[i + 1].pts[k], away, m);
            }
        }
        if (!tube.capped) return;
        for (int end = 0; end < 2; end++) {
            const Ring& r = end ? rings.back() : rings.front();
            V3 away = r.center + r.tangent * (end ? -1.f : 1.f);
            for (int k = 0; k < n; k++) facet(r.pts[k], r.pts[(k + 1) % n], r.center, r.center, away, r.mix);
        }
    };

    ImVec4 skin{0.86f, 0.66f, 0.54f, 1.f};
    ImVec4 legs{body.x * 0.5f, body.y * 0.5f, body.z * 0.5f, 1.f};
    ImVec4 dark{0.2f, 0.14f, 0.16f, 1.f};
    struct Part {
        V3 o, s;
        ImVec4 c;
    };
    float arm = look.slim ? 3.f : 4.f;
    const Part figure[] = {{{-4, 24, -4}, {8, 8, 8}, skin},  {{-4, 12, -2}, {8, 12, 4}, body},       {{-4 - arm, 12, -2}, {arm, 12, 4}, skin},
                           {{4, 12, -2}, {arm, 12, 4}, skin}, {{-4, 0, -2}, {4, 12, 4}, legs},        {{0, 0, -2}, {4, 12, 4}, legs},
                           {{-2.8f, 27.f, 4.f}, {1.6f, 1.8f, 0.05f}, dark}, {{1.2f, 27.f, 4.f}, {1.6f, 1.8f, 0.05f}, dark}};
    for (auto& p : figure) emitCube({p.o, p.s, {}, 0, 0, -1}, Place{}, nullptr, p.c, 1.f);

    for (auto& w : worn) {
        const Item* item = w.item;
        if (!item) continue;
        auto tintOf = [&](int i, ImVec4 fallback) {
            if (i < 0 || i >= (int)item->tints.size()) return fallback;
            size_t k = size_t(i);
            return k < w.tints.size() ? w.tints[k] : item->tints[k].color;
        };
        for (auto& bone : item->bones) {
            std::vector<const Cube*> order;
            for (auto& c : bone.cubes) order.push_back(&c);
            if (bone.physics.cloth)
                std::stable_sort(order.begin(), order.end(), [](const Cube* a, const Cube* b) { return a->origin.y + a->size.y > b->origin.y + b->size.y; });

            BoneSim& sim = rig.bones[&bone];
            integrate(bone, sim, rig, order);

            Place place;
            place.bone = &bone;
            place.sim = &sim;
            place.order = &order;
            float target[3];
            animTarget(bone, sim, rig, target, place.move);
            const float* pose = bone.physics.spring ? sim.ang : target;
            for (int i = 0; i < 3; i++) place.deg[i] = pose[i];

            for (auto& tube : bone.tubes) {
                ImVec4 a = tintOf(tube.tint, {1, 1, 1, 1});
                emitTube(tube, place, a, tintOf(tube.tint2, a));
            }
            if (!item->texture) continue;
            for (size_t i = 0; i < order.size(); i++) {
                const Cube& cube = *order[i];
                ImVec4 tint = tintOf(cube.tint, {1, 1, 1, 1});
                if (cube.tint2 >= 0) {
                    ImVec4 other = tintOf(cube.tint2, tint);
                    tint = {tint.x + (other.x - tint.x) * cube.mix, tint.y + (other.y - tint.y) * cube.mix, tint.z + (other.z - tint.z) * cube.mix, 1.f};
                }
                float alpha = bone.anim.kind == Motion::Sparkle ? 0.55f + 0.45f * std::sin(sim.phase * 2.f + bone.anim.phase + float(i) * 1.7f) : 1.f;
                Place cp = place;
                cp.cube = int(i);
                emitCube(cube, cp, item, tint, alpha);
            }
        }
    }

    std::stable_sort(draws.begin(), draws.end(), [](const Draw& a, const Draw& b) { return a.depth < b.depth; });
    auto& pio = ImGui::GetPlatformIO();
    bool canSwitch = pio.DrawCallback_SetSamplerNearest && pio.DrawCallback_SetSamplerLinear;
    bool nearestOn = false;
    for (auto& d : draws) {
        if (canSwitch && d.nearest != nearestOn) {
            dl->AddCallback(d.nearest ? pio.DrawCallback_SetSamplerNearest : pio.DrawCallback_SetSamplerLinear, nullptr);
            nearestOn = d.nearest;
        }
        dl->AddImageQuad(d.tex, d.p[0], d.p[1], d.p[2], d.p[3], d.uv[0], d.uv[1], d.uv[2], d.uv[3], d.col);
    }
    if (canSwitch && nearestOn) dl->AddCallback(pio.DrawCallback_SetSamplerLinear, nullptr);
}

}
