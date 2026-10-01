#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "Cosmetics.hpp"
#include "core/Log.hpp"
#include "core/Paths.hpp"
#include "render/Ui.hpp"

#include <imgui_internal.h>
#include <json.hpp>
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include <stb_image.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace cosmetics {

namespace {

std::vector<Item> list;
ImTextureData* white = nullptr;

std::vector<unsigned char> readFile(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

ImTextureData* upload(const unsigned char* rgba, int w, int h) {
    auto* td = IM_NEW(ImTextureData)();
    td->Create(ImTextureFormat_RGBA32, w, h);
    std::memcpy(td->GetPixels(), rgba, size_t(w) * size_t(h) * 4);
    ImGui::RegisterUserTexture(td);
    return td;
}

void retire(ImTextureData* td) {
    if (td) td->WantDestroyNextFrame = true;
}

V3 vec(const nlohmann::json& j, V3 fallback = {}) {
    if (!j.is_array() || j.size() < 3) return fallback;
    return {j[0].get<float>(), j[1].get<float>(), j[2].get<float>()};
}

ImVec4 hex(const std::string& s) {
    unsigned v = s.size() >= 7 ? (unsigned)std::strtoul(s.c_str() + 1, nullptr, 16) : 0xffffff;
    return {((v >> 16) & 255) / 255.f, ((v >> 8) & 255) / 255.f, (v & 255) / 255.f, 1.f};
}

Motion motionOf(const std::string& s) {
    static const std::pair<const char*, Motion> names[] = {{"flap", Motion::Flap},   {"sway", Motion::Sway}, {"bob", Motion::Bob},
                                                          {"wag", Motion::Wag},     {"twitch", Motion::Twitch}, {"float", Motion::Float},
                                                          {"spin", Motion::Spin}};
    for (auto& [n, m] : names)
        if (s == n) return m;
    return Motion::None;
}

bool loadItem(const std::filesystem::path& dir, Item& out) {
    std::ifstream in(dir / L"item.json");
    if (!in) return false;
    auto j = nlohmann::json::parse(in, nullptr, false);
    if (!j.is_object()) return false;
    out.id = j.value("id", "");
    out.name = j.value("name", out.id);
    out.slot = j.value("slot", "");
    if (out.id.empty() || !j.contains("bones")) return false;

    if (j.contains("tint") && j["tint"].is_array())
        for (auto& t : j["tint"]) out.tints.push_back({t.value("name", ""), hex(t.value("default", "#ffffff"))});

    for (auto& b : j["bones"]) {
        Bone bone;
        bone.pivot = vec(b.value("pivot", nlohmann::json()));
        bone.rotation = vec(b.value("rotation", nlohmann::json()));
        if (b.contains("anim") && b["anim"].is_object()) {
            auto& a = b["anim"];
            std::string axis = a.value("axis", "y");
            bone.anim = {motionOf(a.value("type", "")), axis == "x" ? 0 : axis == "z" ? 2 : 1, a.value("amplitude", 0.f), a.value("speed", 1.f), a.value("phase", 0.f)};
        }
        for (auto& c : b.value("cubes", nlohmann::json::array())) {
            Cube cube;
            cube.origin = vec(c.value("origin", nlohmann::json()));
            cube.size = vec(c.value("size", nlohmann::json()), {1, 1, 1});
            auto uv = c.value("uv", std::vector<int>{0, 0});
            cube.u = uv.size() > 0 ? uv[0] : 0;
            cube.v = uv.size() > 1 ? uv[1] : 0;
            std::string tint = c.value("tint", "");
            for (size_t i = 0; i < out.tints.size(); i++)
                if (out.tints[i].name == tint) cube.tint = int(i);
            bone.cubes.push_back(cube);
        }
        out.bones.push_back(std::move(bone));
    }

    auto png = readFile(dir / std::filesystem::path(j.value("texture", "tex.png")));
    int w = 0, h = 0, n = 0;
    unsigned char* pixels = png.empty() ? nullptr : stbi_load_from_memory(png.data(), int(png.size()), &w, &h, &n, 4);
    if (!pixels) return false;
    out.texW = w;
    out.texH = h;
    out.texture = upload(pixels, w, h);
    stbi_image_free(pixels);
    return true;
}

}

const std::vector<Item>& items() { return list; }

const Item* find(const std::string& id) {
    for (auto& i : list)
        if (i.id == id) return &i;
    return nullptr;
}

void reload() {
    for (auto& i : list) retire(i.texture);
    list.clear();
    auto root = paths::root() / L"cosmetics";
    std::ifstream in(root / L"index.json");
    if (!in) return;
    auto j = nlohmann::json::parse(in, nullptr, false);
    if (!j.is_object() || !j.contains("items")) return;
    for (auto& e : j["items"]) {
        Item item;
        if (loadItem(root / std::filesystem::path(e.value("id", "")), item)) list.push_back(std::move(item));
        else logger::warn("cosmetic {} could not be loaded", e.value("id", ""));
    }
}

namespace {

struct Face {
    V3 corner[4];
    V3 normal;
    int region;
};

V3 rotateAxis(V3 p, int axis, float deg) {
    float a = deg * 0.0174533f, c = std::cos(a), s = std::sin(a);
    if (axis == 0) return {p.x, p.y * c - p.z * s, p.y * s + p.z * c};
    if (axis == 1) return {p.x * c + p.z * s, p.y, -p.x * s + p.z * c};
    return {p.x * c - p.y * s, p.x * s + p.y * c, p.z};
}

float animAngle(const Anim& a, double t) {
    float k = float(t) * a.speed * 6.2832f + a.phase;
    switch (a.kind) {
    case Motion::Flap:
    case Motion::Sway:
    case Motion::Wag: return a.amplitude * std::sin(k);
    case Motion::Twitch: return a.amplitude * std::max(0.f, std::sin(k * 0.5f)) * std::max(0.f, std::sin(k * 7.f));
    case Motion::Spin: return float(t) * a.speed * 360.f;
    default: return 0.f;
    }
}

float animOffset(const Anim& a, double t) {
    if (a.kind != Motion::Bob && a.kind != Motion::Float) return 0.f;
    return a.amplitude * std::sin(float(t) * a.speed * 6.2832f + a.phase);
}

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

// minecraft box layout: top and bottom on the first row, then right, front, left, back
void regionUv(const Cube& c, int region, float& u0, float& v0, float& u1, float& v1) {
    float w = c.size.x, h = c.size.y, d = c.size.z;
    float u = float(c.u), v = float(c.v);
    switch (region) {
    case 4: u0 = u + d; v0 = v; u1 = u0 + w; v1 = v + d; break;
    case 5: u0 = u + d + w; v0 = v; u1 = u0 + w; v1 = v + d; break;
    case 3: u0 = u; v0 = v + d; u1 = u0 + d; v1 = v0 + h; break;
    case 0: u0 = u + d; v0 = v + d; u1 = u0 + w; v1 = v0 + h; break;
    case 2: u0 = u + d + w; v0 = v + d; u1 = u0 + d; v1 = v0 + h; break;
    default: u0 = u + 2 * d + w; v0 = v + d; u1 = u0 + w; v1 = v0 + h; break;
    }
}

struct Draw {
    ImVec2 p[4];
    ImVec2 uv[4];
    float depth;
    ImU32 col;
    ImTextureRef tex;
};

}

void drawPreview(ImDrawList* dl, ImVec2 center, float unit, float yaw, float pitch, const std::vector<const Item*>& equipped, ImVec4 body) {
    if (!white) {
        const unsigned char px[4] = {255, 255, 255, 255};
        white = upload(px, 1, 1);
    }
    double t = ui::time();
    std::vector<Draw> draws;

    auto project = [&](V3 p) {
        V3 r = rotateAxis(rotateAxis(p, 1, yaw), 0, pitch);
        return V3{center.x + r.x * unit, center.y - (r.y - 16.f) * unit, r.z};
    };
    auto emit = [&](const Cube& cube, const Bone* bone, const Item* item, ImVec4 tint, ImTextureData* tex, float texW, float texH) {
        std::vector<Face> faces;
        addFaces(faces, cube);
        for (auto& f : faces) {
            V3 n = f.normal;
            V3 pts[4];
            for (int i = 0; i < 4; i++) {
                V3 p = f.corner[i];
                if (bone) {
                    p = {p.x - bone->pivot.x, p.y - bone->pivot.y, p.z - bone->pivot.z};
                    p = rotateAxis(rotateAxis(rotateAxis(p, 0, bone->rotation.x), 1, bone->rotation.y), 2, bone->rotation.z);
                    float a = animAngle(bone->anim, t);
                    if (a != 0.f) p = rotateAxis(p, bone->anim.axis, a);
                    p = {p.x + bone->pivot.x, p.y + bone->pivot.y + animOffset(bone->anim, t), p.z + bone->pivot.z};
                }
                pts[i] = p;
            }
            if (bone) {
                n = rotateAxis(rotateAxis(rotateAxis(n, 0, bone->rotation.x), 1, bone->rotation.y), 2, bone->rotation.z);
                float a = animAngle(bone->anim, t);
                if (a != 0.f) n = rotateAxis(n, bone->anim.axis, a);
            }
            V3 vn = rotateAxis(rotateAxis(n, 1, yaw), 0, pitch);
            if (vn.z < -0.02f && !item) continue;
            Draw d{};
            float depth = 0.f;
            for (int i = 0; i < 4; i++) {
                V3 sp = project(pts[i]);
                d.p[i] = {sp.x, sp.y};
                depth += sp.z * 0.25f;
            }
            float shade = 0.62f + 0.38f * std::clamp(vn.y * 0.5f + vn.z * 0.8f + 0.2f, 0.f, 1.f);
            d.col = ImGui::GetColorU32({tint.x * shade, tint.y * shade, tint.z * shade, 1.f});
            d.depth = depth;
            if (tex) {
                float u0, v0, u1, v1;
                regionUv(cube, f.region, u0, v0, u1, v1);
                u0 /= texW; u1 /= texW; v0 /= texH; v1 /= texH;
                d.uv[0] = {u0, v0}; d.uv[1] = {u1, v0}; d.uv[2] = {u1, v1}; d.uv[3] = {u0, v1};
                d.tex = tex->GetTexRef();
            } else {
                for (auto& uv : d.uv) uv = {0.5f, 0.5f};
                d.tex = white->GetTexRef();
            }
            draws.push_back(d);
        }
    };

    ImVec4 skin{0.86f, 0.66f, 0.54f, 1.f};
    ImVec4 legs{body.x * 0.5f, body.y * 0.5f, body.z * 0.5f, 1.f};
    struct Part {
        V3 o, s;
        ImVec4 c;
    };
    const Part figure[] = {{{-4, 24, -4}, {8, 8, 8}, skin}, {{-4, 12, -2}, {8, 12, 4}, body}, {{-8, 12, -2}, {4, 12, 4}, skin},
                           {{4, 12, -2}, {4, 12, 4}, skin},  {{-4, 0, -2}, {4, 12, 4}, legs}, {{0, 0, -2}, {4, 12, 4}, legs}};
    for (auto& p : figure) emit({p.o, p.s, 0, 0, -1}, nullptr, nullptr, p.c, nullptr, 1.f, 1.f);

    for (const Item* item : equipped) {
        if (!item || !item->texture) continue;
        for (auto& bone : item->bones)
            for (auto& cube : bone.cubes) {
                ImVec4 tint = cube.tint >= 0 && cube.tint < (int)item->tints.size() ? item->tints[size_t(cube.tint)].color : ImVec4(1, 1, 1, 1);
                emit(cube, &bone, item, tint, item->texture, float(item->texW), float(item->texH));
            }
    }

    std::stable_sort(draws.begin(), draws.end(), [](const Draw& a, const Draw& b) { return a.depth < b.depth; });
    for (auto& d : draws) dl->AddImageQuad(d.tex, d.p[0], d.p[1], d.p[2], d.p[3], d.uv[0], d.uv[1], d.uv[2], d.uv[3], d.col);
}

}
