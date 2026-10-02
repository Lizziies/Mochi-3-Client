#pragma once

#include "gui/Gui.hpp"
#include "modules/Module.hpp"
#include "modules/common/Colors.hpp"
#include "modules/common/Needs.hpp"
#include "render/Draw.hpp"
#include "render/Ui.hpp"
#include "sdk/Game.hpp"

#include <algorithm>
#include <cmath>

class HotbarAnimation : public Module {
public:
    HotbarAnimation()
        : Module("Hotbar Animation", "A smooth selection frame that glides between hotbar slots, with a small pop when you switch.", Category::Visual, {"cosmetic"}) {
        sub("HUD parts");
        require(need::player | need::inventory, need::sigs({"LocalPlayer", "Inventory"}));
        glowSize_.visible = [this] { return glow_.b; };
        glowColor_.visible = [this] { return glow_.b; };
    }

    void onRender(ImDrawList* dl) override {
        auto& st = game::state();
        if (gui::editingHud() || st.player.mode == game::Mode::Spectator) return;
        auto ds = ImGui::GetIO().DisplaySize;
        float scale = guiScale_.f > 0.f ? guiScale_.f : std::clamp(std::floor(std::min(ds.x / 320.f, ds.y / 240.f)), 1.f, 6.f);
        float pitch = 20.f * scale, left = (ds.x - 182.f * scale) * 0.5f + 1.f * scale;
        float top = ds.y - 22.f * scale + bottom_.f;

        int slot = std::clamp(st.player.slot, 0, 8);
        if (last_ < 0) {
            pos_ = float(slot);
            last_ = slot;
        }
        if (slot != last_) {
            pop_ = 1.f;
            last_ = slot;
        }
        pos_ = draw::approach(pos_, float(slot), speed_.f);
        pop_ = draw::approach(pop_, 0.f, 9.f);

        float cx = left + 1.f * scale + pitch * (pos_ + 0.5f) - scale * 0.5f, cy = top + 11.f * scale;
        float half = (12.f + pop_ * (popSize_.f)) * scale;
        ImVec2 a{cx - half, cy - half}, b{cx + half, cy + half};
        ImVec4 c = color_.color;
        float r = rounding_.f * scale * 0.5f;
        if (glow_.b) draw::glow(dl, a, b, r, ImGui::GetColorU32(glowColor_.color), glowSize_.f * scale);
        dl->AddRect(a, b, ImGui::GetColorU32(c), r, 0, thickness_.f * scale * 0.5f);
        if (fill_.f > 0.f) dl->AddRectFilled(a, b, ImGui::GetColorU32(withAlpha(c, fill_.f)), r);
    }

private:
    Setting& guiScale_ = slider("guiScale", "GUI scale (0 = automatic)", 0.f, 0.f, 6.f, "%.0f");
    Setting& bottom_ = slider("bottom", "Vertical correction", 0.f, -20.f, 20.f, "%.0f");
    Setting& speed_ = slider("speed", "Glide speed", 16.f, 4.f, 40.f, "%.0f");
    Setting& popSize_ = slider("pop", "Pop when switching", 2.f, 0.f, 6.f, "%.1f");
    Setting& thickness_ = slider("thickness", "Frame thickness", 3.f, 1.f, 8.f, "%.1f");
    Setting& rounding_ = slider("rounding", "Rounding", 3.f, 0.f, 10.f, "%.0f");
    Setting& fill_ = slider("fill", "Fill", 0.f, 0.f, 0.6f, "%.2f");
    Setting& glow_ = toggleSetting("glow", "Glow", true);
    Setting& glowSize_ = slider("glowSize", "Glow size", 6.f, 2.f, 14.f, "%.0f");
    Setting& color_ = colorSetting("color", "Frame", {0.23f, 0.65f, 0.93f, 1.f});
    Setting& glowColor_ = colorSetting("glowColor", "Glow color", {0.23f, 0.65f, 0.93f, 1.f});
    float pos_ = 0.f;
    float pop_ = 0.f;
    int last_ = -1;
};
