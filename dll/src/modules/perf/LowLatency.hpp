#pragma once

#include "I18n.hpp"
#include "Tuning.hpp"
#include "hook/Dx.hpp"
#include "hook/Input.hpp"
#include "modules/Module.hpp"
#include "system/GpuLatency.hpp"
#include "system/FrameStats.hpp"

#include <windows.h>

#include <algorithm>

class LowLatency : public Module {
public:
    LowLatency()
        : Module("Low Latency", "Shorter frame queue, optional tearing instead of VSync, a higher priority render thread and a precise FPS limiter. Shows the time from click to frame so you can compare.",
                 Category::Performance, {"performance"}) {
        sub("Frame timing");
        limit_.visible = [this] { return useLimit_.b; };
        driver_.visible = [] {
            const auto status = gpuLatency::status();
            return status.vendor == gpuLatency::Vendor::Nvidia && status.stage != gpuLatency::Stage::Unsupported ||
                   status.vendor == gpuLatency::Vendor::Amd && status.frameStartVerified;
        };
    }

    void onDisable() override {
        restore();
        gpuLatency::set(gpuLatency::Mode::Off);
    }

    void onFrame() override {
        if (queue_.b) perf::lowLatency();
        if (tearing_.b) perf::tearing();
        if (useLimit_.b) perf::limit(limit_.f);
        if (priority_.b) boost();
        else restore();
        gpuLatency::set(static_cast<gpuLatency::Mode>(driver_.i));
        track();
    }

    void drawSettings() override {
        auto& fi = dx::frame();
        const auto frames = frames_.summary();
        ImGui::Spacing();
        if (frames.count)
            ImGui::TextDisabled(i18n::tr("Frame intervals: %.2f ms average, %.2f ms P95, %.2f ms P99"),
                                frames.mean, frames.p95, frames.p99);
        const auto driver = gpuLatency::status();
        const char* state = "GPU latency unavailable";
        switch (driver.stage) {
        case gpuLatency::Stage::Off: state = "GPU latency off"; break;
        case gpuLatency::Stage::DriverOnly: state = "NVIDIA driver latency active; input pacing not connected"; break;
        case gpuLatency::Stage::BeforeInput: state = "GPU pacing connected before input"; break;
        case gpuLatency::Stage::Failed: state = "GPU latency driver rejected the request"; break;
        default: break;
        }
        ImGui::TextDisabled("%s", i18n::tr(state));
        if (driver.vendor == gpuLatency::Vendor::Amd && !driver.frameStartVerified)
            ImGui::TextDisabled("%s", i18n::tr("AMD Anti-Lag 2 needs a verified frame start hook"));
        if (driver.error) ImGui::TextDisabled(i18n::tr("Driver error: %d"), driver.error);
        ImGui::TextDisabled(i18n::tr("Status: %s, %s"), i18n::tr(fi.lowLatencyActive ? "short queue active" : "default queue"),
                            i18n::tr(fi.tearingSupported ? "tearing possible" : "tearing not allowed by the game"));
        if (count_) ImGui::TextDisabled(i18n::tr("Click event to Present return: %.1f ms (%d samples)"), sum_ / count_, count_);
        else ImGui::TextDisabled(i18n::tr("Click to measure the latency."));
        ImGui::TextDisabled("%s", i18n::tr("This does not measure input to the displayed image."));
        if (ImGui::SmallButton(i18n::tr("Reset measurement"))) {
            sum_ = 0;
            count_ = 0;
            frames_.clear();
        }
    }

private:
    void boost() {
        if (thread_) return;
        thread_ = OpenThread(THREAD_SET_INFORMATION | THREAD_QUERY_INFORMATION, FALSE, GetCurrentThreadId());
        if (!thread_) return;
        base_ = GetThreadPriority(thread_);
        SetThreadPriority(thread_, std::max(base_, (int)THREAD_PRIORITY_ABOVE_NORMAL));
    }

    void restore() {
        if (!thread_) return;
        SetThreadPriority(thread_, base_);
        CloseHandle(thread_);
        thread_ = nullptr;
    }

    void track() {
        auto& fi = dx::frame();
        if (fi.presentQpc != frameSeen_) {
            frameSeen_ = fi.presentQpc;
            frames_.add(fi.frameMs);
        }
        int64_t click = input::lastClickQpc();
        if (!click || click == seen_ || fi.presentQpc <= click) return;
        seen_ = click;
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        double ms = double(fi.presentQpc - click) * 1000.0 / double(f.QuadPart);
        if (ms < 250) {
            sum_ += (float)ms;
            count_++;
        }
    }

    Setting& queue_ = toggleSetting("queue", "Short frame queue", true);
    Setting& driver_ = choice("gpuLatency", "GPU latency mode", {"Off", "On", "On + NVIDIA Boost"});
    Setting& tearing_ = toggleSetting("tearing", "Allow tearing (VSync off)", false);
    Setting& priority_ = toggleSetting("priority", "Render thread with higher priority", true);
    Setting& useLimit_ = toggleSetting("limit", "Custom FPS limiter", false);
    Setting& limit_ = slider("fps", "FPS limit", 240.f, 30.f, 1000.f, "%.0f");
    HANDLE thread_ = nullptr;
    int base_ = 0;
    int64_t seen_ = 0;
    int64_t frameSeen_ = 0;
    timing::FrameStats frames_;
    float sum_ = 0.f;
    int count_ = 0;
};
