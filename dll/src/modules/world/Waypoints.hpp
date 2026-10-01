#pragma once

#include "gui/Gui.hpp"
#include "gui/Theme.hpp"
#include "modules/Module.hpp"
#include "modules/common/Needs.hpp"
#include "modules/common/Text.hpp"
#include "render/Draw.hpp"
#include "render/Fonts.hpp"
#include "render/Ui.hpp"
#include "sdk/Game.hpp"

#include <json.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <format>

class Waypoints : public Module {
public:
    Waypoints()
        : Module("Waypoints", "Your own markers with beam, name and distance, also as an edge arrow.",
                 Category::Visual, {"hud-self"}) {
        sub("World");
        require(need::player | game::Domain::Camera, need::sigs({"LocalPlayer"}));
        data_.hidden = true;
        beamHeight_.visible = [this] { return beam_.b; };
    }

    void onKey(KeyEvent& ev) override {
        sync();
        if (ev.down && !ev.repeat && addKey_.i && ev.vk == addKey_.i) addHere(i18n::fmt("Point {}", list_.size() + 1));
    }

    void onFrame() override {
        sync();
        for (auto& e : game::events())
            if (e.kind == game::EventKind::Death && deathPoint_.b) {
                auto& pos = game::state().player.pos;
                std::string death = i18n::tr("Death");
                std::erase_if(list_, [&](const Wp& w) { return w.name == death; });
                list_.push_back({death, pos.x, pos.y, pos.z, game::state().player.dimension, {1.f, 0.4f, 0.45f}});
                save();
            }
    }

    void onRender(ImDrawList* dl) override {
        sync();
        if (gui::open()) return;
        auto& p = game::state().player;
        auto ds = ImGui::GetIO().DisplaySize;
        float s = ui::scale();
        for (auto& w : list_) {
            if (w.dim != p.dimension) continue;
            game::Vec3 pos{w.x + 0.5f, w.y + 1.0f, w.z + 0.5f};
            float dist = game::distance(p.pos, {w.x, w.y, w.z});
            if (maxDist_.f > 0.f && dist > maxDist_.f) continue;
            float alpha = fade_.b ? std::clamp((dist - 2.f) / 6.f, 0.f, 1.f) : 1.f;
            ImVec4 c{w.color[0], w.color[1], w.color[2], alpha};
            ImU32 col = ImGui::GetColorU32(c);

            if (beam_.b) {
                ImVec2 a, b;
                if (game::projectLine({pos.x, w.y, pos.z}, {pos.x, w.y + beamHeight_.f, pos.z}, a, b)) dl->AddLine(a, b, ImGui::GetColorU32(withAlpha(c, 0.55f)), 2.5f * s);
            }
            auto sp = game::project(pos);
            bool onScreen = sp && sp->x > 0 && sp->x < ds.x && sp->y > 0 && sp->y < ds.y;
            if (onScreen) {
                std::string label = w.name;
                if (distance_.b) label += std::format("  {:.0f} m", dist);
                icon(dl, *sp, col, s);
                ImVec2 ts = fonts::bold()->CalcTextSizeA(13.f * s * labelScale_.f, FLT_MAX, 0.f, label.c_str());
                dl->AddText(fonts::bold(), 13.f * s * labelScale_.f, *sp + ImVec2(-ts.x * 0.5f + 1, 11 * s + 1), IM_COL32(0, 0, 0, int(160 * alpha)), label.c_str());
                dl->AddText(fonts::bold(), 13.f * s * labelScale_.f, *sp + ImVec2(-ts.x * 0.5f, 11 * s), col, label.c_str());
            } else if (edge_.b) {
                edgeArrow(dl, pos, col, dist, s);
            }
        }
    }

    struct Mark {
        std::string name;
        float x, y, z;
        float color[3];
    };

    std::vector<Mark> marks(int dimension) {
        sync();
        std::vector<Mark> out;
        for (auto& w : list_)
            if (w.dim == dimension) out.push_back({w.name, w.x, w.y, w.z, {w.color[0], w.color[1], w.color[2]}});
        return out;
    }

    void drawSettings() override {
        sync();
        ImGui::Spacing();
        ImGui::SetNextItemWidth(180);
        ImGui::InputText("##wpname", name_, sizeof(name_));
        ImGui::SameLine();
        if (ImGui::SmallButton(i18n::tr("Add here"))) addHere(name_[0] ? name_ : i18n::tr("Dot"));
        int remove = -1;
        for (size_t i = 0; i < list_.size(); i++) {
            auto& w = list_[i];
            ImGui::PushID(int(i));
            ImGui::ColorEdit3("##c", w.color, ImGuiColorEditFlags_NoInputs);
            ImGui::SameLine();
            ImGui::Text("%s  (%.0f, %.0f, %.0f)", w.name.c_str(), w.x, w.y, w.z);
            ImGui::SameLine();
            if (ImGui::SmallButton(i18n::tr("Delete"))) remove = int(i);
            ImGui::PopID();
        }
        if (remove >= 0) {
            list_.erase(list_.begin() + remove);
            save();
        }
    }

private:
    struct Wp {
        std::string name;
        float x, y, z;
        int dim;
        float color[3];
    };

    void addHere(const std::string& name) {
        auto& p = game::state().player;
        list_.push_back({name, std::floor(p.pos.x), std::floor(p.pos.y), std::floor(p.pos.z), p.dimension, {color_.color.x, color_.color.y, color_.color.z}});
        save();
    }

    void icon(ImDrawList* dl, ImVec2 c, ImU32 col, float s) {
        float r = 6.f * s;
        switch (iconStyle_.i) {
        case 0: dl->AddQuadFilled({c.x, c.y - r}, {c.x + r, c.y}, {c.x, c.y + r}, {c.x - r, c.y}, col); break;
        case 1: dl->AddCircleFilled(c, r * 0.8f, col); break;
        default: draw::heart(dl, c, r * 2.f, col); break;
        }
    }

    void edgeArrow(ImDrawList* dl, game::Vec3 target, ImU32 col, float dist, float s) {
        auto ds = ImGui::GetIO().DisplaySize;
        auto& cam = game::state().camera;
        float yaw = cam.yaw * 0.0174533f;
        float dx = target.x - cam.pos.x, dz = target.z - cam.pos.z;
        float fwd = dx * -std::sin(yaw) + dz * std::cos(yaw), right = dx * -std::cos(yaw) + dz * -std::sin(yaw);
        float ang = std::atan2(right, fwd);
        ImVec2 c{ds.x * 0.5f, ds.y * 0.5f};
        float rx = ds.x * 0.45f, ry = ds.y * 0.42f;
        ImVec2 p{c.x + std::sin(ang) * rx, c.y - std::cos(ang) * ry * 0.9f};
        ImVec2 d{std::sin(ang), -std::cos(ang)};
        ImVec2 n{-d.y, d.x};
        float k = 9.f * s;
        dl->AddTriangleFilled(p + d * k, p - d * k * 0.6f + n * k * 0.7f, p - d * k * 0.6f - n * k * 0.7f, col);
        if (distance_.b) dl->AddText(fonts::bold(), 12.f * s, p + ImVec2(-12 * s, 12 * s), col, std::format("{:.0f} m", dist).c_str());
    }

    void sync() {
        if (data_.text == synced_) return;
        synced_ = data_.text;
        load();
    }

    void load() {
        list_.clear();
        auto j = nlohmann::json::parse(data_.text, nullptr, false);
        if (!j.is_array()) return;
        for (auto& e : j) {
            if (!e.is_object()) continue;
            Wp w{e.value("name", "?"), e.value("x", 0.f), e.value("y", 0.f), e.value("z", 0.f), e.value("dim", 0), {1.f, 1.f, 1.f}};
            if (e.contains("c") && e["c"].is_array() && e["c"].size() == 3)
                for (int k = 0; k < 3; k++) w.color[k] = e["c"][k].get<float>();
            list_.push_back(w);
        }
    }

    void save() {
        nlohmann::json j = nlohmann::json::array();
        for (auto& w : list_) j.push_back({{"name", w.name}, {"x", w.x}, {"y", w.y}, {"z", w.z}, {"dim", w.dim}, {"c", {w.color[0], w.color[1], w.color[2]}}});
        data_.text = j.dump();
        synced_ = data_.text;
    }

    Setting& addKey_ = keySetting("addKey", "Set a point at my position", 0);
    Setting& color_ = colorSetting("color", "Default color", {1.f, 0.49f, 0.71f, 1.f});
    Setting& iconStyle_ = choice("icon", "Icon", {"Diamond", "Dot", "Heart"});
    Setting& distance_ = toggleSetting("distance", "Show distance", true);
    Setting& labelScale_ = slider("labelScale", "Text size", 1.f, 0.6f, 2.f, "%.2fx");
    Setting& beam_ = toggleSetting("beam", "Light beam", true);
    Setting& beamHeight_ = slider("beamHeight", "Beam height", 60.f, 10.f, 256.f, "%.0f");
    Setting& edge_ = toggleSetting("edge", "Arrow at the screen edge", true);
    Setting& fade_ = toggleSetting("fade", "Fade out when close", true);
    Setting& maxDist_ = slider("maxDist", "Maximum distance (0 = unlimited)", 0.f, 0.f, 2000.f, "%.0f");
    Setting& deathPoint_ = toggleSetting("death", "Set the death point automatically", true);
    Setting& data_ = textSetting("data", "Data", "[]");
    std::vector<Wp> list_;
    std::string synced_ = "[]";
    char name_[64] = "";
};
