#pragma once

#include "I18n.hpp"
#include "gui/Theme.hpp"
#include "hook/Dx.hpp"
#include "hook/Input.hpp"
#include "modules/HudModule.hpp"
#include "render/Fonts.hpp"

#include <windows.h>

#include <algorithm>
#include <array>
#include <format>

class LatencyHud : public HudModule {
public:
    LatencyHud()
        : HudModule("Latency", "Measures frame time and the time from click to the next frame. Use it to compare settings.",
                    {"hud-self"}, {0.70f, 0.02f}) {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        qpf_ = double(f.QuadPart);
    }

    void onFrame() override {
        auto& fi = dx::frame();
        times_[head_] = (float)fi.frameMs;
        head_ = (head_ + 1) % times_.size();

        int64_t click = input::lastClickQpc();
        if (click && click != seenClick_ && fi.presentQpc > click) {
            seenClick_ = click;
            double ms = double(fi.presentQpc - click) * 1000.0 / qpf_;
            if (ms < 250) {
                clicks_[clickHead_] = (float)ms;
                clickHead_ = (clickHead_ + 1) % clicks_.size();
                clickCount_ = std::min<size_t>(clickCount_ + 1, clicks_.size());
                flash_ = 1.f;
            }
        }
    }

protected:
    ImVec2 content(ImDrawList* dl, ImVec2 o, float s) override {
        auto& t = theme::current();
        auto& fi = dx::frame();
        float w = 220 * s, h = 46 * s;
        float y = 0;

        float avgClick = 0;
        for (size_t i = 0; i < clickCount_; i++) avgClick += clicks_[i];
        if (clickCount_) avgClick /= clickCount_;

        y += drawText(dl, o, s, i18n::fmt("Frame {:.2f} ms", fi.frameMs), textColor()).y;
        y += drawText(dl, o + ImVec2(0, y), s, clickCount_ ? i18n::fmt("Click to frame {:.1f} ms", avgClick) : std::string(i18n::tr("Click to frame: click once")), accentColor()).y;
        std::string mode = i18n::fmt("{}  ·  {}  ·  {} buffered", i18n::tr(fi.lowLatencyActive ? "Low latency on" : "Low latency off"),
                                      i18n::tr(dx::tuning().allowTearing && fi.tearingSupported ? "Tearing" : "VSync/default"), fi.bufferCount);
        float small = fonts::hudSize() * s * 0.7f;
        dl->AddText(fonts::hud(), small, o + ImVec2(0, y + 2 * s), theme::col(t.textDim), mode.c_str());
        y += small + 6 * s;

        ImVec2 g0 = o + ImVec2(0, y), g1 = g0 + ImVec2(w, h);
        dl->AddRectFilled(g0, g1, IM_COL32(0, 0, 0, 60), 4 * s);
        float maxMs = 1.f;
        for (float v : times_) maxMs = std::max(maxMs, v);
        maxMs = std::min(maxMs * 1.2f, 50.f);
        size_t n = times_.size();
        for (size_t i = 1; i < n; i++) {
            float a = times_[(head_ + i - 1) % n], b = times_[(head_ + i) % n];
            ImVec2 p0{g0.x + w * (i - 1) / (n - 1), g1.y - h * std::min(a / maxMs, 1.f)};
            ImVec2 p1{g0.x + w * i / (n - 1), g1.y - h * std::min(b / maxMs, 1.f)};
            dl->AddLine(p0, p1, theme::col(t.accent), 1.5f * s);
        }
        if (flashTest_.b && flash_ > 0.01f) {
            dl->AddRectFilled(g1 - ImVec2(18 * s, h), g1 - ImVec2(0, h - 18 * s), IM_COL32(255, 255, 255, int(255 * flash_)));
            flash_ = std::max(0.f, flash_ - 0.25f);
        }
        return {w, y + h};
    }

private:
    Setting& flashTest_ = toggleSetting("flash", "Flash test (white square on click)", false);
    std::array<float, 120> times_{};
    size_t head_ = 0;
    std::array<float, 20> clicks_{};
    size_t clickHead_ = 0;
    size_t clickCount_ = 0;
    int64_t seenClick_ = 0;
    float flash_ = 0;
    double qpf_ = 1;
};
