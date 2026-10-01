#pragma once

#include "gui/Theme.hpp"
#include "hook/Input.hpp"
#include "modules/HudModule.hpp"
#include "render/Draw.hpp"
#include "render/Fonts.hpp"

#include <windows.h>

#include <array>
#include <format>

class Keystrokes : public HudModule {
public:
    bool defaultEnabled() const override { return true; }

    Keystrokes() : HudModule("Keystrokes", "Zeigt WASD, Leertaste und Maustasten live an.", {"hud-self"}, {0.01f, 0.62f}) {
        sub("Eigene Werte");
        background_.b = false;
    }

protected:
    ImVec2 content(ImDrawList* dl, ImVec2 o, float s) override {
        float k = keySize_.f * s, gap = 4 * s;
        float row = 0;

        key(dl, 0, o + ImVec2(k + gap, 0), {k, k}, forward_.i, nullptr, s);
        row += k + gap;
        key(dl, 1, o + ImVec2(0, row), {k, k}, left_.i, nullptr, s);
        key(dl, 2, o + ImVec2(k + gap, row), {k, k}, back_.i, nullptr, s);
        key(dl, 3, o + ImVec2((k + gap) * 2, row), {k, k}, right_.i, nullptr, s);
        row += k + gap;

        float full = k * 3 + gap * 2;
        if (mouse_.b) {
            float half = (full - gap) * 0.5f;
            std::string lc = cpsInside_.b ? std::format("{} CPS", input::cps(MouseButton::Left)) : "";
            std::string rc = cpsInside_.b ? std::format("{} CPS", input::cps(MouseButton::Right)) : "";
            key(dl, 4, o + ImVec2(0, row), {half, k * 0.8f}, VK_LBUTTON, "LMB", s, lc);
            key(dl, 5, o + ImVec2(half + gap, row), {half, k * 0.8f}, VK_RBUTTON, "RMB", s, rc);
            row += k * 0.8f + gap;
        }
        if (space_.b) {
            key(dl, 6, o + ImVec2(0, row), {full, k * 0.45f}, jump_.i, "", s);
            row += k * 0.45f;
        }
        return {full, row};
    }

private:
    void key(ImDrawList* dl, int idx, ImVec2 p, ImVec2 size, int vk, const char* label, float s, const std::string& sub = "") {
        auto& t = theme::current();
        bool down = input::down(vk);
        float& a = anim_[idx];
        a = draw::approach(a, down ? 1.f : 0.f, (down ? 30.f : 12.f));

        ImVec4 bg = theme::mix(idle_.color, pressed_.color, a);
        float r = rounding_.f * s;
        dl->AddRectFilled(p, p + size, ImGui::GetColorU32(bg), r);
        if (a > 0.01f && t.gradient) draw::glow(dl, p, p + size, r, theme::col(pressed_.color, 0.5f * a), 5 * s);

        std::string text = label ? label : keyLabel(vk);
        if (!label && text.size() > 3) text = text.substr(0, 1);
        ImU32 tc = ImGui::GetColorU32(theme::mix(textColor_.color, pressedText_.color, a));
        float fs = fonts::hudSize() * s * (sub.empty() ? 1.f : 0.85f);
        ImVec2 ts = fonts::hud()->CalcTextSizeA(fs, FLT_MAX, 0, text.c_str());
        float yoff = sub.empty() ? 0 : -6 * s;
        dl->AddText(fonts::hud(), fs, p + (size - ts) * 0.5f + ImVec2(0, yoff), tc, text.c_str());
        if (!sub.empty()) {
            float ss = fs * 0.6f;
            ImVec2 st = fonts::hud()->CalcTextSizeA(ss, FLT_MAX, 0, sub.c_str());
            dl->AddText(fonts::hud(), ss, p + ImVec2((size.x - st.x) * 0.5f, size.y * 0.5f + 4 * s), tc, sub.c_str());
        }
    }

    static std::string keyLabel(int vk) {
        if (vk >= 'A' && vk <= 'Z') return std::string(1, char(vk));
        if (vk >= '0' && vk <= '9') return std::string(1, char(vk));
        switch (vk) {
        case VK_UP: return "^";
        case VK_DOWN: return "v";
        case VK_LEFT: return "<";
        case VK_RIGHT: return ">";
        }
        return "?";
    }

    Setting& keySize_ = slider("size", "Tastengröße", 38.f, 20.f, 70.f, "%.0f");
    Setting& mouse_ = toggleSetting("mouse", "Maustasten", true);
    Setting& cpsInside_ = toggleSetting("cps", "CPS in Maustasten", true);
    Setting& space_ = toggleSetting("space", "Leertaste", true);
    Setting& idle_ = colorSetting("idle", "Taste", {0.10f, 0.06f, 0.12f, 0.55f});
    Setting& pressed_ = colorSetting("pressed", "Taste gedrückt", {1.f, 0.49f, 0.71f, 0.9f});
    Setting& pressedText_ = colorSetting("pressedText", "Text gedrückt", {1.f, 1.f, 1.f, 1.f});
    Setting& forward_ = keySetting("forward", "Vorwärts", 'W');
    Setting& left_ = keySetting("left", "Links", 'A');
    Setting& back_ = keySetting("back", "Rückwärts", 'S');
    Setting& right_ = keySetting("right", "Rechts", 'D');
    Setting& jump_ = keySetting("jump", "Springen", VK_SPACE);
    std::array<float, 8> anim_{};
};
