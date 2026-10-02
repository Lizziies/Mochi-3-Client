#pragma once

#include "gui/Notify.hpp"
#include "gui/Theme.hpp"
#include "modules/HudModule.hpp"
#include "render/Fonts.hpp"
#include "render/Ui.hpp"

#include <windows.h>

#include <cmath>
#include <format>

class Pomodoro : public HudModule {
public:
    Pomodoro()
        : HudModule("Pomodoro", "Study and break timer: work phase, short and long break, with a notice on every change.", {"hud-self"}, {0.01f, 0.18f}) {
        sub("Timer");
        long_.visible = [this] { return rounds_.i > 0; };
        ring_.visible = [this] { return style_.i == 0; };
    }

    bool persistent() const override { return false; }

    void onKey(KeyEvent& ev) override {
        if (!ev.down || ev.repeat) return;
        if (ev.vk == startKey_.i) {
            if (running_) done_ = elapsed();
            else start_ = ui::time();
            running_ = !running_;
        } else if (ev.vk == skipKey_.i) {
            advance();
        } else if (ev.vk == resetKey_.i) {
            phase_ = 0;
            round_ = 0;
            running_ = false;
            done_ = 0;
        }
    }

    void onEnable() override {
        done_ = 0;
        running_ = false;
        phase_ = 0;
        round_ = 0;
    }

    void onFrame() override {
        if (running_ && left() <= 0.0) advance();
    }

protected:
    ImVec2 content(ImDrawList* dl, ImVec2 o, float s) override {
        auto& t = theme::current();
        double rem = left();
        double total = phaseLength();
        float frac = total > 0 ? float(1.0 - rem / total) : 0.f;
        ImVec4 col = phase_ == 0 ? t.accent : t.ok;
        int m = int(rem) / 60, sec = int(rem) % 60;
        std::string clock = std::format("{:02}:{:02}", m, sec);
        const char* names[] = {"Work", "Break", "Long break"};
        std::string label = std::string(i18n::tr(names[phase_])) + (running_ ? "" : i18n::tr("  (paused)"));

        float lineH = fonts::hudSize() * s * 1.1f;
        if (style_.i == 0) {
            float r = 26 * s;
            ImVec2 c = o + ImVec2(r, r);
            dl->AddCircle(c, r, theme::col(t.textDim, 0.3f), 40, 4 * s);
            dl->PathArcTo(c, r, -1.5708f, -1.5708f + 6.2832f * frac, 40);
            dl->PathStroke(ImGui::GetColorU32(ring_.b ? col : t.accent), 0, 4 * s);
            auto sz = textSize(s, clock);
            drawText(dl, c - sz * 0.5f, s, clock, textColor());
            auto lsz = drawText(dl, o + ImVec2(r * 2 + 10 * s, r - lineH * 0.5f), s, label, ImGui::GetColorU32(col));
            return {r * 2 + 10 * s + lsz.x, r * 2};
        }
        float w = 170 * s;
        float y = drawText(dl, o, s, clock + "  " + label, textColor()).y;
        ImVec2 b0 = o + ImVec2(0, y + 3 * s);
        dl->AddRectFilled(b0, b0 + ImVec2(w, 6 * s), IM_COL32(0, 0, 0, 70), 3 * s);
        dl->AddRectFilled(b0, b0 + ImVec2(w * frac, 6 * s), ImGui::GetColorU32(col), 3 * s);
        float h = y + 9 * s;
        if (cycles_.b) h += drawText(dl, o + ImVec2(0, h), s, i18n::fmt("Round {}", round_ + 1), ImGui::GetColorU32(t.textDim)).y;
        return {w, h};
    }

private:
    double phaseLength() const {
        if (phase_ == 0) return work_.f * 60.0;
        if (phase_ == 1) return short_.f * 60.0;
        return long_.f * 60.0;
    }

    double elapsed() const { return running_ ? done_ + (ui::time() - start_) : done_; }
    double left() const { return phaseLength() - elapsed(); }

    void advance() {
        const char* title = "";
        if (phase_ == 0) {
            round_++;
            bool big = rounds_.i > 0 && round_ % rounds_.i == 0;
            phase_ = big ? 2 : 1;
            title = i18n::tr(big ? "Long break" : "Short break");
        } else {
            phase_ = 0;
            title = i18n::tr("Back to work");
        }
        done_ = 0;
        start_ = ui::time();
        running_ = auto_.b;
        if (toast_.b) notify::push("Pomodoro", title, notify::Kind::Info, 6.f);
        if (beep_.b) MessageBeep(MB_ICONASTERISK);
    }

    Setting& work_ = slider("work", "Work (min)", 25.f, 5.f, 90.f, "%.0f");
    Setting& short_ = slider("short", "Short break (min)", 5.f, 1.f, 30.f, "%.0f");
    Setting& long_ = slider("long", "Long break (min)", 15.f, 5.f, 60.f, "%.0f");
    Setting& rounds_ = intSlider("rounds", "Rounds until the long break", 4, 0, 8);
    Setting& auto_ = toggleSetting("auto", "Start the next phase automatically", true);
    Setting& style_ = choice("style", "Display style", {"Ring", "Bar"});
    Setting& ring_ = toggleSetting("phaseColor", "Ring color by phase", true);
    Setting& cycles_ = toggleSetting("cycles", "Round counter", true);
    Setting& toast_ = toggleSetting("toast", "Notice on change", true);
    Setting& beep_ = toggleSetting("beep", "Beep", true);
    Setting& startKey_ = keySetting("startKey", "Start / pause", VK_F10);
    Setting& skipKey_ = keySetting("skipKey", "Skip phase", 0);
    Setting& resetKey_ = keySetting("resetKey", "Reset", 0);
    bool running_ = false;
    int phase_ = 0;
    int round_ = 0;
    double start_ = 0;
    double done_ = 0;
};
