#pragma once

#include "Tuning.hpp"
#include "hook/Dx.hpp"
#include "modules/Module.hpp"

class LowLatency : public Module {
public:
    LowLatency()
        : Module("Low Latency", "Kurze Bild-Warteschlange, optional Tearing und eigener FPS-Limiter.",
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
        ImGui::TextDisabled("Status: %s, %s", fi.lowLatencyActive ? "kurze Warteschlange aktiv" : "Standard-Warteschlange",
                            fi.tearingSupported ? "Tearing möglich" : "Tearing vom Spiel nicht freigegeben");
    }

private:
    Setting& queue_ = toggleSetting("queue", "Kurze Bild-Warteschlange", true);
    Setting& tearing_ = toggleSetting("tearing", "Tearing erlauben (VSync aus)", false);
    Setting& useLimit_ = toggleSetting("limit", "Eigener FPS-Limiter", false);
    Setting& limit_ = slider("fps", "FPS-Limit", 240.f, 30.f, 1000.f, "%.0f");
};
