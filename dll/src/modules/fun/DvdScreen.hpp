#pragma once

#include "core/Build.hpp"
#include "gui/Theme.hpp"
#include "modules/Module.hpp"
#include "render/Draw.hpp"
#include "render/Fonts.hpp"
#include "render/Ui.hpp"

#include <cmath>

class DvdScreen : public Module {
public:
    DvdScreen() : Module("DVD Screen", "A bouncing logo like the DVD screensaver.", Category::Fun, {"cosmetic"}) {
        sub("Spiele");
    }

    void onRender(ImDrawList* dl) override {
        float s = ui::scale();
        auto ds = ImGui::GetIO().DisplaySize;
        ImVec2 size{150 * s, 46 * s};
        float sp = speed_.f * s * ui::dt();
        pos_ += vel_ * sp;
        if (pos_.x < 0 || pos_.x + size.x > ds.x) {
            vel_.x = -vel_.x;
            pos_.x = std::clamp(pos_.x, 0.f, ds.x - size.x);
            hue_ += 0.23f;
        }
        if (pos_.y < 0 || pos_.y + size.y > ds.y) {
            vel_.y = -vel_.y;
            pos_.y = std::clamp(pos_.y, 0.f, ds.y - size.y);
            hue_ += 0.37f;
        }
        float r, g, b;
        ImGui::ColorConvertHSVtoRGB(std::fmod(hue_, 1.f), 0.45f, 1.f, r, g, b);
        ImU32 col = IM_COL32(int(r * 255), int(g * 255), int(b * 255), int(255 * opacity_.f));
        draw::heart(dl, pos_ + ImVec2(22 * s, 24 * s), 40 * s, col);
        dl->AddText(fonts::bold(), 32 * s, pos_ + ImVec2(48 * s, 6 * s), col, build::name);
    }

private:
    Setting& speed_ = slider("speed", "Speed", 160.f, 40.f, 600.f, "%.0f");
    Setting& opacity_ = slider("opacity", "Opacity", 0.8f, 0.1f, 1.f, "%.2f");
    ImVec2 pos_{100, 100};
    ImVec2 vel_{0.7071f, 0.7071f};
    float hue_ = 0.9f;
};
