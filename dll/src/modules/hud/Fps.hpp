#pragma once

#include "hook/Dx.hpp"
#include "modules/HudModule.hpp"

#include <algorithm>
#include <array>
#include <format>

class Fps : public TextHud {
public:
    bool defaultEnabled() const override { return true; }

    Fps() : TextHud("FPS", "Zeigt deine Bilder pro Sekunde.", {"hud-self"}, {0.01f, 0.02f}) {}

    void onFrame() override {
        double ms = dx::frame().frameMs;
        if (ms <= 0) return;
        samples_[head_] = ms;
        head_ = (head_ + 1) % samples_.size();
        count_ = std::min(count_ + 1, samples_.size());

        elapsed_ += ms;
        if (elapsed_ >= interval_.f * 1000.0) {
            double sum = 0;
            for (size_t i = 0; i < count_; i++) sum += samples_[i];
            fps_ = sum > 0 ? count_ * 1000.0 / sum : 0;

            std::array<double, 512> sorted{};
            std::copy_n(samples_.begin(), count_, sorted.begin());
            std::sort(sorted.begin(), sorted.begin() + count_, std::greater<>());
            size_t n = std::max<size_t>(1, count_ / 100);
            double worst = 0;
            for (size_t i = 0; i < n; i++) worst += sorted[i];
            low_ = worst > 0 ? n * 1000.0 / worst : 0;
            elapsed_ = 0;
        }
    }

protected:
    std::string label() const override { return "FPS"; }

    std::string value() override {
        if (lowShown_.b) return std::format("{:.0f}  ·  1%: {:.0f}", fps_, low_);
        return std::format("{:.0f}", fps_);
    }

private:
    Setting& lowShown_ = toggleSetting("low", "1% Low anzeigen", false);
    Setting& interval_ = slider("interval", "Aktualisierung (s)", 0.5f, 0.1f, 2.f, "%.1f s");
    std::array<double, 512> samples_{};
    size_t head_ = 0;
    size_t count_ = 0;
    double elapsed_ = 0;
    double fps_ = 0;
    double low_ = 0;
};
