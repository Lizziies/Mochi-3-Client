#pragma once

#include "I18n.hpp"
#include "Tuning.hpp"
#include "hook/Dx.hpp"
#include "modules/Module.hpp"

class LowLatency : public Module {
public:
    LowLatency()
        : Module("Low Latency", "Shorter frame queue, optional tearing instead of VSync and a precise FPS limiter. Compare before and after with the Latency HUD.",
                 Category::Performance, {"performance"}) {
        sub("Frame-Timing");
        limit_.visible = [this] { return useLimit_.b; };
    }

    void onFrame() override {
        if (queue_.b) perf::lowLatency();
        if (tearing_.b) perf::tearing();
        if (useLimit_.b) perf::limit(limit_.f);
    }

    void drawSettings() override {
        auto& fi = dx::frame();
        ImGui::Spacing();
        ImGui::TextDisabled(i18n::tr("Status: %s, %s"), i18n::tr(fi.lowLatencyActive ? "short queue active" : "default queue"),
                            i18n::tr(fi.tearingSupported ? "tearing possible" : "tearing not allowed by the game"));
    }

private:
    Setting& queue_ = toggleSetting("queue", "Short frame queue", true);
    Setting& tearing_ = toggleSetting("tearing", "Allow tearing (VSync off)", false);
    Setting& useLimit_ = toggleSetting("limit", "Custom FPS limiter", false);
    Setting& limit_ = slider("fps", "FPS limit", 240.f, 30.f, 1000.f, "%.0f");
};
