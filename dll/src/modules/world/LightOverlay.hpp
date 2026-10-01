#pragma once

#include "gui/Gui.hpp"
#include "modules/Module.hpp"
#include "modules/common/Needs.hpp"
#include "render/Fonts.hpp"
#include "render/Ui.hpp"
#include "sdk/Game.hpp"

#include <cmath>
#include <format>

class LightOverlay : public Module {
public:
    LightOverlay()
        : Module("Light Overlay", "Marks the tiles around you where monsters can spawn, with colors or light numbers.", Category::Visual, {"info-others"}) {
        sub("World");
        require(need::light | need::player | game::Domain::Camera, need::sigs({"LocalPlayer", "LightLevels"}));
        numbers_.visible = [this] { return style_.i == 2; };
        dangerMax_.visible = [this] { return mode_.i != 0; };
        warnMax_.visible = [this] { return mode_.i == 1; };
        warnColor_.visible = [this] { return mode_.i == 1; };
    }

    void onRender(ImDrawList* dl) override {
        if (gui::open()) return;
        auto& g = game::state().light;
        auto& p = game::state().player;
        if (!g.valid()) return;
        float s = ui::scale();
        int r = std::min(g.radius, int(range_.f));
        for (int dz = -r; dz <= r; dz++)
            for (int dx = -r; dx <= r; dx++) {
                if (dx * dx + dz * dz > r * r) continue;
                int level = g.at(dx, dz);
                bool danger = level <= (mode_.i == 0 ? 0 : int(dangerMax_.f));
                bool warn = mode_.i == 1 && !danger && level <= int(warnMax_.f);
                if (!danger && !warn && !(style_.i == 2 && numbers_.b)) continue;
                float x = float(g.baseX + dx), z = float(g.baseZ + dz), y = float(g.baseY) + 1.02f;
                ImVec4 c = danger ? dangerColor_.color : warn ? warnColor_.color : safeColor_.color;
                float fade = std::clamp(1.f - std::sqrt(float(dx * dx + dz * dz)) / float(r + 1), 0.15f, 1.f);
                c.w *= opacity_.f * (fade_.b ? fade : 1.f);
                ImVec2 q[4];
                bool ok = true;
                float inset = style_.i == 1 ? 0.18f : 0.08f;
                const float cx[4] = {x + inset, x + 1.f - inset, x + 1.f - inset, x + inset}, cz[4] = {z + inset, z + inset, z + 1.f - inset, z + 1.f - inset};
                for (int k = 0; k < 4 && ok; k++) {
                    auto pt = game::project({cx[k], y, cz[k]});
                    if (!pt) ok = false;
                    else q[k] = *pt;
                }
                if (!ok) continue;
                ImU32 col = ImGui::GetColorU32(c);
                if (style_.i == 0) dl->AddConvexPolyFilled(q, 4, col);
                else if (style_.i == 1) {
                    dl->AddLine(q[0], q[2], col, 2.f * s);
                    dl->AddLine(q[1], q[3], col, 2.f * s);
                } else if (danger || warn || numbers_.b) {
                    std::string t = std::to_string(level);
                    ImVec2 mid = (q[0] + q[2]) * 0.5f;
                    ImVec2 ts = fonts::bold()->CalcTextSizeA(13.f * s, FLT_MAX, 0.f, t.c_str());
                    dl->AddText(fonts::bold(), 13.f * s, mid - ts * 0.5f + ImVec2(1, 1), IM_COL32(0, 0, 0, int(150 * c.w)), t.c_str());
                    dl->AddText(fonts::bold(), 13.f * s, mid - ts * 0.5f, col, t.c_str());
                }
            }
        (void)p;
    }

private:
    Setting& mode_ = choice("mode", "Show", {"Only where monsters can spawn", "Spawn and warning tiles", "Several light levels"});
    Setting& style_ = choice("style", "Style", {"Filled tiles", "Crosses", "Light numbers"});
    Setting& numbers_ = toggleSetting("numbers", "Numbers on safe tiles too", false);
    Setting& range_ = slider("range", "Range (blocks)", 8.f, 3.f, 16.f, "%.0f");
    Setting& dangerMax_ = slider("dangerMax", "Danger up to light level", 0.f, 0.f, 7.f, "%.0f");
    Setting& warnMax_ = slider("warnMax", "Warning up to light level", 7.f, 1.f, 14.f, "%.0f");
    Setting& opacity_ = slider("opacity", "Opacity", 0.6f, 0.1f, 1.f, "%.2f");
    Setting& fade_ = toggleSetting("fade", "Fade out with distance", true);
    Setting& dangerColor_ = colorSetting("danger", "Danger color", {1.f, 0.25f, 0.3f, 1.f});
    Setting& warnColor_ = colorSetting("warn", "Warning color", {1.f, 0.82f, 0.3f, 1.f});
    Setting& safeColor_ = colorSetting("safe", "Safe color", {0.5f, 1.f, 0.6f, 1.f});
};
