#pragma once

#include "modules/common/GameHud.hpp"
#include "modules/common/Inventory.hpp"
#include "modules/common/Needs.hpp"
#include "render/Draw.hpp"
#include "render/Ui.hpp"
#include "sdk/Game.hpp"

#include <algorithm>
#include <cmath>

class Paperdoll : public GameList {
public:
    Paperdoll()
        : GameList("Paperdoll", "Kleine Figur im HUD, die deine Rüstung trägt, schleicht, sprintet und bei Treffern rot aufblitzt.", need::player | game::Domain::Inventory,
                   need::sigs({"LocalPlayer", "Inventory"}), {"cosmetic"}, {0.90f, 0.62f}) {
        sub("Eigene Werte");
        background_.b = false;
    }

    void onFrame() override {
        for (auto& e : game::events())
            if (e.kind == game::EventKind::Hurt) flash_ = 1.f;
        flash_ = std::max(0.f, flash_ - ui::dt() * 3.f);
        auto& p = game::state().player;
        lean_ = draw::approach(lean_, p.sprinting ? 1.f : 0.f, 10.f);
        crouch_ = draw::approach(crouch_, p.sneaking ? 1.f : 0.f, 14.f);
    }

protected:
    ImVec2 content(ImDrawList* dl, ImVec2 o, float s) override {
        auto& p = game::state().player;
        float u = size_.f * s / 32.f;
        float t = float(ui::time());
        float walk = std::hypot(p.vel.x, p.vel.z) > 0.3f ? std::sin(t * 9.f) * (p.sprinting ? 0.6f : 0.4f) : 0.f;
        float h = 32.f * u;
        ImVec2 base = o + ImVec2(12 * u, h);
        float drop = crouch_ * 3.f * u;
        float shear = lean_ * 2.f * u + crouch_ * 1.5f * u;

        auto flash = [&](ImU32 c) {
            if (flash_ <= 0.f || !hurt_.b) return c;
            ImVec4 v = ImGui::ColorConvertU32ToFloat4(c);
            v = ImVec4(v.x + (1.f - v.x) * flash_ * 0.6f, v.y * (1.f - flash_ * 0.6f), v.z * (1.f - flash_ * 0.6f), v.w);
            return ImGui::ColorConvertFloat4ToU32(v);
        };
        auto armorColor = [&](int slot, ImU32 fallback) {
            auto& a = p.armor[size_t(slot)];
            return flash(armor_.b && !a.empty() ? materialColor(a.name) : fallback);
        };
        auto box = [&](float x0, float y0, float x1, float y1, ImU32 c, float topShear = 0.f) {
            ImVec2 a = base + ImVec2(x0 * u + topShear, -(32 - y0) * u + drop), b = base + ImVec2(x1 * u + topShear, -(32 - y1) * u + drop);
            dl->AddRectFilled(a, b, c, 1.5f * u);
        };

        ImU32 skin = flash(ImGui::GetColorU32(skin_.color)), shirt = flash(ImGui::GetColorU32(shirt_.color)), pants = flash(ImGui::GetColorU32(pants_.color));
        ImU32 hair = flash(ImGui::GetColorU32(hair_.color));

        float legSwing = walk * 3.f * u;
        box(2, 20, 6, 32, armorColor(2, pants), 0.f);
        dl->AddRectFilled(base + ImVec2(2 * u + legSwing, -12 * u + drop), base + ImVec2(6 * u + legSwing, drop), armorColor(3, pants), 1.f * u);
        dl->AddRectFilled(base + ImVec2(6 * u - legSwing, -12 * u + drop), base + ImVec2(10 * u - legSwing, drop), armorColor(3, pants), 1.f * u);
        box(2, 10, 10, 20, armorColor(1, shirt), shear * 0.5f);
        float arm = walk * 3.f * u;
        dl->AddRectFilled(base + ImVec2(-2 * u - arm + shear * 0.5f, -22 * u + drop), base + ImVec2(2 * u - arm + shear * 0.5f, -10 * u + drop), armorColor(1, skin), 1.f * u);
        dl->AddRectFilled(base + ImVec2(10 * u + arm + shear * 0.5f, -22 * u + drop), base + ImVec2(14 * u + arm + shear * 0.5f, -10 * u + drop), armorColor(1, skin), 1.f * u);
        box(2, 2, 10, 10, skin, shear);
        box(2, 2, 10, 4, hair, shear);
        auto& helm = p.armor[0];
        bool helmet = armor_.b && !helm.empty();
        if (helmet) box(1.5f, 1.5f, 10.5f, 10.5f, armorColor(0, skin), shear);
        ImU32 eye = helmet ? IM_COL32(40, 30, 50, 255) : IM_COL32(60, 40, 70, 255);
        box(3, 6, 4.5f, 7.5f, eye, shear);
        box(7.5f, 6, 9, 7.5f, eye, shear);

        if (item_.b && !p.held().empty())
            dl->AddRectFilled(base + ImVec2(12 * u + arm + shear * 0.5f, -16 * u + drop), base + ImVec2(17 * u + arm + shear * 0.5f, -10 * u + drop), materialColor(p.held().name), 1.f * u);
        return {18 * u, h + 4 * u};
    }

private:
    Setting& size_ = slider("size", "Größe", 90.f, 40.f, 220.f, "%.0f");
    Setting& armor_ = toggleSetting("armor", "Rüstung anzeigen", true);
    Setting& item_ = toggleSetting("item", "Item in der Hand", true);
    Setting& hurt_ = toggleSetting("hurt", "Rot bei Treffern", true);
    Setting& skin_ = colorSetting("skin", "Hautfarbe", {1.f, 0.82f, 0.72f, 1.f});
    Setting& hair_ = colorSetting("hair", "Haarfarbe", {0.45f, 0.28f, 0.22f, 1.f});
    Setting& shirt_ = colorSetting("shirt", "Oberteil", {1.f, 0.55f, 0.75f, 1.f});
    Setting& pants_ = colorSetting("pants", "Hose", {0.55f, 0.5f, 0.85f, 1.f});
    float flash_ = 0.f;
    float lean_ = 0.f;
    float crouch_ = 0.f;
};
