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

    Keystrokes()
        : HudModule("Keystrokes", "Shows WASD, space and the mouse buttons live. Glow, borders, custom texts, spacing and animation speed.", {"hud-self"},
                    {0.26f, 0.68f}) {
        sub("Info displays");
        background_.b = false;
        cpsFormat_.visible = [this] { return cpsInside_.b; };
        glowColor_.visible = [this] { return glowPressed_.b; };
        idleGlowColor_.visible = [this] { return glowIdle_.b; };
        keyBorderWidth_.visible = [this] { return keyBorder_.b; };
        keyBorderColor_.visible = [this] { return keyBorder_.b; };
        keyBorderPressed_.visible = [this] { return keyBorder_.b; };
        keyShadowColor_.visible = [this] { return keyShadow_.b; };
        spaceWidth_.visible = [this] { return space_.b; };
        spaceHeight_.visible = [this] { return space_.b; };
        spaceText_.visible = [this] { return space_.b; };
        lmbText_.visible = [this] { return mouse_.b; };
        rmbText_.visible = [this] { return mouse_.b; };
    }

protected:
    ImVec2 content(ImDrawList* dl, ImVec2 o, float s) override {
        float k = keySize_.f * s, gap = spacing_.f * s;
        float row = 0;

        key(dl, 0, o + ImVec2(k + gap, 0), {k, k}, forward_.i, wText_.text, s);
        row += k + gap;
        key(dl, 1, o + ImVec2(0, row), {k, k}, left_.i, aText_.text, s);
        key(dl, 2, o + ImVec2(k + gap, row), {k, k}, back_.i, sText_.text, s);
        key(dl, 3, o + ImVec2((k + gap) * 2, row), {k, k}, right_.i, dText_.text, s);
        row += k + gap;

        float full = k * 3 + gap * 2;
        if (mouse_.b) {
            float half = (full - gap) * 0.5f;
            std::string lc, rc;
            if (cpsInside_.b) {
                lc = cpsText(input::cps(MouseButton::Left));
                rc = cpsText(input::cps(MouseButton::Right));
            }
            key(dl, 4, o + ImVec2(0, row), {half, k * 0.8f}, VK_LBUTTON, lmbText_.text.empty() ? "LMB" : lmbText_.text, s, lc);
            key(dl, 5, o + ImVec2(half + gap, row), {half, k * 0.8f}, VK_RBUTTON, rmbText_.text.empty() ? "RMB" : rmbText_.text, s, rc);
            row += k * 0.8f + gap;
        }
        if (space_.b) {
            float w = full * spaceWidth_.f;
            key(dl, 6, o + ImVec2((full - w) * 0.5f, row), {w, k * spaceHeight_.f}, jump_.i, spaceText_.text, s, "", true);
            row += k * spaceHeight_.f;
        }
        return {full, row};
    }

private:
    std::string cpsText(int cps) const {
        std::string out = cpsFormat_.text.empty() ? "{value} CPS" : cpsFormat_.text;
        std::string v = std::to_string(cps);
        for (size_t at = out.find("{value}"); at != std::string::npos; at = out.find("{value}", at + v.size())) out.replace(at, 7, v);
        return out;
    }

    void key(ImDrawList* dl, int idx, ImVec2 p, ImVec2 size, int vk, const std::string& custom, float s, const std::string& sub = "", bool space = false) {
        auto& t = theme::current();
        bool down = input::down(vk);
        float& a = anim_[size_t(idx)];
        a = draw::approach(a, down ? 1.f : 0.f, down ? pressSpeed_.f : releaseSpeed_.f);

        float r = rounding_.f * s;
        ImVec2 end = p + size;
        if (keyShadow_.b) draw::glow(dl, p + ImVec2(0, 2 * s), end + ImVec2(0, 2 * s), r, ImGui::GetColorU32(keyShadowColor_.color), 6 * s);
        if (glowIdle_.b && a < 0.99f) draw::glow(dl, p, end, r, theme::col(idleGlowColor_.color, 0.5f * (1.f - a)), 6 * s);
        if (glowPressed_.b && a > 0.01f) draw::glow(dl, p, end, r, theme::col(glowColor_.color, 0.6f * a), 7 * s);
        (void)t;

        ImVec4 bg = theme::mix(idle_.color, pressed_.color, a);
        dl->AddRectFilled(p, end, ImGui::GetColorU32(bg), r);
        if (keyBorder_.b)
            dl->AddRect(p, end, ImGui::GetColorU32(theme::mix(keyBorderColor_.color, keyBorderPressed_.color, a)), r, 0, keyBorderWidth_.f * s);

        std::string text = custom.empty() ? (space || vk == VK_LBUTTON || vk == VK_RBUTTON ? "" : keyLabel(vk)) : custom;
        if (custom.empty() && !space && text.size() > 3) text = text.substr(0, 1);
        ImU32 tc = ImGui::GetColorU32(theme::mix(textColor_.color, pressedText_.color, a));
        float fs = fonts::hudSize() * s * (sub.empty() ? 1.f : 0.85f);
        ImVec2 off{textX_.f * s, textY_.f * s};
        ImVec2 ts = fonts::hud()->CalcTextSizeA(fs, FLT_MAX, 0, text.c_str());
        float yoff = sub.empty() ? 0 : -6 * s;
        if (shadow_.b) dl->AddText(fonts::hud(), fs, p + (size - ts) * 0.5f + ImVec2(0, yoff) + off + ImVec2(shadowOffset_.f * s, shadowOffset_.f * s), IM_COL32(0, 0, 0, 140), text.c_str());
        dl->AddText(fonts::hud(), fs, p + (size - ts) * 0.5f + ImVec2(0, yoff) + off, tc, text.c_str());
        if (!sub.empty()) {
            float ss = fs * 0.6f;
            ImVec2 st = fonts::hud()->CalcTextSizeA(ss, FLT_MAX, 0, sub.c_str());
            dl->AddText(fonts::hud(), ss, p + ImVec2((size.x - st.x) * 0.5f, size.y * 0.5f + 4 * s) + off, tc, sub.c_str());
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

    Setting& keySize_ = slider("size", "Key size", 38.f, 20.f, 70.f, "%.0f");
    Setting& spacing_ = slider("spacing", "Key spacing", 4.f, 0.f, 20.f, "%.0f");
    Setting& mouse_ = toggleSetting("mouse", "Mouse buttons", true);
    Setting& cpsInside_ = toggleSetting("cps", "CPS in mouse buttons", true);
    Setting& cpsFormat_ = textSetting("cpsFormat", "CPS text ({value})", "{value} CPS");
    Setting& space_ = toggleSetting("space", "Space", true);
    Setting& spaceWidth_ = slider("spaceWidth", "Space bar width", 1.f, 0.3f, 1.2f, "%.2fx");
    Setting& spaceHeight_ = slider("spaceHeight", "Space bar height", 0.45f, 0.2f, 1.f, "%.2fx");
    Setting& pressSpeed_ = slider("pressSpeed", "Press animation speed", 30.f, 5.f, 80.f, "%.0f");
    Setting& releaseSpeed_ = slider("releaseSpeed", "Release animation speed", 12.f, 3.f, 60.f, "%.0f");
    Setting& textX_ = slider("textX", "Text offset X", 0.f, -12.f, 12.f, "%.0f");
    Setting& textY_ = slider("textY", "Text offset Y", 0.f, -12.f, 12.f, "%.0f");
    Setting& wText_ = textSetting("wText", "Text for forward", "");
    Setting& aText_ = textSetting("aText", "Text for left", "");
    Setting& sText_ = textSetting("sText", "Text for back", "");
    Setting& dText_ = textSetting("dText", "Text for right", "");
    Setting& lmbText_ = textSetting("lmbText", "Text for left mouse button", "");
    Setting& rmbText_ = textSetting("rmbText", "Text for right mouse button", "");
    Setting& spaceText_ = textSetting("spaceText", "Text for space", "");
    Setting& glowPressed_ = toggleSetting("glowPressed", "Glow on pressed keys", true);
    Setting& glowIdle_ = toggleSetting("glowIdle", "Glow on idle keys", false);
    Setting& keyBorder_ = toggleSetting("keyBorder", "Key border", false);
    Setting& keyBorderWidth_ = slider("keyBorderWidth", "Key border thickness", 1.5f, 0.5f, 5.f, "%.1f");
    Setting& keyShadow_ = toggleSetting("keyShadow", "Key shadow", false);
    Setting& idle_ = colorSetting("idle", "Key", {0.08f, 0.08f, 0.09f, 0.55f});
    Setting& pressed_ = colorSetting("pressed", "Key pressed", {0.23f, 0.65f, 0.93f, 0.9f});
    Setting& pressedText_ = colorSetting("pressedText", "Text pressed", {1.f, 1.f, 1.f, 1.f});
    Setting& glowColor_ = colorSetting("pressedGlow", "Glow when pressed", {0.23f, 0.65f, 0.93f, 1.f});
    Setting& idleGlowColor_ = colorSetting("idleGlowColor", "Glow when idle", {0.6f, 0.5f, 1.f, 1.f});
    Setting& keyBorderColor_ = colorSetting("keyBorderColor", "Key border", {1.f, 1.f, 1.f, 0.35f});
    Setting& keyBorderPressed_ = colorSetting("keyBorderPressed", "Key border pressed", {0.23f, 0.65f, 0.93f, 1.f});
    Setting& keyShadowColor_ = colorSetting("keyShadowColor", "Key shadow color", {0.f, 0.f, 0.f, 0.5f});
    Setting& forward_ = keySetting("forward", "Forward", 'W');
    Setting& left_ = keySetting("left", "Left", 'A');
    Setting& back_ = keySetting("back", "Back", 'S');
    Setting& right_ = keySetting("right", "Right", 'D');
    Setting& jump_ = keySetting("jump", "Jump", VK_SPACE);
    std::array<float, 8> anim_{};
};
