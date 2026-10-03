#pragma once

#include "gui/Gui.hpp"
#include "gui/Theme.hpp"
#include "modules/Module.hpp"
#include "modules/common/Colors.hpp"
#include "modules/common/Context.hpp"
#include "modules/common/Needs.hpp"
#include "render/Draw.hpp"
#include "render/Fonts.hpp"
#include "render/Ui.hpp"
#include "sdk/Effects.hpp"
#include "sdk/Game.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>

inline ImVec4 rainbow(float speed, float sat = 0.6f) {
    float r, g, b;
    ImGui::ColorConvertHSVtoRGB(std::fmod(float(ui::time()) * speed * 0.2f, 1.f), sat, 1.f, r, g, b);
    return {r, g, b, 1.f};
}

class BlockOutline : public Module {
public:
    BlockOutline()
        : Module("Block Outline", "Your own outline for the block you look at: color, thickness, fill, rainbow and pulsing.", Category::Visual,
                 {"cosmetic"}) {
        sub("World");
        require(need::target | game::Domain::Camera, need::sigs({"LocalPlayer", "Target"}));
        fillColor_.visible = [this] { return fill_.b; };
        color_.visible = [this] { return !rainbow_.b; };
        speed_.visible = [this] { return rainbow_.b || pulse_.b; };
    }

    void onFrame() override {
        if (hideVanilla_.b) fx::skip(fx::Id::BlockOutline);
    }

    void onRender(ImDrawList* dl) override {
        auto& t = game::state().target;
        if (t.kind != game::Target::Kind::Block) return;
        float g = grow_.f;
        game::Vec3 mn{float(t.blockX) - g, float(t.blockY) - g, float(t.blockZ) - g};
        game::Vec3 mx{float(t.blockX) + 1.f + g, float(t.blockY) + 1.f + g, float(t.blockZ) + 1.f + g};
        game::Vec3 c[8] = {{mn.x, mn.y, mn.z}, {mx.x, mn.y, mn.z}, {mx.x, mn.y, mx.z}, {mn.x, mn.y, mx.z},
                           {mn.x, mx.y, mn.z}, {mx.x, mx.y, mn.z}, {mx.x, mx.y, mx.z}, {mn.x, mx.y, mx.z}};

        ImVec4 col = rainbow_.b ? rainbow(speed_.f) : color_.color;
        if (pulse_.b) col.w *= 0.65f + 0.35f * std::sin(float(ui::time()) * speed_.f * 4.f);
        ImU32 line = ImGui::GetColorU32(col);

        if (fill_.b) {
            static const int faces[6][4] = {{0, 1, 2, 3}, {4, 5, 6, 7}, {0, 1, 5, 4}, {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 0, 4, 7}};
            auto& cam = game::state().camera.pos;
            for (auto& f : faces) {
                game::Vec3 ctr{(c[f[0]].x + c[f[2]].x) * 0.5f, (c[f[0]].y + c[f[2]].y) * 0.5f, (c[f[0]].z + c[f[2]].z) * 0.5f};
                game::Vec3 mid{(mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f, (mn.z + mx.z) * 0.5f};
                game::Vec3 out{ctr.x - mid.x, ctr.y - mid.y, ctr.z - mid.z};
                game::Vec3 toCam{cam.x - ctr.x, cam.y - ctr.y, cam.z - ctr.z};
                if (out.x * toCam.x + out.y * toCam.y + out.z * toCam.z <= 0.f) continue;
                ImVec2 pts[4];
                bool ok = true;
                for (int k = 0; k < 4; k++) {
                    auto p = game::project(c[f[k]]);
                    if (!p) {
                        ok = false;
                        break;
                    }
                    pts[k] = *p;
                }
                if (ok) dl->AddConvexPolyFilled(pts, 4, ImGui::GetColorU32(fillColor_.color));
            }
        }

        static const int edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
        for (auto& e : edges) {
            ImVec2 a, b;
            if (game::projectLine(c[e[0]], c[e[1]], a, b)) dl->AddLine(a, b, line, thickness_.f);
        }
    }

private:
    Setting& thickness_ = slider("thickness", "Thickness", 2.f, 1.f, 6.f, "%.1f");
    Setting& grow_ = slider("grow", "Distance to the block", 0.003f, 0.f, 0.05f, "%.3f");
    Setting& color_ = colorSetting("color", "Color", {0.23f, 0.65f, 0.93f, 1.f});
    Setting& rainbow_ = toggleSetting("rainbow", "Rainbow", false);
    Setting& pulse_ = toggleSetting("pulse", "Pulse", false);
    Setting& speed_ = slider("speed", "Speed", 1.f, 0.1f, 5.f, "%.1f");
    Setting& fill_ = toggleSetting("fill", "Fill the faces", false);
    Setting& fillColor_ = colorSetting("fillColor", "Fill color", {0.23f, 0.65f, 0.93f, 0.18f});
    Setting& hideVanilla_ = needs(toggleSetting("hideVanilla", "Hide the original outline", true), fx::Id::BlockOutline);
};

class ChunkBorder : public Module {
public:
    ChunkBorder()
        : Module("Chunk Border", "Shows the chunk borders around you as lines, with sub-chunks, range and colors.", Category::Visual, {"cosmetic"}) {
        sub("World");
        require(need::player | game::Domain::Camera, need::sigs({"LocalPlayer"}));
    }

    void onKey(KeyEvent& ev) override {
        if (ev.down && !ev.repeat && key_.i && ev.vk == key_.i) visible_ = !visible_;
    }

    void onRender(ImDrawList* dl) override {
        if (!visible_) return;
        auto& p = game::state().player;
        int cx = int(std::floor(p.pos.x / 16.f)), cz = int(std::floor(p.pos.z / 16.f));
        float y0 = std::floor(p.pos.y) - range_.f, y1 = std::floor(p.pos.y) + range_.f;
        int r = onlyCurrent_.b ? 0 : radius_.i;
        ImU32 col = ImGui::GetColorU32(color_.color), colSub = ImGui::GetColorU32(subColor_.color), colEdge = ImGui::GetColorU32(edgeColor_.color);

        for (int dx = -r; dx <= r; dx++)
            for (int dz = -r; dz <= r; dz++) {
                bool current = dx == 0 && dz == 0;
                float x0 = float((cx + dx) * 16), z0 = float((cz + dz) * 16), x1 = x0 + 16.f, z1 = z0 + 16.f;
                ImU32 c = current ? colEdge : col;
                float th = current ? thickness_.f * 1.4f : thickness_.f;
                game::Vec3 corners[4] = {{x0, 0, z0}, {x1, 0, z0}, {x1, 0, z1}, {x0, 0, z1}};
                for (auto& v : corners) {
                    ImVec2 a, b;
                    if (game::projectLine({v.x, y0, v.z}, {v.x, y1, v.z}, a, b)) dl->AddLine(a, b, c, th);
                }
                auto ring = [&](float y, ImU32 cc, float t) {
                    for (int k = 0; k < 4; k++) {
                        ImVec2 a, b;
                        game::Vec3 s = corners[k], e = corners[(k + 1) % 4];
                        if (game::projectLine({s.x, y, s.z}, {e.x, y, e.z}, a, b)) dl->AddLine(a, b, cc, t);
                    }
                };
                float ys = std::floor(p.pos.y / 16.f) * 16.f;
                ring(y0, c, th);
                ring(y1, c, th);
                if (current) ring(p.pos.y, c, th);
                if (subchunks_.b && current)
                    for (float y = ys - 32.f; y <= ys + 48.f; y += 16.f)
                        if (y >= y0 - 16.f && y <= y1 + 16.f) ring(y, colSub, std::max(1.f, th * 0.7f));
            }
    }

private:
    Setting& key_ = keySetting("toggle", "On/off key", 0);
    Setting& radius_ = intSlider("radius", "Range (chunks)", 1, 0, 3);
    Setting& onlyCurrent_ = toggleSetting("onlyCurrent", "Current chunk only", false);
    Setting& range_ = slider("range", "Height above and below you", 24.f, 4.f, 80.f, "%.0f");
    Setting& subchunks_ = toggleSetting("subchunks", "Sub-chunk levels", true);
    Setting& thickness_ = slider("thickness", "Thickness", 1.5f, 1.f, 4.f, "%.1f");
    Setting& color_ = colorSetting("color", "Color neighbor chunks", {1.f, 0.82f, 0.49f, 0.7f});
    Setting& edgeColor_ = colorSetting("edge", "Color current chunk", {0.23f, 0.65f, 0.93f, 0.95f});
    Setting& subColor_ = colorSetting("sub", "Color levels", {0.7f, 0.6f, 1.f, 0.5f});
    bool visible_ = true;
};

class HideHand : public Module {
public:
    HideHand()
        : Module("Hide Hand", "Hides hand and item in first person, for a clear view or screenshots.", Category::Visual, {"cosmetic"}) {
        sub("Model");
        require(0, {fx::sig(fx::Id::HideHand)});
        onlyEmpty_.visible = [] { return game::has(game::Domain::Inventory); };
    }

    void onFrame() override {
        auto& p = game::state().player;
        if (onlyEmpty_.b && game::has(game::Domain::Inventory) && !p.held().empty()) return;
        if (main_.b) fx::skip(fx::Id::HideHand);
        if (offhand_.b) fx::skip(fx::Id::HideOffhand);
    }

private:
    Setting& main_ = toggleSetting("main", "Hide main hand", true);
    Setting& offhand_ = needs(toggleSetting("offhand", "Hide offhand", true), fx::Id::HideOffhand);
    Setting& onlyEmpty_ = toggleSetting("onlyEmpty", "Only with an empty hand", false);
};

class ViewModel : public Module {
public:
    ViewModel()
        : Module("View Model", "Changes the field of view of hand and item in first person, and where the game version allows it their position, size and rotation.", Category::Visual, {"cosmetic"}) {
        sub("Model");
        requireAny({fx::sig(fx::Id::HandMatrix), fx::sig(fx::Id::ItemFov), fx::sig(fx::Id::HandMatrixThird)});
        itemFov_.visible = [this] { return changeFov_.b; };
        changeFov_.visible = [] { return fx::available(fx::Id::ItemFov); };
        third_.visible = [] { return fx::available(fx::Id::HandMatrixThird); };
    }

    void onFrame() override {
        float k = uniform_.f;
        game::Vec3 move{x_.f, y_.f, z_.f}, scale{sx_.f * k, sy_.f * k, sz_.f * k}, rot{rx_.f, ry_.f, rz_.f};
        fx::transform(fx::Id::HandMatrix, move, scale, rot);
        if (third_.b) fx::transform(fx::Id::HandMatrixThird, move, scale, rot);
        if (changeFov_.b) fx::set(fx::Id::ItemFov, itemFov_.f);
    }

private:
    Setting& changeFov_ = toggleSetting("changeFov", "Change the item field of view", false);
    Setting& itemFov_ = slider("itemFov", "Item field of view", 70.f, 30.f, 140.f, "%.0f");
    Setting& third_ = toggleSetting("third", "Also in third person", false);
    Setting& x_ = needs(slider("posX", "Position X", 0.f, -1.f, 1.f, "%.2f"), fx::Id::HandMatrix);
    Setting& y_ = needs(slider("posY", "Position Y", 0.f, -1.f, 1.f, "%.2f"), fx::Id::HandMatrix);
    Setting& z_ = needs(slider("z", "Position Z", 0.f, -1.f, 1.f, "%.2f"), fx::Id::HandMatrix);
    Setting& uniform_ = needs(slider("scale", "Overall size", 1.f, 0.3f, 2.f, "%.2fx"), fx::Id::HandMatrix);
    Setting& sx_ = needs(slider("sx", "Width", 1.f, 0.3f, 2.f, "%.2fx"), fx::Id::HandMatrix);
    Setting& sy_ = needs(slider("sy", "Height", 1.f, 0.3f, 2.f, "%.2fx"), fx::Id::HandMatrix);
    Setting& sz_ = needs(slider("sz", "Depth", 1.f, 0.3f, 2.f, "%.2fx"), fx::Id::HandMatrix);
    Setting& rx_ = needs(slider("rx", "Rotation X", 0.f, -180.f, 180.f, "%.0f°"), fx::Id::HandMatrix);
    Setting& ry_ = needs(slider("ry", "Rotation Y", 0.f, -180.f, 180.f, "%.0f°"), fx::Id::HandMatrix);
    Setting& rz_ = needs(slider("rz", "Rotation Z", 0.f, -180.f, 180.f, "%.0f°"), fx::Id::HandMatrix);
};
