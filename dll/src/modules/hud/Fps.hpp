#pragma once

#include "hook/Dx.hpp"
#include "modules/HudModule.hpp"

#include <algorithm>
#include <array>
#include <format>

class Fps : public TextHud {
public:
    bool defaultEnabled() const override { return true; }

    Fps() : TextHud("FPS", "Shows your frames per second.", {"hud-self"}, {0.01f, 0.02f}) {
        sub("Info displays");
    }

    void onFrame() override {
        double ms = dx::frame().frameMs;
        if (ms <= 0) return;
        samples_[head_] = ms;
        head_ = (head_ + 1) % samples_.size();
        count_ = std::min(count_ + 1, samples_.size());

        elapsed_ += ms;
        frames_++;
        if (elapsed_ >= interval_.f * 1000.0) {
            fps_ = frames_ * 1000.0 / elapsed_;
            frames_ = 0;

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
    std::string label() const override { return show_.i == 1 ? "MS" : "FPS"; }

    std::string value() override {
        double ms = fps_ > 0 ? 1000.0 / fps_ : 0.0;
        std::string out = show_.i == 1 ? std::format("{:.1f}", ms) : std::format("{:.0f}", fps_);
        if (show_.i == 2) out += std::format("  ·  {:.1f} ms", ms);
        if (lowShown_.b) out += std::format("  ·  1%: {:.0f}", low_);
        return out;
    }

private:
    Setting& show_ = choice("show", "Show", {"FPS", "Frame time", "Both"});
    Setting& lowShown_ = toggleSetting("low", "Show 1% low", false);
    Setting& interval_ = slider("interval", "Update interval (s)", 0.5f, 0.1f, 2.f, "%.1f s");
    std::array<double, 512> samples_{};
    size_t head_ = 0;
    size_t count_ = 0;
    double elapsed_ = 0;
    int frames_ = 0;
    double fps_ = 0;
    double low_ = 0;
};
