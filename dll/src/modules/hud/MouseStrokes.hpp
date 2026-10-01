#pragma once

#include "gui/Theme.hpp"
#include "hook/Input.hpp"
#include "modules/HudModule.hpp"
#include "render/Draw.hpp"

#include <algorithm>
#include <cmath>
#include <deque>

class MouseStrokes : public HudModule {
public:
    MouseStrokes() : HudModule("Mouse Strokes", "Shows your mouse movement as a dot with a trail.", {"hud-self"}, {0.12f, 0.62f}) {}

protected:
    ImVec2 content(ImDrawList* dl, ImVec2 o, float s) override {
        float size = boxSize_.f * s;
        int dx = 0, dy = 0;
        input::consumeMotion(dx, dy);

        target_.x = std::clamp(target_.x * 0.8f + dx * sensitivity_.f * 0.05f, -1.f, 1.f);
        target_.y = std::clamp(target_.y * 0.8f + dy * sensitivity_.f * 0.05f, -1.f, 1.f);
        dot_.x = draw::approach(dot_.x, target_.x, 18.f);
        dot_.y = draw::approach(dot_.y, target_.y, 18.f);

        ImVec2 c = o + ImVec2(size, size) * 0.5f;
        ImVec2 p = c + dot_ * (size * 0.4f);
        trail_.push_back(p - o);
        while (trail_.size() > 24) trail_.pop_front();

        auto& t = theme::current();
        dl->AddRect(o, o + ImVec2(size, size), theme::col(t.textDim, 0.3f), 6 * s, 0, 1.f * s);
        for (size_t i = 1; i < trail_.size(); i++) {
            float a = float(i) / trail_.size();
            dl->AddLine(o + trail_[i - 1], o + trail_[i], theme::col(color_.color, a * 0.7f), 2.5f * s * a);
        }
        dl->AddCircleFilled(p, 4.5f * s, ImGui::GetColorU32(color_.color));
        return {size, size};
    }

private:
    Setting& boxSize_ = slider("box", "Size", 70.f, 40.f, 160.f, "%.0f");
    Setting& sensitivity_ = slider("sens", "Sensitivity", 1.f, 0.2f, 4.f, "%.1fx");
    Setting& color_ = colorSetting("color", "Dot color", {1.f, 0.49f, 0.71f, 1.f});
    ImVec2 target_{0, 0};
    ImVec2 dot_{0, 0};
    std::deque<ImVec2> trail_;
};
